#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "btree.h"
#include <thread>

uint32_t ELEM = 10000000;
uint32_t THREADS = 32;
uint32_t TOTAL_OPS = 10000000;
uint32_t READ_PCT = 80;

void do_work (bplustree *tree, uint32_t OPS, uint32_t tid) {
    uint32_t local_tid = tid;

    for (uint64_t i = 0; i < OPS; i++) {
        uint64_t key = rand_r(&local_tid) % ELEM;

        uint32_t type = rand_r(&local_tid) % 100;
        if (type <= READ_PCT) {
            auto value = (uint64_t)tree->search(tree, (unsigned char *)&key, 8);
            if (value != key && value != 0) {
                std::cout << "Error: value: " << value << " != key: " << key << std::endl;
            }
        }
        else {
            //During execution only insert odd keys
            key |= 1;
            tree->insert(tree, (unsigned char *)&key, 8, (void *)key);
        }
    }
    //cout << "realsead 002 " << endl;
}

int main()
{
    srand(0);
    std::vector<std::thread> myThreads(THREADS);
    for (uint32_t t = 1; t <= THREADS; t *= 2) {
        //On each thread iteration re-initialize tree, to avoid all keys being present already
        bplustree *tree = new bplustree();

        //Initialize tree with even keys
        std::vector<uint64_t> keys;
        for (uint64_t i = 0; i < ELEM; i += 2) {
            keys.push_back(i);
        }
        std::random_shuffle(keys.begin(), keys.end());
        for (uint64_t i = 0; i < keys.size(); i++) {
            auto ret = tree->insert(tree, (const unsigned char *)&keys[i], 8, (void *)keys[i]);
        }

        uint32_t OPS = TOTAL_OPS / t;
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        for (uint32_t tt = 0; tt < t; tt++) {
            myThreads[tt] = std::thread(do_work, tree, OPS, tt);
        }
        for (uint32_t tt = 0; tt < t; tt++) {
            myThreads[tt].join();
        }
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
        std::cout << "Threads: " << t << " Duration: " << duration << std::endl;
    }
    return 1;
}


/* ============= on Mac ==============

Threads: 1 Duration: 4338092144
Threads: 1 Duration: 11745500779
Threads: 2 Duration: 5966759720
Threads: 4 Duration: 3121587119
Threads: 8 Duration: 1622782727
Threads: 16 Duration: 1312977131
Threads: 32 Duration: 1330577964

Threads: 1 Duration: 4684510984
Threads: 2 Duration: 1973600057
Threads: 4 Duration: 988974563
Threads: 8 Duration: 521650066
Threads: 16 Duration: 352157905
Threads: 32 Duration: 354936896

Threads: 1 Duration: 3939292834
Threads: 2 Duration: 1986559390
Threads: 4 Duration: 1370837637
Threads: 8 Duration: 860684491
Threads: 16 Duration: 772310549
Threads: 32 Duration: 537863281
Threads: 64 Duration: 476427691

// only read lock.
Threads: 1  Duration: 5378417762
Threads: 2  Duration: 4785590912
Threads: 4  Duration: 5444239651
Threads: 8  Duration: 7468589005
Threads: 16 Duration: 6726375244
Threads: 32 Duration: 9993284679

single run 32 once.
Threads: 32 Duration: 9054604157

remove the read-lock
Threads: 1 Duration: 5922378010
Threads: 2 Duration: 4182949028

use read-lock and write-lock:


Threads: 32 Duration: 85519321517

 */

/*  teacher give the record on homework
Threads: 1 Duration: 4338092144
Threads: 2 Duration: 2290980730
Threads: 4 Duration: 1139997354
Threads: 8 Duration: 637408697
Threads: 16 Duration: 358104310
Threads: 32 Duration: 271899611

*/