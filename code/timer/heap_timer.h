#ifndef WEB_SERVER_TIMER_HEAP_TIMER_H
#define WEB_SERVER_TIMER_HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <chrono>
#include <arpa/inet.h>
#include <time.h>
#include <assert.h>

#include "../log/log.h"

typedef std::function<void()> TimeOutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
    int id;
    TimeStamp expires;
    TimeOutCallBack cb;
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};

// The parent Node is smaller than it's child Nodes.
// eg:
//          Node<0, 1>
//           /      \
//    Node<1, 4>  Node<2, 3>
//
class HeapTimer {
private:
    std::vector<TimerNode> heap;

    // ref.first: heap[index]::id.
    // ref.second: index of heap.
    std::unordered_map<int, size_t> ref;

    void Delete(size_t index);
    
    // Upward adjustment from Node i.
    void SiftUp(size_t i);

    // Downward adjustment from Node index to Node n.
    bool SiftDown(size_t index, size_t n);
    
    void SwapNode(size_t i, size_t j);
    
public:
    HeapTimer() { this->heap.reserve(64); }

    ~HeapTimer() { Clear(); }

    void Adjust(int id, int timeout);

    void Add(int id, int time_out, const TimeOutCallBack& cb);

    void DoWork(int id);

    void Clear();

    void Tick();

    void Pop();

    int GetNextTick();
};

#endif
