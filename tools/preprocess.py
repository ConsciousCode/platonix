import re
import sys

FSIG = re.compile(r'''^(\S+)\((.*?)\)\s*=>\s*(.+?)$''')
COMMA = re.compile(r'''\s*,\s*''')
SPACE = re.compile(r'''\s+''')
SOL = re.compile("^", re.M)
ERR = re.compile(r'''^(\S+)\s*//\s*(.+)''')

comment = '''\
/**
 * This file was generated using `python preprocess.py`
**/

'''

def indent(v, level):
	return SOL.sub('\t'*level, v)

alias = "req"
cmt = '''case C_%s: {{
	auto& %s = p->tx.%s;
%s
	break;
}}'''
def case(n, x):
	return cmt%(n.upper(), alias, n, indent(x, 1))

casetpl = '''case C_{0}: {{
	auto& %s = p->tx.{1};
	lookup(%s.fd).{1}({2});
	break;
}}'''%(alias, alias)
casedup = case("dup", '''\
res = createResponse(p, sizeof(big<fid_t>));
res->rx.dup.fd = dup(%s);
'''%alias).format()
casedrop = case("drop", "drop(%s);"%alias).format()
casepath = case("path", '''\
res = createResponse(p, sizeof(big<fid_t>));
res->rx.path.fd = path(%s);
'''%alias).format()
caseopen = case("open", "open(%s);"%alias).format()
caseclose = case("close", "close(%s);"%alias).format()
casewhois = '''\
case C_WHOIS: {
	auto& x = p->tx.whois;
	auto ret = whois(x.uid);
	res = createResponse(p, sizeof(u8) + ret.length());
	res->rx.whois.name = ret;
	break;
}
'''
casewhoami = '''\
case C_WHOAMI: {
	auto ret = whoami();
	res = createResponse(p, sizeof(u8) + ret.length());
	res->rx.whoami.uid = ret.uid;
	res->rx.whoami.name = ret.name;
	break;
}
'''
casereturn = '''case C_{0}: {{
	auto& %s = p->tx.{1};
	auto ret = fdtab[%s.fid].{1}({2});{3}
	res = createResponse(p, {4});
{5}
	break;
}}'''%(alias, alias)

ITYPES = set(
	'u16 u32 u64 fid_t uid_t gid_t off_t block_t'.split(' ')
)

class Param:
	def __init__(self, s):
		t, n = SPACE.split(s)

		if t in ITYPES:
			self.type = "big<%s>"%t
		elif t in {"bytes", "string"}:
			self.type = t.title()
		else:
			self.type = t
		
		self.name = n
	
	def __str__(self):
		if self.type == "string[]":
			return "Bytes /* string[] */ %s;"%self.name
		else:
			return "%s %s;"%(self.type, self.name)

class ErrorCode:
	def __init__(self, s):
		name, desc = ERR.match(s).groups()

		self.name = name.upper()
		self.desc = desc
	
	def case(self):
		return 'case %s: return "%s";'%(self.name, self.desc)

class Command:
	def __init__(self, s):
		name, args, res = FSIG.match(s).groups()
		self.name = name

		if not args or args.isspace():
			self.args = []
		else:
			self.args = [Param(c) for c in COMMA.split(args)]
		
		if res == "void":
			self.result = []
		else:
			self.result = [Param(x) for x in COMMA.split(res)]
		self.cmd = "C_" + name.upper()

	def tx(self):
		return "struct {\n%s\n} %s;"%(indent('\n'.join(str(x) for x in self.args), 1), self.name)
	
	def rx(self):
		if len(self.result) == 0:
			return "struct {} %s;"%self.name
		else:
			return "struct {\n%s\n} %s;"%(indent('\n'.join(str(x) for x in self.result), 1), self.name)
	
	def case(self):
		if self.name == "path":
			return casepath
		elif self.name == "open":
			return caseopen
		elif self.name == "close":
			return caseclose
		elif self.name == "whois":
			return casewhois
		elif self.name == "whoami":
			return casewhoami
		elif len(self.result) == 0:
			return casetpl.format(
				self.name.upper(), self.name,
				', '.join("x.%s"%c.name for c in self.args)
			)
		else:
			v = []
			packlen = []
			postproc = []

			if len(self.result) == 1:
				ret = "ret"
			else:
				tpl = "ret.{1};"
			
			tpl = "res->rx.{0}.{1} = %s;"%ret
			
			for r in self.result:
				if r.type == "string[]":
					v.append("res->rx.%s.%s = sls;"%(self.name, r.name))
					postproc.append("auto sls = strls_to_bytes(%s);"%ret)
					packlen.append("sls.size()")
				else:
					v.append(tpl.format(self.name, r.name))

					if r.type == "Bytes":
						packlen.append("sizeof(Bytes::size)")
						packlen.append("ret.size()")
					elif r.type == "String":
						packlen.append("sizeof(String::length)")
						packlen.append("ret.length()")
					else:
						packlen.append("sizeof(%s)"%r.type)
			
			if len(packlen) == 0:
				packlen = ["0"]
			
			if len(postproc) == 0:
				pp = ""
			else:
				pp = "\n" + indent('\n'.join(postproc), 1)

			return casereturn.format(
				self.name.upper(), self.name,
				', '.join("x.%s"%c.name for c in self.args),
				pp, " + ".join(packlen),
				indent("\n".join(v), 1)
			)

cmds = [
	Command(line)
	for line in open("methods.rpc", "rt").readlines()
	if line and not line.isspace()
]

errs = [
	ErrorCode(line)
	for line in open("errors.rpc", "rt").readlines()
	if line and not line.isspace()
]

tpl = open("protocol.tpl.hpp", "rt").read()
with open("protocol.hpp", "wt") as out:
	out.write(comment + tpl%(
		"\t%s\n"%('\n'.join("E_%s, // %s"%(e.name, e.desc)) for e in errs)
			+ "\t%s // %s"%(errs[-1].name, errs[-1].desc),
		indent('\n'.join(c.tx() for c in cmds), 3),
		indent('\n'.join(c.rx() for c in cmds), 3),
		indent(',\n'.join(c.cmd for c in cmds), 1)
	))

with open("server-dispatch.inc.cpp", "wt") as out:
	out.write(comment + '\n'.join(c.case() for c in cmds))

with open("error.inc.cpp", "wt") as out:
	out.write(comment + "\n".join(e.case() for e in errs))