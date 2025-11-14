#ifndef TRIENODE_HPP
#define TRIENODE_HPP

#include "../Global/Logger.hpp"
#include <map>

// Node class for the trie tree
// Uses std::map for node storage
template <typename T> class TrieNode
{
private:
	std::map<char, TrieNode<T> *> _children;
	bool _isEndOfPath;
	T *_data;

public:
	// Constructors
	TrieNode() : _isEndOfPath(false), _data(NULL)
	{
	}

	TrieNode(T *data) : _isEndOfPath(false), _data(data ? new T(*data) : NULL)
	{
	}

	TrieNode(const TrieNode<T> &other) : _isEndOfPath(false), _data(NULL)
	{
		try
		{
			_isEndOfPath = other._isEndOfPath;
			_data = other._data ? new T(*other._data) : NULL;

			typename std::map<char, TrieNode<T> *>::const_iterator it;
			for (it = other._children.begin(); it != other._children.end(); ++it)
			{
				TrieNode<T> *newChild = new TrieNode<T>(*it->second);
				_children[it->first] = newChild;
			}
		}
		catch (const std::bad_alloc &)
		{
			clear();
			delete _data;
			_data = NULL;
			throw;
		}
	}

	~TrieNode()
	{
		if (_data)
		{
			delete _data;
			_data = NULL;
		}
		clear();
	}

	TrieNode &operator=(const TrieNode<T> &rhs)
	{
		if (this != &rhs)
		{
			TrieNode<T> temp(rhs);

			bool tmpBool = _isEndOfPath;
			_isEndOfPath = temp._isEndOfPath;
			temp._isEndOfPath = tmpBool;

			T *tmpData = _data;
			_data = temp._data;
			temp._data = tmpData;

			_children.swap(temp._children);
		}
		return *this;
	}

	// Accessors
	bool isEndOfPath() const
	{
		return _isEndOfPath;
	}

	T *getData() const
	{
		return _data;
	}

	const std::map<char, TrieNode<T> *> &getChildren() const
	{
		return _children;
	}

	TrieNode<T> *getChild(char c) const
	{
		typename std::map<char, TrieNode<T> *>::const_iterator it = _children.find(c);
		if (it != _children.end())
		{
			return it->second;
		}
		return NULL;
	}

	bool hasChild(char c) const
	{
		return _children.find(c) != _children.end();
	}

	bool hasChildren() const
	{
		return !_children.empty();
	}

	// Mutators
	void setEndOfPath(bool isEnd)
	{
		_isEndOfPath = isEnd;
	}

	void setData(T *data)
	{
		if (_data != data)
		{
			delete _data;
			_data = data;
		}
	}

	// Methods
	TrieNode<T> *addChild(char c)
	{
		if (!hasChild(c))
		{
			try
			{
				_children[c] = new TrieNode<T>();
			}
			catch (const std::bad_alloc &)
			{
				Logger::log(Logger::ERROR, "Memory allocation failed in TrieNode::addChild");
				return NULL;
			}
		}
		return _children[c];
	}

	bool removeChild(char c)
	{
		typename std::map<char, TrieNode<T> *>::iterator it = _children.find(c);
		if (it != _children.end())
		{
			delete it->second;
			_children.erase(it);
			return true;
		}
		return false;
	}

	void clear()
	{
		typename std::map<char, TrieNode<T> *>::iterator it;
		for (it = _children.begin(); it != _children.end(); ++it)
		{
			delete it->second;
		}
		_children.clear();
	}
};

#endif