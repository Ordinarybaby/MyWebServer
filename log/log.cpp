#include "log.h"

// struct tm
// {
//     int tm_sec;  /*秒，正常范围0-59， 但允许至61*/
//     int tm_min;  /*分钟，0-59*/
//     int tm_hour; /*小时， 0-23*/
//     int tm_mday; /*日，即一个月中的第几天，1-31*/
//     int tm_mon;  /*月， 从一月算起，0-11*/  1+p->tm_mon;
//     int tm_year;  /*年， 从1900至今已经多少年*/  1900＋ p->tm_year;
//     int tm_wday; /*星期，一周中的第几天， 从星期日算起，0-6*/
//     int tm_yday; /*从今年1月1日到目前的天数，范围0-365*/
//     int tm_isdst; /*日光节约时间的旗标*/
// };

// C++ 时间函数详解：https://www.runoob.com/w3cnote/cpp-time_t.html


// 默认构造函数定义
Log::Log() {
    count_ = 0;
    is_async_ = false;
}
// 析构函数定义
Log::~Log() {
    if (fp_ != NULL) {
        fclose(fp_);
    }
}
// 初始化日志，异步设置阻塞队列的大小，同步不需要设置
bool Log::init(const char* file_name, int log_buf_size, int split_lines, int max_queue_size) {
    if (max_queue_size > 0) {
        is_async_ = true;
        log_queue_ = new BlockQueue<std::string>(max_queue_size);
        pthread_t tid;
        pthread_create(&tid, NULL, flush_log_thread, NULL);
    }

    log_buf_size_ = log_buf_size;
    buf_ = new char[log_buf_size];
    memset(buf_, '\0', log_buf_size);
    split_lines_ = split_lines;

    time_t t = time(NULL);  // 获得从1970年1月1日，到现在经历了多少秒
    struct tm* sys_tm = localtime(&t);  // //把当前时间系统所偏移的秒数时间转换为本地时间
    struct tm my_tm = *sys_tm;

    const char* p = strrchr(file_name, '/'); // 返回file_name最后一个/字符及其之后的所有字符串
    char log_full_name[256] = {0};
    // 格式化log文件的名字
    if (p == NULL) {
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, 
        my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    } else {
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);  // 相当于迭代器作差，返回两个指针之间的距离
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900,
        my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }
    today_ = my_tm.tm_mday;
    fp_ = fopen(log_full_name, "a");
    if (fp_ == NULL) return false;
    return true;
}
// 写入一个日志
void Log::write_log(int level, const char* format, ...) {
    struct timeval now = {0, 0}; // 秒和微秒，能达到微妙精度
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch (level) 
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[erro]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }
    mutex_.lock();
    count_++; // 日志行+1

    // 判断是否按天分，是否超过单个日志最大行
    if (today_ != my_tm.tm_mday || count_ % split_lines_ == 0) {
        char new_log[256] = {0};
        fflush(fp_);
        fclose(fp_);
        char tail[16] = {0};

        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (today_ != my_tm.tm_mday) {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            today_ = my_tm.tm_mday;
            count_ = 0;
        } else {
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, count_ / split_lines_);
        }
        fp_ = fopen(new_log, "a");
    }

    mutex_.unlock();

    va_list valst; // 获取函数的变参，可以看成一个指针
    va_start(valst, format);  // 指针指向format中的第一个元素

    std::string log_str;
    mutex_.lock();
    // 写入的具体时间内容格式
    int n = snprintf(buf_, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    int m = vsnprintf(buf_ + n, log_buf_size_ - 1, format, valst);
    buf_[n + m] = '\n';
    buf_[n + m + 1] = '\0';
    log_str = buf_;
    mutex_.unlock();

    if (is_async_ && !log_queue_->full()) {
        log_queue_->push(log_str);  // ## 这里满了以后直接写入是否有问题？？
    } else {
        mutex_.lock();
        fputs(log_str.c_str(), fp_);
        mutex_.unlock();
    }
    va_end(valst);  // 将valst置为无效
}

void Log::flush(void) {
    mutex_.lock();
    // 强行刷新写入流缓冲区
    fflush(fp_);
    mutex_.unlock();
}