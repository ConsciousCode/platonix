#ifndef PLATONIX_PROTOCOL_HPP
#define PLATONIX_PROTOCOL_HPP

#include "endian.hpp"

#include <cstring>
#include <vector>
#include <string>

#define IS_TX(x) (x > 0)
#define IS_RX(x) (x < 0)

#define MAKE_TX(x) (x)
#define MAKE_RX(x) (-(x))

enum ModeFlag {
	READ = 1<<0,
	WRITE = 1<<1,
	NOFOLLOW = 1<<2,

	TRUNC = 1<<3,
	APPEND = 1<<4,
	NOATIME = 1<<5,
	NOBUFFER = 1<<6
};

enum ErrorCode {
	E_OK = 0,

%s
};

struct BasePacket {
	i8 tag;
	u8 cmd;
	u16 payload_len;
};
struct Packet : public BasePacket {
	/**
	 * Easily access strings
	**/
	struct String {
		u8 length;
		u8 data[];

		inline operator std::string() {
			return std::string((char*)data, (size_t)length);
		}

		inline String& operator=(const std::string& s) {
			length = s.length();
			memcpy(data, s.c_str(), length);
			return *this;
		}
	};

	struct Bytes {
		u8 size;
		u8 data[];

		inline Bytes& operator=(const std::vector<u8>& v) {
			size = v.size();
			memcpy(data, v.data(), size);
			return *this;
		}
	};

	union {
		u8 payload[];
		ErrorCode error : 8;
		union tx {
%s
		} tx;
		union rx {
%s
		} rx;
	};
};

enum Command {
	C_ERROR = 0,

%s
};

#endif
