#include "./sql_connect_pool.h"

SqlConnectPool::SqlConnectPool() {
    use_count = 0;
    free_count = 0;
}

SqlConnectPool::~SqlConnectPool() {
    ClosePool();
}

SqlConnectPool* SqlConnectPool::Instance() {
    static SqlConnectPool connect_pool;
    return &connect_pool;
}

void SqlConnectPool::Init(const char* host, int port,
                          const char* user, const char* pwd,
                          const char* db_name, int connect_size) {
    assert(connect_size > 0);
    for(int i = 0; i < connect_size; ++i) {
        MYSQL* sql = nullptr;
        sql = mysql_init(sql);
        if(!sql) {
            LOG_ERROR("MySql init error!");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host,
                                 user, pwd,
                                 db_name, port, nullptr, 0);
        if(!sql) {
            LOG_ERROR("MySql Connect error!");
        }
        connect_queue.push(sql);
        MAX_CONNECT = connect_size;
        sem_init(&sem_id, 0, MAX_CONNECT);
    }
}

MYSQL* SqlConnectPool::GetConnect() {
    MYSQL* sql = nullptr;
    if(connect_queue.empty()) {
        LOG_WARN("SqlConnectPool busy!");
        return nullptr;
    }
    sem_wait(&sem_id);
    {
        std::lock_guard<std::mutex> locker(mtx);
        sql = connect_queue.front();
        connect_queue.pop();
    }
    return sql;
}

void SqlConnectPool::FreeConnect(MYSQL* sql) {
    assert(sql);
    std::lock_guard<std::mutex> locker(mtx);
    connect_queue.push(sql);
    sem_post(&sem_id);
}

void SqlConnectPool::ClosePool() {
    std::lock_guard<std::mutex> locker(mtx);
    while(!connect_queue.empty()) {
        auto item = connect_queue.front();
        connect_queue.pop();
        mysql_close(item);
    }
    mysql_library_end();
}

int SqlConnectPool::GetFreeConnectCount() {
    std::lock_guard<std::mutex> locker(mtx);
    return connect_queue.size();
}
