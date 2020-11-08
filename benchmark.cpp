#include <benchmark/benchmark.h>
#include <random>
#include <thread>

#include "pit.hpp"

// Segmentation fault:
static void BM_FixedInsert_MultipleThreads(benchmark::State& state) {
    void (IntervalTree<int>::*insertFunc)(const IntervalTree<int>::P&, const IntervalTree<int>::P&) = &IntervalTree<int>::insert;
    for (auto _ : state) {
        IntervalTree<int> t;
        std::thread th1(insertFunc, std::ref(t), 5, 10);
        std::thread th2(insertFunc, std::ref(t), 15, 25);
        std::thread th3(insertFunc, std::ref(t), 1, 12);
        std::thread th4(insertFunc, std::ref(t), 8, 16);
        std::thread th5(insertFunc, std::ref(t), 14, 20);
        std::thread th6(insertFunc, std::ref(t), 18, 21);
        std::thread th7(insertFunc, std::ref(t), 2, 8);
        th1.join();
        th2.join();
        th3.join();
        th4.join();
        th5.join();
        th6.join();
        th7.join();
        t.print();
    }
}
BENCHMARK(BM_FixedInsert_MultipleThreads);

static void BM_FixedInsert_SingleThread(benchmark::State& state) {
    for (auto _ : state) {
        IntervalTree<int> t;
        t.insert(5,10);
        t.insert(15,25);
        t.insert(1,12);
        t.insert(8,16);
        t.insert(14,20);
        t.insert(18,21);
        t.insert(2,8);
    }
}
BENCHMARK(BM_FixedInsert_SingleThread);


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

BENCHMARK_MAIN();