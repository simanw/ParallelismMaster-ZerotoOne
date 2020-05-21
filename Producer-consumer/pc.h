#ifndef PC_H
#define PC_H

#include <chrono>
#include <iostream>
#include <ctime>
#include <vector>
#include <cstdint>
#include <queue>
#include <set>
#include <thread>
#include <atomic>
#include <mutex>
#include <unistd.h>

#define LOOKUP_RATIO 10
#define NUM_ITEM 1000000
#define NUM_ITER 1024*1024
#define MAX_THREADS 64
#define NUM_PRODUCE 128
#define NUM_SETS 32

enum {
    INSERT,
    DELETE,
    LOOKUP,
};

struct task {
    int action;
    int item;
};

// Single thread version of inition
void init();

// Multi-threaded version of inition 
void init_partitioned(int n_consumers);

// Single-threaded version
//void produce(unsigned int *seed);
//int consume();

// 
//void producer1(unsigned int *seed);
//void consumer1(int *num_ops, int tid);

//
//void producer2(unsigned int *seed, int n_consumers);
//void consumer2(int *num_ops, int n_consumers, int tid);

// 
void producer3(unsigned int *seed, int n_consumers);
void consumer3(int *num_ops, int n_consumers, int tid);

//
// void producer4(unsigned int *seed, int n_consumers);
// void consumer4(int *num_ops, int n_consumers, int tid);

#endif