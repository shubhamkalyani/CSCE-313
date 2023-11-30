#include "BoundedBuffer.h"
#include <mutex>
#include <condition_variable>

#include <cstring>

using namespace std;

BoundedBuffer::BoundedBuffer(int _cap) : cap(_cap) {}

BoundedBuffer::~BoundedBuffer() {}

void BoundedBuffer::push(char* msg, int size) {
    vector<char> data(msg, msg + size);
    unique_lock<mutex> lock(m);
    not_full.wait(lock, [this] { return q.size() < static_cast<size_t>(cap); });
    q.push(data);
    not_empty.notify_one();
}

int BoundedBuffer::pop(char* msg, int size) {
    unique_lock<mutex> lock(m);
    not_empty.wait(lock, [this] { return q.size() > 0; });

    vector<char> data = q.front();
    q.pop();
    lock.unlock();

    if (data.size() > static_cast<size_t>(size)) {
        throw runtime_error("size error");
    }
    
    memcpy(msg, data.data(), data.size());

    not_full.notify_one();

    return static_cast<int>(data.size());
}

size_t BoundedBuffer::size() {
    // unique_lock<mutex> lock(m);
    return q.size();
}
