#include "queue.h"

#include <iostream>
#include <queue>

void Queue::operator=(const Queue &q) {
    this->data = q.data;
}

int Queue::enqueue(uint8_t *array, int bytesToWrite) {
    std::lock_guard<std::mutex> lock(lockMutex);
    data.insert(data.end(), array, array + bytesToWrite);
    return bytesToWrite;
}

int Queue::dequeue(uint8_t *array, int bytesToRead) {
    std::lock_guard<std::mutex> lock(lockMutex);

    /** TODO: consider move instead of copy */
    std::copy_n(data.begin(), bytesToRead, array);
    data.erase(data.begin(), data.begin() + bytesToRead);
    return bytesToRead;
}
