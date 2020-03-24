#include "btree.h"

bplustree::bplustree()
{
    root = new bplustree_leaf_node();
    memset(root, 0, sizeof(struct bplustree_leaf_node));
    root->next = NULL;
    root->is_leaf = true;
    root->key_count = 0;
    root->rwlatch_ = new RWMutex();
}

void* bplustree::search(struct bplustree *t, const unsigned char *key, int key_len)
{
    LOG("Get key=" << std::string((const char *)key, key_len));    
    std::stack <struct bplustree_node *> ancestors;
    struct bplustree_leaf_node *pleaf =
            (struct bplustree_leaf_node *)LeafSearch(t, key, key_len, ancestors);

    void *value = NULL;
    int idx = 0;
    volatile uint64_t v = 0;
    int ret = -1;
    if(!pleaf) {
        LOG ("could not find key");
        return NULL;
    }


    int keycount = pleaf->key_count;
    for(idx = 0; idx < keycount; idx++) {

        unsigned char *router_key = pleaf->slots[idx].key;

        if((ret = memcmp((void *)router_key,
                         (void *)key, EMBEDEDSIZE)) >= 0) {
            value = (void *)pleaf->slots[idx].val[0];
            break;
        }
    }

    // 侧滑=
    if(ret < 0) {
        struct bplustree_leaf_node * next =
                (struct bplustree_leaf_node *) pleaf->next;
        bool retry = false;
        //cout << "ret < 0 in search() show in single thread" << endl;
        while(next && (memcmp((void *)key, (void *)next->slots[0].key,
                              EMBEDEDSIZE) >= 0)) {
            retry = true;
            pleaf = next;
            //cout << "next ret < 0 in search() not show in single thread" << endl;
            next = (struct bplustree_leaf_node *) pleaf->next;
        }
    }

#ifdef USE_READ_LOCK
    pleaf->rwlatch_->RUnlock();      // 2020-02-19
#endif

    if(0 == ret) {
        LOG("value = " << (void*)value);
        LOG("   found value, slot="
                    << idx
                    << ", value="
                    << value);
        return value;
    }

    LOG("cannot find");
    return NULL;
}

bool bplustree::FindNext(const unsigned char *key, int key_len,
                         struct bplustree_node *node,
                         struct bplustree_node **next)
{
    const int keycount = node->key_count;
    int idx = 0;
    LOG("FindNext keycount = " << keycount);
    for(idx = 0; idx < keycount; idx++) {
        unsigned char *router_key =
                (unsigned char *) ((struct bplustree_index_node *)node)->slots[idx].key;

        LOG("FindNext idx = " << idx);
        LOG("FindNext key = " << std::string((const char *)key, key_len));

        LOG("FindNext rounter_key = " << std::string((const char *)GET_KEY_STR(router_key),
                                                     GET_KEY_LEN(router_key)));

        if(memcmp((void *)router_key,
                  (void *)key, EMBEDEDSIZE) >= 0) {
            break;
        }

    }

    if(idx < keycount) {
        LOG("ptr = " << (void *)((struct bplustree_index_node *)node)->slots[idx].ptr);
        *next =  ((struct bplustree_index_node *)node)->slots[idx].ptr;
        return true;
    }

    struct bplustree_node * sibling = node->next;

    if(sibling && (memcmp((void *)key,
                          (void *)((struct bplustree_index_node *)sibling)->slots[0].key,
                          EMBEDEDSIZE) >=0))
    {
        cout << "sibling not show in single thread" << endl;
        *next = sibling;
        return false;
    }


    LOG("ptr = " << (void *)((struct bplustree_index_node *)node)->slots[keycount].ptr);
    *next =  ((struct bplustree_index_node *)node)->slots[keycount].ptr;
    return true;

}

bplustree::bplustree_node* bplustree::LeafSearch(struct bplustree *t,
                                                 const unsigned char *key, int key_len,
                                                 std::stack <struct bplustree_node *> &ancestors)

{
    //cout << "realsead 001 " << endl;
#ifdef USE_READ_LOCK
    t->root->rwlatch_->RLock();      // 2020-02-19
#endif
    //cout << "realsead 003 " << endl;

    struct bplustree_node* pnode = t->root;
    bool move_downwards = false;
    struct bplustree_node *next = NULL;
    if(!pnode) {
        return NULL;
    }
#ifdef INDEX_STATS
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
#endif

#ifdef USE_PREFETCH
    bplustree_node_prefetch(pnode);
#endif
    while(!pnode->is_leaf) {

        LOG("searchleaf access " << pnode);
        move_downwards = FindNext(key, key_len, pnode, &next);
        assert(next);
        // search() don't need this opt
        // if(move_downwards) ancestors.push(pnode);
#ifdef USE_READ_LOCK
        next->rwlatch_->RLock();     // 2020-02-19
        pnode->rwlatch_->RUnlock();  // 2020-02-19
#endif
        pnode = next;
#ifdef USE_PPREFETCH
        bplustree_node_prefetch(pnode);
#endif
        assert(pnode);
    }

#ifdef INDEX_STATS
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    search_time += duration;
#endif
    return pnode;
}


bplustree::bplustree_node* bplustree::LeafSearchForInsert(struct bplustree *t,
                                                 const unsigned char *key, int key_len,
                                                 std::stack <struct bplustree_node *> &ancestors)

{
#ifdef USE_WRITE_LOCK
    t->root->rwlatch_->WLock();      // 2020-02-20
#endif
    struct bplustree_node* pnode = t->root;
    bool move_downwards = false;
    struct bplustree_node *next = NULL;
    if(!pnode) {
        return NULL;
    }

    while(!pnode->is_leaf) {

        move_downwards = FindNext(key, key_len, pnode, &next);
        assert(next);
        if(move_downwards) {
            // 检查该节点是否安全，如果安全, 释放该节点所有祖先节点的写锁
            // 节点安全定义如下：如果对于插入操作，如果再插入一个元素，不会产生分裂
            if(pnode->key_count < BPLUSTREE_LEAF_NODE_KEYS) {
                while(ancestors.size()) {
                    auto parent = ancestors.top();
                    ancestors.pop();
                    #ifdef USE_WRITE_LOCK
                    parent->rwlatch_->WUnlock();   // 2020-02-20
                    #endif
                }
            }
            ancestors.push(pnode);
        }
#ifdef USE_WRITE_LOCK
        next->rwlatch_->WLock();     // 2020-02-20
#endif
        pnode = next;
        assert(pnode);
    }

    return pnode;
}

int bplustree::insert(struct bplustree *t, const unsigned char *key,
                      int key_len, const void *value)
{
    LOG("Put key=" << std::string((const char *)key, key_len));
    
    std::stack <struct bplustree_node *> ancestors;
    bool need_split = false;
    const unsigned char* split_key;
    struct bplustree_leaf_node *p_new_leaf = NULL;

    struct bplustree_leaf_node *p_leaf =
            (struct bplustree_leaf_node *)LeafSearchForInsert(t, key, key_len, ancestors);

    if(!p_leaf) {
        assert(false);
        return 0;
    }

#ifdef INDEX_STATS
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
#endif

    struct bplustree_leaf_node * next =
            (struct bplustree_leaf_node *) p_leaf->next;
    while(next && (memcmp((void *)key,
                          (void *)next->slots[0].key, EMBEDEDSIZE) >= 0)) {
        LOG("leaf = " << (void *)p_leaf);
        LOG("leaf->next = " << (void *)p_leaf->next);
        cout << "next in insert() not show in single thread" << endl;

        p_leaf = next;
        next = (struct bplustree_leaf_node *) p_leaf->next;
        LOG("Leaf update next " << (void *)next);
    }


    int idx = 0;
    bool exist = false;

    int ret;
    int keycount = p_leaf->key_count;
    for(idx = 0; idx < keycount; idx++) {
        unsigned char * router_key = p_leaf->slots[idx].key;


        if((ret = memcmp((void *)router_key,
                         (void *)key, EMBEDEDSIZE)) >= 0) {
            if(ret == 0)
                exist = true;
            break;
        }
    }

    // if same address, then change the val.
    if(exist) {
        p_leaf->slots[idx].val[0] = (uint64_t) value;
        // only run on work's insert()
        //cout << "if same address, then change the val. ancestors.size() = " << ancestors.size() << endl;
#ifdef INDEX_STATS
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
	update_time += duration;
#endif
#ifdef USE_WRITE_LOCK
        p_leaf->rwlatch_->WUnlock();     // 2020-02-20

        // remove the stack's node's lock
        while(ancestors.size()) {
            auto parent = ancestors.top();
            ancestors.pop();
            parent->rwlatch_->WUnlock();   // 2020-02-20
        }
#endif
        return 0;
    }
    LOG("Insert at Slot = " << idx);

    // insert + split
    // 逐个往后copy，移动，
    for(int i = keycount - 1;  i>= idx; i--) {
        memcpy(&p_leaf->slots[i + 1], &p_leaf->slots[i],
               sizeof(struct bplustree_kvslot));
    }

    memcpy((void *)p_leaf->slots[idx].key,
           key, EMBEDEDSIZE);

    p_leaf->slots[idx].val[0] = (uint64_t)value;
    p_leaf->key_count += 1;

    // split
    if(p_leaf->key_count > BPLUSTREE_LEAF_NODE_KEYS) {
#ifdef INDEX_STATS
        split_nums++;
#endif
        LOG("split, keycount = " << p_leaf->key_count);

        need_split = true;

        // Now split
        p_new_leaf = (struct bplustree_leaf_node *)
                malloc(sizeof(struct bplustree_leaf_node));
        memset(p_new_leaf, 0, sizeof(struct bplustree_leaf_node));
        p_new_leaf->is_leaf = true;
        p_new_leaf->rwlatch_ = new RWMutex();
        //init new leaf

        split_key = p_leaf->slots[BPLUSTREE_LEAF_MIDPOINT].key;

        LOG("split_key = " << std::string((const char *)GET_KEY_STR(split_key),
                                          GET_KEY_LEN(split_key)));
        keycount = p_leaf->key_count;

        // node 的 keys 右半部分 拷贝 到 新 node 里面。
        for(int i = BPLUSTREE_LEAF_UPPER; i < keycount; i++) {
            memcpy(&p_new_leaf->slots[i - BPLUSTREE_LEAF_UPPER],
                   &p_leaf->slots[i],
                   sizeof(struct bplustree_kvslot));
        }

        p_new_leaf->key_count = keycount - BPLUSTREE_LEAF_MIDPOINT - 1;

        p_leaf->key_count = BPLUSTREE_LEAF_MIDPOINT + 1;

        LOG("new_leaf keycount " << p_new_leaf->key_count);
        LOG("old_leaf keycount " << p_leaf->key_count);

        p_new_leaf->next = (struct bplustree_node *)p_leaf->next;

        p_leaf->next = (struct bplustree_node *)p_new_leaf;

        // creat new root node
        if((struct bplustree_node *)p_leaf == t->root) {
            assert(ancestors.empty());

            struct bplustree_index_node *p_root;
            p_root = (struct bplustree_index_node *)
                    malloc(sizeof(struct bplustree_index_node));

            memset(p_root, 0, sizeof(struct bplustree_index_node));
            memcpy((void*)p_root->slots[0].key,
                   GET_KEY_STR(split_key),
                   GET_KEY_LEN(split_key));

            p_root->slots[0].ptr = (struct bplustree_node *) p_leaf;
            p_root->slots[1].ptr = (struct bplustree_node *) p_new_leaf;
            p_root->key_count = 1;
            p_root->is_leaf = false;
            p_root->rwlatch_ = new RWMutex();
            t->root = (struct bplustree_node *)p_root;
        }
    }

    if(need_split) {
        InsertInternal(t, (const unsigned char *)GET_KEY_STR(split_key),
                       GET_KEY_LEN(split_key),
                       (struct bplustree_node *)p_new_leaf,
                       ancestors);
    }

#ifdef INDEX_STATS
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    update_time += duration;
#endif

#ifdef USE_WRITE_LOCK
    p_leaf->rwlatch_->WUnlock();     // 2020-02-20

    // remove the stack's node's lock
    while(ancestors.size()) {
        auto parent = ancestors.top();
        ancestors.pop();
        parent->rwlatch_->WUnlock();   // 2020-02-20
    }
#endif

    return 1;
}


void bplustree::InsertInternal(struct bplustree *t, const unsigned char *key, int key_len,
                               struct bplustree_node *right,
                               std::stack <struct bplustree_node *> ancestors)
{
    if(ancestors.empty()) {
        //root split
        //assert(left.oid.off == D_RW(bplustree)->root.oid.off);
        return;
    }

    struct bplustree_index_node *p_parent =
            (struct bplustree_index_node *)ancestors.top();
    ancestors.pop();

    struct bplustree_index_node * next =
            (struct bplustree_index_node *)p_parent->next;

    while(next && (memcmp((void *)key,
                          (void *)next->slots[0].key, EMBEDEDSIZE) >= 0)) {

        p_parent = next;
        next = (struct bplustree_index_node*) p_parent->next;
    }

    int keycount = p_parent->key_count;
    int idx = 0;
    for(idx = 0; idx < keycount; idx++) {
        const unsigned char *router_key = p_parent->slots[idx].key;
        if(memcmp((void *)router_key, (void *)key, EMBEDEDSIZE) >= 0) {
            break;
        }
    }

    for(int i = keycount;  i>= idx; i--) {
        memcpy(&p_parent->slots[i + 1], &p_parent->slots[i],
               sizeof(struct bplustree_kpslot));
    }

    memcpy((void*)p_parent->slots[idx].key,
           key, EMBEDEDSIZE);

    p_parent->slots[idx+1].ptr = right;

    p_parent->key_count += 1;

    if(p_parent->key_count <= BPLUSTREE_NODE_KEYS) {
        return;
    }

    LOG("internal split, keycount = " << p_parent->key_count);

    struct bplustree_index_node *p_new_parent;
    p_new_parent = (struct bplustree_index_node *)
            malloc(sizeof(struct bplustree_index_node));
    memset(p_new_parent, 0, sizeof(struct bplustree_index_node));

    p_new_parent->is_leaf = false;
    p_new_parent->rwlatch_ = new RWMutex();
    unsigned char *split_key = p_parent->slots[BPLUSTREE_NODE_MIDPOINT].key;
    keycount = p_parent->key_count;

    LOG("split_key = " << std::string((const char *)GET_KEY_STR(split_key),
                                      GET_KEY_LEN(split_key)));
    for(int i = BPLUSTREE_NODE_UPPER; i <= keycount; i++) {
        memcpy(&p_new_parent->slots[i - BPLUSTREE_NODE_UPPER],
               &p_parent->slots[i],
               sizeof(struct bplustree_kpslot));
    }

    p_new_parent->key_count = keycount - BPLUSTREE_NODE_MIDPOINT - 1;
    p_new_parent->next = (struct bplustree_node *)p_parent->next;

    //update old parent
    p_parent->next = (struct bplustree_node *)p_new_parent;

    p_parent->key_count = BPLUSTREE_NODE_MIDPOINT;

    LOG("new_parent keycount " << p_new_parent->key_count);
    LOG("old_parent keycount " << p_parent->key_count);
    if((struct bplustree_node *) p_parent == t->root) {
        assert(ancestors.empty());
        struct bplustree_index_node *p_root;
        p_root = (struct bplustree_index_node *)
                malloc(sizeof(struct bplustree_index_node));
        memset(p_root, 0, sizeof(struct bplustree_index_node));
        memcpy((void*)p_root->slots[0].key,
               GET_KEY_STR(split_key),
               GET_KEY_LEN(split_key));

        p_root->slots[0].ptr = (struct bplustree_node *) p_parent;
        p_root->slots[1].ptr = (struct bplustree_node *) p_new_parent;
        p_root->key_count = 1;
        p_root->is_leaf = false;
        p_root->rwlatch_ = new RWMutex();
        t->root = (struct bplustree_node *)p_root;
    }

    InsertInternal(t, (const unsigned char *) GET_KEY_STR(split_key),
                   GET_KEY_LEN(split_key),
                   (struct bplustree_node *)p_new_parent,
                   ancestors);
    return;
}

int bplustree::remove(struct bplustree *t, const unsigned char *key, int key_len)
{
    return 0;
}
