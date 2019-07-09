#ifndef PLATONIX_ENDIAN_HPP
#define PLATONIX_ENDIAN_HPP

#include "types.hpp"

inline u8 hton(u8 v) {
	return v;
}
inline u16 hton(u16 v) {
	return htons(v);
}
inline u32 hton(u32 v) {
	return htonl(v);
}

inline u8 ntoh(u8 v) {
	return v;
}
inline u16 ntoh(u16 v) {
	return ntohs(v);
}
inline u32 ntoh(u32 v) {
	return ntohl(v);
}

#ifdef _WIN32
	#include <winsock.h>
	inline u64 hton(u64 v) {
		return htonll(v);
	}
	inline u64 ntoh(u64 v) {
		return ntohll(v):
	}
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	inline u64 _endian_swap(u64 v) {
		union {
			u32 words[2];
			u64 value;
		};

		value = v;
		u32 tmp = words[0];
		words[0] = words[1];
		words[1] = tmp;

		return value;
	}

	inline u64 hton(u64 v) {
		return _endian_swap(v);
	}
	inline u64 ntoh(u64 v) {
		return _endian_swap(v);
	}
#elif __BYTE_ORDER == __ORDER_BIG_ENDIAN__
	inline u64 hton(u64 v) {
		return v;
	}
	inline u64 ntoh(u64 v) {
		return v;
	}
#else
	#error "Unsupported host endianness"
#endif

/**
 * big<type> maintains big-endian order in-memory while
 *  allowing seamless conversion to host endianness.
 *  Treated as a POD object, it can read big endian values
 *  as if they were host endian.
**/
template<typename T>
struct big {
	T value;

	auto& operator=(T v) {
		value = hton(v);
		return *this;
	}

	operator T() const {
		return ntoh(value);
	}
};

#endif
