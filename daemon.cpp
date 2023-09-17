#include <unordered_map>
#include <queue>
#include <memory>

#include <dlfcn.h>
#include <sys/socket.h>
#include <sys/un.h>

typedef uint32_t contype_t;
typedef uint32_t fid_t;
typedef int32_t sfid_t;

struct XAttrFile : public File {
	list()
}

struct XAttr : public File {
	read()
	write()
	stat() // override default to error
}

//Example:

PNix pnix(Directory({
	{"a", AFile()},
	{"b", BFile()},
	{"c", CFile()}
}))

#io/cmd ioctl
## fcntl
	##mode
	##offset
	##dup
	##lock
	##refs number of open file descriptors
#xattr/attr
#stat/attr
#type contype
#index index of all subfiles regardless of file type


/**
 * File representing a name in the filesystem used to simplify calls
**/
struct Name {
	File* parent;
	std::string_view name;
	
	Name lookup();
	Handle* open(mode_t mode);
	
	void link(Handle* target);
	void unlink();
	
	void trunc(off_t off);
	ssize_t alloc(int mode, off_t off, off_t len);
	ssize_t copy(Name* fn_in, off_t off_in, Name* fn_out, off_t off_out, size_t size, int flags);
	
	bool access(int mode);
	void poll(struct fuse_pollhandle *ph);
}

/**
 * An open file which can be read/written to
**/
struct Handle {
	Name* lookup();
	Handle* open(mode_t mode);
	
	
	File* lookup(std::string_view name);
	int create(std::string_view name, mode_t mode, dev_t);
	int remove(std::string_view name);
	int rename(std::string_view src, std::string_view dst, uint flags);
	int trunc(std::string_view name, off_t off);
	int open(std::string_view name);
	int read(std::string_view name, char* data, size_t sz, off_t off);
	int write(std::string_view name, const char* data, size_t sz, off_t off);
	int access(std::string_view name, int mode);
	int bmap(std::string_view name, size_t blocksz, uint64_t idx);
	int poll(std::string_view name, struct fuse_pollhandle *ph);
	int write_buf(std::string_view name, struct fuse_bufvec* buf, off_t off); // use fuse_buf_copy
	int read_buf(std::string_view name, struct fuse_bufvec** bufp, size_t sz, off_t off);
	int alloc(<>, int mode, off_t off, off_t len);
	ssize_t(* 	copy )(const char *path_in, struct fuse_file_info *fi_in, off_t offset_in, const char *path_out, struct fuse_file_info *fi_out, off_t offset_out, size_t size, int flags)
	int seek(<>, off_t off, int whence);
	int flush();
	void release();
	int fsync();

	setattr()
}

struct Directory {
	Name* lookup(std::string_view name);
}

struct File {
	Name* lookup(std::string_view name);
	void link(Handle* target);
	void unlink();
	
	void trunc(off_t off);
	int open(mode_t mode);
	int read(std::string_view name, char* data, size_t sz, off_t off);
	int write(std::string_view name, const char* data, size_t sz, off_t off);
	int access(std::string_view name, int mode);
	int bmap(std::string_view name, size_t blocksz, uint64_t idx);
	int poll(std::string_view name, struct fuse_pollhandle *ph);
	int write_buf(std::string_view name, struct fuse_bufvec* buf, off_t off); // use fuse_buf_copy
	int read_buf(std::string_view name, struct fuse_bufvec** bufp, size_t sz, off_t off);
	int alloc(<>, int mode, off_t off, off_t len);
	ssize_t(* 	copy )(const char *path_in, struct fuse_file_info *fi_in, off_t offset_in, const char *path_out, struct fuse_file_info *fi_out, off_t offset_out, size_t size, int flags)
	int seek(<>, off_t off, int whence);
	int flush();
	int release();
	int fsync();

	setattr()
}

struct PNix {
	std::map<ino_t, std::unique_ptr<INode>> inotab;
	
	pthread_mutex_t mutex;
	int debug;
	int writeback;
	int flock;
	int xattr;
	const char *source;
	double timeout;
	int cache;
	int timeout_set;
	struct lo_inode root; /* protected by lo->mutex */
};

void fuse_init(void* userdata, struct fuse_conn_info* conn) {
	
}

void fuse_destroy(void* userdata) {
	
}

int fuse_statfs(const char *, struct statvfs *) {
	return 0;
}

File* pnix_inode(fuse_req_t req, fuse_ino_t parent) {
	PNix* ud = (auto)fuse_req_userdata(req, parent);
	
	auto it = ud->inotab.find(parent);
	if(it == ud->inotab.end()) return nullptr;
	
	return *it;
}

void fuse_lookup(fuse_req_t req, fuse_ino_t parent, const char* name) {
	struct fuse_entry_param e;
	File* f = pnix_inode(req, parent);
	
	std::string_view nsv = name;
	size_t p = 0;
	for(;;) {
		size_t np = nsv.find_first_of("/#", p);
		
		f = f->lookup(nsv.substr(p, np));
		
		if(np == nsv.npos) break;
		
		if(nsv[np] == '/') ++np;
		
		p = np;
	}
	
	struct fuse_entry_param e;
	e->attr.st_ino = f->ino;
	e->attr.st_dev = f->dev;
	fuse_reply_entry(req, &e);
}

void fuse_forget(fuse_req_t req, fuse_ino_t ino, uint64_t nlookup) {
	
}

struct fuse_lowlevel_ops = {
	.init = fuse_init,
	.destroy = fuse_destroy,
	.lookup = fuse_lookup,
	.forget = fuse_forget,

	void (*forget) (fuse_req_t req, fuse_ino_t ino, uint64_t nlookup);

	void (*getattr) (fuse_req_t req, fuse_ino_t ino,
					struct fuse_file_info *fi);

	void (*setattr) (fuse_req_t req, fuse_ino_t ino, struct stat *attr,
					int to_set, struct fuse_file_info *fi);

	void (*readlink) (fuse_req_t req, fuse_ino_t ino);

	void (*mknod) (fuse_req_t req, fuse_ino_t parent, const char *name,
				mode_t mode, dev_t rdev);

	void (*mkdir) (fuse_req_t req, fuse_ino_t parent, const char *name,
				mode_t mode);

	void (*unlink) (fuse_req_t req, fuse_ino_t parent, const char *name);

	void (*rmdir) (fuse_req_t req, fuse_ino_t parent, const char *name);

	void (*symlink) (fuse_req_t req, const char *link, fuse_ino_t parent,
					const char *name);

	void (*rename) (fuse_req_t req, fuse_ino_t parent, const char *name,
					fuse_ino_t newparent, const char *newname,
					unsigned int flags);

	void (*link) (fuse_req_t req, fuse_ino_t ino, fuse_ino_t newparent,
				const char *newname);

	void (*open) (fuse_req_t req, fuse_ino_t ino,
				struct fuse_file_info *fi);

	void (*read) (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
				struct fuse_file_info *fi);

	void (*write) (fuse_req_t req, fuse_ino_t ino, const char *buf,
				size_t size, off_t off, struct fuse_file_info *fi);

	void (*flush) (fuse_req_t req, fuse_ino_t ino,
				struct fuse_file_info *fi);

	void (*release) (fuse_req_t req, fuse_ino_t ino,
					struct fuse_file_info *fi);

	void (*fsync) (fuse_req_t req, fuse_ino_t ino, int datasync,
				struct fuse_file_info *fi);

	void (*opendir) (fuse_req_t req, fuse_ino_t ino,
					struct fuse_file_info *fi);

	void (*readdir) (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
					struct fuse_file_info *fi);

	void (*releasedir) (fuse_req_t req, fuse_ino_t ino,
						struct fuse_file_info *fi);

	void (*fsyncdir) (fuse_req_t req, fuse_ino_t ino, int datasync,
					struct fuse_file_info *fi);

	void (*statfs) (fuse_req_t req, fuse_ino_t ino);

	void (*setxattr) (fuse_req_t req, fuse_ino_t ino, const char *name,
					const char *value, size_t size, int flags);

	void (*getxattr) (fuse_req_t req, fuse_ino_t ino, const char *name,
					size_t size);

	void (*listxattr) (fuse_req_t req, fuse_ino_t ino, size_t size);

	void (*removexattr) (fuse_req_t req, fuse_ino_t ino, const char *name);

	void (*access) (fuse_req_t req, fuse_ino_t ino, int mask);

	void (*create) (fuse_req_t req, fuse_ino_t parent, const char *name,
					mode_t mode, struct fuse_file_info *fi);

	void (*getlk) (fuse_req_t req, fuse_ino_t ino,
				struct fuse_file_info *fi, struct flock *lock);

	void (*setlk) (fuse_req_t req, fuse_ino_t ino,
				struct fuse_file_info *fi,
				struct flock *lock, int sleep);

	void (*bmap) (fuse_req_t req, fuse_ino_t ino, size_t blocksize,
				uint64_t idx);

	void (*ioctl) (fuse_req_t req, fuse_ino_t ino, unsigned int cmd,
				void *arg, struct fuse_file_info *fi, unsigned flags,
				const void *in_buf, size_t in_bufsz, size_t out_bufsz);

	void (*poll) (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi,
				struct fuse_pollhandle *ph);

	void (*write_buf) (fuse_req_t req, fuse_ino_t ino,
					struct fuse_bufvec *bufv, off_t off,
					struct fuse_file_info *fi);

	void (*retrieve_reply) (fuse_req_t req, void *cookie, fuse_ino_t ino,
							off_t offset, struct fuse_bufvec *bufv);

	void (*forget_multi) (fuse_req_t req, size_t count,
						struct fuse_forget_data *forgets);

	void (*flock) (fuse_req_t req, fuse_ino_t ino,
				struct fuse_file_info *fi, int op);

	void (*fallocate) (fuse_req_t req, fuse_ino_t ino, int mode,
				off_t offset, off_t length, struct fuse_file_info *fi);

	void (*readdirplus) (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
					struct fuse_file_info *fi);

	void (*copy_file_range) (fuse_req_t req, fuse_ino_t ino_in,
							off_t off_in, struct fuse_file_info *fi_in,
							fuse_ino_t ino_out, off_t off_out,
							struct fuse_file_info *fi_out, size_t len,
							int flags);

	void (*lseek) (fuse_req_t req, fuse_ino_t ino, off_t off, int whence,

struct subfile_ops {
	ssize_t (*read)(struct fnode*, size_t, void*);
	ssize_t (*write)(struct fnode*, size_t, void*);
	
	int (*open)(struct fnode*, const char*, mode_t, struct fnode*);
	// dup = open(fd, null, mode, nfd)
	// rename = dup(from, to, ~KEEP_ORIGINAL)
	// remove = rename(fnode, null)
	int (*sync)(struct fnode*);
	int (*release)(struct fnode*);
	
	// Returns 0 (success) or < 0 (failure with errno)
	int (*access)(struct fnode*, struct user*);
	
	ssize_t (*copy)(struct fnode*, struct fnode*, size_t);
	
	int (*mount)(struct fnode*, const char*, struct subfile_ops*);
};

struct syscall_ops {
	ssize_t (*read)(fid_t, size_t, void*);
	ssize_t (*write)(fid_t, size_t, void*);
	
	sfid_t (*open)(fid_t, const char*, mode_t);
	int (*sync)(fid_t);
	int (*release)(fid_t);
	
	// Returns 0 (success) or < 0 (failure with errno)
	int (*access)(fid_t, struct user*);
	
	ssize_t (*copy)(fid_t, fid_t, size_t);
	int (*dup)(fid_t, fid_t, mode_t);
	// rename = dup(from, to, ~KEEP_ORIGINAL)
	// remove = rename(fnode, null)
};

open(file 1); open(file 2); copy(,,-1); release(); // piping
open(file); open(name, O_PATH); dup(,, 0); release(); // rename
open(file); read(size); release(); // quick read
open(file); write(data, size); release(); // quick write

struct Lib {
	std::string name;
	void* lib;
	subfile_ops*(*get_contype_handler)(contype_t);
	
	Lib(std::string name):name(name), lib(nullptr) {}
	
	~Lib() {
		close();
	}
	
	void close() {
		if(lib) dlclose(lib);
	}
	
	subfile_ops* getHandler(contype_t ct) {
		if(!lib) {
			// We handle lazy opening, library only exposes get_contype_handler
			lib = dlopen(name.c_str(), RTLD_NOW|RTLD_LOCAL);
			get_contype_handler = (subfile_ops*(*)(contype_t))dlsym(lib, "get_contype_handler");
		}
		
		if(get_contype_handler) {
			return get_contype_handler(ct);
		}
		return nullptr;
	}
};

struct Handler {
	Lib* lib;
	subfile_ops* ops;
	
	void deinit() {
		ops = nullptr;
		lib->close();
	}
};

constexpr unsigned LRU_SIZE = 1024;

struct HandlerManager {
protected:
	std::unordered_map<contype_t, Handler> handlers;
	std::unordered_map<std::string, std::unique_ptr<Lib>> libs;
	std::queue<contype_t> cache;
	
	void use(contype_t ct) {
		if(handlers.size() + 1 >= LRU_SIZE) {
			// Edge case: if the LRU is the next contype, don't deinit
			if(cache.front() != ct) {
				handlers[cache.front()].deinit();
			}
			cache.pop();
		}
		cache.push(ct);
	}
public:
	void addLib(const std::string& name, const std::vector<contype_t>& contypes) {
		auto lib = new Lib(name);
		lib->name = name;
		lib->lib = nullptr;
		libs.emplace(name, lib);
		
		for(auto ct : contypes) {
			handlers[ct] = {lib, nullptr};
		}
	}
	
	const subfile_ops* getHandler(contype_t ct) {
		use(ct);
		
		if(handlers.count(ct)) {
			auto h = handlers[ct];
			auto ops = h.ops;
			
			if(ops) return ops;
			
			return h.ops = h.lib->getHandler(ct);
		}s FNode {
	public:
		contype_t type;
};

class File {
	protected:
		struct subfile_ops* ops;
		
	public:
		
};

user - view the root filesystem
nobody
anybody
app
lib
root
gui
daemon
startup
init
i2c
i2c-1


filesystem per usergroup
if a user is created as part of a usergroup, it sees and can make all modifications to the filesystem available to that usergroup

/exe
/user
	(name)
		/bin
		/var
		/lib
		/app
		/pkg
/run (/proc)
	/tmp
	
/etc
/sys
	/boot
/var
	/log
	
/lib
	/include
/app
/pkg
	(package manager)
		(name)
			(version)
/io
	/net
	/media
	/bus

altman - manages global /exe and /lib namespace


class MountTree {
	protected:
		struct MountPoint {
			fs::path p;
			std::unique_ptr<Mount> m;
			std::unique_ptr<MountTree> tree;
		};
		std::map<std::string, MountPoint> mounts;
	
	public:
		void mount(fs::path p, Mount* m) {
			auto x = p.begin();
			
			if(mounts.count(*x)) {
				auto& mp = mounts[*x++];
				auto y = mp.p.begin();
				
				for(;x != p.end() && y != mp.p.end() && *x == *y; ++x, ++y) {}
				
				if(x == p.end()) {
					if(y == mp.p.end()) {
						// Remount
						mp.m->umount();
						mp.m = m;
					}
					else {
						
					}
				}
			}
			for( it != p.end(); ++it) {
				
			}
		}
};

class FEntry {
	protected:
		File*
}

Filesystem objects:
* Mount - filesystem managed by a daemon
* Index/Directory - Underlying structure tying it all together
* Subfile - automatically mounted when opened
* Metafile - stack of responsibility, fs/subfile
* Whiteout - FS says file DNE and ends lookup without deferring

class VFS {
	protected:
		std::vector<std::unique_ptr<FNode>> fnode_cache;
		std::vector<size_t> fnode_avail;
	public:
		std::unique_ptr<File> root;
};

class Client {
	protected:
		int sock;
		std::vector<int> fdstack;
		uint8_t buffer[1<<16];
		int offset;
	
	public:
		int getfd(int mode) {
			switch(mode) {
				case 0:
					/* pop from stack */
					if(fdstack.size()) {
						int fd = fdstack.back();
						fdstack.pop_back();
						return fd;
					}
					else {
						return 0;
					}
				
				case 1:
					/* peek from stack */
					if(fdstack.size()) {
						return fdstack.back();
					}
					else {
						return 0;
					}
				
				case 2:
					/* immediate constant */
					return buffer[++offset];
				
				case 3: {
					/* immediate index */
					int imm = buffer[++offset];
					
					if(imm > fdstack.size()) {
						return 0;
					}
					else {
						return fdstack[fdstack.size() - imm - 1];
					}
				}
			}
		}
		
		void process() {
			recv(sock, buffer, sizeof(buffer), 0);
			
			if(recv(sock, buffer, 1<<16, 0) > 0) {
				return;
			}
			
			int fd;
			
			switch(buffer[0]) {
				case C_READ:
					(fd)
					next(2) as u16 size
					return read
					
				case C_WRITE:
					(fd)
					next(2) as u16 size
					(consume that many)
					return count
				
				case C_OPEN:
					(fd)
					next(4) as u32 flags
					next(2) as u12, high 4 discarded
					(consume path)
					return open fd
					
				case C_SYNC:
					(fd)
				case C_RELEASE:
					(fd)
					
				case C_ACCESS:
					(fd)
					next(4) as uint32 user index
				case C_COPY:
					(fd)
					(fd)
					next(2) as u16 size
				case C_MOUNT:
					(fd)
			}
		}
};

class Daemon {
	protected:
		VFS vfs;
		int sock;
		std::vector<Client> clients;
	
	public:
		Daemon() {
			sock = socket(AF_LOCAL, SOCK_STREAM, 0);
			if(sock < 0) {
				perror("socket");
				exit(1);
			}
		}
		
		void connect(const std::string& path) {
			struct sockaddr_un addr = {AF_UNIX};
			memcpy(&addr.sun_path[0], path.c_str(), path.length() + 1);
			::connect(sock, (struct sockaddr*)&addr, sizeof(addr.sun_family) + path.length() + 1);
			listen(sock, nconn);
		}
		
		void ping() {
			struct sockaddr_un addr;
			socklen_t len;
			int client = ::accept(sock, (struct sockaddr*)&addr, &len);
		}
		
		void process() {
			for(auto& client : clients) {
				if(recv(client))
			}
		}
};
VFS
	FNode - lightweight, lazily loaded file
	File - an instantiated FNode 
	