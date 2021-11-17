#ifndef WEB_SERVER_POOL_SQL_CONNECT_POOL_H
#define WEB_SERVER_POOL_SQL_CONNECT_POOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <semaphore.h>

#include "../log/log.h"

class SqlConnectPool {
private:
    int MAX_CONNECT;
    int use_count;
    int free_count;

    std::queue<MYSQL*> connect_queue;
    std::mutex mtx;
    sem_t sem_id;

    SqlConnectPool();
    ~SqlConnectPool();

public:
    static SqlConnectPool* Instance();

    MYSQL* GetConnect();
    void FreeConnect(MYSQL* connect);
    int GetFreeConnectCount();

    void Init(const char* host, int port,
              const char* user, const char* pwd,
              const char* db_name, int connect_size);
    void ClosePool();

};

#endif
