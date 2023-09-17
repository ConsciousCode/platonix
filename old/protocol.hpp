/**
 * This file was generated using `python preprocess.py`
**/

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

	<generator object <genexpr> at 0x7f1228cf30f0>
	NOTOPEN // The fd is open as a path and not a file
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
			struct {
				big<fid_t> fd;
			} dup;
			struct {
				big<fid_t> fd;
				String name;
			} path;
			struct {
				big<fid_t> fd;
				mode_t mode;
			} open;
			struct {
				big<fid_t> fd;
			} close;
			struct {
				big<fid_t> fd;
				chunk_t size;
			} read;
			struct {
				big<fid_t> fd;
				Bytes data;
			} write;
			struct {
				big<fid_t> fd;
				Bytes data;
			} insert;
			struct {
				big<fid_t> fd;
				big<off_t> size;
			} erase;
			struct {
				big<fid_t> fd;
				big<off_t> off;
			} seek;
			struct {
				big<fid_t> fd;
			} tell;
			struct {
				big<fid_t> src;
				big<fid_t> dst;
			} move;
			struct {
				big<fid_t> fd;
			} remove;
			struct {
				big<fid_t> fd;
			} list;
			struct {
				big<fid_t> src;
				big<fid_t> dst;
			} copy;
			struct {
				big<fid_t> src;
				big<fid_t> dst;
			} link;
			struct {
				big<fid_t> fd;
			} stat;
			struct {
				big<fid_t> fd;
				Stat stat;
			} wstat;
			struct {
				big<fid_t> fd;
			} lsxattr;
			struct {
				big<fid_t> fd;
				String name;
			} getxattr;
			struct {
				big<fid_t> fd;
				String name;
				Bytes value;
			} setxattr;
			struct {
				big<fid_t> fd;
				String name;
			} rmxattr;
			struct {
				big<fid_t> fd;
				big<off_t> size;
			} resize;
			struct {
				big<uid_t> uid;
			} whois;
			struct {
				
			} whoami;
		} tx;
		union rx {
			struct {
				big<fid_t> fd;
			} dup;
			struct {
				big<fid_t> fd;
			} path;
			struct {} open;
			struct {} close;
			struct {
				Bytes data;
			} read;
			struct {
				big<off_t> size;
			} write;
			struct {
				big<off_t> size;
			} insert;
			struct {
				big<off_t> size;
			} erase;
			struct {} seek;
			struct {
				big<off_t> off;
			} tell;
			struct {} move;
			struct {} remove;
			struct {
				Bytes /* string[] */ ls;
			} list;
			struct {} copy;
			struct {} link;
			struct {
				Stat stat;
			} stat;
			struct {} wstat;
			struct {
				Bytes /* string[] */ ls;
			} lsxattr;
			struct {
				Bytes value;
			} getxattr;
			struct {} setxattr;
			struct {} rmxattr;
			struct {
				big<off_t> size;
			} resize;
			struct {
				String name;
			} whois;
			struct {
				big<uid_t> uid;
				String name;
			} whoami;
		} rx;
	};
};

enum Command {
	C_ERROR = 0,

	C_DUP,
	C_PATH,
	C_OPEN,
	C_CLOSE,
	C_READ,
	C_WRITE,
	C_INSERT,
	C_ERASE,
	C_SEEK,
	C_TELL,
	C_MOVE,
	C_REMOVE,
	C_LIST,
	C_COPY,
	C_LINK,
	C_STAT,
	C_WSTAT,
	C_LSXATTR,
	C_GETXATTR,
	C_SETXATTR,
	C_RMXATTR,
	C_RESIZE,
	C_WHOIS,
	C_WHOAMI
};

#endif
