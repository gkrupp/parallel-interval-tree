#pragma once

#include <cstddef>
#include <array>
#include <algorithm>
#include <iostream>
#include <vector>


template <typename T>
T &max(T&& a, T&& b) {
    if (a < b) return b;
    else return a;
}


template <typename T>
class Point : public std::vector<T> {
public:
    Point(const T &p1) : std::vector<T>({p1}), dim(1) { }
    Point(std::initializer_list<T> il) : std::vector<T>(il), dim(il.size()) {}
    Point(const Point<T> &p) : std::vector<T>(p), dim(p.dim) {}
    Point() {}

    Point<T>& operator=(const Point<T>& p) {
        std::vector<T>::operator=(p);
        dim = p.dim;
        return *this;
    }
public:
    size_t dim;
};


template <typename T>
class Interval {
public:
    typedef T value_t;
    typedef Point<T> P;
    Interval(const P &begin, const P &end) : begin(begin), end(end), dim(max(begin.dim,end.dim)) {}

    bool operator<(const Interval<T> &other) const {
        if (begin < other.begin) return true;
        else if (other.begin < begin) return false;
        else return (end < other.end);
    }
    
public:
    size_t dim;
    P begin;
    P end;
};


template <class Interval, typename T = typename Interval::value_t>
class IntervalTreeNode {
public:
    typedef T value_t;
    typedef Point<T> P;
    typedef Interval I;
    IntervalTreeNode(const P &begin, const P &end, IntervalTreeNode* left, IntervalTreeNode *right) : begin(begin), end(end), max(end), multip(1), height(1), left(left), right(right) {}
    IntervalTreeNode(const P &begin, const P &end) : IntervalTreeNode(begin, end, nullptr, nullptr) {}
    IntervalTreeNode(const Interval &I, IntervalTreeNode* left, IntervalTreeNode *right) : IntervalTreeNode(I.begin, I.end, left, right) {}
    IntervalTreeNode(const Interval &I) : IntervalTreeNode(I.begin, I.end, nullptr, nullptr) {}
    ~IntervalTreeNode() {
        if (left != nullptr) {
            delete left; 
        }
        if (right != nullptr) {
            delete right;
        }
    }
public:
    P begin;
    P end;
    P max;
    size_t multip;
    int height;
    IntervalTreeNode *left, *right;
};




// NOTE: https://www.guru99.com/avl-tree.html
// NOTE: http://www.davismol.net/2016/02/07/data-structures-augmented-interval-tree-to-search-for-interval-overlapping/

template <typename T>
class IntervalTree {
public:
    typedef T value_t;
    typedef Point<T> P;
    typedef Interval<T> I;
    typedef IntervalTreeNode<Interval<T>> Node;
    IntervalTree(const size_t dim) : dim(dim) {}
    IntervalTree() : IntervalTree(1) {}

    void insert(const I &interval) { insert(interval.begin, interval.end); }
    void insert(const P &begin, const P &end) { root = node_insert(root, begin, end); }

    void remove(const I &interval) { remove(interval.begin, interval.end); }
    void remove(const P &begin, const P &end) { root = node_remove(root, begin, end); }

    size_t query(const P &p) { return node_query(root, p); }

    // 1D print
    void print() {
        node_print(root);
    }
    void node_print(Node *node) {
        if (node != nullptr) {
            std::cout << "("; node_print(node->left);
            std::cout << "," << node->begin[0] << "-" << node->end[0] << "-" << node->max[0] << "-" << node->height << ",";
            node_print(node->right); std::cout << ")";
        } else {
            //std::cout << "[-]\n";
        }
    }

    ~IntervalTree() {
        if (root != nullptr) {
            delete root; 
        }
    }

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
        if (node == nullptr) {
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
        if (node == nullptr) {
            return nullptr;
        }

        if (begin <= node->begin) {
            if (begin==node->begin && end==node->end) {
                if (node->multip > multip) {
                    node->multip -= multip;
                    return node;
                }
                if (node->left != nullptr) {
                    Node *up = node_rightmost(node->left);
                    node->begin = up->begin;
                    node->end = up->end;
                    node->multip = up->multip;
                    node->left = node_remove(node->left, up->begin, up->end, up->multip);
                } else if (node->right != nullptr) {
                    Node *up = node_leftmost(node->right);
                    node->begin = up->begin;
                    node->end = up->end;
                    node->multip = up->multip;
                    node->right = node_remove(node->right, up->begin, up->end, up->multip);
                } else {
                    delete node;
                    return nullptr;
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
        if (node == nullptr) {
            return 0;
        }
        //std::cout << node->begin[0] << ' ' << node->end[0] << ' ' << p[0] << std::endl;
        if (p < node->begin) {
            return node_query(node->left, p);
        } else if (p < node->max) {
            size_t subquery = node_query(node->left, p) + node_query(node->right, p);
            if (p < node->end) return subquery + node->multip;
            else return subquery;
        } else {
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
        while (node->left != nullptr)
            node = node->left;
        return node;
    }
    Node *node_rightmost(Node* node) {
        while (node->right != nullptr)
            node = node->right;
        return node;
    }

private:
    size_t dim;
    Node *root = nullptr;
};

