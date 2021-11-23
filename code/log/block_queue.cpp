#include <assert.h>

#include "./block_queue.h"

template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity) : m_capacity(MaxCapacity) {
    assert(MaxCapacity > 0);
    m_is_closed = false;
}

template<class T>
BlockDeque<T>::~BlockDeque() {
    Close();
}

template<class T>
void BlockDeque<T>::Close() {
    {
        std::lock_guard<std::mutex> locker(m_mtx);
        m_deq.clear();
        m_is_closed = true;
    }

    m_cond_customer.notify_all();
    m_cond_producer.notify_all();
}

template<class T>
void BlockDeque<T>::flush() {
    m_cond_customer.notify_one();
}

template<class T>
void BlockDeque<T>::clear() {
    std::lock_guard<std::mutex> locker(m_mtx);
}

template<class T>
T BlockDeque<T>::front() {
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_deq.front();
}

template<class T>
T BlockDeque<T>::back() {
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_deq.back();
}

template<class T>
size_t BlockDeque<T>::size() {
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_deq.size();
}

template<class T>
size_t BlockDeque<T>::capacity() {
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_capacity;
}

template<class T>
void BlockDeque<T>::push_back(const T& item) {
    std::unique_lock<std::mutex> locker(m_mtx);
    while(m_deq.size() >= m_capacity) {
        m_cond_producer.wait(locker);
    }

    m_deq.push_back(item);
    m_cond_customer.notify_one();
}

template<class T>
void BlockDeque<T>::push_front(const T& item) {
    std::unique_lock<std::mutex> locker(m_mtx);
    while(m_deq.size() >= m_capacity) {
        m_cond_producer.wait(locker);
    }
    m_deq.push_front(item);
    m_cond_customer.notify_one();
}

template<class T>
bool BlockDeque<T>::empty() {
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_deq.empty();
}

template<class T>
bool BlockDeque<T>::full() {
    std::lock_guard<std::mutex> locker(m_mtx);
    return m_deq.size() >= m_capacity;
}

template<class T>
bool BlockDeque<T>::pop(T& item) {
    std::unique_lock<std::mutex> locker(m_mtx);
    while(m_deq.empty()) {
        m_cond_customer.wait(locker);
        if(m_is_closed) {
            return false;
        }
    }

    item = m_deq.front();
    m_deq.pop_front();
    m_cond_producer.notify_one();
    return true;
}

template<class T>
bool BlockDeque<T>::pop(T& item, int timeout) {
    std::unique_lock<std::mutex> locker(m_mtx);
    while(m_deq.empty()) {
        if(m_cond_customer.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout) {
            return false;
        }
        if(m_is_closed) {
            return false;
        }
    }

    item = m_deq.front();
    m_deq.pop_front();
    m_cond_producer.notify_one();
    return true;
}
