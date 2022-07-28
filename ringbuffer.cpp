#include "ringbuffer.h"
#include <cstdint>

RingBuffer::RingBuffer() {
    data = new uint8_t[256];
    size=256;
    readpos = 0;
    writepos = 0;
    maxindex = this->size - 1;
    bufferFullError = false;
};

RingBuffer::RingBuffer(size_t buflen) {
    data = new uint8_t[buflen];
    size=buflen;
    readpos = 0;
    writepos = 0;
    maxindex = this->size - 1;
    bufferFullError = false;
};

RingBuffer::~RingBuffer() {
    delete data;

};

uint16_t RingBuffer::GetRxCountInBuffer() {
    return writepos - readpos;
}

bool RingBuffer::canRead(size_t length) {
	const size_t readpos = this->readpos;
	const size_t writepos = this->writepos;

	if (readpos <= writepos)
		return readpos + length <= writepos;
	else
		return readpos + length <= this->size + writepos;
}

bool RingBuffer::canWrite(size_t length) {
	const size_t readpos = this->readpos;
	const size_t writepos = this->writepos;

	if (writepos < readpos)
		return writepos + length < readpos;
	else
		return writepos + length < this->size + readpos;
}

size_t RingBuffer::tryRead(uint8_t* data, size_t length) {
	size_t read = 0;

	while (readpos != this->writepos && read < length) {
		*data++ = this->data[readpos++];
		readpos = (readpos > maxindex) ? 0 : readpos;
		read++;
	}
	return read;
}

bool RingBuffer::write(uint8_t* data, size_t length) {
	if (!this->canWrite(length)) {
        this->bufferFullError = true;
        return 0;
    }

	this->writeUnchecked(data, length);
	return 1;
}

void RingBuffer::writeUnchecked(const uint8_t* datain, size_t length) {
	while (length--) {
		this->data[writepos++] = *datain++;
        writepos = (writepos > maxindex) ? 0 : writepos;
	}
}

bool RingBuffer::getFullError() {
    return bufferFullError;
}

void RingBuffer::clearFullError() {
    this->bufferFullError = false;
}

void RingBuffer::reset() {
    readpos = 0;
    writepos = 0;
    bufferFullError = false;
}