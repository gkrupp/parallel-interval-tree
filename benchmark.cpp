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




// DATA GENERATION

typedef int TYP;
size_t dim = 1;
Data<TYP>               DAT_EMPTY(0,   0,   0, dim, 0,   1,    0,    0);
Data<TYP>              DAT_INSERT(1E4, 0, 100, dim, 0,   1,    0,    0);
Data<TYP>       DAT_INSERT_REMOVE(1E4, 0, 100, dim, 0,   0.7,  0.25, 0.05);
Data<TYP> DAT_INSERT_QUERY_REMOVE(1E4, 0, 100, dim, 0.8, 0.15, 0.04, 0.01);
Data<TYP>               DAT_QUERY(1E4, 0, 100, dim, 1,   0,    0,    0);

void threadFunc(ParallelIntervalTree<TYP> &pt, const Data<TYP>& DAT, const size_t offset, const size_t step) {
    for (size_t i = offset; i < DAT.tsks.size(); i += step) {
        auto& tsk = DAT.tsks[i];
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

template <class THnum>
static void BM_Parallel(benchmark::State& state, const bool prepare, const Data<TYP>& PRE, const Data<TYP>& DAT, const THnum threads) {
    if (prepare) {
        ParallelIntervalTree<int> pt;
        threadFunc(pt, PRE, 0, 1);
        for (auto _ : state) {
            std::vector<std::thread> ths;
            for (size_t i = 0; i < threads; ++i)
                ths.emplace_back(threadFunc, std::ref(pt), std::ref(DAT), i, threads);
            for (size_t i = threads; i < 4; ++i)
                ths.emplace_back(dummyThread);
            for (size_t i = 0; i < ths.size(); ++i)
                ths[i].join();
        }
    } else {
        for (auto _ : state) {
            ParallelIntervalTree<int> pt;
            std::vector<std::thread> ths;
            for (size_t i = 0; i < threads; ++i)
                ths.emplace_back(threadFunc, std::ref(pt), std::ref(DAT), i, threads);
            for (size_t i = 0; i < threads; ++i)
                ths[i].join();
        }
    }
}


BENCHMARK_CAPTURE(BM_Parallel, Insert/TH1, false, std::ref(DAT_EMPTY), std::ref(DAT_INSERT), 1)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_Parallel, Insert/TH2, false, std::ref(DAT_EMPTY), std::ref(DAT_INSERT), 2)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_Parallel, Insert/TH3, false, std::ref(DAT_EMPTY), std::ref(DAT_INSERT), 3)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_Parallel, Insert/TH4, false, std::ref(DAT_EMPTY), std::ref(DAT_INSERT), 4)->Unit(benchmark::kMillisecond);

BENCHMARK_CAPTURE(BM_Parallel, InsertRemove/TH1, false, std::ref(DAT_EMPTY), std::ref(DAT_INSERT_REMOVE), 1)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_Parallel, InsertRemove/TH2, false, std::ref(DAT_EMPTY), std::ref(DAT_INSERT_REMOVE), 2)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_Parallel, InsertRemove/TH3, false, std::ref(DAT_EMPTY), std::ref(DAT_INSERT_REMOVE), 3)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_Parallel, InsertRemove/TH4, false, std::ref(DAT_EMPTY), std::ref(DAT_INSERT_REMOVE), 4)->Unit(benchmark::kMillisecond);

BENCHMARK_CAPTURE(BM_Parallel, Query/TH1, true, std::ref(DAT_INSERT), std::ref(DAT_QUERY), 1)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_Parallel, Query/TH2, true, std::ref(DAT_INSERT), std::ref(DAT_QUERY), 2)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_Parallel, Query/TH3, true, std::ref(DAT_INSERT), std::ref(DAT_QUERY), 3)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_Parallel, Query/TH4, true, std::ref(DAT_INSERT), std::ref(DAT_QUERY), 4)->Unit(benchmark::kMillisecond);

BENCHMARK_CAPTURE(BM_Parallel, InsertQueryRemove/TH1, false, std::ref(DAT_EMPTY), std::ref(DAT_INSERT_QUERY_REMOVE), 1)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_Parallel, InsertQueryRemove/TH2, false, std::ref(DAT_EMPTY), std::ref(DAT_INSERT_QUERY_REMOVE), 2)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_Parallel, InsertQueryRemove/TH3, false, std::ref(DAT_EMPTY), std::ref(DAT_INSERT_QUERY_REMOVE), 3)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(BM_Parallel, InsertQueryRemove/TH4, false, std::ref(DAT_EMPTY), std::ref(DAT_INSERT_QUERY_REMOVE), 4)->Unit(benchmark::kMillisecond);


BENCHMARK_MAIN();

