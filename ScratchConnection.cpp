#include "ScratchConnection.h"

#include <cstdlib>
#include <iostream>

#include <cstdlib>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>

const char * ScratchConnection::ScratchHost = "127.0.0.1";
const unsigned short ScratchConnection::ScratchPort = 42001;
const size_t ScratchConnection::BufferSize = 240;

bool ScratchConnection::Connect() {
	struct sockaddr_in serv_addr;
	struct hostent *server;

	sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		goto error;
	}
	server = ::gethostbyname(ScratchHost);
	if (server == NULL) {
		goto error;
	}
	::bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	::bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(ScratchPort);
	if (::connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		goto error;
	}
	std::cerr << "Established connection with Scratch" << std::endl;
	return true;
error:
	if (sockfd >= 0) ::close(sockfd);
	sockfd = -1;
	std::cerr << "Unable to establish connection with Scratch: " << strerror(errno) << std::endl;
	return false;
}

void ScratchConnection::Disconnect() {
	::close(sockfd);
	sockfd = -1;
	std::cerr << "Lost connection with Scratch" << std::endl;
}

void ScratchConnection::SendRaw(size_t size, const char * data) {
	if (sockfd < 0) if (!Connect()) return;

	char header[5] = {
		'c',
		(char)((size >> 24) & 0xFF),
		(char)((size >> 16) & 0xFF),
		(char)((size >>  8) & 0xFF),
		(char)(size & 0xFF)
	};
	if (::send(sockfd, header, sizeof(header), 0) < 0) {
		std::cerr << "Error " << errno << " while sending the header: "<< strerror(errno) << std::endl;
		Disconnect();
		return;
	}
	std::cerr << "Message of size " << size << " to be sent" << std::endl;
	if (::send(sockfd, data, size, 0) < 0) {
		std::cerr << "Error " << errno << " while sending the data: "<< strerror(errno) << std::endl;
		Disconnect();
		return;
	}
}

void ScratchConnection::ReceiveRaw() {
	if (sockfd < 0) if (!Connect()) return;

	char buffer[BufferSize+1];
	int rtn = ::recv(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT);
	if (rtn < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			std::cerr << "Error " << errno << " while receiving the message: "<< strerror(errno) << std::endl;
			Disconnect();
		}
		return;
	}
	buffer[BufferSize] = '\0';
	std::cerr << "Message received: " << buffer+4 << std::endl;
}

