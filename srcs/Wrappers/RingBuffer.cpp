#include "RingBuffer.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

RingBuffer::RingBuffer()
{
	_buffer.resize(sysconf(_SC_PAGESIZE));
	_head = 0;
	_tail = 0;
	_capacity = sysconf(_SC_PAGESIZE);
}

RingBuffer::RingBuffer(size_t size)
{
	_buffer.resize(size);
	_head = 0;
	_tail = 0;
	_capacity = size;
}

RingBuffer::RingBuffer(const RingBuffer &src)
{
	*this = src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

RingBuffer::~RingBuffer()
{
	clear();
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

RingBuffer &RingBuffer::operator=(RingBuffer const &rhs)
{
	if (this != &rhs)
	{
		this->_buffer = rhs._buffer;
		this->_head = rhs._head;
		this->_tail = rhs._tail;
		this->_capacity = rhs._capacity;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

// Convenience: read + consume
size_t RingBuffer::readBuffer(char *dest, size_t len)
{
	size_t n = peekBuffer(dest, len);
	consume(n);
	return n;
}

size_t RingBuffer::readBuffer(std::string &dest, size_t len)
{
	size_t n = peekBuffer(dest, len);
	consume(n);
	return n;
}

// Returns position of data in buffer
size_t RingBuffer::contains(const char *data, size_t len) const
{
	size_t pos = 0;
	while (pos < _capacity)
	{
		if (std::memcmp(&_buffer[pos], data, len) == 0)
			return pos;
		pos++;
	}
	return _capacity;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

// Write from another RingBuffer (peek without consuming source)
size_t RingBuffer::writeBuffer(const RingBuffer &src, size_t len)
{
	size_t availableToRead = src.readable();
	size_t availableToWrite = writable();
	size_t toTransfer = std::min(len, std::min(availableToRead, availableToWrite));

	if (toTransfer == 0)
		return 0;

	// Handle potential wrap-around in source buffer
	size_t srcTail = src._tail;
	size_t firstChunk = std::min(toTransfer, src._capacity - srcTail);

	// Write first chunk
	size_t written1 = writeBuffer(&src._buffer[srcTail], firstChunk);

	// Write second chunk if source wraps around
	size_t secondChunk = toTransfer - written1;
	size_t written2 = 0;
	if (secondChunk > 0 && written1 == firstChunk)
	{
		written2 = writeBuffer(&src._buffer[0], secondChunk);
	}

	return written1 + written2;
}

// Transfer with consumption from source
size_t RingBuffer::transferFrom(RingBuffer &src, size_t len)
{
	size_t transferred = writeBuffer(src, len);
	src.consume(transferred);
	return transferred;
}

// Transfer to destination with consumption from this buffer
size_t RingBuffer::transferTo(RingBuffer &dest, size_t len)
{
	return dest.transferFrom(*this, len);
}

// Append entire contents
size_t RingBuffer::appendBuffer(const RingBuffer &src)
{
	return writeBuffer(src, src.readable());
}

// This is the space available to read from the buffer
// Not the space available to write to the buffer
size_t RingBuffer::readable() const
{
	if (_head >= _tail)
		return _head - _tail;
	return _capacity - _tail + _head;
}

// This is the space available to write to the buffer
// Not the space available to read from the buffer
size_t RingBuffer::writable() const
{
	return _capacity - readable() - 1;
}

bool RingBuffer::empty() const
{
	return _head == _tail;
}

bool RingBuffer::full() const
{
	return _advance(_head, 1) == _tail;
}

// Read data from buffer without consuming
size_t RingBuffer::peekBuffer(char *dest, size_t len) const
{
	size_t avail = readable();
	size_t toRead = std::min(len, avail);

	size_t firstPart = std::min(toRead, _capacity - _tail);
	std::memcpy(dest, &_buffer[_tail], firstPart);

	size_t secondPart = toRead - firstPart;
	if (secondPart > 0)
		std::memcpy(dest + firstPart, &_buffer[0], secondPart);

	return toRead;
}

size_t RingBuffer::peekBuffer(std::string &dest, size_t len) const
{
	size_t avail = readable();
	size_t toRead = std::min(len, avail);
	dest.assign(&_buffer[_tail], toRead);
	return toRead;
}

/*
** --------------------------------- MUTATORS ---------------------------------
*/

size_t RingBuffer::writeBuffer(const char *data, size_t len)
{
	size_t toWrite = std::min(len, writable());

	// Two-step copy if wrapping around
	size_t firstChunk = std::min(toWrite, _capacity - _head);
	std::memcpy(&_buffer[_head], data, firstChunk);

	size_t secondChunk = toWrite - firstChunk;
	if (secondChunk > 0)
		std::memcpy(&_buffer[0], data + firstChunk, secondChunk);

	_head = _advance(_head, toWrite);
	return toWrite;
}

void RingBuffer::reserve(size_t newCapacity)
{
	if (newCapacity > _capacity)
	{
		_buffer.resize(newCapacity);
		_capacity = newCapacity;
	}
}

// Consume bytes after reading
void RingBuffer::consume(size_t n)
{
	size_t toConsume = std::min(n, readable());
	_tail = _advance(_tail, toConsume);
}

size_t RingBuffer::_advance(size_t pos, size_t n) const
{
	return (pos + n) % _capacity;
}

void RingBuffer::reset()
{
	_head = 0;
	_tail = 0;
	_buffer.clear();
}

void RingBuffer::clear()

{
	_buffer.clear();
	_head = 0;
	_tail = 0;
	_capacity = 0;
}

/* ************************************************************************** */
