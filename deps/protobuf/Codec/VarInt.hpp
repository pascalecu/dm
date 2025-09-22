#pragma once
#include <cstddef>
#include <cstdint>

namespace proto
{
namespace codec
{
// Reads a varint32, returns false if buffer ends early.
static bool readVarint32(uint8_t const *&ptr, uint8_t const *end, uint32_t &out)
{
	if (ptr >= end)
	{
		return false;
	}

	// fast path: single byte
	uint8_t byte = *ptr++;
	if (!(byte & 0x80))
	{
		out = byte;
		return true;
	}

	uint32_t result = byte & 0x7F;
	int shift = 7;

	// We can have at most 32 bits, and the closest multiple of 7 is 35.
	while (ptr < end && shift < 7 * 5)
	{
		byte = *ptr++;
		result |= (byte & 0x7F) << shift;
		if (!(byte & 0x80))
		{
			out = result;
			return true;
		}
		shift += 7;
	}

	return false; // malformed or buffer too short
}

// Reads a varint64, returns false if buffer ends early.
static bool readVarint64(uint8_t const *&ptr, uint8_t const *end, uint64_t &out)
{
	if (ptr >= end)
	{
		return false;
	}

	// fast path: single byte
	uint8_t byte = *ptr++;
	if (!(byte & 0x80))
	{
		out = byte;
		return true;
	}

	uint64_t result = byte & 0x7F;
	int shift = 7;

	// Same reasoning as with readVarint32.
	while (ptr < end && shift < 7 * 10)
	{
		byte = *ptr++;
		result |= (uint64_t)(byte & 0x7F) << shift;
		if (!(byte & 0x80))
		{
			out = result;
			return true;
		}
		shift += 7;
	}

	return false; // malformed or buffer too short
}

// Writes a varint32, returns pointer advanced past written bytes.
static uint8_t *writeVarint32(uint8_t *ptr, uint32_t value)
{
	// fast path for single byte
	if (value < 0x80)
	{
		*ptr++ = static_cast<uint8_t>(value);
		return ptr;
	}

	// multi-byte path
	while (value >= 0x80)
	{
		*ptr++ = static_cast<uint8_t>(value | 0x80);
		value >>= 7;
	}
	*ptr++ = static_cast<uint8_t>(value);
	return ptr;
}

// Writes a varint64, returns pointer advanced past written bytes.
static uint8_t *writeVarint64(uint8_t *ptr, uint64_t value)
{
	if (value < 0x80)
	{
		*ptr++ = static_cast<uint8_t>(value);
		return ptr;
	}

	while (value >= 0x80)
	{
		*ptr++ = static_cast<uint8_t>(value | 0x80);
		value >>= 7;
	}
	*ptr++ = static_cast<uint8_t>(value);
	return ptr;
}

inline uint32_t zigZagEncode32(int32_t n)
{
	return (n << 1) ^ (n >> 31);
}

inline int32_t zigZagDecode32(uint32_t n)
{
	return (n >> 1) ^ -static_cast<int32_t>(n & 1);
}

inline uint64_t zigZagEncode64(int64_t n)
{
	return (n << 1) ^ (n >> 63);
}

inline int64_t zigZagDecode64(uint64_t n)
{
	return (n >> 1) ^ -static_cast<int64_t>(n & 1);
}

// Write/read signed integers (sint32/sint64) using ZigZag + varint

inline uint8_t *writeSint32(uint8_t *ptr, int32_t value)
{
	uint32_t raw = zigZagEncode32(value);
	if (raw < 0x80) // single byte
	{
		*ptr++ = static_cast<uint8_t>(raw);
		return ptr;
	}
	return writeVarint32(ptr, raw);
}

inline uint8_t *writeSint64(uint8_t *ptr, int64_t value)
{
	uint64_t raw = zigZagEncode64(value);
	if (raw < 0x80) // single byte
	{
		*ptr++ = static_cast<uint8_t>(raw);
		return ptr;
	}
	return writeVarint64(ptr, raw);
}

inline bool readSint32(uint8_t const *&ptr, uint8_t const *end, int32_t &out)
{
	if (ptr >= end)
	{
		return false;
	}
	uint8_t first = *ptr++;
	if (!(first & 0x80))
	{
		out = zigZagDecode32(first);
		return true;
	}
	ptr--; // fallback to full read
	uint32_t raw;
	if (!readVarint32(ptr, end, raw))
	{
		return false;
	}
	out = zigZagDecode32(raw);
	return true;
}

inline bool readSint64(uint8_t const *&ptr, uint8_t const *end, int64_t &out)
{
	if (ptr >= end)
	{
		return false;
	}
	uint8_t first = *ptr++;
	if (!(first & 0x80))
	{
		out = zigZagDecode64(first);
		return true;
	}
	ptr--; // fallback to full read
	uint64_t raw;
	if (!readVarint64(ptr, end, raw))
	{
		return false;
	}
	out = zigZagDecode64(raw);
	return true;
}

} // namespace codec
} // namespace proto
