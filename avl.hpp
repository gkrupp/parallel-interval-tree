#pragma once

#include <cstddef>
#include <algorithm>
#include <iostream>

using namespace std;


template<typename K, typename D>
class AVL_tree_node {

public:
    typedef K key_t;
    typedef D data_t;
    AVL_tree_node(const K &key, const D &data, AVL_tree_node* left, AVL_tree_node *right) : key(key), height(1), data(data), left(left), right(right) {}
    AVL_tree_node(const K &key, const D &data) : AVL_tree_node(key, data, nullptr, nullptr) {}

public:
    K key;
    D data;
    size_t height;
    AVL_tree_node *left, *right;
};




// Based on: https://www.guru99.com/avl-tree.html

template <class Node, typename K = typename Node::key_t, typename D = typename Node::data_t>
class AVL_tree {

public:
    AVL_tree() {}

    bool insert(const K key, const D data) {
        root = node_insert(root, key, data);
        return true;
    }
    bool insert(const K key) {
        //cout << "insert " << key << endl;
        return insert(key, key);
    }
    bool remove(const K key) {
        root = node_remove(root, key);
        return true;
    }

    void print() {
        node_print(root);
    }
    void node_print(Node *node) {
        if (node != nullptr) {
            cout << "("; node_print(node->left);
            cout << "," << node->key << ",";
            node_print(node->right); cout << ")";
        } else {
            //cout << "[-]\n";
        }
    }


private:

    size_t node_height(Node *node) {
        if (node->left && node->right) {
            return max(node->left->height, node->right->height) +1 ;
        } else if (node->left) {
            return node->left->height + 1;
        } else if (node->right) {
            return node->right->height + 1;
        } else {
            return 1;
        }
    }
    size_t node_bf(Node *node) {
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

    Node *node_insert(Node *node, const K &key, const D &data) {
        if (node == nullptr) {
            return new Node(key, data);
        } else {
            if (key <= node->key) {
                node->left = node_insert(node->left, key, data);
            } else {
                node->right = node_insert(node->right, key, data);
            }
        }

        node->height = node_height(node);
        //cout << node->key << " " << node-> height << " --- ";

        if      (node_bf(node)== 2 && node_bf(node->left)==  1) { node = node_llrotation(node); }
        else if (node_bf(node)==-2 && node_bf(node->right)==-1) { node = node_rrrotation(node); }
        else if (node_bf(node)==-2 && node_bf(node->right)== 1) { node = node_rlrotation(node); }
        else if (node_bf(node)== 2 && node_bf(node->left)== -1) { node = node_lrrotation(node); }

        //cout << node->key << " " << node-> height << endl;

        return node;

    }
    Node *node_remove(Node *node, const K &key) {
        if (node->left == nullptr && node->right == nullptr) {
            if (node == root) root = nullptr;
            delete node;
            return nullptr;
        }

        Node *t, *q;
        if (node->key < key){
            node->right = node_remove(node->right, key);
        } else if (node->key > key) {
            node->left = node_remove(node->left, key);
        } else {
            if (node->left != nullptr) {
                q = node_rightmost(node->left);
                node->key = q->key;
                node->data = q->data;
                node->left = node_remove(node->left, q->data);
            } else {
                q = node_leftmost(node->right);
                node->key = q->key;
                node->data = q->data;
                node->right = node_remove(node->right, q->data);
            }
        }

        if      (node_bf(node)== 2 && node_bf(node->left)==  1) { node = node_llrotation(node); }                  
        else if (node_bf(node)== 2 && node_bf(node->left)==- 1) { node = node_lrrotation(node); }
        else if (node_bf(node)== 2 && node_bf(node->left)==  0) { node = node_llrotation(node); }
        else if (node_bf(node)==-2 && node_bf(node->right)==-1) { node = node_rrrotation(node); }
        else if (node_bf(node)==-2 && node_bf(node->right)== 1) { node = node_rlrotation(node); }
        else if (node_bf(node)==-2 && node_bf(node->right)== 0) { node = node_llrotation(node); }

        return node;
    }

    Node *node_llrotation(Node *node) {
        //cout << "LL\n";
        Node *p = node, *tp = p->left;
        p->left = tp->right;
        tp->right = p;
        p->height--;
        return tp; 
    }
    Node *node_rrrotation(Node *node) {
        //cout << "RR\n";
        Node *p = node, *tp = p->right;
        p->right = tp->left;
        tp->left = p;
        p->height--;
        return tp; 
    }
    Node *node_rlrotation(Node *node) {
        //cout << "RL\n";
        Node *p = node, *tp = p->right, *tp2 = p->right->left;
        p->right = tp2->left;
        tp->left = tp2->right;
        tp2->right = tp;
        tp2->left = p;
        p->height--;
        tp2->height++;
        return tp2; 
    }
    Node *node_lrrotation(Node *node) {
        //cout << "LR\n";
        Node *p = node, *tp = p->left, *tp2 = p->left->right;
        p->left = tp2->right;
        tp->right = tp2->left;
        tp2->right = p;
        tp2->left = tp;
        p->height--;
        tp2->height++;
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
    Node *root = nullptr;
};