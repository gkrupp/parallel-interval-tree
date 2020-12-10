#include <benchmark/benchmark.h>
#include <cstddef>
#include <random>
#include <thread>
#include <vector>
#include <algorithm>
#include <set>

#include "it.hpp"
#include "pit.hpp"
#include "datagen.hpp"



static void BM_FixedInsert_SingleThread(benchmark::State& state) {
    for (auto _ : state) {
        IntervalTree<int> t;
        t.insert(5,10);
        t.insert(15,25);
        t.insert(1,12);
        t.insert(8,16);
        t.insert(14,20);
        t.insert(18,21);
        t.remove(15,25);
        t.insert(2,8);
    }
}
BENCHMARK(BM_FixedInsert_SingleThread)->Unit(benchmark::kMillisecond);


void dummyThread() {}
static void BM_FixedInsert_SingleThread_ThreadCompensated(benchmark::State& state) {
    for (auto _ : state) {
        std::thread th1(dummyThread);
        std::thread th2(dummyThread);
        std::thread th3(dummyThread);
        std::thread th4(dummyThread);
        std::thread th5(dummyThread);
        std::thread th6(dummyThread);
        std::thread th7(dummyThread);
        std::thread th8(dummyThread);
        th1.join();
        th2.join();
        th3.join();
        th4.join();
        th5.join();
        th6.join();
        th7.join();
        th8.join();
        IntervalTree<int> t;
        t.insert(5,10);
        t.insert(15,25);
        t.insert(1,12);
        t.insert(8,16);
        t.insert(14,20);
        t.insert(18,21);
        t.remove(15,25);
        t.insert(2,8);
    }
}
BENCHMARK(BM_FixedInsert_SingleThread_ThreadCompensated)->Unit(benchmark::kMillisecond);


static void BM_FixedInsert_MultipleThreads(benchmark::State& state) {
    void (ParallelIntervalTree<int>::*insertFunc)(const ParallelIntervalTree<int>::P&, const ParallelIntervalTree<int>::P&) = &ParallelIntervalTree<int>::insert;
    void (ParallelIntervalTree<int>::*removeFunc)(const ParallelIntervalTree<int>::P&, const ParallelIntervalTree<int>::P&) = &ParallelIntervalTree<int>::remove;
    size_t (ParallelIntervalTree<int>::*queryFunc)(const ParallelIntervalTree<int>::P&) const = &ParallelIntervalTree<int>::query;
    for (auto _ : state) {
        ParallelIntervalTree<int> pt;
        std::thread th1(insertFunc, std::ref(pt), 5, 10);
        std::thread th2(insertFunc, std::ref(pt), 15, 25);
        std::thread th3(insertFunc, std::ref(pt), 1, 12);
        std::thread th4(insertFunc, std::ref(pt), 8, 16);
        std::thread th5(insertFunc, std::ref(pt), 14, 20);
        std::thread th6(insertFunc, std::ref(pt), 18, 21);
        std::thread th7(removeFunc, std::ref(pt), 15, 25);
        std::thread th8(insertFunc, std::ref(pt), 2, 8);
        th1.join();
        th2.join();
        th3.join();
        th4.join();
        th5.join();
        th6.join();
        th7.join();
        th8.join();
    }
}
BENCHMARK(BM_FixedInsert_MultipleThreads)->Unit(benchmark::kMillisecond);






typedef int TYP;
Data<int> DAT(20000, 0, 100);

void threadFunc(ParallelIntervalTree<TYP> &pt, const size_t offset, const size_t step) {
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



static void BM_Parallel_Threads_1(benchmark::State& state) {
    for (auto _ : state) {
        ParallelIntervalTree<int> pt;
        threadFunc(pt, 0, 1);
        //std::thread th1(threadFunc, std::ref(pt), 0, 1);
        /*std::thread th2(dummyThread);
        std::thread th3(dummyThread);
        std::thread th4(dummyThread);*/
        //th1.join();
        /*th2.join();
        th3.join();
        th4.join();*/
    }
}
BENCHMARK(BM_Parallel_Threads_1)->Unit(benchmark::kMillisecond);


static void BM_Parallel_Threads_2(benchmark::State& state) {
    for (auto _ : state) {
        ParallelIntervalTree<int> pt;
        std::thread th1(threadFunc, std::ref(pt), 0, 2);
        std::thread th2(threadFunc, std::ref(pt), 1, 2);
        std::thread th3(dummyThread);
        std::thread th4(dummyThread);
        th1.join();
        th2.join();
        th3.join();
        th4.join();
    }
}
BENCHMARK(BM_Parallel_Threads_2)->Unit(benchmark::kMillisecond);


static void BM_Parallel_Threads_3(benchmark::State& state) {
    for (auto _ : state) {
        ParallelIntervalTree<int> pt;
        std::thread th1(threadFunc, std::ref(pt), 0, 3);
        std::thread th2(threadFunc, std::ref(pt), 1, 3);
        std::thread th3(threadFunc, std::ref(pt), 2, 3);
        std::thread th4(dummyThread);
        th1.join();
        th2.join();
        th3.join();
        th4.join();
    }
}
BENCHMARK(BM_Parallel_Threads_3)->Unit(benchmark::kMillisecond);


static void BM_Parallel_Threads_4(benchmark::State& state) {
    for (auto _ : state) {
        ParallelIntervalTree<int> pt;
        std::thread th1(threadFunc, std::ref(pt), 0, 4);
        std::thread th2(threadFunc, std::ref(pt), 1, 4);
        std::thread th3(threadFunc, std::ref(pt), 2, 4);
        std::thread th4(threadFunc, std::ref(pt), 3, 4);
        th1.join();
        th2.join();
        th3.join();
        th4.join();
    }
}
BENCHMARK(BM_Parallel_Threads_4)->Unit(benchmark::kMillisecond);




BENCHMARK_MAIN();

