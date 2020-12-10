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
            //std::cout << " query " << tsk.a[0] << '\n';
            break;
        case DAT.INSERT:
            pt.insert(tsk.a, tsk.b);
            //std::cout << " insert " << tsk.a[0] << ' ' << tsk.b[0] << '\n';
            break;
        case DAT.REMOVE:
            pt.remove(tsk.a, tsk.b);
            //std::cout << " remove " << tsk.a[0] << ' ' << tsk.b[0] << '\n';
            break;
        }
    }
}



int main() {
    {
        ParallelIntervalTree<int> pt;
        std::cout << "DAT_INSERT" << std::endl;
        threadFunc(std::ref(pt), std::ref(DAT_INSERT), 0, 1);
    }
    {
        ParallelIntervalTree<int> pt;
        std::cout << "DAT_INSERT_REMOVE" << std::endl;
        threadFunc(std::ref(pt), std::ref(DAT_INSERT_REMOVE), 0, 1);
    }
    {
        ParallelIntervalTree<int> pt;
        std::cout << "DAT_INSERT_QUERY_REMOVE" << std::endl;
        threadFunc(std::ref(pt), std::ref(DAT_INSERT_QUERY_REMOVE), 0, 1);
    }
    {
        ParallelIntervalTree<int> pt;
        std::cout << "DAT_INSERT + DAT_QUERY" << std::endl;
        threadFunc(std::ref(pt), std::ref(DAT_INSERT), 0, 1);
        threadFunc(std::ref(pt), std::ref(DAT_QUERY), 0, 1);
    }
}