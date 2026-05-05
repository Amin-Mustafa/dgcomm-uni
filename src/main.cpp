#include <Arduino.h>
#include <DHT.h>
#include "presence.h"
#include "gas_sensor.h"
#include "sensor_fusion.h"

// --- Configuration ---
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_broker = "8fa6532b48c4440fa03edd9ddfd662f6.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "Amin-Mustafa";
const char* mqtt_password = "Silentlibrary322";

// --- Validated Hardware Pins ---
#define GAS_PIN D0       
#define PRESENCE_PIN D1  
#define DHT_PIN D2
#define BUZZER_PIN D3 
#define LED_GRN D8
#define LED_YEL D9
#define LED_RED D10   

#define DHTTYPE DHT22

DHT dht(DHT_PIN, DHTTYPE);
Presence::Detector presence_detector(PRESENCE_PIN);
Gas::Sensor gas_sensor(GAS_PIN, Gas::Type::ALCOHOL);
SensorFusion::StateTracker sensor_fusion;

void setup() {
	Serial.begin(115200);
  
	pinMode(GAS_PIN, INPUT);
	pinMode(PRESENCE_PIN, INPUT);
	pinMode(LED_RED, OUTPUT);
	pinMode(LED_YEL, OUTPUT);
	pinMode(LED_GRN, OUTPUT);
	pinMode(BUZZER_PIN, OUTPUT);

	analogReadResolution(12); 
	
	dht.begin();
}

void loop() {
	gas_sensor.read();
}