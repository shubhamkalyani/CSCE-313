#include "BoundedBuffer.h"
#include <mutex>
#include <condition_variable>

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
    std::unique_lock<std::mutex> lock(m);

    not_empty.wait(lock, [this] { return !q.empty(); });

    std::vector<char> data = std::move(q.front());
    q.pop();

    size_t dataSize = data.size();
    if (dataSize > static_cast<size_t>(size)) {
        return -1;
    }

    std::copy(data.begin(), data.end(), msg);

    not_full.notify_one();

    return static_cast<int>(dataSize);
}

size_t BoundedBuffer::size() {
    unique_lock<mutex> lock(m);
    return q.size();
}