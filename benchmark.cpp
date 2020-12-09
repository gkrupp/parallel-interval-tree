#include <benchmark/benchmark.h>
#include <random>
#include <thread>
#include <vector>

#include "it.hpp"
#include "pit.hpp"


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
BENCHMARK(BM_FixedInsert_MultipleThreads);

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
BENCHMARK(BM_FixedInsert_SingleThread_ThreadCompensated);


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
BENCHMARK(BM_FixedInsert_SingleThread);



/*
static void BM_MultipleInsert_SingleThread(benchmark::State& state) {
    IntervalTree<int> t;
    std::mt19937 gen(1);
    std::uniform_int_distribution<int> dist(std::numeric_limits<int>::min());
    for (auto _ : state) {
        auto a = dist(gen);
        auto b = dist(gen);
        if (a < b) {
            auto s = a;
            a = b;
            b = s;
        }
        t.insert(a, b);
    }
}
BENCHMARK(BM_MultipleInsert_SingleThread);
*/

BENCHMARK_MAIN();