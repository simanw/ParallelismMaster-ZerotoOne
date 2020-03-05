#include "pc.h"

std::mutex q_mtx; // A global lock on one task queue
std::queue<task> q;

std::mutex qm_mtx[MAX_THREADS];
std::queue<task> qm[MAX_THREADS];

std::mutex s_mtx; // A global lock on one element set
std::set<int> s; 

std::set<int> sm[MAX_THREADS];
std::vector<std::mutex> sm_mtx(NUM_SETS);

// Naive global locks on one queue and one set
// One thread runs as a producer
void produce(unsigned int *seed) {
    q_mtx.lock();
    while (q.size() > 100) {
        q_mtx.unlock();
        q_mtx.lock();
    }

    task t;
    int action = rand_r(seed) % LOOKUP_RATIO;

    switch (action) {
        case 0:
            t.action = INSERT;
            break;
        case 1:
            t.action = DELETE;
            break;
        default:
            t.action = LOOKUP;
            break;
    }
    t.item = rand_r(seed) % NUM_ITEM;

    q.push(t);

    q_mtx.unlock();
}

// Naive global locks on one queue and one set
// One thread runs a consumer
int consumer() {
    int num_ops = 0;

    q_mtx.lock();
    while (q.empty()) {
        q_mtx.unlock();
        q_mtx.lock();
    }

    task t = q.front();
    q.pop();
    q_mtx.unlock();

    s_mtx.lock();
    switch (t.action) {
        case INSERT: {
            auto res = s.insert(t.item);
            num_ops += res.second;
            break;
        }
        case DELETE: {
            num_ops += s.erase(t.item);
            break;
        }
        default: {
            auto res = s.find(t.item);
            num_ops += (res != s.end());
            break;
        }
    }
    s_mtx.unlock();
    return num_ops;
}

/* 
* One thread runs a producer, n threads run as n consumer
* n queues - n consumers, one-by-one
*/ 
void produce_multiq(int qid, unsigned int *seed) {
    qm_mtx[qid].lock();
    while (qm[qid].size() > 100) {
        qm_mtx[qid].unlock();
        qm_mtx[qid].lock();
    }

    task t;
    int action = rand_r(seed) % LOOKUP_RATIO;

    switch (action) {
        case 0:
            t.action = INSERT;
            break;
        case 1:
            t.action = DELETE;
            break;
        default:
            t.action = LOOKUP;
            break;
    }

    t.item = rand_r(seed) % NUM_ITEM;
    qm[qid].push(t);
    qm_mtx[qid].unlock();
}

// Producer produces multiple tasks once 
// and append them at the corresponding queue
void produce_multiq_batch(int qid, unsigned int *seed) {
    qm_mtx[qid].lock();
    while (qm[qid].size() > 100) {
        qm_mtx[qid].unlock();
        // Producer shoud sleep for a little while
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        qm_mtx[qid].lock();
    }

    for (int i = 0; i < NUM_PRODUCE; i++) {
        task i;
        int action = rand_r(seed) % LOOKUP_RATIO;

        switch (action) {
            case 0:
                t.action = INSERT;
                break;
            case 1:
                t.action = DELETE;
                break;
            default:
                t.action = LOOKUP;
                break;
        }
        t.item = rand_r(seed) % NUM_ITEM;
        qm[qid].push(t);
    }
    qm_mtx[qid].unlock();
}

int consume_multiq(int qid) {
    int num_ops = 0;

    qm_mtx[qid].lock();
    while (qm[qid].empty()) {
        qm_mtx[qid].unlock();
        qm_mtx[qid].lock();
    }

    task t = qm[qid].front();
    qm[qid].pop();
    qm_mtx[qid].unlock();

    // Start to process the set
    s_mtx.lock();
    switch (t.action) {
        case INSERT: {
            auto res = s.insert(t.item);
            num_ops += res.second;
            break;
        }
        case DELETE: {
            num_ops += s.erase(t.item);
            break;
        }
        default: {
            auto res = s.find(t.item);
            num_ops += (res != s.end());
            break;
        }
    }
    s_mtx.unlock();
    return num_ops;
}

// n consumers - n queues - n sets
int consume_multiq_multis(int qid) {
    int num_ops = 0;

    qm_mtx[qid].lock();
    while (qm[qid].empty()) {
        qm_mtx[qid].unlock();
        qm_mtx[qid].lock();
    }

    task t = qm[qid].front();
    qm[qid].pop();
    qm_mtx[qid].unlock();

    // Start to process the corresponding set
    int sid = t.item % NUM_SETS;
    sm_mtx[sid].lock();

    switch (t.action) {
        case INSERT: {
            auto res = sm[sid].insert(t.item);
            num_ops += res.second;
            break;
        }
        case DELETE: {
            num_ops += sm[sid].erase(t.item);
            break;
        }
        default: {
            auto res = sm[sid].find(t.item);
            num_ops += (res != sm[sid].end());
            break;
        }
    }

    sm_mtx[sid].unlock();

    return num_ops;
}

// 
void producer1(unsigned int *seed) {
    for (int i = 0; i < NUM_ITER; i++)
        produce(seed);
};

void consumer1(int* num_ops, int tid) {
    for (int i = 0; i < NUM_ITER; i++)
        *num_ops += consume();
};

void producer2(unsigned int *seed, int n_consumers) {
    for (int i = 0; i < NUM_ITER; i++)
        produce_multiq(i % n_consumers, seed);
};

void consumer2(int* num_ops, int n_consumers, int tid) {
    for (int i = 0; i < NUM_ITER/n_consumers; i++)
        *num_ops += consume_multiq(tid);
};

void producer3(unsigned int *seed, int n_consumers) {
    for (int i = 0; i < NUM_ITER / NUM_PRODUCE; i++)
       produce_multiq_batch(i % n_consumers, seed);
};

void consumer3(int* num_ops, int n_consumers, int tid) {
    for (int i = 0; i < NUM_ITER/n_consumers, i++) 
        *num_ops += consume_multiq(tid);
};

void producer4(unsigned int *seed, int n_consumers) {
    for (int i = 0; i < NUM_ITER / NUM_PRODUCE; i ++)
        produce_multiq_batch(i % n_consumers, seed);
};

void consumer4(int* num_ops, int n_consumers, int tid) {
    for (int i = 0; i < NUM_ITER/n_consumers; i++)
        *num_ops += consume_multiq_multis(tid);
};


void init() {
    unsigned int seed = 0;

    for (int i = 0; i < NUM_ITEM / 2; i++)
        s.insert(rand_r(&seed) % NUM_ITEM);
}

// For producer4() and consumer4()
void init_partitioned(int n_consumers) {
    unsigned int seed = 0;

    for (int i = 0; i < NUM_ITEM / 2; i++) {
        int item = rand_r(&seed) % NUM_ITEM;
        int sid = item % NUM_SETS;
        sm[sid].insert(item);
    }
}