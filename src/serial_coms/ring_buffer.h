#pragma once
#include <stdint.h>
#include <atomic>

#define RING_BUFFER_SIZE 512

class RingBuffer
{
public:
    RingBuffer();

    bool push(uint8_t byte); // Called from ISR
    bool pop(uint8_t &byte); // Called from task
    bool isEmpty() const;
    void clear();

private:
    uint8_t buffer[RING_BUFFER_SIZE];
    std::atomic<uint16_t> head;
    std::atomic<uint16_t> tail;
};
