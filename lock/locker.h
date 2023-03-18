#ifndef LOCKER_H
#define LOCKER_H
#include <exception>
#include <semaphore.h>
#include <pthread.h>

class Sem {
public:
    Sem() {
        if (sem_init(&sem_, 0, 0) != 0) {
            throw std::exception();
        }
    }

    Sem(int num) {
        if (sem_init(&sem_, 0, num) != 0) {
            throw std::exception();
        }
    }

    ~Sem() {
        sem_destroy(&sem_);
    }

    bool wait() {
        return sem_wait(&sem_) == 0;
    }

    bool post() {
        return sem_post(&sem_) == 0;
    }

private:
    sem_t sem_;  // 类成员变量以下划线结尾
};

// 互斥锁类
class Locker {
public:
    Locker() {
        if (pthread_mutex_init(&mutex_, NULL) != 0) {  // 初始化，NULL表示使用默认值
            throw std::exception();
        }
    }
    ~Locker() {
        pthread_mutex_destroy(&mutex_);
    }
    bool lock() {
        return pthread_mutex_lock(&mutex_) == 0;
    }
    bool unlock() {
        return pthread_mutex_unlock(&mutex_) == 0;
    }
    pthread_mutex_t* get() {
        return &mutex_;
    }
private:
    pthread_mutex_t mutex_;
};

// 条件变量类
class Cond {
public:
    Cond() {
        if (pthread_cond_init(&cond_, NULL) != 0) {
            throw std::exception();
        }
    }
    ~Cond() {
        pthread_cond_destroy(&cond_);
    }
    bool wait(pthread_mutex_t* mutex) {
        int ret = 0;
        ret = pthread_cond_wait(&cond_, mutex);
        return ret == 0;
    }
    bool timewait(pthread_mutex_t* mutex, struct timespec t) {
        int ret = 0;
        ret = pthread_cond_timedwait(&cond_, mutex, &t);
        return ret;
    }
    bool signal() {
        return pthread_cond_signal(&cond_) == 0;
    }
    bool broadcast() {
        return pthread_cond_broadcast(&cond_) == 0;
    }
private:
    pthread_cond_t cond_;
};
#endif