#include <chrono>
#include <iostream>
#include <ctime>
#include <vector>
#include <cstdint>
#include <thread>
#include <atomic>
#include <mutex>

const long ITER = 10000000;
const int SIZE = 1024;
const int MONEY_START_AMT = 1000;
std::vector<int64_t> money(SIZE, MONEY_START_AMT);
std::vector<std::atomic<int64_t>> money_atomic(SIZE);

std::mutex mtx;
std::vector<std::mutex> mtxs(SIZE);
//jjjjjjj
void verify()
{
    int res = 0;
  
    for (auto i : money)
    res += i;

    if (res != SIZE * MONEY_START_AMT)
        std::cout << "Verify failed: " << res << std::endl;
}

void transfer()
{
    unsigned int s0 = 0, s1 = 2, s2 = 3;
    int d;
    int s;
    int amount;

    for (long i=0; i < ITER; i++) {
    s = rand_r(&s0) % SIZE;
    d = rand_r(&s1) % SIZE;
    amount = rand_r(&s2) % 1000;

    money[s] -= amount;
    money[d] += amount;
    }
}

void transfer_multithread(int tid)
{
    unsigned int s0 = tid + 1, s1 = (tid + 1) * 2, s2 = (tid + 1) * 3;
    int d;
    int s;
    int amount;

    for (long i=0; i < ITER; i++) {
	s = rand_r(&s0) % SIZE;
	d = rand_r(&s1) % SIZE;
	amount = rand_r(&s2) % 1000;

	money[s] -= amount;
	money[d] += amount;
    }
}

void transfer_atomic(int tid)
{
    unsigned int s0 = tid + 1, s1 = (tid + 1) * 2, s2 = (tid + 1) * 3;
    int d;
    int s;
    int amount;

    for (long i=0; i < ITER; i++) {
	s = rand_r(&s0) % SIZE;
	d = rand_r(&s1) % SIZE;
	amount = rand_r(&s2) % 1000;

	money_atomic[s] -= amount;
	money_atomic[d] += amount;
    }
}

void transfer_critical(int tid)
{
    unsigned int s0 = tid + 1, s1 = (tid + 1) * 2, s2 = (tid + 1) * 3;
    int d;
    int s;
    int amount;

    for (long i=0; i < ITER; i++) {
	s = rand_r(&s0) % SIZE;
	d = rand_r(&s1) % SIZE;
	amount = rand_r(&s2) % 1000;

	mtx.lock();
	if ((money[s] + money[d]) > 0) {
	    money[s] -= amount;
	    money[d] += amount;
	}
	mtx.unlock();
    }
}

void transfer_critical_fine(int tid)
{
    unsigned int s0 = tid + 1, s1 = (tid + 1) * 2, s2 = (tid + 1) * 3;
    int d;
    int s;
    int amount;

    for (long i=0; i < ITER; i++) {
	s = rand_r(&s0) % SIZE;
	d = rand_r(&s1) % SIZE;
	amount = rand_r(&s2) % 1000;

	if (s == d)
	    continue;

	// if (s > d) {
	//     mtxs[s].lock();
	//     mtxs[d].lock();
	// }
	// else {
	//     mtxs[d].lock();
	//     mtxs[s].lock();
	// }
	mtxs[s].lock();
	mtxs[d].lock();
	if ((money[s] + money[d]) > 0) {
	    money[s] -= amount;
	    money[d] += amount;
	}
	mtxs[d].unlock();
	mtxs[s].unlock();

    }
}

void init ()
{
    for (int i = 0; i < SIZE; i++) {
	money_atomic[i] = MONEY_START_AMT;
    }
    verify();
}

int main( int argc, const char* argv[] ) {

    int exercise = atoi(argv[1]);

    for (int num_thread = 1; num_thread < 33; num_thread *= 2) {
	std::vector<std::thread> threads(num_thread);

	auto start = std::chrono::system_clock::now();
	for (int i = 0; i < num_thread; i++) {
	    switch (exercise) {
	    case 0:
		threads[i] = std::thread(transfer_multithread, i);
		break;
	    case 1:
		threads[i] = std::thread(transfer_atomic, i);
		break;
	    case 2:
		threads[i] = std::thread(transfer_critical, i);
		break;
	    case 3:
		threads[i] = std::thread(transfer_critical_fine, i);
		break;
	    default:
		break;
	    }
	}
	for (int i = 0; i < num_thread; i++)
	    threads[i].join();
	auto end = std::chrono::system_clock::now();

	auto elapsed =
	    std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << (num_thread * ITER) / elapsed.count() <<  '\n';
	if (exercise == 1)
	    verify();
    }

    return 0;
}
