/* 
 * File:   plug_controller.cpp
 * Author: dmarkey
 * 
 * Created on 30 October 2015, 15:34
 */

#include "plug_controller.h"
/*#include <SmingCore/SmingCore.h>

extern "C" {
    #include"include/driver/hw_timer.h"
}*/

PlugController * current_pc;

PlugController::PlugController() {}


void PlugController::ticker() {

    Serial.println("TICK!");
    
    if (current_pc->sent_index == 0){
        
        hw_timer_arm(current_pc->timings[current_pc->sent_index]);
        current_pc->sent_index++;
    }
    
    bool value = current_pc->pin_levels[current_pc->sent_index-1];
    digitalWrite(current_pc->rf_pin, value);

    hw_timer_arm(current_pc->timings[current_pc->sent_index]);
    
   

    if (current_pc->sent_index == current_pc->level_index + 1) {
        Serial.println("Done sending signals");
        hw_timer_arm(false);
        return;
    }
    current_pc->sent_index++;
    
            
    //hw_timer_set_func(&PlugController::ticker);

    //taskTimer.initializeUs(delay, TimerDelegate(&PlugController::ticker, this)).startOnce();

    ;


}

void PlugController::activate_plug(unsigned char plug_number, boolean value) {

    in_use = true;
    

    pinMode(this->rf_pin, OUTPUT);
    //digitalWrite(RF_DATA_PIN, LOW);

    this->add_write_with_delay(1000, LOW);

    unsigned long signal = this->plugs[plug_number][value][swap];

    //this->add_write_with_delay(1000000, LOW);

    for (unsigned char i = 0; i < 4; i++) { // repeat 1st signal sequence 4 times
        send_sync();
        for (unsigned char k = 0; k < 24; k++) { //as 24 long and short signals, this loop sends each one and if it is long, it takes it away from total delay so that there is a short between it and the next signal and viceversa
            send_value(bitRead(signal, 23 - k), short_delay);
        }
    }
    for (unsigned char i = 0; i < 4; i++) { // repeat 2nd signal sequence 4 times with NORMAL DELAY
        long_sync();
        for (unsigned char k = 0; k < 24; k++) {
            send_value(bitRead(signal, 23 - k), normal_delay);
        }
    }

    //taskTimer.repeating = false;
    current_pc = this;
    hw_timer_set_func(PlugController::ticker);
    hw_timer_init(NMI_SOURCE, 0);
    

    ticker();

}


void PlugController::send_sync() {

    /*digitalWrite(RF_DATA_PIN, HIGH);
     delayMicroseconds(SHORT_DELAY);*/
    this->add_write_with_delay(short_delay, HIGH);

    //digitalWrite(RF_DATA_PIN, LOW);
    this->add_write_with_delay(sync_delay - short_delay, LOW);
    //delayMicroseconds(SYNC_DELAY - SHORT_DELAY);
}

void PlugController::long_sync() {
    this->add_write_with_delay(extrashort_delay, HIGH);
    //digitalWrite(RF_DATA_PIN, HIGH);
    //delayMicroseconds(EXTRASHORT_DELAY);
    //digitalWrite(RF_DATA_PIN, LOW);
    this->add_write_with_delay(extra_delay - extrashort_delay, LOW);
    //delayMicroseconds(EXTRA_DELAY - EXTRASHORT_DELAY);
}

void PlugController::send_value(boolean value, unsigned int base_delay) {
    unsigned long d = value ? signal_delay - base_delay : base_delay;
    add_write_with_delay(d, HIGH);
    //digitalWrite(RF_DATA_PIN, HIGH);
    //delayMicroseconds(d);
    add_write_with_delay(signal_delay - d, LOW);
    //digitalWrite(RF_DATA_PIN, LOW);
    //delayMicroseconds(SIGNAL_DELAY - d);
}

void PlugController::add_write_with_delay(int delay, bool value) {

    timings[level_index] = delay;
    pin_levels[level_index] = value;
    level_index++;

}


