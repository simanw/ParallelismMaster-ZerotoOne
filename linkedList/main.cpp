#include "ll.h"

int main (int argc, const char* argv[]) {
    (void)argc;
    (void)argv;

    std::atomic<struct list *> ll;
    ll = new struct list();
    init_multithread(&ll);

    for (int num_thread = 2; num_thread < 129; num_thread *=2) {
        auto start = std::chrono::system_clock::now();

        std::vector<std::thread> threads(num_thread);
        for (int i = 0; i < num_thread; i++) {
            threads[i] = std::thread(test_multithread, &ll, i, num_thread);
        }
        for (int i = 0; i < num_thread; i++) {
            threads[i].join();
        }
        auto end = std::chrono::system_clock::now();

        auto elapsed = 
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << num_thread << " threads took " << elapsed.count() << '\n';
    }
    ll.load()->delete_elems();
    // delete(ll);
}