#ifndef LOG_H
#define LOG_H
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#include "block_queue.h"

class Log {
public:
    // C++11之后，使用局部静态变量懒汉不用加锁
    static Log* get_instance() {
        static Log instance;
        return &instance;
    }

    static void* flush_log_thread(void* args) {
        Log::get_instance()->async_write_log();
    }
    // 参数：日志文件、日志缓冲区大小、最大行数、最长日志条队列
    bool init(const char* file_name, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);
    
    void write_log(int level, const char* format, ...);

    void flush(void);
private:
    Log();
    virtual ~Log();

    // 异步写日志
    void* async_write_log() {
        std::string single_log;
        // 从阻塞队列中取出一个日志string，写入文件
        while (log_queue_->pop(single_log)) {
            mutex_.lock();
            fputs(single_log.c_str(), fp_);
            mutex_.unlock();
        }
    }
private:
    char dir_name[128]; // 路径名
    char log_name[128]; // log文件名
    int split_lines_;   // 日志最大行数
    int log_buf_size_;  // 日志缓冲区大小
    long long count_;   // 日志行数统计
    int today_;         // 记录当前时间是哪一天
    FILE* fp_ ;         // 打开log的文件指针
    char* buf_;         // 缓冲区
    BlockQueue<std::string>* log_queue_; // 阻塞队列
    bool is_async_;     // 是否同步标志位
    Locker mutex_;      // 互斥锁
};

#define LOG_DEBUG(format, ...) Log::get_instance()->write_log(0, format, ##__VA_ARGS__);
#define LOG_INFO(format, ...) Log::get_instance()->write_log(1, format, ##__VA_ARGS__);
#define LOG_WARN(format, ...) Log::get_instance()->write_log(2, format, ##__VA_ARGS__);
#define LOG_ERROR(format, ...) Log::get_instance()->write_log(3, format, ##__VA_ARGS__);


#endif