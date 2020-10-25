#include <iostream>
#include "avl.hpp"

using namespace std;

int main() {
    
    AVL_tree<AVL_tree_node<int,int>> t;
    t.insert(20);
    t.insert(25);
    t.insert(15);
    t.insert(10);
    t.remove(25);
    /*t.insert(30);
    t.insert(5);
    t.insert(35);
    t.insert(67);
    t.insert(43);
    t.insert(21);
    t.insert(10);
    t.insert(89);
    t.insert(38);
    t.insert(69);*/
    t.print();

}