#ifndef SENSOR_FUSION_H
#define SENSOR_FUSION_H

#include <cstdint>

namespace SensorFusion {

enum class Sensor : uint8_t {
    TEMPERATURE = 0, GAS = 1, PRESENCE = 2
};

enum class State : uint8_t {
    LOW_POWER = 0, ENV_WARNING, ALERT_VACANT, CRITICAL_VACANT,
    MONITORING, SAFETY_WARNING, SAFETY_DANGER, SAFETY_CRITICAL
};

class StateTracker{
private:
    uint8_t sense_state;
public:
    StateTracker() :sense_state{0} {}

    StateTracker& operator=(const StateTracker& other) {
        sense_state = other.sense_state;
        return *this; // Added missing return
    }

    State sensor_state() const {return static_cast<State>(sense_state);}
    void set_sensor(Sensor sensor, bool value) {
        uint8_t sensor_num = static_cast<uint8_t>(sensor);
        sense_state = (sense_state & ~(1UL << sensor_num)) | (value << sensor_num);
    }
};
}

#endif