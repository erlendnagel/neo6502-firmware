#pragma once

#include <unordered_map>
#include <cstdint>
#include <vector>
#include <bitset>
#include <format>

#include "interface/console.h"

struct Gamepad
{
    uint8_t dev_addr;
    uint8_t instance;

    int16_t dpad_x;
    int16_t dpad_y;

    std::bitset<16> buttons;

    uint8_t dpad_offset;
    uint8_t buttons_offset;

    bool dpad_available;
    uint8_t button_count;

    uint8_t dpad_size;
};

class GamepadController
{
    public:
        bool update(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
        {
            for(auto &gamepad : gamepads)
            {
                if(gamepad.dev_addr == dev_addr && gamepad.instance == instance)
                {
                    if(gamepad.button_count > 0)
                    {
                        uint8_t i = gamepad.buttons_offset / 8;
                        if(gamepad.button_count > 8)
                        {
                            uint16_t mask = 0xFFFF >> (16 - gamepad.button_count);
                            gamepad.buttons = (*(uint16_t*)(report + i)) & mask;
                        }
                        else
                        {
                            gamepad.buttons = report[i];
                        }
                    }

                    if(gamepad.dpad_available)
                    {
                        uint8_t i = gamepad.dpad_offset / 8;
                        uint8_t s = gamepad.dpad_offset % 8;
                        gamepad.dpad_x = (report[i] >> s) & 0x3;
                        gamepad.dpad_y = (report[i] >> (s+2)) & 0x3;
                    }

                    CONWriteString(std::format("Gamepad state: Buttons {} Dpad {}.{}\r", gamepad.buttons.to_string(), gamepad.dpad_x, gamepad.dpad_y).c_str());

                }
            }

            return true;
        };


        bool add(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
        {
            Gamepad gamepad;
            gamepad.dev_addr = dev_addr;
            gamepad.instance = instance;

            uint8_t current_offset = 0;
            uint8_t report_count = 0;
            uint8_t report_size = 0;

            enum Controls
            {
                UNKNOWN,
                BUTTONS,
                AXIS
            };

            Controls control = UNKNOWN;

            // check header
            if(desc_len >= 4 
                && desc_report[0] == 0x05
                && desc_report[1] == 0x01
                && desc_report[2] == 0x09
                && desc_report[3] == 0x05
            )
            {
                CONWriteString("Found Game Pad!\r");
                for(int i = 4; i < desc_len; i=i+2)
                {
                    switch (desc_report[i])
                    {
                    case 0xA1:
                        CONWriteString("Collection ");

                        switch (desc_report[i+1])
                        {
                            case 0x01:
                                CONWriteString("(Application)\r");
                                break;

                            default:
                                CONWriteString(std::format("Unknown code {:#X}\r", desc_report[i+1]).c_str());
                                break;
                        }
                        break;

                    case 0x05:
                        CONWriteString("Usage Page ");

                        switch (desc_report[i+1])
                        {
                            case 0x09:
                                CONWriteString("(Button)\r");
                                control = BUTTONS;
                                break;

                            default:
                                CONWriteString(std::format("Unknown code {:#X}\r", desc_report[i+1]).c_str());
                                break;
                        }
                        break;

                    case 0x09:
                        CONWriteString("Usage ");

                        switch (desc_report[i+1])
                        {
                            case 0x30:
                                CONWriteString("(X)\r");
                                control = AXIS;
                                break;

                            case 0x31:
                                CONWriteString("(X)\r");
                                control = AXIS;
                                break;

                            default:
                                CONWriteString(std::format("Unknown code {:#X}\r", desc_report[i+1]).c_str());
                                break;
                        }
                        break;
                    
                    case 0x75:
                        CONWriteString(std::format("Report Size ({})\r", desc_report[i+1]).c_str());
                        report_size = desc_report[i+1];
                        break;

                    case 0x81:
                        CONWriteString("Input \r");
                        
                        switch (control)
                        {
                        case BUTTONS:
                            CONWriteString(std::format("Will add {} buttons.\r", report_count).c_str());
                            gamepad.button_count = report_count;
                            gamepad.buttons_offset = current_offset;
                            break;
                        case AXIS:
                            CONWriteString(std::format("Will add XY axis with count {} size {}.\r", report_count, report_size).c_str());
                            gamepad.dpad_available = true;
                            gamepad.dpad_offset = current_offset;
                            gamepad.dpad_size = report_size;
                            break;
                        default:
                            break;
                        }

                        control = UNKNOWN;
                        current_offset += report_count * report_size;
                        report_count = 0;
                        report_size = 0;
                        break;
                    
                    case 0x95:
                        CONWriteString(std::format("Report Count ({})\r", desc_report[i+1]).c_str());
                        report_count = desc_report[i+1];
                        break;
                    
                    default:
                        CONWriteString(std::format("Unknown code {:#X}\r", desc_report[i]).c_str());
                        break;
                    }
                }

                gamepads.push_back(gamepad);
                CONWriteString(std::format("Successfully added gamepad {}\r", gamepads.size()).c_str());
                return true;
            }
            else
            {
                CONWriteString("No Game Pad, abort!\r");
                return false;
            }
        };

        bool remove(uint8_t dev_addr, uint8_t instance)
        {
            return false;
        };

    private:
        std::vector<Gamepad> gamepads;
};