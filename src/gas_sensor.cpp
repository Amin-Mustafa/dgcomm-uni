#include "gas_sensor.h"
#include <Arduino.h>
#include "MQUnifiedsensor.h"
#include <array>

#define RATIO_MQ2 9.83f
#define R0_VALUE  10.0f

namespace Gas {

struct RegressionCoeff {
    float a, b;
};

static constexpr std::array<RegressionCoeff, static_cast<size_t>(Type::COUNT)> gas_lut = {
    RegressionCoeff{ 987.99 , -2.162 }, // H2
    RegressionCoeff{ 574.25 , -2.222 }, // LPG
    RegressionCoeff{ 36974  , -3.109 }, // CO
    RegressionCoeff{ 3616.1 , -2.675 }, // ALCOHOL
    RegressionCoeff{ 658.71 , -2.168 }, // PROPANE
};

Sensor::Sensor(uint16_t pin, Type gas)
    :sensor_dev{std::make_unique<MQUnifiedsensor >("ESP-32", 3.3, 12, pin, "MQ2")} {
        RegressionCoeff coeff = gas_lut[static_cast<size_t>(gas)];

        sensor_dev->setRegressionMethod(1);
        sensor_dev->setA(coeff.a);
        sensor_dev->setB(coeff.b);

        sensor_dev->init();

        sensor_dev->setR0(R0_VALUE / 10);
    }

Sensor::~Sensor()=default;

float Sensor::read() {
    sensor_dev->update();
    return sensor_dev->readSensor();
}

}
