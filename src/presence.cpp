#include "presence.h"
#include <Arduino.h>

namespace Presence {

void Detector::update() {
    int pin_value = digitalRead(pin);   //read pin value

    switch(state) {
        case State::VACANT:
            if(pin_value == HIGH) {
                check_timer.set_period(MIN_OCCUPIED_TIME);
                check_timer.start();
                state = State::CHECKING_OCCUPIED;
            }
            break;
        case State::CHECKING_OCCUPIED:
            if(pin_value == LOW) {
                state = State::VACANT;
            } else {
                if(check_timer.elapsed()) {
                    state = State::OCCUPIED;
                }
            }
            break;
        case State::OCCUPIED:
            if(pin_value == LOW) {
                check_timer.set_period(MIN_VACANT_TIME);
                check_timer.start();
                state = State::CHECKING_VACANT;
            }
            break;
        case State::CHECKING_VACANT:
            if(pin == HIGH) {
                state = State::OCCUPIED;
            } else {
                if(check_timer.elapsed()) {
                    state = State::VACANT;
                }
            }
            break;
        default: break;
    }
}

void Timer::start() {
    start_time = millis();
}

bool Timer::elapsed() const {
    return (millis() - start_time) >= period;
}

} //Presence