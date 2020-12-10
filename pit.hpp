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

        ParallelIntervalTreeNode()
            : is_null(true) {}

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
    ParallelIntervalTree(const size_t dim) : node_count(0), ops_until_rebalance(2), dim(dim), root(new Node()) {}
    ParallelIntervalTree() : ParallelIntervalTree(1) {}

    void insert(const I &interval) { insert(interval.begin, interval.end); }
    void insert(const P &begin, const P &end) { node_insert(begin, end); }

    void remove(const I &interval) { remove(interval.begin, interval.end); }
    void remove(const P &begin, const P &end) { 
        node_remove(begin, end);
    }

    size_t query(const P &p) const { return node_query(p); }

    // 1D print
    void print() const { node_print(root); }

    ~ParallelIntervalTree() {
        rw_lock.lock_write();
        root->rw_lock.lock_write();
        delete root;
        rw_lock.unlock_write();
    }
private:
    ReadWriteLock rw_lock;
    size_t node_count;
    size_t ops_until_rebalance;

    void node_print(Node *node) const {
        if (!node->is_null) {
            std::cout << "("; node_print(node->left);
            std::cout << "," << node->begin[0] << "-" << node->end[0]  << "-" << node->max[0] << ",";
            node_print(node->right); std::cout << ")";
        }
    }

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
        if (p->is_null) return;
        Node* c = root->right;
        for (; bound > 0; --bound) {
            if (c->is_null) break;
            rotate_left(gp, p, c);
            gp = c;
            if (gp->is_null) break;
            p = gp->right;
            if (p->is_null) break;
            c = p->right;
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

    void check_rebalance(int op_cost, int count_change) {
        rw_lock.lock_write();
        node_count += count_change;
        if (ops_until_rebalance > static_cast<size_t>(op_cost)) {
            ops_until_rebalance -= op_cost;
        }
        else if (node_count > 2) {
            rebalance();
            ops_until_rebalance = max(static_cast<size_t>(3), static_cast<size_t>(floor(sqrt(node_count))));
        }
        rw_lock.unlock_write();
    }

    void node_insert(const P &begin, const P &end) {
        rw_lock.lock_write();
        root->rw_lock.lock_write();
        int change = 0;
        if (root->is_null) {
            delete root;
            root = new Node(begin, end);
            change = 1;
            rw_lock.unlock_write();
        }
        else {
            rw_lock.unlock_write();
            node_insert(root, begin, end, change);
        }

        check_rebalance(change, change);
    }

    void node_insert(Node *node, const P &begin, const P &end, int& change) {
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
                change = 0;
                return;
            }

            // Locking left, then unlocking current node before returning
            node->left->rw_lock.lock_write();
            if (node->left->is_null) {
                delete node->left;
                node->left = new Node(begin, end);
                node->rw_lock.unlock_write();
                change = 1;
                return;
            }
            else {
                Node* left = node->left;
                node->rw_lock.unlock_write();
                node_insert(left, begin, end, change);
                return;
            }
        }
        else {
            // Locking right, then unlocking current node before returning
            node->right->rw_lock.lock_write();
            if (node->right->is_null) {
                delete node->right;
                node->right = new Node(begin, end);
                node->rw_lock.unlock_write();
                change = 1;
                return;
            }
            else {
                Node* right = node->right;
                node->rw_lock.unlock_write();
                node_insert(right, begin, end, change);
                return;
            }
        }
    }

    void node_remove(const P &begin, const P &end) {
        rw_lock.lock_write();
        root->rw_lock.lock_write();
        int change = 0;
        node_remove(root, begin, end, change, true);

        check_rebalance(-change, change);
    }

    void node_remove(Node *node, const P &begin, const P &end, int& change, const bool is_root = false) {
        // node must be write locked,
        // must unlock node before returning.

        // Does not maintain the max value of a node (that requires a rebalance)
        // Will write locke the entire subtree under the removed element.
        
        if (node->is_null) {
            node->rw_lock.unlock_write();
            if (is_root) rw_lock.unlock_write();
            return;
        }
        
        if (begin <= node->begin) {
            if (begin==node->begin && end==node->end) {
                if (node->multip > 1) {
                    node->multip -= 1;
                    node->rw_lock.unlock_write();
                    if (is_root) rw_lock.unlock_write();
                    change = 0;
                    return;
                }
                node->left->rw_lock.lock_write();
                change = -1;
                if (!node->left->is_null) {
                    Node *up = node_remove_rightmost(node);
                    node->begin = up->begin;
                    node->end = up->end;
                    node->multip = up->multip;
                    delete up;
                    node->rw_lock.unlock_write();
                    if (is_root) rw_lock.unlock_write();
                    return;
                } 
                else {
                    node->right->rw_lock.lock_write();
                    node->left->rw_lock.unlock_write();
                    if (!node->right->is_null) {
                        Node *up = node_remove_leftmost(node);
                        node->begin = up->begin;
                        node->end = up->end;
                        node->multip = up->multip;
                        delete up;
                        node->rw_lock.unlock_write();
                        if (is_root) rw_lock.unlock_write();
                        return;
                    } 
                    else {
                        node->is_null = true;
                        node->left->rw_lock.lock_write();
                        delete node->left;
                        delete node->right;
                        node->rw_lock.unlock_write();
                        if (is_root) rw_lock.unlock_write();
                        return;
                    }
                }
            } 
            else {
                node->left->rw_lock.lock_write();
                Node* left = node->left;
                node->rw_lock.unlock_write();
                if (is_root) rw_lock.unlock_write();
                node_remove(left, begin, end, change);
                return;
            }
        } 
        else {
            node->right->rw_lock.lock_write();
            Node* right = node->right;
            node->rw_lock.unlock_write();
            if (is_root) rw_lock.unlock_write();
            node_remove(right, begin, end, change);
            return;
        }
    }

    size_t node_query(const P &p) const {
        rw_lock.lock_read();
        root->rw_lock.lock_read();
        return node_query(root, p, true);
    }

    size_t node_query(const Node *node, const P &p, const bool is_root = false) const {
        // node must already be read locked!
        // Before return, node must be read unlocked!
        if (node->is_null) {
            node->rw_lock.unlock_read();
            if (is_root) rw_lock.unlock_read();
            return 0;
        }

        if (p < node->begin) {
            // Locking child first, then unlocking current node
            Node* left = node->left;
            left->rw_lock.lock_read();
            node->rw_lock.unlock_read();
            if (is_root) rw_lock.unlock_read();
            return node_query(left, p);
        } 
        else if (p < node->max) {
            size_t subquery = p < node->end ? node->multip : 0;
            // Locking both children for subquery, then unlocking current node
            Node* left = node->left;
            Node* right = node->right;
            left->rw_lock.lock_read();
            right->rw_lock.lock_read();
            node->rw_lock.unlock_read();
            if (is_root) rw_lock.unlock_read();
            subquery += node_query(left, p);
            subquery += node_query(right, p);
            return subquery;
        }
        else {
            node->rw_lock.unlock_read();
            if (is_root) rw_lock.unlock_read();
            return 0;
        }
    }

    Node *node_remove_leftmost(Node* deleted_node) {
        // deleted_node and deleted_node->right must be locked;
        // After return deleted_node is still locked, deleted_node->right is unlocked 
        // the returned node is locked, but its children (the null nodes) are not locked.
        Node* child = deleted_node->right;
        child->left->rw_lock.lock_write();
        if (child->left->is_null) {
            child->left->rw_lock.unlock_write();
            deleted_node->right = new Node();
            return child;
        }
        else {
            return node_remove_leftmost(child, child->left);
        }
    }
    Node *node_remove_leftmost(Node* parent, Node* child) {
        // parent and child must be locked, but after return they are unlocked,
        // the returned node is locked (but its children, the null nodes are not)

        child->left->rw_lock.lock_write();
        while (!child->left->is_null) {
            parent->rw_lock.unlock_write();
            parent = child;
            child = child->left;
            child->left->rw_lock.lock_write();
        }

        parent->left = new Node();
        parent->rw_lock.unlock_write();
        child->left->rw_lock.unlock_write();
        return child;
    }

    Node *node_remove_rightmost(Node* deleted_node) {
        // deleted_node and deleted_node->left must be locked;
        // After return deleted_node is still locked, deleted_node->left is unlocked 
        // the returned node is locked, but its children (the null nodes) are not locked.
        Node* child = deleted_node->left;
        child->right->rw_lock.lock_write();
        if (child->right->is_null) {
            child->right->rw_lock.unlock_write();
            deleted_node->left = new Node();
            return child;
        }
        else {
            return node_remove_rightmost(child, child->right);
        }
    }
    Node *node_remove_rightmost(Node* parent, Node* child) {
        // parent and child must be locked, but after return they are unlocked,
        // the returned node is locked (but its children, the null nodes are not)

        child->right->rw_lock.lock_write();
        while (!child->right->is_null) {
            parent->rw_lock.unlock_write();
            parent = child;
            child = child->right;
            child->right->rw_lock.lock_write();
        }

        parent->right = new Node();
        parent->rw_lock.unlock_write();
        child->right->rw_lock.unlock_write();
        return child;
    }

private:
    size_t dim;
    Node *root;
};

