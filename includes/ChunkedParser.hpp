#ifndef CHUNKED_PARSER_HPP
#define CHUNKED_PARSER_HPP

#include <string>
#include <sstream>
#include <cstdlib>
#include "Logger.hpp"
#include "Constants.hpp"

class ChunkedParser
{
public:
	enum ChunkState
	{
		CHUNK_SIZE,
		CHUNK_DATA,
		CHUNK_TRAILER,
		CHUNK_COMPLETE,
		CHUNK_ERROR
	};

private:
	ChunkState _state;
	size_t _currentChunkSize;
	size_t _bytesRead;
	std::string _chunkBuffer;
	std::string _decodedData;
	bool _isComplete;

	// Non-copyable
	ChunkedParser(const ChunkedParser &);
	ChunkedParser &operator=(const ChunkedParser &);

	// Helper methods
	size_t _parseHexSize(const std::string &hexStr) const;
	bool _isValidHexChar(char c) const;

public:
	ChunkedParser();
	~ChunkedParser();

	// Main processing method
	ChunkState processBuffer(const std::string &buffer);

	// State queries
	bool isComplete() const;
	bool hasError() const;
	const std::string &getDecodedData() const;
	size_t getTotalSize() const;

	// Reset for reuse
	void reset();
};

#endif /* *************************************************** CHUNKEDPARSER_H */
