/*  Firmata GUI-friendly queries test
 *  Copyright 2010, Paul Stoffregen (paul@pjrc.com)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "firmata_test.h"
#endif

#include "wx/wxprec.h"
#include "ScratchFirmata.h"
#include "Firmata.h"
#include "ScratchConnection.h"
#include "PerformanceTimer.h"

//------------------------------------------------------------------------------
// ScratchFirmataFrame
//------------------------------------------------------------------------------

using namespace Firmata;
Device device;

struct PinXtraInfo {
	PerformanceTimer timer;
	uint32_t value;
	PinXtraInfo() : value(0) { }
	static const float ScratchValueTimerWait = 250.0; // in milliseconds
	bool checkIfMustSend(uint32_t curr_value);
	void reportSend(uint32_t curr_value);
};

bool PinXtraInfo::checkIfMustSend(uint32_t curr_value) {
	if (timer.isStopped() || (value != curr_value && timer.getElapsedMilliseconds() > ScratchValueTimerWait)) {
		timer.start();
		value = curr_value;
		return true;
	}
	return false;
}

void PinXtraInfo::reportSend(uint32_t curr_value) {
	timer.start();
	value = curr_value;
}

PinXtraInfo pin_xinfo[Device::MAX_PINS];

wxMenu *port_menu;

ScratchConnection scratch_conn;

class ScratchFirmataException : public std::exception {
public:
	ScratchFirmataException(const std::string & message) throw() : msg(message) { }
	virtual ~ScratchFirmataException() throw() { }
	const char * what() const throw() { return msg.c_str(); }
private:
	std::string msg;  // Exception message
};

BEGIN_EVENT_TABLE(ScratchFirmataFrame,wxFrame)
	EVT_MENU(wxID_ABOUT, ScratchFirmataFrame::OnAbout)
	EVT_MENU(wxID_EXIT, ScratchFirmataFrame::OnQuit)
	EVT_MENU_RANGE(9000, 9999, ScratchFirmataFrame::OnPort)
	EVT_CHOICE(-1, ScratchFirmataFrame::OnModeChange)
	EVT_IDLE(ScratchFirmataFrame::OnIdle)
	EVT_TOGGLEBUTTON(-1, ScratchFirmataFrame::OnToggleButton)
	EVT_SCROLL_THUMBTRACK(ScratchFirmataFrame::OnSliderDrag)
	EVT_MENU_OPEN(ScratchFirmataMenu::OnShowPortList)
	EVT_MENU_HIGHLIGHT(-1, ScratchFirmataMenu::OnHighlight)
	EVT_CLOSE(ScratchFirmataFrame::OnCloseWindow)
	//EVT_SIZE(ScratchFirmataFrame::OnSize)
END_EVENT_TABLE()

ScratchFirmataFrame::ScratchFirmataFrame( wxWindow *parent, wxWindowID id, const wxString &title,
    const wxPoint &position, const wxSize& size, long style ) :
    wxFrame( parent, id, title, position, size, style )
{
	#ifdef LOG_MSG_TO_WINDOW
	wxLog::SetActiveTarget(new wxLogWindow(this, _("Debug Messages")));
	#endif
	wxMenuBar *menubar = new wxMenuBar;
	wxMenu *menu = new wxMenu;
	menu->Append( wxID_EXIT, _("Quit"), _(""));
	menubar->Append(menu, _("File"));
	menu = new wxMenu;
	menubar->Append(menu, _("Port"));
	SetMenuBar(menubar);
	port_menu = menu;
	CreateStatusBar(1);

	scroll = new wxScrolledWindow(this);
	scroll->SetScrollRate(20, 20);
	grid = new wxFlexGridSizer(0, 3, 4, 4);
	scroll->SetSizer(grid);

	init_data();
#if 0
	// For testing only, add many controls so something
	// appears in the window without requiring any
	// actual communication...
	for (int i=0; i<80; i++) {
		device.pin_info[i].supported_modes = 7;
		add_pin(i);
	}
#endif
}

void ScratchFirmataFrame::init_data(void)
{
	grid->Clear(true);
	grid->SetRows(0);
	device.reset();
	UpdateStatus();
	new_size();
}

void ScratchFirmataFrame::new_size(void)
{
	grid->Layout();
	scroll->FitInside();
	Refresh();
}

void ScratchFirmataFrame::add_item_to_grid(int row, int col, wxWindow *item)
{
	int num_col = grid->GetCols();
	int num_row = grid->GetRows();

	if (num_row < row + 1) {
		printf("adding rows, row=%d, num_row=%d\n", row, num_row);
		grid->SetRows(row + 1);
		while (num_row < row + 1) {
			printf("  add %d static text\n", num_col);
			for (int i=0; i<num_col; i++) {
				grid->Add(new wxStaticText(scroll, -1, _("")));
			}
			num_row++;
		}
	}
	int index = row * num_col + col;
	//printf("index = %d: ", index);
	wxSizerItem *existing = grid->GetItem(index);
	if (existing != NULL) {
		wxWindow *old = existing->GetWindow();
		if (old) {
			grid->Replace(old, item);
			old->Destroy();
			wxSizerItem *newitem = grid->GetItem(item);
			if (newitem) {
				newitem->SetFlag(wxALIGN_CENTER_VERTICAL);
			}
		}
	} else {
		printf("WARNING, using insert\n");
		grid->Insert(index, item);
	}
}

void ScratchFirmataFrame::add_pin(int pin)
{
	wxString *str = new wxString();
	str->Printf(_("Pin %d"), pin);
	wxStaticText *pin_name = new wxStaticText(scroll, -1, *str);
	add_item_to_grid(pin, 0, pin_name);

	wxArrayString list;
	if (device.doesPinSupportMode(pin, PinInfo::MODE_INPUT))  list.Add(_("Input"));
	if (device.doesPinSupportMode(pin, PinInfo::MODE_OUTPUT)) list.Add(_("Output"));
	if (device.doesPinSupportMode(pin, PinInfo::MODE_ANALOG)) list.Add(_("Analog"));
	if (device.doesPinSupportMode(pin, PinInfo::MODE_PWM))    list.Add(_("PWM"));
	if (device.doesPinSupportMode(pin, PinInfo::MODE_SERVO))  list.Add(_("Servo"));
	wxPoint pos = wxPoint(0, 0);
	wxSize size = wxSize(-1, -1);
	wxChoice *modes = new wxChoice(scroll, 8000+pin, pos, size, list);
	if (device.getCurrentPinMode(pin) == PinInfo::MODE_INPUT)  modes->SetStringSelection(_("Input"));
	if (device.getCurrentPinMode(pin) == PinInfo::MODE_OUTPUT) modes->SetStringSelection(_("Output"));
	if (device.getCurrentPinMode(pin) == PinInfo::MODE_ANALOG) modes->SetStringSelection(_("Analog"));
	if (device.getCurrentPinMode(pin) == PinInfo::MODE_PWM)    modes->SetStringSelection(_("PWM"));
	if (device.getCurrentPinMode(pin) == PinInfo::MODE_SERVO)  modes->SetStringSelection(_("Servo"));
	printf("create choice, mode = %d (%s)\n", device.getCurrentPinMode(pin),
		(const char *)modes->GetStringSelection().c_str());
	add_item_to_grid(pin, 1, modes);
	modes->Validate();
	wxCommandEvent cmd = wxCommandEvent(wxEVT_COMMAND_CHOICE_SELECTED, 8000+pin);
	//modes->Command(cmd);
	OnModeChange(cmd);
}

uint32_t ScratchFirmataFrame::set_pin_value(int pin, uint32_t value)
{
	return device.getCurrentPinValue(pin);
}

void ScratchFirmataFrame::UpdateStatus(void)
{
	wxString status;
	if (device.isOpen()) {
		status.Printf(wxString(device.getPortName(), wxConvUTF8) + _("    ") +
			wxString(device.getFirmataName(), wxConvUTF8) + _("    Tx:%u Rx:%u"),
			device.getTxCount(), device.getRxCount());
	} else {
		status = _("Please choose serial port");
	}
	SetStatusText(status);
}

void ScratchFirmataFrame::SendPinConfigurationToScratch(int pin)
{
	int mode = device.getCurrentPinMode(pin);
	if (mode == PinInfo::MODE_OUTPUT) {
		scratch_conn.SendScratchFormattedMessage("sensor-update \"pin%d.cfg\" OUTPUT", pin);
	} else if (mode == PinInfo::MODE_INPUT) {
		scratch_conn.SendScratchFormattedMessage("sensor-update \"pin%d.cfg\" INPUT", pin);
	} else if (mode == PinInfo::MODE_ANALOG) {
		scratch_conn.SendScratchFormattedMessage("sensor-update \"pin%d.cfg\" ANALOG", pin);
	} else if (mode == PinInfo::MODE_SERVO) {
		scratch_conn.SendScratchFormattedMessage("sensor-update \"pin%d.cfg\" SERVO", pin);
	} else if (mode == PinInfo::MODE_PWM) {
		scratch_conn.SendScratchFormattedMessage("sensor-update \"pin%d.cfg\" PWM", pin);
	}
	int value = device.getCurrentPinValue(pin);
	scratch_conn.SendScratchFormattedMessage("sensor-update \"pin%d\" %d", pin, value);
	pin_xinfo[pin].reportSend(value);
}

void ScratchFirmataFrame::SendPinValueToScratch(int pin)
{
	int value = device.getCurrentPinValue(pin);
	if (pin_xinfo[pin].checkIfMustSend(value)) {
		scratch_conn.SendScratchFormattedMessage("sensor-update \"pin%d\" %d", pin, value);
	}
}

void ScratchFirmataFrame::OnModeChange(wxCommandEvent &event)
{
	int id = event.GetId();
	int pin = id - 8000;
	if (pin < 0 || pin > 127) return;
	wxChoice *ch = (wxChoice *)FindWindowById(id, scroll);
	wxString sel = ch->GetStringSelection();
	printf("Mode Change, id = %d, pin=%d, ", id, pin);
	printf("Mode = %s\n", (const char *)sel.c_str());
	int mode = 255;
	if (sel.IsSameAs(_("Input")))  mode = PinInfo::MODE_INPUT;
	if (sel.IsSameAs(_("Output"))) mode = PinInfo::MODE_OUTPUT;
	if (sel.IsSameAs(_("Analog"))) mode = PinInfo::MODE_ANALOG;
	if (sel.IsSameAs(_("PWM")))    mode = PinInfo::MODE_PWM;
	if (sel.IsSameAs(_("Servo")))  mode = PinInfo::MODE_SERVO;
	if (mode != device.getCurrentPinMode(pin)) {
		// send the mode change message
		uint8_t buf[4];
		buf[0] = 0xF4;
		buf[1] = pin;
		buf[2] = mode;
		device.write(buf, 3);
		device.setCurrentPinMode(pin, mode);
	}
	// create the 3rd column control for this mode
	if (mode == PinInfo::MODE_OUTPUT) {
		wxToggleButton *button = new  wxToggleButton(scroll, 7000+pin,
			device.getCurrentPinValue(pin) ? _("High") : _("Low"));
		button->SetValue(device.getCurrentPinValue(pin));
		add_item_to_grid(pin, 2, button);
	} else if (mode == PinInfo::MODE_INPUT) {
		wxStaticText *text = new wxStaticText(scroll, 5000+pin,
			device.getCurrentPinValue(pin) ? _("High") : _("Low"));
		wxSize size = wxSize(128, -1);
		text->SetMinSize(size);
		text->SetWindowStyle(wxALIGN_CENTRE);
		add_item_to_grid(pin, 2, text);
	} else if (mode == PinInfo::MODE_ANALOG) {
		wxString val;
		val.Printf(_("%d"), device.getCurrentPinValue(pin));
		wxStaticText *text = new wxStaticText(scroll, 5000+pin, val);
		wxSize size = wxSize(128, -1);
		text->SetMinSize(size);
		text->SetWindowStyle(wxALIGN_CENTRE);
		add_item_to_grid(pin, 2, text);
	} else if (mode == PinInfo::MODE_PWM || mode == PinInfo::MODE_SERVO) {
		int maxval = (mode == PinInfo::MODE_PWM) ? 255 : 180;
		wxSlider *slider = new wxSlider(scroll, 6000+pin,
		  device.getCurrentPinValue(pin), 0, maxval);
		wxSize size = wxSize(128, -1);
		slider->SetMinSize(size);
		add_item_to_grid(pin, 2, slider);
	}
	SendPinConfigurationToScratch(pin);
	new_size();
}

void ScratchFirmataFrame::OnToggleButton(wxCommandEvent &event)
{
	int id = event.GetId();
	int pin = id - 7000;
	if (pin < 0 || pin > 127) return;
	wxToggleButton *button = (wxToggleButton *)FindWindowById(id, scroll);
	int val = button->GetValue() ? 1 : 0;
	printf("Toggle Button, id = %d, pin=%d, val=%d\n", id, pin, val);
	button->SetLabel(val ? _("High") : _("Low"));
	device.setCurrentPinValue(pin, val);
	int port_num = pin / 8;
	int port_val = 0;
	for (int i=0; i<8; i++) {
		int p = port_num * 8 + i;
		if (device.getCurrentPinMode(p) == PinInfo::MODE_OUTPUT || device.getCurrentPinMode(p) == PinInfo::MODE_INPUT) {
			if (device.getCurrentPinValue(p)) {
				port_val |= (1<<i);
			}
		}
	}
	uint8_t buf[3];
	buf[0] = 0x90 | port_num;
	buf[1] = port_val & 0x7F;
	buf[2] = (port_val >> 7) & 0x7F;
	device.write(buf, 3);
	UpdateStatus();
}

void ScratchFirmataFrame::OnSliderDrag(wxScrollEvent &event)
{
	int id = event.GetId();
	int pin = id - 6000;
	if (pin < 0 || pin > 127) return;
	wxSlider *slider = (wxSlider *)FindWindowById(id, scroll);
	int val = slider->GetValue();
	printf("Slider Drag, id = %d, pin=%d, val=%d\n", id, pin, val);
	if (pin <= 15 && val <= 16383) {
		uint8_t buf[3];
		buf[0] = 0xE0 | pin;
		buf[1] = val & 0x7F;
		buf[2] = (val >> 7) & 0x7F;
		device.write(buf, 3);
	} else {
		uint8_t buf[12];
		int len=4;
		buf[0] = 0xF0;
		buf[1] = 0x6F;
		buf[2] = pin;
		buf[3] = val & 0x7F;
		if (val > 0x00000080) buf[len++] = (val >> 7) & 0x7F;
		if (val > 0x00004000) buf[len++] = (val >> 14) & 0x7F;
		if (val > 0x00200000) buf[len++] = (val >> 21) & 0x7F;
		if (val > 0x10000000) buf[len++] = (val >> 28) & 0x7F;
		buf[len++] = 0xF7;
		device.write(buf, len);
	}
	UpdateStatus();
}

void ScratchFirmataFrame::OnPort(wxCommandEvent &event)
{
	int id = event.GetId();
	wxString name = port_menu->FindItem(id)->GetLabel();

	device.close();
	init_data();
	printf("OnPort, id = %d, name = %s\n", id, (const char *)name.c_str());
	if (id == 9000) return;

	UpdateStatus();
	if (device.open(name.mb_str())) {
		wxWakeUpIdle();
	} else {
		printf("error opening port\n");
	}
	UpdateStatus();
}

void ScratchFirmataFrame::OnIdle(wxIdleEvent &event)
{
	bool request_more = false;
	//printf("Idle event\n");

	scratch_conn.ReceiveScratchMessages(*this);
	request_more = device.ReadFromDevice(*this);

	if (request_more)
		event.RequestMore(true);
}

void ScratchFirmataFrame::FirmataStatusChanged() { // IFirmataListener
	UpdateStatus();
}

void ScratchFirmataFrame::PinValueChanged(unsigned int pin_num) { // IFirmataListener
	if (device.getCurrentPinMode(pin_num) == PinInfo::MODE_ANALOG) {
		wxStaticText *text = (wxStaticText *)
		FindWindowById(5000 + pin_num, scroll);
		if (text) {
			wxString val;
			val.Printf(_("A%d: %d"),
				device.getCurrentPinAnalogChannel(pin_num),
				device.getCurrentPinValue(pin_num)
			);
			text->SetLabel(val);
		}
		SendPinValueToScratch(pin_num);
	}

	else if (device.getCurrentPinMode(pin_num) == PinInfo::MODE_INPUT) {
		wxStaticText *text = (wxStaticText *)
		FindWindowById(5000 + pin_num, scroll);
		if (text) {
			text->SetLabel(device.getCurrentPinValue(pin_num) ? _("High") : _("Low"));
		}
		SendPinValueToScratch(pin_num);
	}
}

void ScratchFirmataFrame::PinFound(unsigned int pin_num) { // IFirmataListener
	add_pin(pin_num);
}

void ScratchFirmataFrame::OnAbout( wxCommandEvent &event ) {
	wxMessageDialog dialog( this, _("Scratch Firmata Connector, by Miriam Ruiz\n based on Firmata Test 1.0, by Paul Stoffregen"),
		wxT("Scratch Firmata Connector"), wxOK|wxICON_INFORMATION );
	dialog.ShowModal();
}

void ScratchFirmataFrame::OnQuit( wxCommandEvent &event ) {
	Close( true );
}

void ScratchFirmataFrame::OnCloseWindow( wxCloseEvent &event ) {
	// if ! saved changes -> return
	Destroy();
}

void ScratchFirmataFrame::OnSize( wxSizeEvent &event ) {
	event.Skip( true );
}

static int str_to_int(const char str[], size_t length) {
	int res = 0;
	size_t i = 0;
	bool is_negative = false;

	while (i < length) {
		if (str[i] == ' ' || str[i] == '\t') { ++i; continue; } // Remove trailing whitespaces
		if (str[i] == '-') { ++i; is_negative = true; } // Negative number
		if (str[i] == '+') { ++i; is_negative = false; } // Positive number
		break;
	}

	int j = length - 1;
	while (j > 0) {
		if (str[j] == ' ' || str[j] == '\t') { --length; --j; } // Remove ending whitespaces
		else break;
	}

	while (i < length) {
		if (str[i] <= '9' && str[i] >= '0') {
			int temp_res;
			temp_res = res * 10 + str[i] - '0';
			if ((temp_res + '0' - str[i]) / 10 != res) { // Overflow
				throw ScratchFirmataException("Integer overflow");
				return 0;
			} else {
				res = temp_res;
			}
		} else { // Invalid input
			throw ScratchFirmataException("Character not allowed");
			return 0;
		}
		++i;
	}

	if (is_negative) return -1 * res;
	else return res;
}

// IScratchListener
void ScratchFirmataFrame::ReceiveScratchMessage(unsigned int num_params, const char * param[], unsigned int param_size[]) {
	//for (unsigned int i = 0; i < num_params; i++) {
	//	std::cerr << "  Parameter " << i << ": ";
	//	std::cerr.write(param[i], param_size[i]);
	//	std::cerr << std::endl;
	//}

	if (strncasecmp(param[0], "sensor-update", param_size[0]) == 0) {
		int value = 0;
		if (num_params >= 3) {
			try{
				if (strncasecmp(param[2], "on", 2) == 0 || strncasecmp(param[2], "high", 4) == 0) {
					value = 1;
				} else if (strncasecmp(param[2], "off", 3) == 0 || strncasecmp(param[2], "low", 3) == 0) {
					value = 0;
				} else {
					value = str_to_int(param[2], param_size[2]);
				}

				if (strncasecmp(param[1], "pin", 3) == 0 && param_size[1] > 3) {
					int pin = str_to_int(param[1] + 3, param_size[1] - 3);
					if (device.isPinOutput(pin)) {
						std::cerr << "Setting pin " << pin << " to " << value << std::endl;
					} else {
						std::cerr << "Unable to set pin " << pin << " to " << value << std::endl;
					}
				}
			} catch (ScratchFirmataException & caught) {
				std::cerr << "Got exception: " << caught.what() << std::endl;
			}
		}
	} else if (strncasecmp(param[0], "broadcast", param_size[0]) == 0) {
	} else { // Unknown message
	}
}

//------------------------------------------------------------------------------
// Port Menu
//------------------------------------------------------------------------------

ScratchFirmataMenu::ScratchFirmataMenu(const wxString& title, long style) : wxMenu(title, style)
{
}

void ScratchFirmataMenu::OnShowPortList(wxMenuEvent &event)
{
	wxMenu *menu;
	wxMenuItem *item;
	int num, any=0;

	menu = event.GetMenu();
	printf("OnShowPortList, %s\n", (const char *)menu->GetTitle().c_str());
	if (menu != port_menu) return;

	wxMenuItemList old_items = menu->GetMenuItems();
	num = old_items.GetCount();
	for (int i = old_items.GetCount() - 1; i >= 0; i--) {
		menu->Delete(old_items[i]);
	}
	menu->AppendRadioItem(9000, _(" (none)"));
	wxArrayString list = device.port.port_list();
	num = list.GetCount();
	for (int i=0; i < num; i++) {
		//printf("%d: port %s\n", i, (const char *)list[i]);
		item = menu->AppendRadioItem(9001 + i, list[i]);
		if (device.isOpen() && wxString(device.getPortName(), wxConvUTF8).IsSameAs(list[i])) {
			menu->Check(9001 + i, true);
			any = 1;
		}
	}
	if (!any) menu->Check(9000, true);
}

void ScratchFirmataMenu::OnHighlight(wxMenuEvent &event)
{
}

//------------------------------------------------------------------------------
// ScratchFirmataApp
//------------------------------------------------------------------------------

IMPLEMENT_APP(ScratchFirmataApp)

ScratchFirmataApp::ScratchFirmataApp()
{
}

bool ScratchFirmataApp::OnInit()
{
    ScratchFirmataFrame *frame = new ScratchFirmataFrame( NULL, -1, _("Scratch Firmata Connector"), wxPoint(20,20), wxSize(400,640) );
    frame->Show( true );

    return true;
}

int ScratchFirmataApp::OnExit()
{
    return 0;
}

