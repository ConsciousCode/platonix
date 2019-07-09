#ifndef PLATONIX_TCP_HPP
#define PLATONIX_TCP_HPP

#include "types.hpp"
#include "server.hpp"

#include <system_error>
#include <thread>
#include <unordered_map>

#define SOCKQLEN 8

struct tcp {
	class Error : public std::system_error {
		public:
			Error(int err, const char* msg);
			Error(const char* msg);
	};

	class Socket {
		protected:
			fid_t sock;
		
		public:
			void send(u8* data, size_t size);
			void recv(u8* data, size_t size);

			void close();
	};

	class Server {
		protected:
			fid_t sock;
			std::thread poll_thread;
			std::atomic_flag running;
			int af;

		public:
			Server();

			/**
			 * Unimplemented - defined by pnix::Server which inherits from this class
			**/
			void accept(pnix::Session<Server>&& sess);

			void bind(int port);
			void listen();
			void poll();
			void stop();
	};

	class Client {

	};
};

#endif
