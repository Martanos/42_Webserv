#include "../../includes/RingBuffer.hpp"
#include "../../includes/Logger.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

RingBuffer::RingBuffer() : _head(0), _tail(0), _capacity(4096)
{
	_buffer.resize(_capacity);
}

RingBuffer::RingBuffer(size_t size) : _head(0), _tail(0), _capacity(size)
{
	if (_capacity == 0)
		_capacity = 4096;
	_buffer.resize(_capacity);
}

RingBuffer::RingBuffer(const RingBuffer &src) : _buffer(src._buffer), _head(src._head), _tail(src._tail), _capacity(src._capacity)
{
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

RingBuffer::~RingBuffer()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

RingBuffer &RingBuffer::operator=(const RingBuffer &rhs)
{
	if (this != &rhs)
	{
		_buffer = rhs._buffer;
		_head = rhs._head;
		_tail = rhs._tail;
		_capacity = rhs._capacity;
	}
	return *this;
}

/*
** --------------------------------- ACCESSORS --------------------------------
*/

size_t RingBuffer::readable() const
{
	if (_tail >= _head)
		return _tail - _head;
	else
		return _capacity - _head + _tail;
}

size_t RingBuffer::writable() const
{
	return _capacity - readable() - 1; // Leave one byte free to distinguish full from empty
}

bool RingBuffer::empty() const
{
	return _head == _tail;
}

bool RingBuffer::full() const
{
	return (_tail + 1) % _capacity == _head;
}

size_t RingBuffer::contains(const char *data, size_t len) const
{
	if (len == 0 || len > readable())
		return 0;

	size_t readPos = _head;
	size_t found = 0;

	for (size_t i = 0; i < readable() && found < len; ++i)
	{
		if (_buffer[readPos] == data[found])
		{
			found++;
		}
		else
		{
			found = 0;
		}
		readPos = (readPos + 1) % _capacity;
	}

	return found;
}

size_t RingBuffer::capacity() const
{
	return _capacity;
}

/*
** --------------------------------- MUTATORS --------------------------------
*/

size_t RingBuffer::writeBuffer(const char *data, size_t len)
{
	if (len == 0)
		return 0;

	size_t available = writable();
	size_t toWrite = std::min(len, available);

	if (toWrite == 0)
		return 0;

	// Write data to buffer
	for (size_t i = 0; i < toWrite; ++i)
	{
		_buffer[_tail] = data[i];
		_tail = (_tail + 1) % _capacity;
	}

	return toWrite;
}

size_t RingBuffer::writeBuffer(const RingBuffer &src, size_t len)
{
	size_t toRead = std::min(len, src.readable());
	if (toRead == 0)
		return 0;

	size_t written = 0;
	size_t srcPos = src._head;

	for (size_t i = 0; i < toRead; ++i)
	{
		if (writable() == 0)
			break;

		_buffer[_tail] = src._buffer[srcPos];
		_tail = (_tail + 1) % _capacity;
		srcPos = (srcPos + 1) % src._capacity;
		written++;
	}

	return written;
}

size_t RingBuffer::readBuffer(std::string &dest, size_t len)
{
	size_t toRead = std::min(len, readable());
	if (toRead == 0)
	{
		dest.clear();
		return 0;
	}

	dest.resize(toRead);
	size_t readPos = _head;

	for (size_t i = 0; i < toRead; ++i)
	{
		dest[i] = _buffer[readPos];
		readPos = (readPos + 1) % _capacity;
	}

	_head = readPos;
	return toRead;
}

size_t RingBuffer::peekBuffer(std::string &dest, size_t len) const
{
	size_t toRead = std::min(len, readable());
	if (toRead == 0)
	{
		dest.clear();
		return 0;
	}

	dest.resize(toRead);
	size_t readPos = _head;

	for (size_t i = 0; i < toRead; ++i)
	{
		dest[i] = _buffer[readPos];
		readPos = (readPos + 1) % _capacity;
	}

	return toRead;
}

size_t RingBuffer::transferFrom(RingBuffer &src, size_t len)
{
	size_t toTransfer = std::min(len, std::min(src.readable(), writable()));
	if (toTransfer == 0)
		return 0;

	size_t transferred = 0;
	for (size_t i = 0; i < toTransfer; ++i)
	{
		_buffer[_tail] = src._buffer[src._head];
		_tail = (_tail + 1) % _capacity;
		src._head = (src._head + 1) % src._capacity;
		transferred++;
	}

	return transferred;
}

size_t RingBuffer::transferTo(RingBuffer &dest, size_t len)
{
	return dest.transferFrom(*this, len);
}

size_t RingBuffer::appendBuffer(const RingBuffer &src)
{
	return writeBuffer(src, src.readable());
}

size_t RingBuffer::readBuffer(char *dest, size_t len)
{
	size_t toRead = std::min(len, readable());
	if (toRead == 0)
		return 0;

	size_t readPos = _head;
	for (size_t i = 0; i < toRead; ++i)
	{
		dest[i] = _buffer[readPos];
		readPos = (readPos + 1) % _capacity;
	}

	_head = readPos;
	return toRead;
}

size_t RingBuffer::peekBuffer(char *dest, size_t len) const
{
	size_t toRead = std::min(len, readable());
	if (toRead == 0)
		return 0;

	size_t readPos = _head;
	for (size_t i = 0; i < toRead; ++i)
	{
		dest[i] = _buffer[readPos];
		readPos = (readPos + 1) % _capacity;
	}

	return toRead;
}

size_t RingBuffer::flushToFile(const std::string &filePath)
{
	std::ofstream file(filePath.c_str(), std::ios::app);
	if (!file.is_open())
	{
		Logger::log(Logger::ERROR, "Failed to open file for writing: " + filePath);
		return 0;
	}

	size_t written = 0;
	size_t readPos = _head;

	while (readPos != _tail)
	{
		file.put(_buffer[readPos]);
		readPos = (readPos + 1) % _capacity;
		written++;
	}

	file.close();
	_head = _tail; // Mark all data as consumed
	return written;
}

size_t RingBuffer::flushToFile(FileDescriptor &fd)
{
	if (!fd.isOpen())
	{
		Logger::log(Logger::ERROR, "File descriptor is not open");
		return 0;
	}

	size_t written = 0;
	size_t readPos = _head;

	while (readPos != _tail)
	{
		std::string data(1, _buffer[readPos]);
		ssize_t result = fd.sendData(data);
		if (result <= 0)
			break;

		readPos = (readPos + 1) % _capacity;
		written++;
	}

	_head = readPos; // Mark consumed data
	return written;
}

void RingBuffer::reserve(size_t newCapacity)
{
	if (newCapacity <= _capacity)
		return;

	std::vector<char> newBuffer(newCapacity);
	size_t dataSize = readable();

	if (dataSize > 0)
	{
		size_t readPos = _head;
		for (size_t i = 0; i < dataSize; ++i)
		{
			newBuffer[i] = _buffer[readPos];
			readPos = (readPos + 1) % _capacity;
		}
	}

	_buffer = newBuffer;
	_capacity = newCapacity;
	_head = 0;
	_tail = dataSize;
}

void RingBuffer::consume(size_t len)
{
	size_t toConsume = std::min(len, readable());
	_head = (_head + toConsume) % _capacity;
}

void RingBuffer::reset()
{
	_head = 0;
	_tail = 0;
}

void RingBuffer::clear()
{
	reset();
}

/*
** --------------------------------- PRIVATE METHODS --------------------------
*/

size_t RingBuffer::_advance(size_t pos, size_t n) const
{
	return (pos + n) % _capacity;
}