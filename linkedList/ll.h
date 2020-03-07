#ifndef LL_H
#define LL_H

#include <cstddef>
#include <iostream>
#include <chrono>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

#define ELEMS 100
#define ITER 10000000
#define RATIO 10000

extern std::atomic<unsigned long> tls[64*8];

struct list {
    std::mutex mtx;
    struct list_elem {
        int id_;
        struct list_elem *next_;

        list_elem();
        list_elem(int _id);
    };

    struct list_elem sentinel_;
    unsigned long version_;

    list();

    struct list_elem* lookup(int _id, struct list_elem **prev);
    struct list_elem* lookup(int _id);
    bool push_front(int _id);
    bool remove(int _id);
    void delete_elems();
    struct list* copy();
};

void init(struct list *ll);
void test(struct list *ll, int tid);

void init_multithread(std::atomic<struct list*> *ll_);
void test_multithread(std::atomic<struct list*> *ll_, int tid, int n_threads);

void init_multithread_gc(std::atomic<struct list*> *ll_);
void test_multithread_gc(std::atomic<struct list*> *ll_, int tid, int n_threads);

#endif