/*
* Author: Siman
* Date: 01/23/2020
*
* Use std::atomic library to control synchronization in multiple threads. 
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
vector<atomic<int64_t>> accounts;               // Accounts

/*
* Verify() function verifies the correctness of multithreaded programs. 
* If total amount of all banking accounts after transfering is extactly equal to original amounts, it is considered correct.
*/
void verify()
{
    int64_t res = 0;

    for (auto & i : accounts){
        if (i < 0) cout << "Balance of account " << i << " is negative!!!" << endl;
        res += i;
    }
    if (res != account_size * MONEY_START_AMT)
        std::cout << "Verify failed: " << res << std::endl;
}

/*
* Atomic object does not support comparison. There lies coherence problem in comparison condition.
* Atomic class member function compare_exchange_strong() guarantees that newest value is read.
*/
void func_atomic(int tid, int num_threads){
    unsigned int s0 = 0 + tid, s1 = 2 + tid, s2 = 3 + tid;
    int d;
    int s;
    int amount;

    for (long i = 0; i < ITER / num_threads; i++) {
        s = rand_r(&s0) % account_size;
        d = rand_r(&s1) % account_size;
        amount = rand_r(&s2) % 1000;

        int64_t current_s = accounts[s];
        int64_t current_d = accounts[d];

        if(accounts[s] >= amount){
            bool ifsuccess = accounts[s].compare_exchange_strong(current_s, current_s - amount, std::memory_order_seq_cst);
            if(ifsuccess) accounts[d] += amount;
        }

    }
}


void transfer(int num_threads){
    thread t[num_threads];
    for (int i = 0; i < num_threads; ++i) {
        t[i] = thread(func_atomic, i, num_threads);
    }
    for(auto& th: t)
        th.join();
}

/*
* Initialize each element of type of atomic<int_64> by for loop
* Fixed bug: accounts[i] = MONEY_START_AMT. An atomic<int64> type cannot be assigned by an int type.
*/
void initial_accounts_vector(int account_num){
    accounts = vector<atomic<int64_t>> (account_num);
    for(int i = 0; i < account_num; ++i){
        atomic<int64_t > accounts_start_amt(MONEY_START_AMT);
        accounts[i] += accounts_start_amt;
    }
}

/*
* transfer_critical() function creates multiple threads, and return after all sub-threads return.
*/
int main( int argc, const char* argv[] ) {
    if(argc == 1){
        cout<<"You need input thread num!"<<endl;
        return 0;
    }
    int thread_num = atoi(argv[1]);
    if( argc == 3 )
        account_size = atoi(argv[2]);

    initial_accounts_vector(account_size);

    // Measure time consumption of multithreaded programs
    auto start = std::chrono::system_clock::now();
    transfer(thread_num);
    auto end = std::chrono::system_clock::now();
    auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Atomic:  " << elapsed.count() << '\n';//the time consumption of transfer

    verify();

    return 0;
}




