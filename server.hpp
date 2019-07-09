#ifndef PLATONIX_SERVER_HPP
#define PLATONIX_SERVER_HPP

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <memory>
#include <functional>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include "protocol.hpp"

namespace pnix {

class Mount;
	class Directory;
	class DeviceMount;
	class NativeMount;
	class UnionMount;

class File;
	class NativeFile;
	class DeviceFile;

class Error : public std::runtime_error {
	protected:
		ErrorCode code;
	
	public:
		Error(ErrorCode code);

		virtual const char* what() const noexcept;
};

class Mount {
	protected:
		std::unordered_map<std::string, std::unique_ptr<File>> dir;
	public:
		virtual File* open(fs::path::iterator cur, fs::path::iterator end) = 0;
		virtual void move(fid_t src, fid_t dst) = 0;
		virtual void remove(fid_t fd) = 0;
		virtual std::vector<std::string> list() = 0;

		move(fid_t src, fid_t dst) => void
		remove(fid_t fd) => void
		list(fid_t fd) => void
		copy(fid_t src, fid_t dst) => void
		link(fid_t src, fid_t dst) => void
		stat(fid_t fd) => Stat stat
		wstat(fid_t fd, Stat stat) => void
		lsxattr(fid_t fd) => bytes data
		getxattr(fid_t fd, String name) => bytes value
		setxattr(fid_t fd, String name, bytes value) => void
		rmxattr(fid_t fd, String name) => void
		resize(fid_t fd, off_t size) => off_t size
};

class Directory : public Mount {
	protected:
		std::unordered_map<std::string, std::unique_ptr<Mount>> dir;
	public:
		virtual File* lookup(fs::path::iterator cur, fs::path::iterator end) override;
		virtual std::vector<std::string> list() override;
};

class DeviceMount : public Mount {
	protected:
		std::function<File*(fs::path::iterator, fs::path::iterator)> lookup_fn;
		std::function<std::vector<std::string>()> list_fn;
	public:
		DeviceMount(std::function<File*(fs::path::iterator, fs::path::iterator)> lu, std::function<std::vector<std::string>()> ls);
		
		virtual File* lookup(fs::path::iterator cur, fs::path::iterator end) override;
		virtual std::vector<std::string> list() override;
};

class NativeMount : public Mount {
	protected:
		fs::path root;
	public:
		NativeMount(const std::string& p);
		
		virtual File* lookup(fs::path::iterator cur, fs::path::iterator end) override;
		virtual std::vector<std::string> list() override;
};

class UnionMount : public Mount {
	protected:
		std::vector<std::unique_ptr<Mount>> mounts;
	public:
		virtual File* lookup(fs::path::iterator cur, fs::path::iterator end) override;
		virtual std::vector<std::string> list() override;
};

std::vector<u8> strls_to_bytes(const std::vector<std::string>& ls);

class File {
	public:
		Mount* mnt;
		fs::path path;

		File(fs::path p);

		virtual void open(mode_t mode);
		virtual void close();

		virtual off_t read(u8* data, chunk_t size);
		virtual off_t write(u8* data, chunk_t size);
		virtual void seek(off_t off);
		virtual off_t tell();
		virtual void insert(u8* data, chunk_t size);
		virtual void erase(off_t size);

		virtual std::vector<std::string> lsxattr();
		virtual std::vector<u8> getxattr(const std::string& name);
		virtual void setxattr(const std::string& name, const std::vector<u8>& data);
		virtual void rmxattr(const std::string& name);

		virtual void move(File* dst);
		virtual void remove();
		
		virtual std::vector<std::string> list();

		virtual void copy(File* dst);
		virtual void link(File* dst);
		virtual Stat stat();
		virtual void wstat(Stat stat);
		virtual off_t resize(off_t size);
};

class Path : public File {
	public:
		virtual void open(mode_t mode) override;

		virtual std::vector<std::string> lsxattr() override;
		virtual std::vector<u8> getxattr(const std::string& name) override;
		virtual void setxattr(const std::string& name, const std::vector<u8>& data) override;
		virtual void rmxattr(const std::string& name) override;

		virtual void move(File* dst) override;
		virtual void remove() override;
		
		virtual std::vector<std::string> list() override;

		virtual void copy(File* dst) override;
		virtual void link(File* dst) override;
		virtual Stat stat() override;
		virtual void wstat(Stat stat) override;
		virtual off_t resize(off_t size) override;
};

class NativeFile : public File {
	protected:
		FILE* file;
	public:
		NativeFile(std::string path, mode_t mode);
		
		virtual void close() override;
		
		virtual off_t read(u8* data, chunk_t size) override;
		virtual off_t write(u8* data, chunk_t size) override;
		virtual void seek(off_t off) override;
		virtual off_t tell() override;
		virtual void insert(u8* data, chunk_t size) override;
		virtual void erase(off_t size) override;

		virtual std::vector<std::string> lsxattr() override;
		virtual std::vector<u8> getxattr(const std::string& name) override;
		virtual void setxattr(const std::string& name, const std::vector<u8>& data) override;
		virtual void rmxattr(const std::string& name) override;
};

class VFS : public Mount {
	protected:
		std::unique_ptr<File> root;
	public:
		virtual File* lookup(fs::path::iterator cur, fs::path::iterator end) override;
		virtual std::vector<std::string> list() override;

		void mount(fs::path path, File* file);
};

template<typename Transport>
class Session : public Transport::Socket {
	protected:
		struct Filedes {
			std::unique_ptr<File> file;
			std::path path;
		};
		std::unordered_map<fid_t, Filedes> fdtab;
		std::vector<u8> buf;
		u8 last_tag;
		fid_t last_fid;
		VFS& fs;

		u8 nextTag() {
			return last_tag++;
		}

		fid_t nextFid() {
			return last_fid++;
		}

		Filedes& lookup(fid_t fd) {
			auto it = fdtab.find(fd);
			if(it == fdtab.end()) {
				throw Error(E_BADFD);
			}
			else {
				return it->second;
			}
		}

		fid_t dup(Packet::tx::dup& req) {
			auto& fe = lookup(req.fd);
			Filedes fc = {nullptr, fe.path}

			if(fe.file) {
				fc.file = fe.file->clone();
			}

			fdtab.insert(nextFid(), fc);
		}

		void drop(Packet::tx::drop& req) {
			auto& fe = lookup(req.fd);
			if(fe.file) {
				fe.file->close();
			}

			fdtab.erase(req.fd);
		}

		fid_t path(Packet::tx::path& req) {
			fid_t fd = nextFid();
			fdtab.insert(fd, {nullptr, fdtab[req.dir].path / req.name});
			return fd;
		}

		void open(Packet::tx::open& req) {
			auto& fe = lookup(req.fd);
			if(fe.file) {
				throw Error(E_NOTPATH);
			}
			else {
				fe.file = root.open(fe.path);
			}
		}

		void close(Packet::tx::close& req) {
			auto& fe = lookup(req.fd);
			if(fe.file) {
				fe.file.reset();
			}
			else {
				throw Error(E_NOTFILE);
			}
		}
	
	public:
		Session(fid_t fd, VFS& fs):sock(fd), fs(fs), last_tag(0), last_fid(1) {}

		Packet* createPacket(Command cmd, size_t len) {
			Packet* p = (Packet*)new u8[sizeof(BasePacket) + len];
			p->tag = last_tag++;
			p->cmd = cmd;
			p->payload_len = len;
			return p;
		}
		Packet* createResponse(Packet* p, size_t len) {
			Packet* np = (Packet*)new u8[sizeof(BasePacket) + len];
			np->tag = p->tag;
			np->cmd = p->cmd;
			np->payload_len = len;
			return np;
		}

		void send(Packet* p) {
			Transport::Socket::send((u8*)p, sizeof(BasePacket) + p->payload_len)
		}

		void error(ErrorCode err) {
			Packet* p = createPacket(C_ERROR, 1);
			p->error = err;
			send(p);
			delete p;
		}

		void recv(u8* data, size_t size) {
			buf.insert(buf.end(), data, data + size);
			if(buf.size() >= sizeof(BasePacket)) {
				Packet* p = (Packet*)buf.data();
				if(buf.size() - sizeof(BasePacket) >= p->payload_len) {
					handlePacket(p);
					buf.erase(buf.begin(), buf.begin() + buf.size() - sizeof(BasePacket));
				}
			}
		}

		void handlePacket(Packet* p) {
			// Ignore response packets, since the server doesn't
			//  request them.
			if(IS_RX(p->cmd)) return;

			std::unique_ptr<Packet> res = nullptr;
			switch(MAKE_TX(p->cmd)) {
				#include "server-dispatch.inc.cpp"
				
				default:
					error(E_BADCMD);
					break;
			}

			if(res) {
				send(res.get());
			}
		}
};

template<typename Transport>
class Server : public Transport::Server {
	protected:
		std::unique_ptr<File> root;
		std::unordered_set<Session<Transport>> sessions;
		VFS fs;
	
	public:
		void accept(Session<Transport>&& sess) {
			sessions.insert(sess);
		}
};

}

#endif
