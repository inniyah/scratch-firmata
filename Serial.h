#ifndef SERIAL_H_D3821E6C_B4DB_11E2_A4B1_001485C48853
#define SERIAL_H_D3821E6C_B4DB_11E2_A4B1_001485C48853

#include <wx/wx.h>
#include <stdint.h>

#if defined(LINUX)
#include <termios.h>
#elif defined(MACOSX)
#include <termios.h>
#elif defined(WINDOWS)
#include <windows.h>
#endif

class Serial
{
public:
	Serial();
	~Serial();
	wxArrayString port_list();
	int Open(const char * p_name);
	wxString error_message();
	int Set_baud(int baud);
	int Set_baud(const wxString& baud_str);
	int Read(void *ptr, int count);
	int Write(const void *ptr, int len);
	int Input_wait(int msec);
	void Input_discard(void);
	int Set_control(int dtr, int rts);
	void Output_flush();
	void Close(void);
	int Is_open(void);
	const char * get_name(void) const;
private:
	int port_is_open;
	wxString port_name;
	int baud_rate;
	wxString error_msg;
private:
#if defined(LINUX) || defined(MACOSX)
	int port_fd;
	struct termios settings_orig;
	struct termios settings;
#elif defined(WINDOWS)
	HANDLE port_handle;
	COMMCONFIG port_cfg_orig;
	COMMCONFIG port_cfg;
#endif
};

#endif // SERIAL_H_D3821E6C_B4DB_11E2_A4B1_001485C48853
