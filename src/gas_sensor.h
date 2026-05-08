#ifndef GAS_SENSOR_H
#define GAS_SENSOR_H

#include <cstdint>
#include <memory>

class MQUnifiedsensor ;

namespace Gas {

enum class Type {H2, LPG, CO, ALCOHOL, PROPANE, COUNT};

constexpr float ALCOHOL_THRESHOLD = 150.0f;

class Sensor {
public:
    Sensor(uint16_t pin, Type gas);
    ~Sensor();
    float read();

private:
    std::unique_ptr<MQUnifiedsensor > sensor_dev;
};

constexpr bool gas_is_high(const float reading) {
    return reading > ALCOHOL_THRESHOLD;
}

}

#endif