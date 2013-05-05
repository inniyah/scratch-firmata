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

bool ScratchConnection::Connect() {
	struct sockaddr_in serv_addr;
	struct hostent *server;

	sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		goto error;
	}
	server = ::gethostbyname("127.0.0.1");
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
	std::cerr << "Unable to establish connection with Scratch" << std::endl;
	return false;
}

void ScratchConnection::Disconnect() {
	::close(sockfd);
	sockfd = -1;
	std::cerr << "Lost connection with Scratch" << std::endl;

}

void ScratchConnection::SendRaw(size_t size, const char * data) {
	if (sockfd < 0) if (!Connect()) return;

	char header[4] = {
		(char)((size >> 24) & 0xFF),
		(char)((size >> 16) & 0xFF),
		(char)((size >>  8) & 0xFF),
		(char)(size & 0xFF)
	};
	if (::send(sockfd, header, sizeof(header), 0) < 0) {
		Disconnect();
		return;
	}
	if (::send(sockfd, data, size, 0) < 0) {
		Disconnect();
		return;
	}
}

void ScratchConnection::ReceiveRaw() {
	if (sockfd < 0) if (!Connect()) return;

	char header[4];
	int rtn = ::recv(sockfd, header, sizeof(header), MSG_DONTWAIT);
	if (rtn < 0) {
		if (rtn != EAGAIN && rtn != EWOULDBLOCK) {
					Disconnect();
		}
		return;
	}
	size_t size =
		(((size_t)header[0]) << 24) +
		(((size_t)header[0]) << 16) +
		(((size_t)header[0]) << 8) +
		((size_t)header[0]);
	char buffer[size+1];
	if (::recv(sockfd, buffer, size, 0) < 0) {
		Disconnect();
		return;
	}
	buffer[size] = '\0';
	std::cerr << buffer << std::endl;
}

