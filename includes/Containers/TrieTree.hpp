#ifndef TRIETREE_HPP
#define TRIETREE_HPP

#include "../Global/Logger.hpp"
#include "../Utils/StrUtils.hpp"
#include "TrieNode.hpp"
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

// Custom trie tree implementation for fast lookup of unique values
// Time Complexity:
//   insert: O(k) where k is key length
//   find: O(k)
//   remove: O(k)
//   getAllValues/Keys: O(n*m) where n is number of entries, m is average key length
// Space Complexity: O(ALPHABET_SIZE * n * m) in worst case
// WARNING: This TrieTree stores pointers to T objects and takes ownership.
// The stored objects are deleted when nodes are removed or the tree is destroyed.
// Do not pass pointers to stack objects or objects managed elsewhere.
template <typename T> class TrieTree
{
private:
	TrieNode<T> *_root;
	size_t _size;

	struct IteratorState
	{
		TrieNode<T> *node;
		std::string path;
		typename std::map<char, TrieNode<T> *>::const_iterator childIt;
		bool initialized;
		bool endpointYielded;

		IteratorState(TrieNode<T> *n, const std::string &p)
			: node(n), path(p), childIt(), initialized(false), endpointYielded(false)
		{
			if (node)
			{
				const std::map<char, TrieNode<T> *> &children = node->getChildren();
				childIt = children.begin();
			}
			else
				childIt = typename std::map<char, TrieNode<T> *>::const_iterator();
		}
	};

	// Private helper methods
	void _collectAll(TrieNode<T> *node, std::vector<T> &result) const
	{
		if (!node)
			return;

		if (node->isEndOfPath() && node->getData())
		{
			result.push_back(*node->getData());
		}

		const std::map<char, TrieNode<T> *> &children = node->getChildren();
		typename std::map<char, TrieNode<T> *>::const_iterator it;

		for (it = children.begin(); it != children.end(); ++it)
		{
			_collectAll(it->second, result); // No need for prefix if not used
		}
	}

	void _collectAllKeys(TrieNode<T> *node, const std::string &prefix, std::vector<std::string> &result) const
	{
		if (!node)
		{
			return;
		}

		if (node->isEndOfPath() && node->getData())
		{
			result.push_back(prefix);
		}

		const std::map<char, TrieNode<T> *> &children = node->getChildren();
		typename std::map<char, TrieNode<T> *>::const_iterator it;

		for (it = children.begin(); it != children.end(); ++it)
		{
			_collectAllKeys(it->second, prefix + it->first, result);
		}
	}

	// Validate key - only reject truly invalid keys (empty or containing null bytes)
	// Does NOT transform the key - callers are responsible for any normalization
	bool _isValidKey(const std::string &key) const
	{
		if (key.empty())
			return false;

		if (key.find('\0') != std::string::npos)
		{
			Logger::log(Logger::WARNING, "Invalid key: contains null byte");
			return false;
		}

		return true;
	}

	bool _removeRecursive(TrieNode<T> *node, const std::string &key, size_t depth)
	{
		if (!node)
			return false;

		// We've reached the end of the key
		if (depth == key.length())
		{
			if (!node->isEndOfPath())
				return false;

			node->setEndOfPath(false);
			node->setData(NULL);

			// Return true if this node has no children (can be deleted)
			return !node->hasChildren();
		}

		char c = key[depth];
		TrieNode<T> *child = node->getChild(c);

		if (!child)
			return false;

		bool shouldDeleteChild = _removeRecursive(child, key, depth + 1);

		if (shouldDeleteChild)
		{
			node->removeChild(c);
			// Return true if current node is not an endpoint and has no children
			return !node->isEndOfPath() && !node->hasChildren();
		}

		return false;
	}

public:
	// Forward iterator for TrieTree
	class iterator
	{
	private:
		std::vector<IteratorState> _stack;
		T *_currentData;

		friend class TrieTree<T>;

		// Private constructor for begin/end
		explicit iterator(TrieNode<T> *root) : _currentData(NULL)
		{
			if (root)
			{
				_stack.push_back(IteratorState(root, ""));
				_advance();
			}
		}

		// End iterator constructor
		iterator() : _currentData(NULL)
		{
		}

		void _advance()
		{
			while (!_stack.empty())
			{
				IteratorState &state = _stack.back();

				// Check if node is valid
				if (!state.node)
				{
					_stack.pop_back();
					continue;
				}

				// First visit: check if current node is an endpoint and hasn't been yielded
				if (!state.endpointYielded && state.node->isEndOfPath() && state.node->getData())
				{
					state.endpointYielded = true;
					_currentData = state.node->getData();
					return;
				}

				// Try to visit children
				const std::map<char, TrieNode<T> *> &children = state.node->getChildren();
				if (!state.initialized)
				{
					state.childIt = children.begin();
					state.initialized = true;
				}

				if (state.childIt != children.end())
				{
					char c = state.childIt->first;
					TrieNode<T> *childNode = state.childIt->second;
					++state.childIt;

					_stack.push_back(IteratorState(childNode, state.path + c));
				}
				else
				{
					// No more children, backtrack
					_stack.pop_back();
				}
			}

			// Reached end
			_currentData = NULL;
		}

	public:
		// Orthodox Canonical Form
		iterator(const iterator &other) : _stack(other._stack), _currentData(other._currentData)
		{
		}

		~iterator()
		{
		}

		iterator &operator=(const iterator &rhs)
		{
			if (this != &rhs)
			{
				_stack = rhs._stack;
				_currentData = rhs._currentData;
			}
			return *this;
		}

		// Iterator operations
		T &operator*() const
		{
			return *_currentData;
		}

		T *operator->() const
		{
			return _currentData;
		}

		iterator &operator++()
		{
			_advance();
			return *this;
		}

		iterator operator++(int)
		{
			iterator tmp(*this);
			_advance();
			return tmp;
		}

		bool operator==(const iterator &other) const
		{
			return _currentData == other._currentData;
		}

		bool operator!=(const iterator &other) const
		{
			return !(*this == other);
		}
	};

	// Const iterator
	class const_iterator
	{
	private:
		std::vector<IteratorState> _stack;
		const T *_currentData;

		friend class TrieTree<T>;

		explicit const_iterator(TrieNode<T> *root) : _currentData(NULL)
		{
			if (root)
			{
				_stack.push_back(IteratorState(root, ""));
				_advance();
			}
		}

		const_iterator() : _currentData(NULL)
		{
		}

		void _advance()
		{
			while (!_stack.empty())
			{
				IteratorState &state = _stack.back();

				// Check if node is valid
				if (!state.node)
				{
					_stack.pop_back();
					continue;
				}

				if (!state.endpointYielded && state.node->isEndOfPath() && state.node->getData())
				{
					state.endpointYielded = true;
					_currentData = state.node->getData();
					return;
				}

				const std::map<char, TrieNode<T> *> &children = state.node->getChildren();
				if (!state.initialized)
				{
					state.childIt = children.begin();
					state.initialized = true;
				}

				if (state.childIt != children.end())
				{
					char c = state.childIt->first;
					TrieNode<T> *childNode = state.childIt->second;
					++state.childIt;
					_stack.push_back(IteratorState(childNode, state.path + c));
				}
				else
				{
					_stack.pop_back();
				}
			}
			_currentData = NULL;
		}

	public:
		// Orthodox Canonical Form
		const_iterator(const const_iterator &other) : _stack(other._stack), _currentData(other._currentData)
		{
		}

		// Conversion from iterator
		const_iterator(const iterator &other) : _stack(other._stack), _currentData(other._currentData)
		{
		}

		~const_iterator()
		{
		}

		const_iterator &operator=(const const_iterator &rhs)
		{
			if (this != &rhs)
			{
				_stack = rhs._stack;
				_currentData = rhs._currentData;
			}
			return *this;
		}

		const T &operator*() const
		{
			return *_currentData;
		}

		const T *operator->() const
		{
			return _currentData;
		}

		const_iterator &operator++()
		{
			_advance();
			return *this;
		}

		const_iterator operator++(int)
		{
			const_iterator tmp(*this);
			_advance();
			return tmp;
		}

		bool operator==(const const_iterator &other) const
		{
			return _currentData == other._currentData;
		}

		bool operator!=(const const_iterator &other) const
		{
			return !(*this == other);
		}
	};

	// Iterator accessors
	iterator begin()
	{
		if (!_root)
			return iterator();
		return iterator(_root);
	}

	iterator end()
	{
		return iterator();
	}

	const_iterator begin() const
	{
		if (!_root)
			return const_iterator();
		return const_iterator(_root);
	}

	const_iterator end() const
	{
		return const_iterator();
	}

public:
	// Constructors
	TrieTree() : _root(new TrieNode<T>()), _size(0)
	{
	}

	TrieTree(const TrieTree &other) : _root(NULL), _size(0)
	{
		try
		{
			if (other._root)
			{
				_root = new TrieNode<T>(*other._root);
				_size = other._size;
			}
			else
			{
				_root = new TrieNode<T>();
			}
		}
		catch (const std::bad_alloc &)
		{
			delete _root;
			_root = NULL;
			_size = 0;
			throw;
		}
	}

	~TrieTree()
	{
		delete _root;
		_root = NULL;
	}

	TrieTree<T> &operator=(const TrieTree &rhs)
	{
		if (this != &rhs)
		{
			// Create temporary copy
			TrieTree<T> temp(rhs);
			// internals
			std::swap(_root, temp._root);
			std::swap(_size, temp._size);
			// temp destructor cleans up old data
		}
		return *this;
	}

	// Core operations
	bool insert(const std::string &key, const T &value)
	{
		try
		{
			if (_size == std::numeric_limits<size_t>::max())
			{
				Logger::log(Logger::ERROR, "TrieTree size limit reached");
				return false;
			}

			if (!_root)
			{
				return false;
			}

			if (!_isValidKey(key))
			{
				return false;
			}

			TrieNode<T> *current = _root;

			// Traverse or create path
			for (size_t i = 0; i < key.length(); ++i)
			{
				char c = key[i];
				if (!current->hasChild(c))
				{
					if (!current->addChild(c)) // addChild returns NULL on failure
						return false;
				}
				current = current->getChild(c);
				if (!current)
				{
					Logger::log(Logger::ERROR, "Failed to create trie node for key: " + key);
					return false;
				}
			}

			// Check if value already exists at this key
			if (current->isEndOfPath())
			{
				current->setData(new T(value));
				return true;
			}

			// Insert new value
			current->setEndOfPath(true);
			current->setData(new T(value));
			++_size;
		}
		catch (const std::bad_alloc &e)
		{
			Logger::log(Logger::ERROR, "Memory allocation failed in TrieTree::insert");
			throw std::runtime_error("Memory allocation failed in TrieTree::insert");
		}
		return true;
	}

	// find overload
	T *find(const std::string &key) const
	{
		if (!_root)
		{
			return NULL;
		}

		if (!_isValidKey(key))
		{
			return NULL;
		}

		TrieNode<T> *current = _root;

		// Traverse the path
		for (size_t i = 0; i < key.length(); ++i)
		{
			char c = key[i];
			current = current->getChild(c);

			if (!current)
			{
				return NULL;
			}
		}

		// Return data only if this is an end of a path
		if (current->isEndOfPath())
		{
			return current->getData();
		}

		return NULL;
	}

	T *findLongestPrefix(const std::string &key) const
	{
		if (!_root)
		{
			return NULL;
		}

		if (!_isValidKey(key))
		{
			return NULL;
		}

		TrieNode<T> *current = _root;
		T *lastMatch = NULL;

		// Traverse and remember the last valid endpoint
		for (size_t i = 0; i < key.length(); ++i)
		{
			char c = key[i];
			current = current->getChild(c);

			if (!current)
			{
				break;
			}

			// If this node represents a complete key, remember it
			if (current->isEndOfPath() && current->getData())
			{
				lastMatch = current->getData();
			}
		}

		return lastMatch;
	}

	bool remove(const std::string &key)
	{
		if (!_root)
			return false;

		if (!_isValidKey(key))
			return false;

		bool existed = contains(key);
		if (existed)
		{
			_removeRecursive(_root, key, 0);
			--_size;
		}
		return existed;
	}

	bool contains(const std::string &key) const
	{
		T *result = find(key);
		return result != NULL;
	}

	// Utility methods
	size_t size() const
	{
		return _size;
	}

	void clear()
	{
		try
		{
			TrieNode<T> *newRoot = new TrieNode<T>();
			delete _root;
			_root = newRoot;
			_size = 0;
		}
		catch (const std::bad_alloc &)
		{
			// Keep old state on failure
			Logger::log(Logger::ERROR, "Memory allocation failed in TrieTree::clear");
		}
	}
	bool isEmpty() const
	{
		return _size == 0;
	}

	std::vector<T> getAllValues() const
	{
		std::vector<T> result;
		result.reserve(_size);
		if (_root)
		{
			_collectAll(_root, result);
		}
		return result;
	}

	std::vector<std::string> getAllKeys() const
	{
		std::vector<std::string> result;
		result.reserve(_size);
		if (_root)
		{
			if (_root->isEndOfPath() && _root->getData())
			{
				result.push_back("/");
			}
			_collectAllKeys(_root, "", result);
		}
		return result;
	}

	// Debug
	void printStructure() const
	{
		Logger::log(Logger::INFO, "TrieTree structure:");
		Logger::log(Logger::INFO, "Total entries: " + StrUtils::toString(static_cast<int>(_size)));

		std::vector<std::string> keys = getAllKeys();
		for (size_t i = 0; i < keys.size(); ++i)
		{
			Logger::log(Logger::INFO, "  Key: " + keys[i]);
		}
	}
};

#endif
