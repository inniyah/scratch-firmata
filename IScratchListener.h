#ifndef ISCRATCHLISTENER_H_B1A316DE_B5A3_11E2_BBBB_001485C48853
#define ISCRATCHLISTENER_H_B1A316DE_B5A3_11E2_BBBB_001485C48853

class IScratchListener {
public:
	virtual void ReceiveScratchMessage(unsigned int num_params, const char * param[], unsigned int param_size[]) = 0;
};

#endif // ISCRATCHLISTENER_H_B1A316DE_B5A3_11E2_BBBB_001485C48853
