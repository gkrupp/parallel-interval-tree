#include <iostream>
#include "it.hpp"
#include "pit.hpp"

using namespace std;

int main() {
    
    IntervalTree<int> t;
    t.insert(5,10);
    t.insert(15,25);
    t.insert(1,12);
    t.insert(8,16);
    t.insert(14,20);
    t.insert(18,21);
    t.insert(2,8);

    t.remove(15,25);

    t.print();
    cout << endl;

    //cout << endl << "-----------------------------------" << endl << endl;

    ParallelIntervalTree<int> pt;
    pt.insert(5,10);
    pt.insert(15,25);
    pt.insert(1,12);
    pt.insert(8,16);
    pt.insert(14,20);
    pt.insert(18,21);
    pt.insert(2,8);

    pt.remove(15,25);

    pt.print();
    cout << endl;

    for (int i = 0; i < 30; ++i) {
        cout << i << ' ' << t.query(i) << ' ' << pt.query(i) << endl;
    }


}