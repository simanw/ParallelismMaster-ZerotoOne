/*
* Author: Siman
* Date: 01/23/2020
*
* Use critical section to control synchronization in multiple threads. 
* Synchronization is implemented by mutex.lock and unlock operations around the critical section.
*/
#include <chrono>
#include <iostream>
#include <ctime>
#include <vector>
#include <thread>
#include <cstdint>
#include <mutex>
#include <atomic>

using namespace std;

const long ITER = 10000000;                     // Number of transactions
const int MONEY_START_AMT = 1000;               // Initialized balance of each banking account
int account_size = 1024*1024;                   // Default number of accounts is 1024*1024
vector<int64_t> accounts;                       // Accounts
mutex mtx;

/*
* Verify() function verifies the correctness of multithreaded programs. 
* If total amount of all banking accounts after transfering is extactly equal to original amounts, it is considered correct.
*/
void verify()
{
    int res = 0;

    for (auto & i : accounts){
        if (i < 0) cout << "Balance of account " << i << " is negative!!!" << endl;
        res += i;
    }

    if (res != account_size * MONEY_START_AMT)
        std::cout << "Verify failed: " << res << std::endl;
}

/*
* For each transaction, two accounts are randomly chosen and transfer amount is randomly generated.
* To prevent data races and inconsistency, employ mutex and its lock.
*/
void transfer_critical(int tid, int num_threads){
    unsigned int s0 = 0 + tid, s1 = 2 + tid, s2 = 3 + tid;
    int d;
    int s;
    int amount;

    for (long i = 0; i < ITER / num_threads; i++) {
        s = rand_r(&s0) % account_size;
        d = rand_r(&s1) % account_size;
        amount = rand_r(&s2) % 1000;

        mtx.lock();
        if(amount <= accounts[s]){
            accounts[s] -= amount;
            accounts[d] += amount;
        }
        mtx.unlock();
    }
}

/*
* transfer_critical() function creates multiple threads, and return after all sub-threads return.
*/
void transfer(int num_threads){
    thread t[num_threads];

    for (int i = 0; i < num_threads; ++i) {
        t[i] = thread(transfer_critical, i, num_threads);
    }
    for(auto& th: t)
        th.join();
}

int main( int argc, const char* argv[] ) {
    if(argc == 1){
        cout<<"You need input thread num!"<<endl;
        return 0;
    }
    int thread_num = atoi(argv[1]);
    if( argc == 3 )
        account_size = atoi(argv[2]);

    accounts = vector<int64_t> (account_size, MONEY_START_AMT);

    // Measure time consumption of multithreaded programs
    auto start = std::chrono::system_clock::now();
    transfer(thread_num);
    auto end = std::chrono::system_clock::now();
    auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Critical Section: " << elapsed.count() << '\n';

    // Verify correctness of multithreaded programs
    verify();

    return 0;
}


