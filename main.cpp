#include <iostream>
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

    //t.remove(15,25);

    t.print();
    cout << endl;

    for (int i = 0; i < 30; ++i) {
        cout << i << ' ' << t.quiery(i) << endl;
    }


}