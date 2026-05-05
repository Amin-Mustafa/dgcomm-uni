#ifndef PRESENCE_H
#define PRESENCE_H

#include <cstdint>

namespace Presence {

class Timer {
private:
    uint32_t start_time;
    uint32_t period;

public: 
    Timer(uint32_t period)
        :start_time{0}, period{period} {}

    bool elapsed() const;
    void start();
    void set_period(uint32_t new_period) {
        period = new_period;
    }
};

class Detector {
private:
    enum class State {
        VACANT, CHECKING_VACANT, OCCUPIED, CHECKING_OCCUPIED
    };

    static constexpr uint32_t MIN_VACANT_TIME = 10000;  //10 seconds, to be practical
    static constexpr uint32_t MIN_OCCUPIED_TIME  = 500;    //500ms debounce time
public:
    Detector(uint16_t gpio_pin)
        :pin{gpio_pin},
        check_timer{MIN_OCCUPIED_TIME}, 
        state{State::VACANT} {}

    bool occupied() const {
        return state != State::VACANT;
    };

    void update();

private:
    uint16_t pin;
    Timer check_timer;
    State state;
};

}   //Presence

#endif