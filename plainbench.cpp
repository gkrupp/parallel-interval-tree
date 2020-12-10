#include <cstddef>
#include <random>
#include <thread>
#include <vector>
#include <algorithm>
#include <set>

#include "it.hpp"
#include "pit.hpp"
#include "datagen.hpp"



// DATA GENERATION
typedef int TYP;
Data<TYP>              DAT_INSERT(1E4, 0, 100, 1, 0,   1,    0,    0);
Data<TYP>       DAT_INSERT_REMOVE(1E4, 0, 100, 1, 0,   0.7,  0.25, 0.05);
Data<TYP> DAT_INSERT_QUERY_REMOVE(1E4, 0, 100, 1, 0.8, 0.15, 0.04, 0.01);
Data<TYP>               DAT_QUERY(1E4, 0, 100, 1, 1,   0,    0,    0);

void threadFunc(ParallelIntervalTree<TYP> &pt, const Data<TYP>& DAT, const size_t offset, const size_t step) {
    for (size_t i = offset; i < DAT.tsks.size(); i += step) {
        auto& tsk = DAT.tsks[i];
        if (i%1000 == 0) std::cout << i << std::endl;
        switch (tsk.method) {
        case DAT.QUERY:
            pt.query(tsk.a);
            break;
        case DAT.INSERT:
            pt.insert(tsk.a, tsk.b);
            break;
        case DAT.REMOVE:
            pt.remove(tsk.a, tsk.b);
            break;
        }
    }
}



int main() {
    ParallelIntervalTree<TYP> pt;
    threadFunc(pt, DAT_INSERT, 0, 1);
    std::thread th1(threadFunc, std::ref(pt), std::ref(DAT_QUERY), 0, 4);
    std::thread th2(threadFunc, std::ref(pt), std::ref(DAT_QUERY), 1, 4);
    std::thread th3(threadFunc, std::ref(pt), std::ref(DAT_QUERY), 2, 4);
    std::thread th4(threadFunc, std::ref(pt), std::ref(DAT_QUERY), 3, 4);
    th1.join();
    th2.join();
    th3.join();
    th4.join();
}

