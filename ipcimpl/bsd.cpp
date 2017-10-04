#include <sys/socket.h>

#include "Ctu.h"

/*$IPC$
partial nn::socket::sf::IClient {
  bool passthrough;
}
*/

nn::socket::sf::IClient::IClient(Ctu *_ctu) : IpcService(_ctu) {
	passthrough = _ctu->socketsEnabled;
}

uint32_t nn::socket::sf::IClient::accept(IN uint32_t socket, OUT int32_t& ret, OUT uint32_t& bsd_errno, OUT uint32_t& sockaddr_len, OUT sockaddr * _4, guint _4_size) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::accept");
	if(passthrough) {
		struct sockaddr *addr = (struct sockaddr *) _4;
		socklen_t size = (uint32_t) _4_size;
		ret = ::accept(socket, addr, &size);
		bsd_errno = errno;
		sockaddr_len = size;
		addr->sa_family = htons(addr->sa_family);
	} else {
		ret = 888;
		bsd_errno = 0;
	}
	return 0;
}
uint32_t nn::socket::sf::IClient::bind(IN uint32_t socket, IN sockaddr * _1, guint _1_size, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::bind");
	if(passthrough) {
		struct sockaddr *addr = (struct sockaddr *) _1;
		addr->sa_family = ntohs(addr->sa_family);
		ret = ::bind(socket, addr, (uint32_t) _1_size);
		bsd_errno = errno;
	} else {
		ret = 0;
		bsd_errno = 0;
	}
	return 0;
}
uint32_t nn::socket::sf::IClient::bsd_close(IN uint32_t socket, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::bsd_close");
	if(passthrough) {
		ret = ::close(socket);
		bsd_errno = errno;
	} else {
		ret = 0;
		bsd_errno = 0;
	}
	return 0;
}
uint32_t nn::socket::sf::IClient::connect(IN uint32_t socket, IN sockaddr * _1, guint _1_size, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::connect");
	if(passthrough) {
		struct sockaddr *addr = (struct sockaddr *) _1;
		addr->sa_family = ntohs(addr->sa_family); // yes, this is network byte order on the switch and host byte order on linux
		ret = ::connect(socket, (struct sockaddr *) addr, (socklen_t) _1_size);
		bsd_errno = errno;
	} else {
		ret = 0;
		bsd_errno = 0;
	}
	return 0;
}
uint32_t nn::socket::sf::IClient::getsockname(IN uint32_t socket, OUT int32_t& ret, OUT uint32_t& bsd_errno, OUT uint32_t& sockaddr_len, OUT sockaddr * _4, guint _4_size) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::getsockname");
	if(passthrough) {
		struct sockaddr *addr = (struct sockaddr *) _4;
		socklen_t addr_len = (socklen_t) _4_size;
		ret = ::getsockname(socket, addr, &addr_len);
		errno = bsd_errno;
		sockaddr_len = addr_len;
		addr->sa_family = htons(addr->sa_family);
	} else {
		sockaddr_len = 0;
		ret = 0;
		bsd_errno = 0;
	}
	return 0;
}
uint32_t nn::socket::sf::IClient::listen(IN uint32_t socket, IN uint32_t backlog, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::listen");
	if(passthrough) {
		ret = ::listen(socket, backlog);
		bsd_errno = errno;
	} else {
		ret = 0;
		bsd_errno = 0;
	}
	return 0;
}
uint32_t nn::socket::sf::IClient::recv(IN uint32_t socket, IN uint32_t flags, OUT int32_t& ret, OUT uint32_t& bsd_errno, OUT int8_t * _4, guint _4_size) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::recv");
	if(passthrough) {
		ret = (int32_t) ::recv(socket, _4, _4_size, flags);
		bsd_errno = errno;
	} else {
		ret = 0;
		bsd_errno = 0;
	}
	return 0;
}
uint32_t nn::socket::sf::IClient::send(IN uint32_t socket, IN uint32_t flags, IN int8_t * _2, guint _2_size, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::send");
	if(passthrough) {
		ret = (uint32_t) ::send(socket, _2, (size_t) _2_size, flags);
		bsd_errno = errno;
	} else {
		ret = 0;
		errno = 0;
	}
	return 0;
}
uint32_t nn::socket::sf::IClient::sendto(IN uint32_t socket, IN uint32_t flags, IN int8_t * _2, guint _2_size, IN sockaddr * _3, guint _3_size, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::sendto");
	if(passthrough) {
		struct sockaddr *addr = (struct sockaddr *) _3;
		addr->sa_family = ntohs(addr->sa_family);
		ret = (uint32_t) ::sendto(socket, _2, (size_t) _2_size, flags, (struct sockaddr *) addr, (socklen_t) _3_size);
		bsd_errno = errno;
	} else {
		ret = 0;
		bsd_errno = 0;
	}
	return 0;
}
uint32_t nn::socket::sf::IClient::setsockopt(IN uint32_t socket, IN uint32_t level, IN uint32_t option_name, IN uint8_t * _3, guint _3_size, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::setsockopt");
	return 0;
}
uint32_t nn::socket::sf::IClient::shutdown(IN uint32_t socket, IN uint32_t how, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::shutdown");
	if(passthrough) {
		ret = ::shutdown(socket, how);
	} else {
		ret = 0;
		bsd_errno = 0;
	}
	return 0;
}
uint32_t nn::socket::sf::IClient::socket(IN uint32_t domain, IN uint32_t type, IN uint32_t protocol, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::socket");
	if(passthrough) {
		ret = ::socket(domain, type, protocol);
		bsd_errno = errno;
	} else {
		ret = 777;
		bsd_errno = 0;
	}
	return 0;
}
