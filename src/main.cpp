#include <Arduino.h>
#include <DHT20.h>
#include "presence.h"
#include "gas_sensor.h"
#include "sensor_fusion.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <string>

// --- Configuration ---
const char* ssid = "Galaxy_A16_6FB3";
const char* password = "12345678";
const char* mqtt_broker = "8fa6532b48c4440fa03edd9ddfd662f6.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "Amin-Mustafa";
const char* mqtt_password = "Silentlibrary322";

WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);

unsigned long last_reconnect_attempt = 0;

// --- Validated Hardware Pins ---
#define GAS_PIN D0       
#define PRESENCE_PIN D1
#define BUZZER_PIN D3 
#define LED_GRN D8
#define LED_YEL D9
#define LED_RED D10   

#define TEMPERATURE_THRESHOLD	30.0f

#define POLL_DELAY_NORMAL	1000
#define POLL_DELAY_LONG		10000

constexpr bool temp_is_high(float reading) {
	return (reading > TEMPERATURE_THRESHOLD);
}

void update_sensor_state(SensorFusion::StateTracker& st, unsigned long current_time);
void actuate(const SensorFusion::StateTracker& st, bool blink_state);
void notify_admin(const std::string& msg);
void handle_network(unsigned long current_time);

Presence::Detector presence_detector(PRESENCE_PIN);
Gas::Sensor gas_sensor(GAS_PIN, Gas::Type::ALCOHOL);
SensorFusion::StateTracker sensor_fusion;
DHT20 dht20;    

void setup() {
	Serial.begin(115200);
  
	pinMode(GAS_PIN, INPUT);
	pinMode(PRESENCE_PIN, INPUT);
	pinMode(LED_RED, OUTPUT);
	pinMode(LED_YEL, OUTPUT);
	pinMode(LED_GRN, OUTPUT);
	pinMode(BUZZER_PIN, OUTPUT);

	analogReadResolution(12); 

	espClient.setInsecure(); // Skips certificate validation for prototype testing
    WiFi.begin(ssid, password);
    mqtt_client.setServer(mqtt_broker, mqtt_port);
	
	Wire.begin();
    dht20.begin();
}

unsigned long last_dht_read = 0;
unsigned long last_blink = 0;
bool blink_state = false;
float raw_temp = 0.0f;
float raw_gas = 0.0f;
bool lab_occupied = false;

SensorFusion::State last_state = SensorFusion::State::LOW_POWER;

void loop() {	
	unsigned long time = millis();

	if (time - last_blink >= 500) {
        blink_state = !blink_state;
        last_blink = time;
    }

	handle_network(time);

	update_sensor_state(sensor_fusion, time);

	actuate(sensor_fusion, blink_state);

	SensorFusion::State current_state = sensor_fusion.sensor_state();

	if (current_state != last_state) {
        std::string payload = "State Changed to Code: " + std::to_string(static_cast<uint8_t>(current_state));
        // Print locally for debugging
        Serial.println(payload.c_str());
        // Send to cloud
        notify_admin(payload);
        last_state = current_state;
    }
}

void update_sensor_state(SensorFusion::StateTracker& st, unsigned long current_time) {
    using SensorFusion::Sensor;
    
    // Check Presence
    presence_detector.update();
	lab_occupied = presence_detector.occupied();
    st.set_sensor(Sensor::PRESENCE, lab_occupied);

    // Check Gas (Capture the raw value!)
    raw_gas = gas_sensor.read();
    st.set_sensor(Sensor::GAS, Gas::is_high(raw_gas));

    // Check Temperature & Publish Telemetry (Every 2 seconds)
    if (current_time - last_dht_read >= 2000) {
        
        // The DHT20 requires a read() command before fetching the data
        int status = dht20.read();
        
        // DHT20_OK means the I2C communication was successful
        if (status == DHT20_OK) { 
            raw_temp = dht20.getTemperature(); 
            st.set_sensor(Sensor::TEMPERATURE, temp_is_high(raw_temp));
        } else {
            Serial.println("Warning: DHT20 Sensor disconnected or read error!");
        }
        last_dht_read = current_time;

        // Publish JSON Telemetry
        if (mqtt_client.connected()) {
            char payload[100];
            snprintf(payload, sizeof(payload), "{\"temperature\": %.1f, \"gas_ppm\": %.1f, \"presence\": %d}",
				raw_temp, raw_gas, lab_occupied);
            mqtt_client.publish("lab/telemetry", payload);
        }
    }
}

void actuate(const SensorFusion::StateTracker& st, bool blink_state) {
    using SensorFusion::State;

    uint8_t target_led_pin = LED_GRN; // Default fallback
    bool buzzer_on = false;
    bool led_blink = false;

    // Determine intended hardware behavior based on state
    switch(st.sensor_state()) {
        case State::LOW_POWER: 
        case State::MONITORING:
            target_led_pin = LED_GRN;
            break;
            
        case State::ENV_WARNING:
        case State::SAFETY_WARNING:
            target_led_pin = LED_YEL;
            break;
            
        case State::ALERT_VACANT: 
        case State::CRITICAL_VACANT:
            target_led_pin = LED_RED;
            break;
            
        case State::SAFETY_DANGER:
            target_led_pin = LED_RED;
            buzzer_on = true;
            break;
            
        case State::SAFETY_CRITICAL:
            target_led_pin = LED_RED;
            buzzer_on = true;
            led_blink = true; // Highest priority evacuation
            break;
    }

    // Apply states to hardware
    digitalWrite(BUZZER_PIN, buzzer_on);

    // Turn off LEDs that shouldn't be on, and correctly drive the target LED
    uint8_t all_leds[] = {LED_GRN, LED_YEL, LED_RED};
    for(int i = 0; i < 3; i++) {
        if (all_leds[i] == target_led_pin) {
            // If it needs to blink, follow the global clock. Otherwise, solid HIGH.
            digitalWrite(all_leds[i], led_blink ? blink_state : HIGH);
        } else {
            digitalWrite(all_leds[i], LOW);
        }
    }
}

void handle_network(unsigned long current_time) {
    // If Wi-Fi is disconnected, do nothing and return immediately
    if (WiFi.status() != WL_CONNECTED) {
        return; 
    }

    // If Wi-Fi is connected but MQTT is not, try to connect every 5 seconds
    if (!mqtt_client.connected()) {
        if (current_time - last_reconnect_attempt > 5000) {
            last_reconnect_attempt = current_time;
            
            // Attempt to connect with a unique client ID
            String clientId = "ESP32_LabSafetyNode";
            clientId += String(random(0xffff), HEX);
            
            if (mqtt_client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
                Serial.println("Connected to HiveMQ Cloud!");
            }
        }
    } else {
        // If connected, process incoming messages and keep connection alive
        mqtt_client.loop(); 
    }
}

void notify_admin(const std::string& msg) {
    // Only attempt to send if we have a connection
    if (mqtt_client.connected()) {
        // Publish to the "lab/alerts" topic
        mqtt_client.publish("lab/alerts", msg.c_str());
    }
}