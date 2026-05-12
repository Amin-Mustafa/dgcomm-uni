#ifndef GAS_SENSOR_H
#define GAS_SENSOR_H

#include <cstdint>
#include <memory>

class MQUnifiedsensor ;

namespace Gas {

enum class Type {LPG, CH4, CO, ALCOHOL, BENZENE, HEXANE, COUNT};

constexpr float ALCOHOL_HIGH_TRIGGER = 50.0f; 
constexpr float ALCOHOL_LOW_TRIGGER = 40.0f;

class Sensor {
public:
    Sensor(uint16_t pin, Type gas);
    ~Sensor();
    float read();
    void calibrate();
    bool is_calibrating() const { return calibrating; }

private:
    std::unique_ptr<MQUnifiedsensor> sensor_dev;
    bool calibrating = false;
};

inline bool is_high(float current_ppm) {
    static bool alarm_active = false; 

    if (!alarm_active && current_ppm > ALCOHOL_HIGH_TRIGGER) {
        alarm_active = true;
    } 
    else if (alarm_active && current_ppm < ALCOHOL_LOW_TRIGGER) {
        alarm_active = false;
    }
    return alarm_active;
}

}

#endif