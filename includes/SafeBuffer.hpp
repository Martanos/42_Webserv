#ifndef SAFE_BUFFER_HPP
#define SAFE_BUFFER_HPP

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include "Constants.hpp"

// Wrapper for buffer operations
class SafeBuffer
{
private:
	char *_buffer;
	size_t _size;
	size_t _capacity;

	// Non-copyable
	SafeBuffer(const SafeBuffer &);
	SafeBuffer &operator=(const SafeBuffer &);

	void _reallocate(size_t newCapacity);

public:
	explicit SafeBuffer(size_t initialCapacity = HTTP::DEFAULT_BUFFER_SIZE);
	~SafeBuffer();

	// Buffer operations
	void append(const char *data, size_t length);
	void append(const std::string &data);
	void clear();
	void reserve(size_t newCapacity);

	// Access methods
	const char *data() const;
	char *data();
	size_t size() const;
	size_t capacity() const;
	bool empty() const;

	// String conversion
	std::string toString() const;

	// Utility
	void consume(size_t bytes);
	size_t find(const char *pattern) const;
	size_t find(const std::string &pattern) const;
};

#endif /* SAFE_BUFFER_HPP */
