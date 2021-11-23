#include "./heap_timer.h"

void HeapTimer::SiftUp(size_t i) {
    assert(i >= 0 && i < heap.size());
    
    // Parent Node index.
    size_t j = (i - 1) / 2;

    while(j >= 0) {
        if(heap[j] < heap[i]) break;
        SwapNode(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

bool HeapTimer::SiftDown(size_t index, size_t n) {
    assert(index >= 0 && index <= n && n < heap.size());

    size_t i = index;
    size_t j = i * 2 + 1;
    while(j < n) {
        if(j + 1 < n && heap[j + 1] < heap[j]) ++j;
        if(heap[i] < heap[j]) break;
        SwapNode(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void HeapTimer::SwapNode(size_t i, size_t j) {
    assert(i >= 0 && i < heap.size());
    assert(j >= 0 && j < heap.size());
    std::swap(heap[i], heap[j]);
    ref[heap[i].id] = i;
    ref[heap[j].id] = j;
}

void HeapTimer::Delete(size_t index) {
    assert(!heap.empty() && index >= 0 && index < heap.size());

    // Move the Node to deleted to the end of heap.
    size_t i = index;
    size_t tail_index = heap.size() - 1;
    assert(i <= tail_index);
    if(i < tail_index) {
        SwapNode(i, tail_index);
        if(!SiftDown(i, tail_index))
            SiftUp(i);
    }

    ref.erase(heap.back().id);
    heap.pop_back();
}

void HeapTimer::Add(int id, int timeout, const TimeOutCallBack& cb) {
    assert(id >= 0);
    size_t i;
    if(ref.count(id) == 0) {
        // A new Node is inserted to the end of heap.
        // And then the heap is adjusted.
        i = heap.size();
        ref[id] = i;
        heap.push_back({id, Clock::now() + MS(timeout), cb});
        SiftUp(i);
    }
    else {
        i = ref[id];
        heap[i].expires = Clock::now() + MS(timeout);
        heap[i].cb = cb;
        if(!SiftDown(i, heap.size()))
            SiftUp(i);
    }
}

void HeapTimer::DoWork(int id) {
    // Delete Node id, and trigger the callback function.
    if(heap.empty() || ref.count(id) == 0)
        return;

    size_t i = ref[id];
    TimerNode node = heap[i];
    node.cb();
    Delete(i);
}

// Clears timeout Nodes.
void HeapTimer::Tick() {
    if(heap.empty())
        return;
    while(!heap.empty()) {
        TimerNode node = heap.front();
        if(std::chrono::duration<MS>(node.expires - Clock::now()).count() > 0)
            break;
        node.cb();
        Pop();
    }
}

void HeapTimer::Pop() {
    assert(!heap.empty());
    Delete(0);
}

void HeapTimer::Clear() {
    std::unordered_map<int, size_t>().swap(ref);
    std::vector<TimerNode>().swap(heap);
}

int HeapTimer::GetNextTick() {
    Tick();
    size_t res = -1;
    if(!heap.empty()) {
        res = std::chrono::duration_cast<MS>(heap.front().expires - Clock::now()).count();
        if(res < 0) res = 0;       
    }
    return res;
}

// Deprecated!
void HeapTimer::Adjust(int id, int timeout) {
    assert(!heap.empty() && ref.count(id) > 0);

    heap[ref[id]].expires = Clock::now() + MS(timeout);
    SiftDown(ref[id], heap.size());
}
