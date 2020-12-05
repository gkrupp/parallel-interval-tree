#pragma once

#include <cstddef>
#include <cmath>
#include <array>
#include <algorithm>
#include <iostream>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <atomic>
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
            : begin(begin), end(end), max(end), multip(1), is_null(false), left(left), right(right) {}

        ParallelIntervalTreeNode(const P &begin, const P &end) 
            : ParallelIntervalTreeNode(begin, end, new ParallelIntervalTreeNode(), new ParallelIntervalTreeNode()) {}

        ParallelIntervalTreeNode(const Interval &I, ParallelIntervalTreeNode* left, ParallelIntervalTreeNode *right)
            : ParallelIntervalTreeNode(I.begin, I.end, left, right) {}

        ParallelIntervalTreeNode(const Interval &I) 
            : ParallelIntervalTreeNode(I.begin, I.end, new ParallelIntervalTreeNode(), new ParallelIntervalTreeNode()) {}

        ParallelIntervalTreeNode() : is_null(true) {}

        // Lock for read or write locking current node
        ReadWriteLock rw_lock;
        P begin;
        P end;
        P max;
        size_t multip;
        bool is_null;
        ParallelIntervalTreeNode *left, *right;

        ~ParallelIntervalTreeNode() {
            if (!is_null) {
                left->rw_lock.lock_write();
                delete left;

                right->rw_lock.lock_write();
                delete right;
            }

            // Unlock the mutex of this node, because destructing a mutex while it is locked is UB.
            rw_lock.unlock_write();
        }
    };
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
    ParallelIntervalTree(const size_t dim) : node_count(0), inserts_until_rebalance(2), dim(dim), root(new Node()) {}
    ParallelIntervalTree() : ParallelIntervalTree(1) {}

    void insert(const I &interval) { insert(interval.begin, interval.end); }
    void insert(const P &begin, const P &end) { node_insert(begin, end); }

    // TODO: use locks in remove
    void remove(const I &interval) { remove(interval.begin, interval.end); }
    void remove(const P &begin, const P &end) { root->rw_lock.lock_write(); node_remove(root, begin, end); }

    size_t query(const P &p) { root->rw_lock.lock_read(); return node_query(root, p); }

    // 1D print
    void print() {
        node_print(root);
    }
    void node_print(Node *node) {
        if (!node->is_null) {
            std::cout << "("; node_print(node->left);
            std::cout << "," << node->begin[0] << "-" << node->end[0]  << "-" << node->max[0] << ",";
            node_print(node->right); std::cout << ")";
        } else {
            //std::cout << "[-]\n";
        }
    }

    ~ParallelIntervalTree() {
        root->rw_lock.lock_write();
        delete root;
    }
private:
    ReadWriteLock rw_lock;
    size_t node_count;
    size_t inserts_until_rebalance;


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

    // SOURCE: http://www.geekviewpoint.com/java/bst/dsw_algorithm
    void rebalance() {
        // Get write lock for all nodes
        lock_all(root);

        if (!root->is_null) {
            make_vine();
            balance_vine();

            // Recalculate max values
            update_max(root);
        }

        // Finally, unlock all nodes
        unlock_all(root);
    }

    void make_vine() {
        Node* gp = nullptr; // nullptr can be used here, we won't lock anything during rebalance.
        Node* p = root;
        Node* left;
        while (!p->is_null) {
            left = p->left;
            if (!left->is_null) {
                gp = rotate_right(gp, p, left);
                p = left;
            }
            else {
                gp = p;
                p = p->right;
            }
        }
    }

    Node* rotate_right(Node* gp, Node* p, Node* c) {
        if (gp == nullptr) {
            root = c;
        }
        else {
            gp->right = c;
        }
        p->left = c->right;
        c->right = p;
        return gp;
    }

    void rotate_left(Node* gp, Node* p, Node* c) {
        if (gp == nullptr) {
            root = c;
        }
        else {
            gp->right = c;
        }
        p->right = c->left;
        c->left = p;
    }

    void balance_vine() {
        size_t n = node_count;
        size_t m = greatest_power_of_two_lt(n) - 1;
        std::cout <<  n << " " << m << std::endl;
        make_rotations(n - m);
        while (m > 1) {
            m /= 2;
            make_rotations(m);
        }
    }

    size_t greatest_power_of_two_lt(size_t n) {
        size_t msb = 0;
        while (n > 1) {
            n >>= 1;
            ++msb;
        }
        return 1 << msb;
    }

    void make_rotations(size_t bound) {
        Node* gp = nullptr;
        Node* p = root;
        Node* c = root->right;
        for (; bound > 0; --bound) {
            try {
                if (c->is_null) {
                    break;
                }
                rotate_left(gp, p, c);
                gp = c;
                p = gp->right;
                c = p->right;
            }
            catch (const std::exception& e) {
                std::cout << "Exception caught while rebalancing tree: " << e.what() << std::endl;
                break;
            }
        }
    }

    P update_max(Node* node) {
        if (node->left->is_null && node->right->is_null) {
            node->max = node->end;
        }
        else if (node->left->is_null) {
            node->max = max(node->end, update_max(node->right));
        }
        else if (node->right->is_null) {
            node->max = max(node->end, update_max(node->left));
        }
        else {
            node->max = max(node->end, max(update_max(node->left), update_max(node->right)));
        }
        return node->max;
    }

    void lock_all(Node* node) {
        node->rw_lock.lock_write();
        if (!node->is_null) {
            lock_all(node->left);
            lock_all(node->right);
        }
    }

    void unlock_all(Node* node) {
        node->rw_lock.unlock_write();
        if (!node->is_null) {
            unlock_all(node->left);
            unlock_all(node->right);
        }
    }

    void node_insert(const P &begin, const P &end) {
        root->rw_lock.lock_write();
        if (root->is_null) {
            delete root;
            root = new Node(begin, end);
        }
        else {
            node_insert(root, begin, end, 1);
        }

        rw_lock.scoped_lock_write();
        node_count++;
        if (inserts_until_rebalance > 0) {
            inserts_until_rebalance--;
        }
        else {
            rebalance();
            inserts_until_rebalance = max(static_cast<size_t>(2), static_cast<size_t>(floor(sqrt(node_count))));
        }
    }

    size_t node_insert(Node *node, const P &begin, const P &end, size_t height) {
        // node must already be write locked
        // Before return, node must be write unlocked
        // node should never be null

        // Does not rebalance the tree!

        // The new interval will be inserted in this subtree, so update max, while going down.
        // Any operation coming from above this insert cannot overtake, so from their point of view the tree is consistent.
        node->max = max(node->max, end);

        if (begin <= node->begin) {
            if (begin==node->begin && end==node->end) {
                node->multip += 1;
                node->rw_lock.unlock_write();
                return height;
            }

            // Locking left, then unlocking current node before returning
            node->left->rw_lock.lock_write();
            if (node->left->is_null) {
                delete node->left;
                node->left = new Node(begin, end);
                node->rw_lock.unlock_write();
                return height + 1;
            }
            else {
                Node* left = node->left;
                node->rw_lock.unlock_write();
                height = node_insert(left, begin, end, height+1);
                return height;
            }
        }
        else {
            // Locking right, then unlocking current node before returning
            node->right->rw_lock.lock_write();
            if (node->right->is_null) {
                delete node->right;
                node->right = new Node(begin, end);
                node->rw_lock.unlock_write();
                return height + 1;
            }
            else {
                Node* right = node->right;
                node->rw_lock.unlock_write();
                height = node_insert(right, begin, end, height+1);
                return height;
            }
        }
    }

    Node *node_remove(Node *node, const P &begin, const P &end) {
        return node_remove(node, begin, end, 1);
    }
    Node *node_remove(Node *node, const P &begin, const P &end, const size_t multip) {
        if (node->is_null) {
            return new Node();
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
                    return new Node();
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

        if (p < node->begin) {
            // Locking child first, then unlocking current node
            node->left->rw_lock.lock_read();
            Node* left = node->left;
            node->rw_lock.unlock_read();
            return node_query(left, p);
        } 
        else if (p < node->max) {
            size_t subquery = p < node->end ? node->multip : 0;
            // Locking both children for subquery, then unlocking current node
            node->left->rw_lock.lock_read();
            node->right->rw_lock.lock_read();
            Node* right = node->right;
            Node* left = node->left;
            node->rw_lock.unlock_read();
            subquery += node_query(left, p);
            subquery += node_query(right, p);
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

