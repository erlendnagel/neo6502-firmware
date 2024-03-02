// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      usbdriver.cpp
//      Authors :   Tsvetan Usunov (Olimex)
//                  Paul Robson (paul@robsons.org.uk)
//      Date :      20th November 2023
//      Reviewed :  No
//      Purpose :   USB interface and HID->Event mapper.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "tusb.h"
#include "interface/kbdcodes.h"

#include "gamepad_controller.h"

#include "interface/console.h"
#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <utility>
#include <unordered_map>
#include <bitset>
#include <ranges>

void binary_to_string(const unsigned char* source, unsigned int length, std::string& destination)
{
    destination.clear();
    for(unsigned int i = 0; i < length; i++)
    {
        char digit[3];
        sprintf(digit, "%02x", source[i]);
        destination.append(digit);
    }
}

// ***************************************************************************************
//
//                          Process USB HID Keyboard Report
//
//                  This converts it to a series of up/down key events
//
// ***************************************************************************************

static short lastReport[KBD_MAX_KEYCODE] = { 0 };                               // state at last HID report.

static void usbProcessReport(uint8_t const *report) {
	for (int i = 0;i < KBD_MAX_KEYCODE;i++) lastReport[i] = -lastReport[i];     // So if -ve was present last time.
	for (int i = 2;i < 8;i++) {                                                 // Scan the key press array.        
		uint8_t key = report[i];
		if (key >= KEY_KP1 && key < KEY_KP1+10) {                               // Numeric keypad numbers will work.
			key = key - KEY_KP1 + KEY_1;
		}
		// if (key == KEY_102ND) key = KEY_BACKSLASH; 															// Non US /| mapped.

		if (key != 0 && key < KBD_MAX_KEYCODE) {                                // If key is down, and not too high.
			if (lastReport[key] == 0) KBDEvent(1,key,report[0]);                // It wasn't down before so key press.
			lastReport[key] = 1;                                                // Flag it as now being down.
		}
	} 
	for (int i = 0;i < KBD_MAX_KEYCODE;i++) {                              		// Any remaining -ve keys are up actions.
		if (lastReport[i] < 0) {
			KBDEvent(0,i,0);                                                    // Flag going up.
			lastReport[i] = 0;                                                  // Mark as now up
		}
	}
}

// ***************************************************************************************
//
//                              USB Callback functions
//
// ***************************************************************************************

GamepadController gamepad_controller;

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
	CONWriteString("Added USB device:");
	CONWriteHex(dev_addr);
	CONWriteHex(instance);
	switch(tuh_hid_interface_protocol(dev_addr, instance)) {

		case HID_ITF_PROTOCOL_KEYBOARD:
			CONWriteString(" Keyboard");
			tuh_hid_receive_report(dev_addr, instance);
		break;

		case HID_ITF_PROTOCOL_MOUSE:
			CONWriteString(" Mouse");
			tuh_hid_receive_report(dev_addr, instance);
		break;

		case HID_ITF_PROTOCOL_NONE:
			CONWriteString("Gamepad? Decriptor:");
			std::string desc;
			binary_to_string(desc_report, desc_len, desc);
			CONWriteString(desc.c_str());
			CONWriteString("\r");
			gamepad_controller.add(dev_addr, instance, desc_report, desc_len);
			tuh_hid_receive_report(dev_addr, instance);
		break;
	}
	uint16_t vid, pid;
  	tuh_vid_pid_get(dev_addr, &vid, &pid);
	CONWriteHex(vid);
	CONWriteHex(pid);
	CONWrite('\r');
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
	CONWriteString("Removed USB device:");
	CONWriteHex(dev_addr);
	CONWriteHex(instance);
	CONWrite('\r');
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {

  switch(tuh_hid_interface_protocol(dev_addr, instance)) {
	case HID_ITF_PROTOCOL_KEYBOARD:  
	  tuh_hid_receive_report(dev_addr, instance);
	  usbProcessReport(report);
	  tuh_hid_receive_report(dev_addr, instance);     
	break;
	
	case HID_ITF_PROTOCOL_MOUSE:
	  tuh_hid_receive_report(dev_addr, instance);
	  CONWriteHex(dev_addr);
	  CONWriteHex(instance);
	  for(int i = 0; i < len; i++)
	  {
		CONWriteHex(report[i]);
	  }
	  CONWrite('\r');
	  tuh_hid_receive_report(dev_addr, instance);
	break;

	case HID_ITF_PROTOCOL_NONE:
	{
		tuh_hid_receive_report(dev_addr, instance);
		//CONWriteHex(dev_addr);
		//CONWriteHex(instance);
		//CONWrite(':');

		// struct gamepad_state
		// {
		// 	std::vector<uint8_t> default_state;
		// 	std::vector<uint8_t> state;
		// 	std::unordered_map<uint16_t, uint16_t> mapping;
		// };

		// static gamepad_state state;

		// if(state.default_state.empty())
		// {
		// 	state.default_state = std::vector<uint8_t>(report, report + len);
		// 	CONWriteString("Found new gamepad, default state saved\r");
		// 	CONWriteString("Needs button mapping!\r");
		// }

		// auto getFirstDiff = [](uint8_t const* report, uint16_t len, const std::vector<uint8_t> &default_state) -> std::optional<uint16_t>
		// {
			//auto current_string = std::bitset<16>(*(uint16_t*)report).to_string();
			// auto default_string = std::bitset<16>(*(uint16_t*)default_state.data()).to_string();
			// std::string diff_string;

			// for (const auto [c,d] : std::views::zip(current_string, default_string))
			// {
			// 	diff_string += c != d ? c : ' ';
			// }

			// CONWriteString(("DEFAULT: " + default_string + "\r").c_str());
			//CONWriteString(("CURRENT: " + current_string + "\r").c_str());
			// CONWriteString(("DIFF   : " + diff_string + "\r").c_str());


		// 	for(uint16_t i = 0; i < len; i++)
		// 	{
		// 		for(uint8_t b=0; b<8; b++)
		// 		{
		// 			bool current_bit = (report[i] >> b & 0x1);
		// 			bool default_bit = (default_state[i] >> b & 0x1);
		// 			if(current_bit != default_bit)
		// 			{
		// 				uint16_t mapping = (i << 8) | b;
		// 				std::string output = "Byte " + std::to_string(i) + " bit " + std::to_string(b) + ":" + std::to_string(current_bit) + "\r";
		// 				CONWriteString(output.c_str());
		// 				return mapping;
		// 			}
		// 		}
		// 	}

		// 	return std::nullopt;
		// };



		// CONWrite('\r');
		//sleep_ms(10000);
		gamepad_controller.update(dev_addr, instance, report, len);
		tuh_hid_receive_report(dev_addr, instance);
	}
	break;
	default:
		CONWriteString("Something default.\r");
	break;
  }
}

// ***************************************************************************************
//
//                               Keyboard initialisation
//
// ***************************************************************************************


void KBDInitialise(void) {
	for (int i = 0;i < KBD_MAX_KEYCODE;i++) lastReport[i] = 0;                  // No keys currently known
	tusb_init();
}

// ***************************************************************************************
//
//                                  Keyboard polling
//
// ***************************************************************************************

void __time_critical_func(KBDSync)(void) {
	tuh_task();
	KBDCheckTimer();
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
