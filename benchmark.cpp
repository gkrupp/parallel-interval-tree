#include <benchmark/benchmark.h>
#include <cstddef>
#include <random>
#include <thread>
#include <vector>
#include <algorithm>
#include <set>

#include "it.hpp"
#include "pit.hpp"



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

// SRC: https://stackoverflow.com/a/55275707/3967146
template<class T>
using uniform_distribution = 
typename std::conditional<
    std::is_floating_point<T>::value,
    std::uniform_real_distribution<T>,
    typename std::conditional<
        std::is_integral<T>::value,
        std::uniform_int_distribution<T>,
        void
    >::type
>::type;

template<typename T>
class Data {
public:
    enum methods { QUERY, INSERT, REMOVE, NOOP };
    struct task {
        methods method = NOOP;
        Point<T> a, b;
    };
    
    Data(const size_t N, const T a = 0, const T b = 1, const size_t dim = 1, const double qry = 0.8, const double ins = 0.15, const double erm = 0.04, const double rrm = 0.01, const uint32_t seed = 1)
    : N(N), dim(dim), tsks(N), gen(seed), p_dist(a, b), t_dist(0.0, 1.0) {
        generate(N, qry, ins, erm, rrm);
    }

    void generate(const size_t N, const double qry, const double ins, const double erm, const double rrm) {
        tsks.resize(N);
        size_t removable_limit = N*(erm+rrm);
        std::set<Interval<T>> removable;
        for (size_t i = 0; i < N; ++i) {
            double tsk = t_dist(gen);
            if (tsk < qry) {
                // query
                tsks[i].method = QUERY;
                tsks[i].a = generate_point();

            } else if (tsk < qry + ins) {
                // insert
                Interval<T> iv = generate_interval();
                tsks[i].method = INSERT;
                tsks[i].a = iv.begin;
                tsks[i].b = iv.end;
                if (tsk < qry + ins + erm && removable.size() < removable_limit) {
                    removable.insert(iv);
                }

            } else if (removable.size()) {
                // existing remove
                size_t ith = std::uniform_int_distribution<int>(0,removable.size()-1)(gen);
                auto it = std::begin(removable);
                std::advance(it, ith);
                Interval<int> iv = *it;
                removable.erase(it);
                tsks[i].method = REMOVE;
                tsks[i].a = iv.begin;
                tsks[i].b = iv.end;

            } else {
                // random remove
                Interval<T> iv = generate_interval();
                tsks[i].method = REMOVE;
                tsks[i].a = iv.begin;
                tsks[i].b = iv.end;

            }
        }
    }

    Point<T> generate_point() {
        Point<T> p;
        p.reserve(dim);
        for (size_t d = 0; d < dim; ++d) {
            p.emplace_back(p_dist(gen));
        }
        return p;
    }

    Interval<T> generate_interval() {
        Point<T> p_a, p_b;
        p_a.reserve(dim);
        p_b.reserve(dim);
        for (size_t j = 0; j < dim; ++j) {
            T a = p_dist(gen), b = p_dist(gen);
            p_a.emplace_back(std::min(a,b));
            p_b.emplace_back(std::max(a,b));
        }
        return Interval<T>(p_a, p_b);
    }

public:
    std::vector<task> tsks;
private:
    size_t N;
    size_t dim;
    std::mt19937 gen;
    uniform_distribution<T> p_dist;
    std::uniform_real_distribution<double> t_dist;
};





typedef int TYP;
Data<int> DAT(1E5, 0, 100);

void threadFunc(ParallelIntervalTree<TYP> &pt, const size_t offset, const size_t step) {
    for (size_t i = offset; i < DAT.tsks.size(); i += step) {
        auto& tsk = DAT.tsks[i];
        switch (tsk.method) {
        case DAT.QUERY: pt.query(tsk.a); break;
        case DAT.INSERT: pt.insert(tsk.a, tsk.b); break;
        case DAT.REMOVE: pt.remove(tsk.a, tsk.b); break;
        }
    }
}



static void BM_Parallel_Threads_1(benchmark::State& state) {
    for (auto _ : state) {
        ParallelIntervalTree<int> pt;
        std::thread th1(threadFunc, std::ref(pt), 0, 1);
        std::thread th2(dummyThread);
        std::thread th3(dummyThread);
        std::thread th4(dummyThread);
        th1.join();
        th2.join();
        th3.join();
        th4.join();
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

