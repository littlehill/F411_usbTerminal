#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "mbed.h"

class RingBuffer {
    private:
        uint32_t maxindex;
    	uint8_t* data;
	    size_t size;
	    size_t readpos, writepos;
        bool bufferFullError;
    
    public:
    RingBuffer();
    RingBuffer(size_t buflen);
    ~RingBuffer();
    
    bool canRead(size_t length);
    bool canWrite(size_t length);
    size_t tryRead(uint8_t* data, size_t length);
    bool write(uint8_t* data, size_t length);
    void writeUnchecked(const uint8_t* data, size_t length);
    bool getFullError();
    void clearFullError();
    void reset(); /*W!! clears all data*/
    uint16_t GetRxCountInBuffer();
};

#endif
