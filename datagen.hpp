#pragma once

#include <cstddef>
#include <random>
#include <vector>
#include <algorithm>
#include <set>

#include "it.hpp"



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
                //std::cout << "query " << tsks[i].a[0] << std::endl;

            } else if (tsk < qry + ins) {
                // insert
                Interval<T> iv = generate_interval();
                tsks[i].method = INSERT;
                tsks[i].a = iv.begin;
                tsks[i].b = iv.end;
                //std::cout << "insert " << iv.begin[0] << ' ' << iv.end[0] << std::endl;
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
                //std::cout << "erm " << iv.begin[0] << ' ' << iv.end[0] << std::endl;

            } else {
                // random remove
                Interval<T> iv = generate_interval();
                tsks[i].method = REMOVE;
                tsks[i].a = iv.begin;
                tsks[i].b = iv.end;
                //std::cout << "rrm " << iv.begin[0] << ' ' << iv.end[0] << std::endl;

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

