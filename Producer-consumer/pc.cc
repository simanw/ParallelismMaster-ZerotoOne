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