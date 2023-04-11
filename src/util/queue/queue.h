#ifndef QUEUE_H
#define QUEUE_H

#include <iostream>
#include <mutex>
#include <queue>

/* 
 A Queue is defined as a linear data structure that is open at both ends and 
 the operations are performed in First In First Out (FIFO) order.
 It is thread safe
*/
class Queue {
   public:
    void operator=(const Queue &q);

    int enqueue(uint8_t *array, int bytesToWrite);
    int dequeue(uint8_t *array, int bytesToRead);
    int copy_from(uint8_t *array, int bytesToRead, int offset = 0);
    int remove(int bytesToRemove);
    uint64_t clear();

    uint64_t get_size();
    bool empty();


   private:
    std::mutex lockMutex;
    std::deque<uint8_t> data = {};
};

#endif /* QUEUE_H */
