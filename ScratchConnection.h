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

#include "IScratchListener.h"
#include "PerformanceTimer.h"

class ScratchConnection {
public:
	ScratchConnection() : sockfd(-1) {
	}

	static const char * ScratchHost;
	static const unsigned short ScratchPort;
	static const size_t BufferSize;

	bool Connect();
	void Disconnect();

	inline bool isConnected() const { // true if connected
		return (sockfd >= 0);
	}

	inline void ReceiveScratchMessages(IScratchListener & listener) {
		ReceiveRaw(listener);
	}

	inline void SendScratchMessage(size_t size, const char * data) {
		SendRaw(size, data);
	}

	bool SendScratchFormattedMessage(const char * fmt, ...);

private:
	bool SendRaw(size_t size, const char * data);
	bool ReceiveRaw(IScratchListener & listener);
	void ProcessScratchMessage(IScratchListener & listener, size_t size, const char * data);

private:
	int sockfd;
	PerformanceTimer ConnectionTimer;
	static const float ConnectionTimerWait = 5000.0; // in milliseconds
};

#endif // SCRATCH_CONNECTION_H_9ECEBDBE_B504_11E2_81D8_001485C48853
