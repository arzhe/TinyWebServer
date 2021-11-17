#ifndef WEB_SERVER_LOG_BLOCK_QUEUE_H
#define WEB_SERVER_LOG_BLOCK_QUEUE_H

#include <cstddef>
#include <mutex>
#include <deque>
#include <condition_variable>
#include <chrono>

template <class T>
class BlockDeque{
 private:
    std::deque<T> m_deq;

    size_t m_capacity;

    std::mutex m_mtx;

    bool m_is_closed;

    std::condition_variable m_cond_customer;

    std::condition_variable m_cond_producer;
   
public:
    explicit BlockDeque(size_t MaxCapacity = 1000);

    ~BlockDeque();

    void clear();

    bool empty();

    bool full();

    bool pop(T& item);

    bool pop(T& item, int timeout);

    void Close();

    size_t size();

    size_t capacity();

    T front();

    T back();

    void push_back(const T& item);

    void push_front(const T& item);

    void flush();

};

#endif
