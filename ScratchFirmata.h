#ifndef SCRATCH_FIRMATA_H_EA7D067C_B4DB_11E2_B5FA_001485C48853
#define SCRATCH_FIRMATA_H_EA7D067C_B4DB_11E2_B5FA_001485C48853

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "firmata_test.h"
#endif

#include <wx/wx.h>
#include <wx/tglbtn.h>
#include <stdint.h>

#include "IScratchListener.h"

#define LOG_MSG_TO_STDOUT
//#define LOG_MSG_TO_WINDOW

#if defined(LOG_MSG_TO_WINDOW)
#define printf(...) (wxLogMessage(__VA_ARGS__))
#elif defined(LOG_MSG_TO_STDOUT)
#else
#define printf(...)
#endif

// comment this out to enable lots of printing to stdout



const int ID_MENU = 10000;

//----------------------------------------------------------------------------
// ScratchFirmataFrame
//----------------------------------------------------------------------------

class ScratchFirmataFrame: public wxFrame, public IScratchListener {
public:
	ScratchFirmataFrame( wxWindow *parent, wxWindowID id, const wxString &title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_FRAME_STYLE );

private:
	wxFlexGridSizer *grid;
	wxScrolledWindow *scroll;
	int parse_count;
	int parse_command_len;
	uint8_t parse_buf[4096];

private:
	virtual void ReceiveScratchMessage(unsigned int num_params, const char * param[], unsigned int param_size[]);

private:
	void init_data(void);
	void new_size(void);
	void add_item_to_grid(int row, int col, wxWindow *item);
	void add_pin(int pin);
	void UpdateStatus(void);
	void Parse(const uint8_t *buf, int len);
	void DoMessage(void);
	void OnAbout(wxCommandEvent &event);
	void OnQuit(wxCommandEvent &event);
	void OnIdle(wxIdleEvent &event);
	void OnCloseWindow(wxCloseEvent &event);
	void OnSize(wxSizeEvent &event);
	void OnPort(wxCommandEvent &event);
	void OnToggleButton(wxCommandEvent &event);
	void OnSliderDrag(wxScrollEvent &event);
	void OnModeChange(wxCommandEvent &event);
	DECLARE_EVENT_TABLE()
};


class ScratchFirmataMenu: public wxMenu {
public:
	ScratchFirmataMenu(const wxString& title = _(""), long style = 0);
	void OnShowPortList(wxMenuEvent &event);
	void OnHighlight(wxMenuEvent &event);
};

//----------------------------------------------------------------------------
// ScratchFirmataApp
//----------------------------------------------------------------------------

class ScratchFirmataApp: public wxApp {
public:
	ScratchFirmataApp();
	virtual bool OnInit();
	virtual int OnExit();
};

#endif // SCRATCH_FIRMATA_H_EA7D067C_B4DB_11E2_B5FA_001485C48853
