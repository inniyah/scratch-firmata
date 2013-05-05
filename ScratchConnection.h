#ifndef SCRATCH_CONNECTION_H_9ECEBDBE_B504_11E2_81D8_001485C48853
#define SCRATCH_CONNECTION_H_9ECEBDBE_B504_11E2_81D8_001485C48853

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

class ScratchConnection {
public:
	ScratchConnection() : sockfd(-1) {
	}

	static const unsigned short ScratchPort = 42001;

	bool Connect();
	void Disconnect();
	void SendRaw(size_t size, const char * data);
	void ReceiveRaw();

private:
	int sockfd;
};

#endif // SCRATCH_CONNECTION_H_9ECEBDBE_B504_11E2_81D8_001485C48853
