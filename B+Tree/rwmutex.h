/**
 * rwmutex.h
 *
 * Reader-Writer lock
 */

#pragma once

#include <climits>
#include <condition_variable>
#include <mutex>

namespace cmudb {
    class RWMutex {

        typedef std::mutex mutex_t;
        typedef std::condition_variable cond_t;
        static const uint32_t max_readers_ = UINT_MAX;

    public:
        RWMutex() : reader_count_(0), writer_entered_(false) {}

        ~RWMutex() {
            std::lock_guard<mutex_t> guard(mutex_);
        }
        // 存疑
        RWMutex(const RWMutex &) = delete;
        RWMutex &operator=(const RWMutex &) = delete;

        void WLock() {
            std::unique_lock<mutex_t> lock(mutex_);     // 互斥锁
            while (writer_entered_)
                reader_.wait(lock);
            writer_entered_ = true;
            while (reader_count_ > 0)   // wait all read_lock free.
                writer_.wait(lock);
        }

        void WUnlock() {
            std::lock_guard<mutex_t> guard(mutex_);
            writer_entered_ = false;
            reader_.notify_all();               // 唤醒所有等待的线程
        }

        void RLock() {
            std::unique_lock<mutex_t> lock(mutex_);     // 互斥锁
            while (writer_entered_ || reader_count_ == max_readers_)
                reader_.wait(lock);         // wait 导致当前线程阻塞直至条件变量被通知
            reader_count_++;
        }

        void RUnlock() {
            std::lock_guard<mutex_t> guard(mutex_);
            reader_count_--;
            if (writer_entered_) {
                if (reader_count_ == 0)
                    writer_.notify_one();       // 随机唤醒一个等待的线程
            } else {
                if (reader_count_ == max_readers_ - 1)
                    reader_.notify_one();       // 随机唤醒一个等待的线程
            }
        }

    private:
        mutex_t mutex_;
        cond_t writer_;
        cond_t reader_;
        uint32_t reader_count_;
        bool writer_entered_;
    };
} // namespace cmudb