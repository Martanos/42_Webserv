#include "SafeBuffer.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

SafeBuffer::SafeBuffer(size_t initialCapacity = HTTP::DEFAULT_BUFFER_SIZE)
{
	_buffer = new char[initialCapacity];
	_size = 0;
	_capacity = initialCapacity;
}

SafeBuffer::SafeBuffer(const SafeBuffer &src)
{
	*this = src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

SafeBuffer::~SafeBuffer()
{
	delete[] _buffer;
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

SafeBuffer &SafeBuffer::operator=(SafeBuffer const &rhs)
{
	if (this != &rhs)
	{
		_reallocate(rhs._capacity);
		_size = rhs._size;
		_capacity = rhs._capacity;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void SafeBuffer::append(const char *data, size_t length)
{
	if (_size + length > _capacity)
	{
		_reallocate(_size + length);
	}
	std::memcpy(_buffer + _size, data, length);
	_size += length;
}

void SafeBuffer::append(const std::string &data)
{
	append(data.c_str(), data.size());
}

void SafeBuffer::clear()
{
	_size = 0;
}

void SafeBuffer::reserve(size_t newCapacity)
{
	if (newCapacity > _capacity)
	{
		_reallocate(newCapacity);
	}
	_capacity = newCapacity;
}

void SafeBuffer::_reallocate(size_t newCapacity)
{
	char *newBuffer = new char[newCapacity];
	std::memcpy(newBuffer, _buffer, _size);
	delete[] _buffer;
	_buffer = newBuffer;
	_capacity = newCapacity;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

const char *SafeBuffer::data() const
{
	return _buffer;
}

char *SafeBuffer::data()
{
	return _buffer;
}

size_t SafeBuffer::size() const
{
	return _size;
}

size_t SafeBuffer::capacity() const
{
	return _capacity;
}

bool SafeBuffer::empty() const
{
	return _size == 0;
}

std::string SafeBuffer::toString() const
{
	return std::string(_buffer, _size);
}

void SafeBuffer::consume(size_t bytes)
{
	_size -= bytes;
}

size_t SafeBuffer::find(const char *pattern) const
{
	return std::string(_buffer, _size).find(pattern);
}

size_t SafeBuffer::find(const std::string &pattern) const
{
	return std::string(_buffer, _size).find(pattern);
}

/* ************************************************************************** */
