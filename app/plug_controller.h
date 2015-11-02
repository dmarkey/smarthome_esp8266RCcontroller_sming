/* 
 * File:   plug_controller.h
 * Author: dmarkey
 *
 * Created on 30 October 2015, 15:34
 */

#ifndef PLUG_CONTROLLER_H
#define	PLUG_CONTROLLER_H
#include <SmingCore/SmingCore.h>

extern "C"{
    #include "include/driver/hw_timer.h"
}


class PlugController {
public:
    PlugController();
    static void ticker();
    void activate_plug(unsigned char plug_number, boolean value);
private:
    void add_write_with_delay(int delay, bool value);
    void send_value(boolean value, unsigned int base_delay);
    void long_sync();
    void send_sync();

    bool in_use = false;
    bool pin_levels[1000];
    long int timings[1000];
    int level_index = 0;
    int sent_index = 0;

    int sequence_number = 0;
    long int last_time = 0;
    long int time_to_next_event = 0;
    Timer taskTimer;
    int rf_pin = 3;
    int max_code_cycles = 4;

    int short_delay = 380;
    int normal_delay = 500;
    int signal_delay = 1500;

    const int sync_delay = 2650;
    const int extrashort_delay = 3000;
    const int extra_delay = 10000;
    //int current_plug = 0;
    unsigned char swap = 0;

    long time = 0;

    unsigned long plugs[5][2][4] = {
        { /*A*/
            { 0b101111000001000101011100, 0b101100010110110110101100,
                0b101110101110010001101100, 0b101101000101010100011100},
            {
                0b101101010010011101111100, 0b101111100011110000101100,
                0b101111110111001110001100, 0b101110111000101110111100
            }
        },
        { /*B*/
            { 0b101101110100001000110101, 0b101101101010100111100101,
                0b101110011101111000000101, 0b101100100000100011110101},
            {
                0b101111011001101011010101, 0b101100111011111101000101,
                0b101110001100011010010101, 0b101100001111000011000101
            }
        },
        { /*C*/
            { 0b101101010010011101111110, 0b101111100011110000101110,
                0b101111110111001110001110, 0b101110111000101110111110},
            {
                0b101110101110010001101110, 0b101101000101010100011110,
                0b101111000001000101011110, 0b101100010110110110101110
            }
        },
        { /*D*/
            { 0b101111011001101011010111, 0b101100111011111101000111,
                0b101100001111000011000111, 0b101110001100011010010111},
            {
                0b101101110100001000110111, 0b101100100000100011110111,
                0b101101101010100111100111, 0b101110011101111000000111
            }
        },
        { /*MASTER*/
            { 0b101111100011110000100010, 0b101110111000101110110010,
                0b101101010010011101110010, 0b101111110111001110000010},
            {
                0b101111000001000101010010, 0b101101000101010100010010,
                0b101110101110010001100010, 0b101100010110110110100010
            }
        }
    };
};

#endif	/* PLUG_CONTROLLER_H */

