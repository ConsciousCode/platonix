#include "tcp.hpp"
#include "common.hpp"

#include <unistd.h>

using namespace tcp;

tcp::Error::Error(int err, const char* msg)
	:std::system_error(err, std::generic_category(), msg) {}
tcp::Error::Error(const char* msg)
	:std::system_error(errno, std::generic_category(), msg) {}

void tcp::Socket::send(u8* data, size_t size) {
	::send(sock, data, size, MSG_DONTWAIT);
}
void tcp::Socket::recv(u8* data, size_t size) {
	::recv(sock, data, size, MSG_DONTWAIT);
}
void tcp::Socket::close() {
	::close(sock);
}

tcp::Server::Server() {
	// Try IPv6 first
	sock = socket(af=AF_INET6, SOCK_STREAM, 0);

	if(sock < 0) {
		switch(errno) {
			case EAFNOSUPPORT: {
				// Fall back on IPv4
				sock = socket(af=AF_INET, SOCK_STREAM, 0);
				if(sock > 0) break;

				// No way to salvage it
				[[fallthrough]];
			}

			default:
				throw Error("Couldn't open socket");
		}
	}
}

void tcp::Server::bind(int port) {
	union {
		struct sockaddr_in ipv4;
		struct sockaddr_in6 ipv6;
	} addr;

	switch(af) {
		case AF_INET:
			addr.ipv4 = {
				AF_INET, htons(port), htonl(INADDR_ANY)
			};
			if(::bind(sock, (struct sockaddr*)&addr, sizeof(addr.ipv6)) < 0) {
				throw Error("Couldn't bind socket");
			}
			break;
		
		case AF_INET6:
			addr.ipv6 = {
				AF_INET6, htons(port), 0, htonl(INADDR_ANY), 0
			};
			if(::bind(sock, (struct sockaddr*)&addr, sizeof(addr.ipv4)) < 0) {
				throw Error("Couldn't bind socket");
			}
			break;
		
		default: unreachable();
	}
}

static void poll_loop(tcp::Server& serv, std::atomic_flag running) {
	while(running.test_and_set(std::memory_order_relaxed)) {
		serv.poll();
		std::this_thread::yield();
	}
}
void tcp::Server::listen() {
	if(::listen(sock, SOCKQLEN) < 0) {
		throw Error("Couldn't listen on socket");
	}

	poll_thread = std::thread(poll_loop, *this);
}

void tcp::Server::poll() {
	for(;;) {
		int conn;
		if((conn = accept4(sock, nullptr, nullptr, SOCK_NONBLOCK)) < 0) {
			switch(errno) {
				case EAGAIN:
				#if EAGAIN != EWOULDBLOCK
					case EWOULDBLOCK:
				#endif
				case EINTR:
					break;
				
				case ENETDOWN:
				case ENONET:
						throw Error("Network down");
				
				case EHOSTDOWN:
					throw Error("Host down");
				
				case EHOSTUNREACH:
					throw Error("Host unreachable");
				
				case EBADF:
				case EINVAL:
				case ENOTSOCK:
				case EOPNOTSUPP:
				case EPROTO:
				case ENOPROTOOPT:
				case ESOCKTNOSUPPORT:
				case EPROTONOSUPPORT:
					throw Error("Socket corrupted");

				case ECONNABORTED:
					throw Error("Socket aborted");

				case EMFILE:
				case ENFILE:
				case ENOBUFS:
				case ENOMEM:
				case ENOSR:
					throw Error("Out of resources");

				case EPERM:
					throw Error("Firewall blocking");
				
				case ETIMEDOUT:
					throw Error("Socket timed out");
				
				default:
					throw Error("Unknown error in accept()");
			}
		}
		sessions.emplace(Session<Server>(conn, fs));
	}

	u8 buffer[8192];

	for(auto sess : sessions) {
		sess.second.Socket::recv(buffer, 8192, MSG_DONTWAIT);
	}
}

void tcp::Server::stop() {
	running.clear();
}
