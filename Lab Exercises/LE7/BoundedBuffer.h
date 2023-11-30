#ifndef _BOUNDEDBUFFER_H_
#define _BOUNDEDBUFFER_H_

#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>

class BoundedBuffer {
private:
    // max number of items in the buffer
    int cap;

    // The queue of items in the buffer
    std::queue<std::vector<char>> q;

    // Add necessary synchronization variables and data structures
    std::mutex m;
    std::condition_variable not_full;
    std::condition_variable not_empty;

public:
    BoundedBuffer(int _cap);
    ~BoundedBuffer();

    void push(char* msg, int size);
    int pop(char* msg, int size);

    size_t size();
};

#endif