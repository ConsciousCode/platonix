#include "server.hpp"

#include <dirent.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

#define SOCKQLEN 128
#define BUFLEN 1024

using namespace pnix;

std::vector<u8> strls_to_bytes(const std::vector<std::string>& ls) {
	size_t size = 0;
	for(auto& s : ls) {
		size += s.length();
	}

	std::vector<u8> out(size);
	for(auto& s : ls) {
		out.push_back((u8)s.length());
		out.insert(out.end(), s.begin(), s.end());
	}

	return out;
}

Error::Error(ErrorCode code):code(code) {}

const char* Error::what() const noexcept {
	switch(code) {
		#include "error.inc.cpp"

		default: return "Unknown error";
	}
}

File::File(fs::path p):path(p) {}

/* Default implementation of the protocol methods is to throw an error */

void File::open() {
	throw Error(E_INVALID);
}
void File::close() {
	throw Error(E_INVALID);
}

off_t File::read(u8* data, chunk_t size) {
	throw Error(E_INVALID);
}
off_t File::write(u8* data, chunk_t size) {
	throw Error(E_INVALID);
}
void File::seek(off_t off) {
	throw Error(E_INVALID);
}
off_t File::tell() {
	throw Error(E_INVALID);
}
void File::insert(u8* data, chunk_t size) {
	throw Error(E_INVALID);
}
void File::erase(off_t size) {
	throw Error(E_INVALID);
}

std::vector<std::string> File::lsxattr() {
	throw Error(E_INVALID);
}
std::vector<u8> File::getxattr(const std::string& name) {
	throw Error(E_INVALID);
}
void File::setxattr(const std::string& name, const std::vector<u8>& data) {
	throw Error(E_INVALID);
}
void File::rmxattr(const std::string& name) {
	throw Error(E_INVALID);
}

void File::move(File* dst) {
	throw Error(E_INVALID);
}
void File::remove() {
	throw Error(E_INVALID);
}

std::vector<std::string> File::list() {
	throw Error(E_INVALID);
}

void File::copy(File* dst) {
	throw Error(E_INVALID);
}
void File::link(File* dst) {
	throw Error(E_INVALID);
}
Stat File::stat() {
	throw Error(E_INVALID);
}
void File::wstat(Stat stat) {
	throw Error(E_INVALID);
}
off_t File::resize(off_t size) {
	throw Error(E_INVALID);
}

File* Directory::lookup(fs::path::iterator cur, fs::path::iterator end) {
	if(cur == end) {
		return openSelf();
	}
	
	return dir[*cur]->lookup(++cur, end);
}

DeviceMount::DeviceMount(std::function<File*(fs::path::iterator, fs::path::iterator)> lu, std::function<std::vector<std::string>()> ls):lookup_fn(lu), list_fn(ls) {}

File* DeviceMount::lookup(fs::path::iterator cur, fs::path::iterator end) {
	return lookup_fn(cur, end);
}

std::vector<std::string> DeviceMount::list() {
	return list_fn();
}

NativeMount::NativeMount(const std::string& p):root(p) {}

File* NativeMount::lookup(fs::path::iterator cur, fs::path::iterator end) {
	fs::path path;
	for(; cur != end; ++cur) {
		path /= *cur;
	}
	return new NativeFile(root / path, 0);
}

std::vector<std::string> NativeMount::list() {
	std::vector<std::string> ret;
	DIR* d = opendir(root.c_str());
	
	for(;;) {
		dirent* dent = readdir(d);
		if(dent) {
			ret.push_back(dent->d_name);
		}
		else {
			switch(errno) {
				case 0: goto exit_loop;
			}
		}
	}
	exit_loop:
	
	closedir(d);
	return ret;
}

File* UnionMount::lookup(fs::path::iterator cur, fs::path::iterator end) {
	for(auto& mnt : mounts) {
		File* f = mnt->lookup(cur, end);
		if(f) return f;
	}
	return nullptr;
}

std::vector<std::string> UnionMount::list() {
	std::vector<std::string> ret;

	for(auto& mnt : mounts) {
		auto&& d = mnt->list();
		ret.insert(ret.end(), d.begin(), d.end());
	}
	std::unique(ret.begin(), ret.end());
		
	return ret;
}

NativeFile::NativeFile(std::string path, mode_t mode) {
	file = fopen(path.c_str(), "r");
}

void NativeFile::close() {
	fclose(file);
}

size_t NativeFile::read(u8* data, size_t size) {
	return fread(data, 1, size, file);
}

size_t NativeFile::write(u8* data, size_t size) {
	return fwrite(data, 1, size, file);
}

void NativeFile::seek(size_t off) {
	fseek(file, off, SEEK_SET); 
}

size_t NativeFile::tell() {
	return ftell(file);
}

void NativeFile::insert(u8* data, size_t size) {
	// Calculate offsets
	size_t off = ftell(file);
	fseek(file, 0, SEEK_END);
	size_t flen = ftell(file);
	size_t endlen = size - off;

	// Map the file for easier access
	void* fdata = mmap(nullptr, flen + size, PROT_READ|PROT_WRITE, MAP_LOCKED, fileno(file), off);

	// Move what's there to make room
	memmove(fdata + size, fdata, endlen);
	// Copy the inserted data
	memcpy(fdata, data, size);

	// Cleanup
	munmap(fdata, flen + size);
	fseek(file, off, SEEK_SET);
}

void NativeFile::erase(size_t size) {
	// Calculate offsets
	size_t off = ftell(file);
	fseek(file, 0, SEEK_END);
	size_t flen = ftell(file);
	size_t endlen = size - off;

	// Map the file for easier access
	void* fdata = mmap(nullptr, flen - off, PROT_READ|PROT_WRITE, MAP_LOCKED, fileno(file), off);

	// Move the data afterwards on top of the erased data
	memmove(fdata, fdata + size, size);;

	// Cleanup
	munmap(fdata, flen + size);
	fseek(file, off, SEEK_SET);
	ftruncate(fileno(file), flen - size);
}
