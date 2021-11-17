#ifndef WEB_SERVER_LOG_LOG_H
#define WEB_SERVER_LOG_LOG_H

#include <mutex>
#include <thread>
#include <stdarg.h>    //va_start va_end
#include <sys/time.h>
#include <sys/stat.h>  // mdkir
#include <assert.h>

#include "./block_queue.h"
#include "../buffer/buffer.h"

class Log {
private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int LOG_MAX_LINES = 50000;

    const char* path;
    const char* suffix;

    int MAX_LINES;
    
    int line_count;
    int time_of_day;

    bool is_open;
    
    Buffer buff;
    int level;
    bool is_async;

    FILE* fp;
    std::unique_ptr<BlockDeque<std::string>> deque;
    std::unique_ptr<std::thread> write_thread;
    std::mutex mtx;

public:
    void Init(int level = 1, const char* path = "./log",
              const char* suffix = ".log",
              int max_queue_capacity = 1024);

    static Log* Instance();
    static void FlushLogThread();

    void Write(int level, const char* format,...);
    void Flush();

    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() { return is_open; }

private:
    Log();
    void AppendLogLevelTitle(int level);
    virtual ~Log();
    void AsyncWrite();
};

#define LOG_BASE(level, format, ...)                    \
    do{                                                 \
       Log* log = Log::Instance();                      \
       if(log->IsOpen() && log->GetLevel() <= level) {  \
          log->Write(level, format, ##__VA_ARGS__);     \
          log->Flush();                                 \
       }                                                \
    } while(0);

#define LOG_DEBUG(format, ...) do { LOG_BASE(0, format, ##__VA_ARGS__) } while(0);
#define LOG_INFO(format, ...) do { LOG_BASE(1, format, ##__VA_ARGS__) } while(0);
#define LOG_WARN(format, ...) do { LOG_BASE(2, format, ##__VA_ARGS__) } while(0);
#define LOG_ERROR(format, ...) do { LOG_BASE(3, format, ##__VA_ARGS__) } while(0);

#endif
