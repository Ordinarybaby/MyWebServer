#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H
#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../lock/locker.h"
using namespace std;

// 阻塞队列类（用循环数组实现）
template <class T>
class BlockQueue {
public:
    BlockQueue(int max_size = 1000) {
        if (max_size <= 0) {
            exit(-1);
        }

        max_size_ = max_size;
        array_ = new T[max_size];
        size_ = 0;
        front_ = -1;
        back_ = -1;
    }
    ~BlockQueue() {
        mutex_.lock();
        if (array_ != NULL) {
            delete [] array_;
        }
        mutex_.unlock();
    }

    void clear() {
        mutex_.lock();
        size_ = 0;
        front_ = -1;
        back_ = -1;
        mutex_.unlock();
    }
    // 判断队列是否满了
    bool full() {
        mutex_.lock();
        if (size_ >= max_size_) {
            mutex_.unlock();
            return true;
        }
        mutex_.unlock();
        return false;
    }
    // 判断队列是否为空
    bool empty() {
        mutex_.lock();
        if (size_ == 0) {
            mutex_.unlock();
            return true;
        }
        mutex_.unlock();
        return false;
    }
    // 返回队首元素，传入引用
    bool front(T& value) {
        mutex_.lock();
        if (size_ == 0) {
            mutex_.unlock();
            return false;
        }
        value = array_[front_];
        mutex_.unlock();
        return true;
    }
    // 返回队尾元素，传入引用
    bool back(T& value) {
        mutex_.lock();
        if (size_ == 0) {
            mutex_.unlock();
            return false;
        }
        value = array_[back_];
        mutex_.unlock();
        return true;
    }
    // 返回队列保存的元素个数
    int size() {
        int sz = 0;
        // mutex_.lock();
        sz = size_;
        // mutex_.unlock();
        return sz;
    }
    // 返回队列能保存的最大元素
    int max_size() {
        int max_sz = 0;
        // mutex_.lock();
        max_sz = max_size_;
        // mutex_.unlock();
        return max_sz;
    }
    // 向队列添加元素（首先判断队列是否已满）
    // 1. 唤醒所有阻塞的线程
    // 2. 元素push进队列
    bool push(const T& item) {
        mutex_.lock();
        if (size_ >= max_size_) {
            cond_.broadcast();
            mutex_.unlock();
            return false;
        }

        back_ = (back_ + 1) % max_size_;
        array_[back_] = item;

        size_++;
        cond_.broadcast();
        mutex_.unlock();
        return true;
    }
    // pop时，如果当前队列没有元素，将会等待条件变量
    bool pop(T& item) {
        mutex_.lock();
        while (size_ <= 0) {
            if (!cond_.wait(mutex_.get())) {
                mutex_.unlock();
                return false;
            }
        }

        front_ = (front_ + 1) % max_size_;
        item = array_[front_];
        size_ --;
        mutex_.unlock();
        return true;
    }
    // pop, 增加超时处理
    bool pop(T& item, int timeout) {
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, NULL);
        mutex_.lock();
        if (size_ <= 0) {
            t.tv_sec = now.tv_sec + timeout / 1000;
            t.tv_nsec = (timeout % 1000) * 1000;
            if (!cond_.timewait(mutex_.get(), t)) {
                mutex_.unlock();
                return false;
            }
        }

        if (size_ <= 0) {
            mutex_.unlock();
            return false;
        }

        front_ = (front_ + 1) % max_size_;
        item = array_[front_];
        size_ --;
        mutex_.unlock();
        return true;
    }
    
private:
    Locker mutex_;
    Cond cond_;

    T* array_;
    int size_;
    int max_size_;
    int front_;
    int back_;
};


#endif