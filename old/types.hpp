#ifndef PLATONIX_TYPES_HPP
#define PLATONIX_TYPES_HPP

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include <thread>

#include <cstdint>
#include <arpa/inet.h>
#include <sys/types.h>

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef u16 fid_t;
typedef u8 mode_t;

typedef u8 bytes[];

/**
 * The integer type used for an offset or size of a file.
**/
typedef u64 off_t;
/**
 * The integer type used for the sizes of chunks read or written atomically.
**/
typedef u16 chunk_t;

struct Stat {
	uid_t uid;
	gid_t gid;
	size_t size;

	struct timespec atime;
	struct timespec mtime;
	struct timespec ctime;
};

#endif
