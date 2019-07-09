'use strict';

const
	net = require("net");

interface FS {
	resolve(path: string) {}
}

class VFS implements FS {
	parent: FS;
	children: {[key: string]: FS};
	
	constructor(parent, children) {
		this.parent = parent;
		this.children = children;
	}
	
	resolve(path: string) {
		let cur = this, pv = path.split(/\//g);
		
		for(let p of pv) {
			cur = cur.children[p];
			if(cur instanceof VFS) continue;
			
			if(cur) {
				return cur.resolve(path);
			}
			else {
				throw new Error("Not found");
			}
		}
		
		return cur;
	}
	
	add(name: string, fs: FS) {
		return this.children[name] = fs;
	}
	
	mount(path: string, fs: FS) {
		let cur = this, pv = path.split(/\//g);
		
		for(let p of pv) {
			cur = cur.children[p];
			if(cur instanceof VFS) continue;
			
			if(cur === undefined) {
				cur = cur.add(path, new VFS(cur, {}));
			}
		}
		
		return cur;
	}
}

class Server {
	root: VFS;
	serv: net.Server;
	conn: Map<net.Socket, Session>
	
	constructor() {
		this.root = new VFS(null, {
			"tmp": new TmpFS(),
			"sys": new SysFS(),
			"proc": new ProcFS()
		});
		this.server = net.createServer();
		this.server.on("connection", sock => {
			
		});
		this.server.on("data", data => {
			let [tag, conf] = data;
			let isrx = !!(conf&(1<<7));
			let more = !!(conf&(1<<6));
			let cmd = conf&0x3f;
			let plen = data.readUint16BE(2);
			
			switch(cmd) {
				case AVID_ERROR:
				case AVID_OPEN:
				case AVID_CLOSE:
				case AVID_READ:
				case AVID_WRITE:
				case AVID_SEEK:
				case AVID_ALLOC:
				case AVID_CP:
				case AVID_MV:
			}
		});
		this.server.listen(path);
	}

}
