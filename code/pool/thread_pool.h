#ifndef WEB_SERVER_POOL_THREAD_POOL_H
#define WEB_SERVER_POOL_THREAD_POOL_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <assert.h>

class ThreadPool {
private:
    struct Pool {
        std::mutex mtx;
        std::condition_variable cond;
        bool is_closed;
        std::queue<std::function<void()>> tasks;
    };

    std::shared_ptr<Pool> pool;

public:
    explicit ThreadPool(size_t thread_count = 8) : pool(std::make_shared<Pool>()) {
        assert(thread_count > 0);
        for(size_t i = 0; i < thread_count; ++i) {
            std::thread([p = pool] {
                std::unique_lock<std::mutex> locker(p->mtx);
                while(true) {
                    if(!p->tasks.empty()) {
                        auto task = std::move(p->tasks.front());
                        p->tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    }
                    else if(p->is_closed) break;
                    else p->cond.wait(locker);
                }
            }).detach();
        }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool&&) = default;

    ~ThreadPool() {
        if(static_cast<bool>(pool)) {
            std::lock_guard<std::mutex> locker(pool->mtx);
            pool->is_closed = true;
        }
        pool->cond.notify_all();
    }

    template<class F>
    void AddTask(F&& task) {
        {
            std::lock_guard<std::mutex> locker(pool->mtx);
            pool->tasks.emplace(std::forward<F>(task));
        }
        pool->cond.notify_one();
    }
};

#endif
