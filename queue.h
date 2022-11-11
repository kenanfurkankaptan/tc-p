#ifndef QUEUE_H
#define QUEUE_H

#include <iostream>
#include <mutex>
#include <queue>

class Queue {
   public:
    /** TODO: consider constant sized container */
    std::deque<uint8_t> data;

    void operator=(const Queue &q);

    int enqueue(uint8_t *array, int bytesToWrite);
    int dequeue(uint8_t *array, int bytesToRead);

   private:
    std::mutex lockMutex;
};

#endif /* QUEUE_H */
