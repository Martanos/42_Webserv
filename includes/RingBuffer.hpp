#ifndef RINGBUFFER_HPP
#define RINGBUFFER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <cstddef>
#include "FileDescriptor.hpp"

// Ring buffer is a circular buffer that can be used to store data
// More efficient than most other buffers
class RingBuffer
{
private:
	std::vector<char> _buffer;
	size_t _head;
	size_t _tail;
	size_t _capacity;

	size_t _advance(size_t pos, size_t n) const;

public:
	// Constructors
	RingBuffer();
	RingBuffer(size_t size);
	RingBuffer(const RingBuffer &src);
	~RingBuffer();

	RingBuffer &operator=(const RingBuffer &rhs);

	// Accessors
	size_t readable() const;
	size_t writable() const;
	bool empty() const;
	bool full() const;
	size_t contains(const char *data, size_t len) const;
	size_t capacity() const;

	// Mutators
	size_t writeBuffer(const char *data, size_t len);
	size_t writeBuffer(const RingBuffer &src, size_t len);
	size_t readBuffer(std::string &dest, size_t len);
	size_t peekBuffer(std::string &dest, size_t len) const;
	size_t transferFrom(RingBuffer &src, size_t len);
	size_t transferTo(RingBuffer &dest, size_t len);
	size_t appendBuffer(const RingBuffer &src);
	size_t readBuffer(char *dest, size_t len);
	size_t peekBuffer(char *dest, size_t len) const;
	size_t flushToFile(const std::string &filePath);
	size_t flushToFile(FileDescriptor &fd);
	void reserve(size_t newCapacity);
	void consume(size_t len);
	void reset();
	void clear();
};

#endif /* ****************************************************** RINGBUFFER_H */
