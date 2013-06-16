#include "ScratchConnection.h"

#include <cstdlib>
#include <iostream>
#include <cstring>
#include <cstdarg>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <strings.h>

const char * ScratchConnection::ScratchHost = "127.0.0.1";
const unsigned short ScratchConnection::ScratchPort = 42001;
const size_t ScratchConnection::BufferSize = 240;

bool ScratchConnection::Connect() {
	struct sockaddr_in serv_addr;
	struct hostent *server;

	if (ConnectionTimer.isStopped() || ConnectionTimer.getElapsedMilliseconds() > ConnectionTimerWait) {
		ConnectionTimer.start();
	} else {
		return false;
	}

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

	char header[4] = {
		(char)((size >> 24) & 0xFF),
		(char)((size >> 16) & 0xFF),
		(char)((size >>  8) & 0xFF),
		(char)((size >>  0) & 0xFF)
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

void ScratchConnection::ReceiveRaw(IScratchListener & listener) {
	if (sockfd < 0) if (!Connect()) return;

	char buffer[BufferSize+1]; buffer[BufferSize] = '\0';
	int bytes_read = ::recv(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT);
	if (bytes_read < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { // Error
			std::cerr << "Error " << errno << " while receiving message from Scratch: "<< strerror(errno) << std::endl;
			Disconnect();
			return;
		} else { // No data available at the moment
			return;
		}
	} else if (bytes_read == 0) { // EOF
		Disconnect();
		return;
	}

	std::cerr << "Data read from Scratch " << bytes_read << " bytes" << std::endl;
	int bytes_left = bytes_read;
	const char * buffer_pos = buffer;
	while (bytes_left > 4) {
		size_t size =
			((size_t)(buffer_pos[0]) << 24) +
			((size_t)(buffer_pos[1]) << 16) +
			((size_t)(buffer_pos[2]) << 8) +
			((size_t)(buffer_pos[3]) << 0);
		buffer_pos += 4; bytes_left -= 4;
		if (size > (size_t)bytes_left) return;
		ProcessScratchMessage(listener, (size_t)size, buffer_pos);
		buffer_pos += size; bytes_left -= size;
	}
}

void ScratchConnection::ProcessScratchMessage(IScratchListener & listener, size_t size, const char * data) {
	std::cerr << "Message of length " << size << " received: ";
	std::cerr.write(data, size);
	std::cerr << std::endl;

	static const unsigned int MaxParams = 4;

	const char * param[MaxParams];
	unsigned int param_size[MaxParams];
	unsigned int param_num = 0;

	bool in_param = false;
	bool in_quotes = false;
	const char * data_end = data + size;
	for (const char * pnt = data; pnt < data_end; pnt++) {
		if (param_num >= MaxParams) break; // Prevent a buffer overflow

		if (in_param) { // Inside a parameter
			if (in_quotes) { // In quotes
				if (*pnt == '"') {
					if (pnt + 1 == data_end || *(pnt+1) != '"') { // Double quotes are used to represent quotes inside a quoted parameter
						param_size[param_num] = pnt - param[param_num]; // Parameter is finished
						param_num++;
						in_param = false;
					}
				}
			} else { // Not in quotes
				if (*pnt == ' ') {
					param_size[param_num] = pnt - param[param_num]; // Parameter is finished
					param_num++;
					in_param = false;
				}
			}
		} else { // Not inside a parameter
			if (*pnt == ' ') continue;
			if (*pnt == '"') { in_quotes = true; pnt++; } // Skip the quotes
			in_param = true;
			param[param_num] = pnt;
		}
	}
	if (in_param) {
		param_size[param_num] = data_end - param[param_num]; // Parameter is finished
		param_num++;
		in_param = false;
	}

	listener.ReceiveScratchMessage(param_num, param, param_size);
}

bool ScratchConnection::SendScratchFormattedMessage(const char * fmt, ...) {
	int n;
	int size = 100; // Initial guess: 100 bytes
	char *p, *np;
	va_list ap;

	if ((p = (char*)malloc(size)) == NULL) return false;

	while (true) {
		// Try to print in the allocated space
		va_start(ap, fmt);
		n = vsnprintf(p, size, fmt, ap);
		va_end(ap);

		// If that worked, send the string
		if (n > -1 && n < size) {
			SendRaw(n, p);
			printf("Sending so Scratch: '%s' (%d bytes)\n", p, n);
			return true;
		}

		// Else try again with more space

		if (n > -1) size = n+1; // glibc 2.1: precisely what is needed
		else size *= 2; // glibc 2.0: twice the old size

		if ((np = (char*)realloc (p, size)) == NULL) {
			free(p);
			return false;
		} else {
			p = np;
		}
	}
}
