#include "gas_sensor.h"
#include <Arduino.h>
#include <Preferences.h>
#include "MQUnifiedsensor.h"
#include <array>

#define RATIO_MQ3 60.0f
#define R0_VALUE  10.0f
#define DEFAULT_R0 10.0f // Fallback if flash memory is empty

namespace Gas {

struct RegressionCoeff {
    float a, b;
};

static constexpr std::array<RegressionCoeff, static_cast<size_t>(Type::COUNT)> gas_lut = {
    RegressionCoeff{ 44771  , -3.245 }, // LPG
    RegressionCoeff{ 2e31f  , 19.01  }, // CH4
    RegressionCoeff{ 521853 , -3.821 }, // CO
    RegressionCoeff{ 0.3934 , -1.504 }, // ALCOHOL
    RegressionCoeff{ 4.8387 , -2.68  }, // BENZENE
    RegressionCoeff{ 7585.3 , -2.849 }  // HEXANE
};

Sensor::Sensor(uint16_t pin, Type gas)
    :sensor_dev{std::make_unique<MQUnifiedsensor >("ESP-32", 3.3, 12, pin, "MQ3")} {
        RegressionCoeff coeff = gas_lut[static_cast<size_t>(gas)];

        sensor_dev->setRegressionMethod(1);
        sensor_dev->setA(coeff.a);
        sensor_dev->setB(coeff.b);

        sensor_dev->init();

        Preferences prefs;
        prefs.begin("lab_safety", true); // true = Read-only mode
        float saved_r0 = prefs.getFloat("R0_VALUE", DEFAULT_R0); 
        prefs.end();

        sensor_dev->setR0(saved_r0);
    }

Sensor::~Sensor()=default;

float Sensor::read() {
    // Prevent reading garbage data if calibration is currently running
    if (calibrating) return 0.0f; 
    
    sensor_dev->update();
    return sensor_dev->readSensor() * 500;  // Convert to PPM
}

void Sensor::calibrate() {
    calibrating = true;
    Serial.println("Starting Gas Sensor Calibration (Ensure clean air!)...");
    
    float calcR0 = 0;
    
    // Take 10 samples over 5 seconds
    for(int i = 1; i <= 10; i++) {
        sensor_dev->update(); 
        calcR0 += sensor_dev->calibrate(RATIO_MQ3);
        Serial.print(".");
        delay(500); // Deliberate delay to get distinct temporal samples
    }
    
    float final_r0 = calcR0 / 10.0f;
    sensor_dev->setR0(final_r0);
    
    // Save the new R0 to flash memory
    Preferences prefs;
    prefs.begin("lab_safety", false); // false = Read/Write mode
    prefs.putFloat("R0_VALUE", final_r0);
    prefs.end();
    
    Serial.print(" Calibration Complete! New R0 saved: ");
    Serial.println(final_r0);
    
    calibrating = false;
}


}   // namespace Gas