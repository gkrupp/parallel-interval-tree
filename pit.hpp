#pragma once

#include <cstddef>
#include <array>
#include <algorithm>
#include <iostream>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include "it.hpp"


namespace {
    class ReadWriteLock {
    public:
        using mutex_t = std::shared_mutex;
        using read_lock = std::shared_lock<mutex_t>;
        using write_lock = std::unique_lock<mutex_t>;
    private:
        mutable mutex_t mtx;
    public:
        read_lock scoped_lock_read() const { return read_lock(mtx); }
        write_lock scoped_lock_write() const { return write_lock(mtx); }

        void lock_read() const { mtx.lock_shared(); }
        void unlock_read() const {mtx.unlock_shared(); }

        void lock_write() const { mtx.lock(); }
        void unlock_write() const { mtx.unlock(); }

    };
}

namespace {
    template <class Interval, typename T = typename Interval::value_t>
    class ParallelIntervalTreeNode {
    public:
        typedef T value_t;
        typedef Point<T> P;
        typedef Interval I;

        ParallelIntervalTreeNode(const P &begin, const P &end, ParallelIntervalTreeNode* left, ParallelIntervalTreeNode *right)
            : begin(begin), end(end), max(end), multip(1), height(1), left(left), right(right) {}

        ParallelIntervalTreeNode(const P &begin, const P &end) 
            : ParallelIntervalTreeNode(begin, end, NullNode, NullNode) {}

        ParallelIntervalTreeNode(const Interval &I, ParallelIntervalTreeNode* left, ParallelIntervalTreeNode *right)
            : ParallelIntervalTreeNode(I.begin, I.end, left, right) {}

        ParallelIntervalTreeNode(const Interval &I) 
            : ParallelIntervalTreeNode(I.begin, I.end, NullNode, NullNode) {}

        ParallelIntervalTreeNode() : is_null(true) {}

        static ParallelIntervalTreeNode *NullNode;

        // Lock for read or write locking current node
        ReadWriteLock rw_lock;
        P begin;
        P end;
        P max;
        size_t multip;
        int height;
        bool is_null;
        ParallelIntervalTreeNode *left, *right;

        ~ParallelIntervalTreeNode() {
            left->rw_lock.scoped_lock_write();
            if (!left->is_null) {
                delete left;
            }

            right->rw_lock.scoped_lock_write();
            if (!right->is_null) {
                delete right;
            }
        }
    };

    template <class Interval, typename T>
    ParallelIntervalTreeNode<Interval, T>* ParallelIntervalTreeNode<Interval, T>::NullNode(new ParallelIntervalTreeNode());
}




// NOTE: https://www.guru99.com/avl-tree.html
// NOTE: http://www.davismol.net/2016/02/07/data-structures-augmented-interval-tree-to-search-for-interval-overlapping/

template <typename T>
class ParallelIntervalTree {
public:
    typedef T value_t;
    typedef Point<T> P;
    typedef Interval<T> I;
    typedef ParallelIntervalTreeNode<Interval<T>> Node;
    ParallelIntervalTree(const size_t dim) : dim(dim), root(Node::NullNode) {}
    ParallelIntervalTree() : ParallelIntervalTree(1) {}

    // TODO: use locks in insert
    void insert(const I &interval) { insert(interval.begin, interval.end); }
    void insert(const P &begin, const P &end) { root = node_insert(root, begin, end); }

    // TODO: use locks in remove
    void remove(const I &interval) { remove(interval.begin, interval.end); }
    void remove(const P &begin, const P &end) { root = node_remove(root, begin, end); }

    size_t query(const P &p) { root->rw_lock.lock_read(); return node_query(root, p); }

    // 1D print
    void print() {
        node_print(root);
    }
    void node_print(Node *node) {
        if (!node->is_null) {
            std::cout << "("; node_print(node->left);
            std::cout << "," << node->begin[0] << "-" << node->end[0] << "-" << node->max[0] << "-" << node->height << ",";
            node_print(node->right); std::cout << ")";
        } else {
            //std::cout << "[-]\n";
        }
    }

    // ~ParallelIntervalTree() {
    //     root->rw_lock.scoped_lock_write();
    //     delete root;
    // }
private:

    int node_height(Node *node) {
        if (node->left && node->right) {
            return max(node->left->height, node->right->height) + 1;
        } else if (node->left) {
            return node->left->height + 1;
        } else if (node->right) {
            return node->right->height + 1;
        } else {
            return 1;
        }
    }
    int node_bf(Node *node) {
        if (node->left && node->right) {
            return node->left->height - node->right->height;
        } else if (node->left) {
            return node->left->height;
        } else if (node->right) {
            return -node->right->height;
        } else {
            return 0;
        }
    }
    P &node_max(Node *node) {
        if (node->left && node->right) {
            return max(node->end, max(node->left->max, node->right->max));
        } else if (node->left) {
            return max(node->end, node->left->max);
        } else if (node->right) {
            return max(node->end, node->right->max);
        } else {
            return node->end;
        }
    }

    Node *node_insert(Node *node, const P &begin, const P &end) {
        return node_insert(node, begin, end, 1);
    }
    Node *node_insert(Node *node, const P &begin, const P &end, const size_t multip) {
        if (node->is_null) {
            return new Node(begin, end);
        } else if (begin <= node->begin) {
            if (begin==node->begin && end==node->end) {
                node->multip += multip;
                return node;
            }
            node->left = node_insert(node->left, begin, end);
        } else {
            node->right = node_insert(node->right, begin, end);
        }

        node->height = node_height(node);
        node->max = node_max(node);
        //std::cout << node->begin[0] << " " << node->height << " --- ";

        if      (node_bf(node)== 2 && node_bf(node->left)==  1) { node = node_llrotation(node); }
        else if (node_bf(node)==-2 && node_bf(node->right)==-1) { node = node_rrrotation(node); }
        else if (node_bf(node)==-2 && node_bf(node->right)== 1) { node = node_rlrotation(node); }
        else if (node_bf(node)== 2 && node_bf(node->left)== -1) { node = node_lrrotation(node); }

        //std::cout << node->begin[0] << " " << node->height << std::endl;

        return node;

    }

    Node *node_remove(Node *node, const P &begin, const P &end) {
        return node_remove(node, begin, end, 1);
    }
    Node *node_remove(Node *node, const P &begin, const P &end, const size_t multip) {
        if (node->is_null) {
            return Node::NullNode;
        }

        if (begin <= node->begin) {
            if (begin==node->begin && end==node->end) {
                if (node->multip > multip) {
                    node->multip -= multip;
                    return node;
                }
                if (!node->left->is_null) {
                    Node *up = node_rightmost(node->left);
                    node->begin = up->begin;
                    node->end = up->end;
                    node->multip = up->multip;
                    node->left = node_remove(node->left, up->begin, up->end, up->multip);
                } else if (!node->right->is_null) {
                    Node *up = node_leftmost(node->right);
                    node->begin = up->begin;
                    node->end = up->end;
                    node->multip = up->multip;
                    node->right = node_remove(node->right, up->begin, up->end, up->multip);
                } else {
                    delete node;
                    return Node::NullNode;
                }
            } else {
                node->left = node_remove(node->left, begin, end);
            }
        } else {
            node->right = node_remove(node->right, begin, end);
        }

        node->height = node_height(node);
        node->max = node_max(node);
        //std::cout << node->begin[0] << " " << node->height << " "  << node->max[0] <<  " --- ";


        if      (node_bf(node)== 2 && node_bf(node->left)==  1) { node = node_llrotation(node); }                  
        else if (node_bf(node)== 2 && node_bf(node->left)==- 1) { node = node_lrrotation(node); }
        else if (node_bf(node)== 2 && node_bf(node->left)==  0) { node = node_llrotation(node); }
        else if (node_bf(node)==-2 && node_bf(node->right)==-1) { node = node_rrrotation(node); }
        else if (node_bf(node)==-2 && node_bf(node->right)== 1) { node = node_rlrotation(node); }
        else if (node_bf(node)==-2 && node_bf(node->right)== 0) { node = node_llrotation(node); }

        //std::cout << node->begin[0] << " " << node->height << std::endl;

        return node;
    }

    size_t node_query(const Node *node, const P &p) const {
        // node must already be read locked!
        // Before return, node must be read unlocked!
        if (node->is_null) {
            node->rw_lock.unlock_read();
            return 0;
        }

        //std::cout << node->begin[0] << ' ' << node->end[0] << ' ' << p[0] << std::endl;
        if (p < node->begin) {
            // Locking child first, then unlocking current node
            node->left->rw_lock.lock_read();
            node->rw_lock.unlock_read();
            return node_query(node->left, p);
        } 
        else if (p < node->max) {
            size_t subquery = p < node->end ? node->multip : 0;
            // Locking both children for subquery, then unlocking current node
            node->left->rw_lock.lock_read();
            node->right->rw_lock.lock_read();
            node->rw_lock.unlock_read();
            subquery += node_query(node->left, p);
            subquery += node_query(node->right, p);
            return subquery;
        } 
        else {
            node->rw_lock.unlock_read();
            return 0;
        }
    }

    Node *node_llrotation(Node *node) {
        //std::cout << "LL";
        Node *p = node, *tp = p->left;
        p->left = tp->right;
        tp->right = p;
        // update
        p->height = node_height(p);
        tp->height = node_height(tp);
        p->max = node_max(p);
        tp->max = node_max(tp);
        return tp;
    }
    Node *node_rrrotation(Node *node) {
        //std::cout << "RR";
        Node *p = node, *tp = p->right;
        p->right = tp->left;
        tp->left = p;
        // update
        p->height = node_height(p);
        tp->height = node_height(tp);
        p->max = node_max(p);
        tp->max = node_max(tp);
        return tp;
    }
    Node *node_rlrotation(Node *node) {
        //std::cout << "RL";
        Node *p = node, *tp = p->right, *tp2 = p->right->left;
        p->right = tp2->left;
        tp->left = tp2->right;
        tp2->right = tp;
        tp2->left = p;
        // update
        p->height = node_height(p);
        tp->height = node_height(tp);
        tp2->height = node_height(tp2);
        p->max = node_max(p);
        tp->max = node_max(tp);
        tp2->max = node_max(tp2);
        return tp2;
    }
    Node *node_lrrotation(Node *node) {
        //std::cout << "LR";
        Node *p = node, *tp = p->left, *tp2 = p->left->right;
        p->left = tp2->right;
        tp->right = tp2->left;
        tp2->right = p;
        tp2->left = tp;
        // update
        p->height = node_height(p);
        tp->height = node_height(tp);
        tp2->height = node_height(tp2);
        p->max = node_max(p);
        tp->max = node_max(tp);
        tp2->max = node_max(tp2);
        return tp2;
    }

    Node *node_leftmost(Node* node) {
        while (!node->left->is_null)
            node = node->left;
        return node;
    }
    Node *node_rightmost(Node* node) {
        while (!node->right->is_null)
            node = node->right;
        return node;
    }

private:
    size_t dim;
    Node *root;
};

