#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "Ctu.h"

/*$IPC$
  partial nn::socket::sf::IClient {
  bool passthrough;
  }
  partial nn::socket::resolver::IResolver {
  bool passthrough;
  }
*/

#ifdef __APPLE__
#define fam_ntohs passthru_uint8
#define fam_htons passthru_uint8
__uint8_t passthru_uint8(__uint8_t a) { return a; }
#else
#define fam_htons htons
#define fam_ntohs ntohs
#endif

nn::socket::sf::IClient::IClient(Ctu *_ctu) : IpcService(_ctu) {
	passthrough = _ctu->socketsEnabled;
}

uint32_t nn::socket::sf::IClient::Accept(IN uint32_t socket, OUT int32_t& ret, OUT uint32_t& bsd_errno, OUT uint32_t& sockaddr_len, OUT nn::socket::sockaddr_in * _4, guint _4_size) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::accept");
	if(passthrough) {
		struct sockaddr *addr = (struct sockaddr *) _4;
		socklen_t size = (uint32_t) _4_size;
		ret = ::accept(socket, addr, &size);
		bsd_errno = errno;
		sockaddr_len = size;
		addr->sa_family = fam_htons(addr->sa_family);
	} else {
		ret = 888;
		bsd_errno = 0;
	}
	return 0;
}

uint32_t nn::socket::sf::IClient::Bind(IN uint32_t socket, IN nn::socket::sockaddr_in * _1, guint _1_size, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::bind");
	if(passthrough) {
		struct sockaddr *addr = (struct sockaddr *) _1;
		addr->sa_family = fam_ntohs(addr->sa_family);
		ret = ::bind(socket, addr, (uint32_t) _1_size);
		bsd_errno = errno;
	} else {
		ret = 0;
		bsd_errno = 0;
	}
	return 0;
}
uint32_t nn::socket::sf::IClient::Close(IN uint32_t socket, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
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
uint32_t nn::socket::sf::IClient::Connect(IN uint32_t socket, IN nn::socket::sockaddr_in * _1, guint _1_size, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::connect");
	if(passthrough) {
		struct sockaddr *addr = (struct sockaddr *) _1;
		addr->sa_family = fam_ntohs(addr->sa_family); // yes, this is network byte order on the switch and host byte order on linux
		ret = ::connect(socket, (struct sockaddr *) addr, (socklen_t) _1_size);
		bsd_errno = errno;
	} else {
		ret = 0;
		bsd_errno = 0;
	}
	return 0;
}
uint32_t nn::socket::sf::IClient::GetSockName(IN uint32_t socket, OUT int32_t& ret, OUT uint32_t& bsd_errno, OUT uint32_t& sockaddr_len, OUT nn::socket::sockaddr_in * _4, guint _4_size) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::getsockname");
	if(passthrough) {
		struct sockaddr *addr = (struct sockaddr *) _4;
		socklen_t addr_len = (socklen_t) _4_size;
		ret = ::getsockname(socket, addr, &addr_len);
		errno = bsd_errno;
		sockaddr_len = addr_len;
		addr->sa_family = fam_htons(addr->sa_family);
	} else {
		sockaddr_len = 0;
		ret = 0;
		bsd_errno = 0;
	}
	return 0;
}
uint32_t nn::socket::sf::IClient::Listen(IN uint32_t socket, IN uint32_t backlog, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
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
uint32_t nn::socket::sf::IClient::Recv(IN uint32_t socket, IN uint32_t flags, OUT int32_t& ret, OUT uint32_t& bsd_errno, OUT int8_t * _4, guint _4_size) {
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
uint32_t nn::socket::sf::IClient::Send(IN uint32_t socket, IN uint32_t flags, IN int8_t * _2, guint _2_size, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
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
uint32_t nn::socket::sf::IClient::SendTo(IN uint32_t socket, IN uint32_t flags, IN int8_t * _2, guint _2_size, IN nn::socket::sockaddr_in * _3, guint _3_size, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::sendto");
	if(passthrough) {
		struct sockaddr *addr = (struct sockaddr *) _3;
		addr->sa_family = fam_ntohs(addr->sa_family);
		ret = (uint32_t) ::sendto(socket, _2, (size_t) _2_size, flags, (struct sockaddr *) addr, (socklen_t) _3_size);
		bsd_errno = errno;
	} else {
		ret = 0;
		bsd_errno = 0;
	}
	return 0;
}
uint32_t nn::socket::sf::IClient::SetSockOpt(IN uint32_t socket, IN uint32_t level, IN uint32_t option_name, IN uint8_t * _3, guint _3_size, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::setsockopt");
	return 0;
}
uint32_t nn::socket::sf::IClient::Shutdown(IN uint32_t socket, IN uint32_t how, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::socket::sf::IClient::shutdown");
	if(passthrough) {
		ret = ::shutdown(socket, how);
	} else {
		ret = 0;
		bsd_errno = 0;
	}
	return 0;
}
uint32_t nn::socket::sf::IClient::Socket(IN uint32_t domain, IN uint32_t type, IN uint32_t protocol, OUT int32_t& ret, OUT uint32_t& bsd_errno) {
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


nn::socket::resolver::IResolver::IResolver(Ctu *_ctu) : IpcService(_ctu) {
	passthrough = _ctu->socketsEnabled;
}

static bool pack_addrinfo(const struct addrinfo *ai, uint8_t *buf, int size) {
	struct {
		uint32_t magic;
		uint32_t ai_flags;
		uint32_t ai_family;
		uint32_t ai_socktype;
		uint32_t ai_protocol;
		uint32_t ai_addrlen;
	} ai_packed_header = {
		htonl(0xBEEFCAFE),
		htonl(ai->ai_flags),
		htonl(ai->ai_family),
		htonl(ai->ai_socktype),
		htonl(ai->ai_protocol),
		htonl(ai->ai_addrlen)
	};
	static_assert(sizeof(ai_packed_header) == 6*4, "ai_packed_header size is wrong");
	
	if(size < sizeof(ai_packed_header)) {
		return true;
	}

	memcpy((void*) buf, &ai_packed_header, sizeof(ai_packed_header));
	buf+= sizeof(ai_packed_header);
	size-= sizeof(ai_packed_header);

	if(size < (ai->ai_addrlen == 0 ? 4 : ai->ai_addrlen)) {
		return true;
	}

	if(ai->ai_addrlen == 0) {
		*((uint32_t*) buf) = 0;
	} else {
		switch(ai->ai_family) {
		case AF_INET: {
			struct sockaddr_in *buf_as_sockaddr_in = (struct sockaddr_in*) buf;
			struct sockaddr_in *sockaddr = (struct sockaddr_in*) ai->ai_addr;
			buf_as_sockaddr_in->sin_family = sockaddr->sin_family;
			buf_as_sockaddr_in->sin_port = htons(sockaddr->sin_port); // I think that this is erroneously byteswapped
			buf_as_sockaddr_in->sin_addr.s_addr = htonl(sockaddr->sin_addr.s_addr); // this too
			memset(&buf_as_sockaddr_in->sin_zero, 0, sizeof(buf_as_sockaddr_in->sin_zero));
			break;
		}
		case AF_INET6:
			// TODO
			return true;
		default:
			memcpy((void*) buf, ai->ai_addr, ai->ai_addrlen);
			break;
		}
	}
	buf+= ai->ai_addrlen;
	size-= ai->ai_addrlen == 0 ? 4 : ai->ai_addrlen;

	int canonlen = ai->ai_canonname == nullptr ? 0 : (int) strlen(ai->ai_canonname);
	if(size < canonlen + 1) {
		return true;
	}

	memcpy((void*) buf, ai->ai_canonname, canonlen);
	buf[canonlen] = 0;
	buf+= canonlen + 1;
	size-= canonlen + 1;

	if(ai->ai_next != nullptr) {
		return pack_addrinfo(ai->ai_next, buf, size);
	} else {
		if(size < 4) {
			return true;
		}
		*((uint32_t*) buf) = 0;
		return false;
	}
}

static bool unpack_addrinfo(struct addrinfo **res, const uint8_t *buf, int size) {
	struct {
		uint32_t magic;
		int32_t ai_flags;
		int32_t ai_family;
		int32_t ai_socktype;
		int32_t ai_protocol;
		int32_t ai_addrlen;
	} ai_packed_header;

	static_assert(sizeof(ai_packed_header) == 6*4, "ai_packed_header size is wrong");
	
	struct addrinfo *ai = new struct addrinfo;
	memset(ai, 0, sizeof(*ai));
	
	if(size < sizeof(ai_packed_header)) {
		goto bail;
	}
	memcpy(&ai_packed_header, buf, sizeof(ai_packed_header));
	buf+= sizeof(ai_packed_header);
	size-= sizeof(ai_packed_header);
	
	if(ntohl(ai_packed_header.magic) != 0xBEEFCAFE) {
		goto bail;
	}
	
	ai->ai_flags = ntohl(ai_packed_header.ai_flags);
	ai->ai_family = ntohl(ai_packed_header.ai_family);
	ai->ai_socktype = ntohl(ai_packed_header.ai_socktype);
	ai->ai_protocol = ntohl(ai_packed_header.ai_protocol);
	ai->ai_addrlen = ntohl(ai_packed_header.ai_addrlen);
  
	if(ai->ai_addrlen == 0) {
		ai->ai_addr = nullptr;
		buf+= 4;
		size-= 4;
	} else {
		ai->ai_addr = (struct sockaddr*) new uint8_t[ai->ai_addrlen];
		if(ai->ai_addr == nullptr) {
			goto bail;
		}
		switch(ai->ai_family) {
		case AF_INET: {
			const struct sockaddr_in *buf_as_sockaddr_in = (const struct sockaddr_in*) buf;
			struct sockaddr_in *sockaddr = (struct sockaddr_in*) ai->ai_addr;
			sockaddr->sin_family = buf_as_sockaddr_in->sin_family;
			sockaddr->sin_port = htons(buf_as_sockaddr_in->sin_port); // erroneous byte swapping
			sockaddr->sin_addr.s_addr = htonl(buf_as_sockaddr_in->sin_addr.s_addr); // again
			memset(&sockaddr->sin_zero, 0, sizeof(sockaddr->sin_zero));
			break;
		}
		case AF_INET6:
			//TODO
			goto bail;
		default:
			memcpy(ai->ai_addr, buf, ai->ai_addrlen);
			break;
		}
		buf+= ai->ai_addrlen;
		size-= ai->ai_addrlen;
	}
  
	int canonlen;
	canonlen = (int) strlen((const char*) buf);
	char *canonname;
	if(canonlen > 0) {
		canonname = new char[canonlen+1];
		memcpy(canonname, buf, canonlen+1);
	} else {
		canonname = nullptr;
	}
	ai->ai_canonname = canonname;
  
	buf+= canonlen+1;
	size-= canonlen+1;
  
	if(*((const uint32_t*) buf) == htonl(0xBEEFCAFE)) {
		if(unpack_addrinfo(&ai->ai_next, buf, size)) {
			goto bail;
		}
	}
  
	*res = ai;
	return false;
  
  bail:
	if(ai->ai_addr != nullptr) {
		delete ai->ai_addr;
	}
	if(ai->ai_canonname != nullptr) {
		delete ai->ai_canonname;
	}
	delete ai;
	return true;
}

static void custom_freeaddrinfo(struct addrinfo *ai) {
	if(ai->ai_next != nullptr) {
		custom_freeaddrinfo(ai->ai_next);
	}
	if(ai->ai_addr != nullptr) {
		delete ai->ai_addr;
	}
	if(ai->ai_canonname != nullptr) {
		delete ai->ai_canonname;
	}
	delete ai;
}

uint32_t nn::socket::resolver::IResolver::GetAddrInfo(IN bool enable_nsd_resolve, IN uint32_t _1, IN uint64_t pid_placeholder, IN gpid _3, IN int8_t * host, guint host_size, IN int8_t * service, guint service_size, IN packed_addrinfo * hints, guint hints_size, OUT int32_t& ret, OUT uint32_t& bsd_errno, OUT uint32_t& packed_addrinfo_size, OUT packed_addrinfo * response, guint response_size) {
	if(passthrough) {
		struct addrinfo *hints_unpacked = nullptr;
		if(hints_size > 0 && unpack_addrinfo(&hints_unpacked, hints, (int) hints_size)) {
			ret = -1;
			bsd_errno = 0xFF;
			return 0;
		}

		struct addrinfo *response_ai;
		
		ret = ::getaddrinfo((char*) host, (char*) service, hints_unpacked, &response_ai);
		if(ret != 0) {
			bsd_errno = errno;
			return 0;
		}
		custom_freeaddrinfo(hints_unpacked);

		if(pack_addrinfo(response_ai, response, (int) response_size)) {
			ret = -1;
			bsd_errno = 0xFF;
			return 0;
		}
		packed_addrinfo_size = (int) response_size; // this might trip up the nintendo code
		freeaddrinfo(response_ai);

		ret = 0;
		bsd_errno = 0;
	} else {
		ret = 0;
		bsd_errno = 0;
	}
	return 0;
}
