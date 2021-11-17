#include "./log.h"

Log::Log() {
    line_count = 0;
    time_of_day = 0;
    is_async = false;
    fp = nullptr;
    deque = nullptr;
    write_thread = nullptr;
}

Log::~Log() {
    if(write_thread && write_thread->joinable()) {
        while(!deque->empty()) {
            deque->flush();
        };
        deque->Close();
        write_thread->join();
    }
    if(fp) {
        std::lock_guard<std::mutex> locker(mtx);
        Flush();
        fclose(fp);
    }
}

int Log::GetLevel() {
    std::lock_guard<std::mutex> locker(mtx);
    return level;
}

void Log::SetLevel(int level) {
    std::lock_guard<std::mutex> locker(mtx);
    this->level = level;
}

void Log::Init(int level, const char* path, const char* suffix, int max_queue_capacity) {
    is_open = true;
    this->level = level;
    if(max_queue_capacity > 0) {
        is_async = true;
        if(!deque) {
            std::unique_ptr<BlockDeque<std::string>> new_deque(new BlockDeque<std::string>);
            deque = move(new_deque);

            std::unique_ptr<std::thread> new_thread(new std::thread(FlushLogThread));;
            write_thread = move(new_thread);
        }
    }
    else {
        is_async = false;
    }

    line_count = 0;

    time_t timer = time(nullptr);
    struct tm* sys_time = localtime(&timer);
    struct tm t = *sys_time;
    this->path = path;
    this->suffix = suffix;
    char file_name[LOG_NAME_LEN] = {0};
    snprintf(file_name, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
             this->path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, this->suffix);
    time_of_day = t.tm_mday;

    {
        std::lock_guard<std::mutex> locker(mtx);
        buff.RetrieveAll();
        if(fp) {
            Flush();
            fclose(fp);
        }

        fp = fopen(file_name, "a");
        if(fp == nullptr) {
            mkdir(this->path, 0777);
            fp = fopen(file_name, "a");
        }
        assert(fp != nullptr);
    }
}

void Log::Write(int level, const char* format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t t_sec = now.tv_sec;
    struct tm* sys_time = localtime(&t_sec);
    struct tm t = *sys_time;
    va_list v_list;
    
    if(time_of_day != t.tm_mday || (line_count && (line_count % MAX_LINES == 0))) {
        std::unique_lock<std::mutex> locker(mtx);
        locker.unlock();

        char new_file[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        
        if(time_of_day != t.tm_mday) {
            snprintf(new_file, LOG_NAME_LEN - 72, "%s/%s%s", this->path, tail, this->suffix);
            time_of_day = t.tm_mday;
            line_count = 0;
        }
        else {
            snprintf(new_file, LOG_NAME_LEN -72, "%s/%s-%d%s", this->path, tail, (line_count / MAX_LINES), this->suffix);
        }

        locker.lock();
        Flush();
        fclose(fp);
        fp = fopen(new_file, "a");
        assert(fp != nullptr);
    }

    {
        std::unique_lock<std::mutex> locker(mtx);
        line_count++;
        int n = snprintf(buff.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                         t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                         t.tm_hour, t.tm_min, t.tm_sec, static_cast<long>(now.tv_usec));

        buff.HasWritten(n);
        AppendLogLevelTitle(level);

        va_start(v_list, format);
        int m = vsnprintf(buff.BeginWrite(), buff.WritableBytes(), format, v_list);
        va_end(v_list);

        buff.HasWritten(m);
        buff.Append("\n\0", 2);

        if(is_async && deque && !deque->full()) {
            deque->push_back(buff.RetrieveAllToStr());
        }
        else {
            fputs(buff.Peek(), fp);
        }

        buff.RetrieveAll();
    }
}

void Log::AppendLogLevelTitle(int level) {
    switch(level) {
    case 0:
        buff.Append("[debug]: ", 9);
        break;
    case 1:
        buff.Append("[info] : ", 9);
        break;
    case 2:
        buff.Append("[warn] : ", 9);
        break;
    case 3:
        buff.Append("[error]: ", 9);
        break;
    default:
        buff.Append("[info] : ", 9);
        break;
    }
}

void Log::Flush() {
    if(is_async) {
        deque->flush();
    }
    fflush(fp);
}

void Log::AsyncWrite() {
    std::string str = "";
    while(deque->pop(str)) {
        std::lock_guard<std::mutex> locker(mtx);
        fputs(str.c_str(), fp);
    }
}

Log* Log::Instance() {
    static Log inst;
    return &inst;
}

void Log::FlushLogThread() {
    Log::Instance()->AsyncWrite();
}
