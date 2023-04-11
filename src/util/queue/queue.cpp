#include "queue.h"

#include <iostream>
#include <queue>

void Queue::operator=(const Queue &q) { this->data = q.data; }

int Queue::enqueue(uint8_t *array, int bytesToWrite) {
    std::scoped_lock<std::mutex> lock(lockMutex);

    data.insert(data.end(), array, array + bytesToWrite);
    return bytesToWrite;
}

int Queue::dequeue(uint8_t *array, int bytesToRead) {
    std::scoped_lock<std::mutex> lock(lockMutex);

    std::copy_n(std::make_move_iterator(data.begin()), bytesToRead, array);
    data.erase(data.begin(), data.begin() + bytesToRead);
    return bytesToRead;
}

int Queue::copy_from(uint8_t *array, int bytesToRead, int offset) {
    std::scoped_lock<std::mutex> lock(lockMutex);

    std::copy_n(data.begin() + offset, bytesToRead, array);
    return bytesToRead;
}

int Queue::remove(int bytesToRemove) {
    std::scoped_lock<std::mutex> lock(lockMutex);

    data.erase(data.begin(), data.begin() + bytesToRemove);
    return bytesToRemove;
}

uint64_t Queue::clear() {
    std::scoped_lock<std::mutex> lock(lockMutex);

    uint64_t size = this->data.size();
    this->data.clear();

    return size;
}

uint64_t Queue::get_size() {
    std::scoped_lock<std::mutex> lock(lockMutex);
    return this->data.size();
}

bool Queue::empty() {
    std::scoped_lock<std::mutex> lock(lockMutex);
    return this->data.empty();
}
