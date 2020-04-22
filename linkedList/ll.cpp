#include "ll.h"

std::atomic<unsigned long> tls[64*8];

list::list_elem::list_elem() : id_(-1), next_(NULL) {};
list::list_elem::list_elem(int _id) : id_(_id), next_(NULL) {};

list::list () {};

struct list::list_elem* list::lookup(int _id, struct list_elem **prev) {
	struct list::list_elem* cur = sentinel_.next_;
	*prev = &sentinel_;

	while (cur) {

	    if (cur->id_ == _id)
		return cur;

	    *prev = cur;
	    cur = cur->next_;
	}
	return cur;
}

struct list::list_elem* list::lookup(int _id) {
	struct list::list_elem* prev;

	return lookup(_id, &prev);
}

bool list::push_front(int _id) {
	if (lookup(_id))
	    return false;

	struct list::list_elem *elem = new list::list_elem(_id);
	elem->next_ = sentinel_.next_;
	sentinel_.next_ = elem;
	return true;
}

bool list::remove(int _id) {
	struct list::list_elem *prev;
	struct list::list_elem *elem = lookup(_id, &prev);

	if (elem) {
	    prev->next_ = elem->next_;
	    elem->next_ = NULL;
	    delete(elem);

	    return true;
	}

	return false;
}

void list::delete_elems() {
	struct list::list_elem *cur = sentinel_.next_;
	struct list::list_elem *prev;
	int deleted = 0;

	if (cur) { //skip sentinel
	    prev = cur;
	    cur = cur->next_;		
	}

	while (cur) {
	    delete(prev);
	    deleted++;
	    prev = cur;
	    cur = cur->next_;
	}
	if (prev) {
	    delete(prev);
	    deleted++;
	}
}

struct list* list::copy() {
    struct list *new_list = new list();
    struct list_elem *new_sentinel = &new_list->sentinel_;
    struct list_elem *old_sentinel = &sentinel_;
    struct list_elem *cur = old_sentinel->next_;
    struct list_elem *cur_new = new_sentinel;

    while (cur) {
        struct list_elem *next_new = new list_elem(cur->id_);
        cur_new->next_ = next_new;
        cur_new = cur_new->next_;
        cur = cur->next_;
    }
    return new_list;
}

void init(struct list *ll) {
    unsigned int seed;

    for (int i = 0; i < ELEMS / 2; i++) {
	ll->push_front(rand_r(&seed) % ELEMS);
    }
}

void test(struct list *ll, int tid) {
    unsigned int seed = tid;

    for (int i = 0; i < ITER; i++) {
	unsigned int task = rand_r(&seed) % RATIO;
	unsigned int id = rand_r(&seed) % ELEMS;

	switch (task) {
	case 0:
	    ll->push_front(id);
	    break;
	case 1:
	    ll->remove(id);
	    break;
	default:
	    ll->lookup(id);
	    break;
	}
    }
}

void init_multithread(std::atomic<struct list *> *ll_) {
    unsigned int seed;
    struct list *ll = ll_->load();

    for (int i = 0; i < ELEMS / 2; i++) {
	ll->push_front(rand_r(&seed) % ELEMS);
    }
}

void test_multithread(std::atomic<struct list *> *ll, int tid, int n_threads) {
    unsigned int seed = tid;

    for (int i = 0; i < ITER/n_threads; i++) {
	unsigned int task = rand_r(&seed) % RATIO;
	unsigned int id = rand_r(&seed) % ELEMS;
        struct list *new_list;
        struct list *old_list;

	switch (task) {
	case 0:
            do {
                old_list = ll->load();
                new_list = old_list->copy();
                new_list->push_front(id);
            } while (!ll->compare_exchange_strong(old_list, new_list));
	    break;
	case 1:
            do {
                old_list = ll->load();
                new_list = old_list->copy();
                new_list->remove(id);
            } while (!ll->compare_exchange_strong(old_list, new_list));
	    break;
	default:
            struct list *list = ll->load();
            list->lookup(id);
	    break;
	}
    }
}

bool active_readers(unsigned long new_version, int num_thread, int myself) {
    int advanced = 0;
    for (int i = 0; i<num_thread; i++) {
        if (i == myself)
            continue;

        if (tls[i*8].load() > new_version)
            advanced++;
    }

    return advanced != (num_thread - 1);
}

void init_multithread_gc(std::atomic<struct list *> *ll_) {
    unsigned int seed;
    struct list *ll = ll_->load();

    for (int i = 0; i < ELEMS / 2; i++) {
	ll->push_front(rand_r(&seed) % ELEMS);
    }
}

void test_multithread_gc(std::atomic<struct list *> *ll, int tid, int n_threads) {
    unsigned int seed = tid;

    for (int i = 0; i < ITER; i++) {
	unsigned int task = rand_r(&seed) % RATIO;
	unsigned int id = rand_r(&seed) % ELEMS;
        bool retry = false;
        struct list *new_list;
        struct list *old_list;

	switch (task) {
	case 0:
            do {
                if (retry) {
                    new_list->delete_elems();
                    delete (new_list);
                }
                old_list = ll->load();
                tls[tid*8].store(old_list->version_);
                new_list = old_list->copy();
                new_list->push_front(id);
                new_list->version_ = old_list->version_ + 1;
                retry = true;
            } while (!ll->compare_exchange_strong(old_list, new_list));

            //Wait until nobody acccesses the old list
            while (active_readers(old_list->version_, n_threads, tid)) {}
            old_list->delete_elems();
            delete(old_list);

	    break;
	case 1:
            do {
                if (retry) {
                    new_list->delete_elems();
                    delete (new_list);
                }
                old_list = ll->load();
                tls[tid*8].store(old_list->version_);
                new_list = old_list->copy();
                new_list->remove(id);
                new_list->version_ = old_list->version_ + 1;
                retry = true;
            } while (!ll->compare_exchange_strong(old_list, new_list));
            
            //Wait until nobody acccesses the old list
            while (active_readers(old_list->version_, n_threads, tid)) {}
            old_list->delete_elems();
            delete(old_list);
	    
            break;
	default:
            struct list *list = ll->load();
            tls[tid*8].store(list->version_);
            list->lookup(id);
	    break;
	}
    }

    //On exit set version to infinity so nobody waits on us
    tls[tid*8].store(~0x0UL);
}
