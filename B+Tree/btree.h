#ifndef BTREE_H
#define BTREE_H

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <list>
#include <unistd.h>
#include <stack>
#include <vector>
#include <iostream>

#include <stdlib.h>
#include <stdint.h>
#include <stack>
#include <math.h>
#include "rwmutex.h"
#include <chrono>

#define EMBEDEDSIZE 8
#define BPLUSTREE_NODE_KEYS 16
#define BPLUSTREE_LEAF_NODE_KEYS 16

#define BPLUSTREE_NODE_MIDPOINT (BPLUSTREE_NODE_KEYS / 2)
#define BPLUSTREE_NODE_UPPER (BPLUSTREE_NODE_MIDPOINT + 1)

#define BPLUSTREE_LEAF_MIDPOINT (BPLUSTREE_LEAF_NODE_KEYS / 2)
#define BPLUSTREE_LEAF_UPPER (BPLUSTREE_LEAF_MIDPOINT + 1)

#define USE_READ_LOCK 1
#define USE_WRITE_LOCK 1

#include <mutex>

using namespace std;
using namespace cmudb;

class bplustree {

private:
    struct bplustree_node {
        bool is_leaf;
        uint16_t key_count;
        struct bplustree_node* next;
        RWMutex *rwlatch_;       // read and write lock.
    };


    struct bplustree_kpslot {
        unsigned char key[EMBEDEDSIZE];
        struct bplustree_node *ptr; // child pointer or value pointer
    };

    struct bplustree_kvslot {
        unsigned char key[EMBEDEDSIZE];
        uint64_t val[1];
    };

    // + 2 because b+tree's def is node must have two son or more.
    struct bplustree_leaf_node : public bplustree_node {
        uint64_t pad;
        struct bplustree_kvslot slots[BPLUSTREE_LEAF_NODE_KEYS + 2];
    };

    struct bplustree_index_node  : public bplustree_node {
        uint64_t pad;
        struct bplustree_kpslot slots[BPLUSTREE_NODE_KEYS + 2];
    };

    bplustree_node *root;

public:
    bplustree();
    ~bplustree();

    int insert(struct bplustree *t, const unsigned char *key, int key_len,
               const void *value);

    int remove(struct bplustree *t, const unsigned char *key, int key_len);

    void* search(struct bplustree *t, const unsigned char *key, int key_len);


private:
    struct bplustree_node* LeafSearchForInsert(struct bplustree *t,
                                      const unsigned char *key, int key_len,
                                      std::stack <struct bplustree_node *> &ancestors);
    struct bplustree_node* LeafSearch(struct bplustree *t,
                                      const unsigned char *key, int key_len,
                                      std::stack <struct bplustree_node *> &ancestors);

    void InsertInternal(struct bplustree *t, const unsigned char *key, int key_len,
                        struct bplustree_node *right,
                        std::stack <struct bplustree_node *> ancestors);

    bool FindNext(const unsigned char *key, int key_len,
                  struct bplustree_node *node,
                  struct bplustree_node **next);




    //=======================================================================

#define DO_LOG 0
#define LOG(msg, ...)   \
    do {                \
        if (DO_LOG)     \
            std::cout << "[bplustree]" << msg << "\n";  \
    }while(0)

    static inline int min(int a, int b) {
        return (a < b) ? a : b;
    }




#define GET_KEY_LEN(key) EMBEDEDSIZE

#define GET_KEY_STR(key) key


    static int bplustree_compare(const void* p_slot1, const void* p_slot2)
    {
        struct bplustree_kvslot *slot1 = (struct bplustree_kvslot *) p_slot1;
        struct bplustree_kvslot *slot2 = (struct bplustree_kvslot *) p_slot2;
        return memcmp((void *)slot1->key,
                      (void *)slot2->key, EMBEDEDSIZE);
    }


    static int bplustree_slot_compare(const void* pp_key1, const void* pp_key2)
    {
        unsigned char *p_key1 = *((unsigned char **)pp_key1);
        unsigned char *p_key2 = *((unsigned char **)pp_key2);
        return memcmp((void *)p_key1, (void *)p_key2, EMBEDEDSIZE);
    }

    static void bplustree_node_prefetch(struct bplustree_node *n)
    {
        __builtin_prefetch((char *)n +   0, 0 /* rw */, 3 /* locality */);
        __builtin_prefetch((char *)n +  64, 0 /* rw */, 3 /* locality */);
        __builtin_prefetch((char *)n + 128, 0 /* rw */, 3 /* locality */);
        __builtin_prefetch((char *)n + 192, 0 /* rw */, 3 /* locality */);
        __builtin_prefetch((char *)n + 256, 0 /* rw */, 3 /* locality */);
        __builtin_prefetch((char *)n + 320, 0 /* rw */, 3 /* locality */);

    }

};
#endif
