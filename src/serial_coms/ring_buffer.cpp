#include "ring_buffer.h"

RingBuffer::RingBuffer() : head(0), tail(0) {}

bool RingBuffer::push(uint8_t byte)
{
    uint16_t next = (head + 1) % RING_BUFFER_SIZE;
    if (next == tail)
    {
        return false; // buffer full
    }
    buffer[head] = byte;
    head = next;
    return true;
}

bool RingBuffer::pop(uint8_t &byte)
{
    if (head == tail)
    {
        return false; // buffer empty
    }
    byte = buffer[tail];
    tail = (tail + 1) % RING_BUFFER_SIZE;
    return true;
}

bool RingBuffer::isEmpty() const
{
    return head == tail;
}

void RingBuffer::clear()
{
    head = tail = 0;
}
