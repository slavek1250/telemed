#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <MAX30100.h>
#include <Wire.h>

#define SAMPLING_RATE		MAX30100_SAMPRATE_100HZ
#define IR_LED_CURRENT      MAX30100_LED_CURR_0MA
#define RED_LED_CURRENT     MAX30100_LED_CURR_0MA
#define PULSE_WIDTH         MAX30100_SPC_PW_1600US_16BITS
#define HIGHRES_MODE        true
#define BUFF_SIZE			130

const char* ssid     = "HRSensor";
const char* password = "123456789";

AsyncWebServer server(80);
MAX30100 sensor;

uint8_t buffCurrId = 0;
unsigned long buffMs[BUFF_SIZE] = { 0 };
uint16_t buffIrVals[BUFF_SIZE] = { 0 };
uint16_t buffRedVals[BUFF_SIZE] = { 0 };

LEDCurrent	irLedCurrent = IR_LED_CURRENT,
			redLedCurrent = RED_LED_CURRENT;

void ICACHE_RAM_ATTR handleInterrupt() {
	Serial.println("OK");
}

void setLedCurrentRequest(AsyncWebServerRequest * request) {
	if (request->hasParam("ir")) {
		AsyncWebParameter* p = request->getParam("ir");
		irLedCurrent = (LEDCurrent)p->value().toInt();
	}
	if (request->hasParam("red")) {
		AsyncWebParameter* p = request->getParam("red");
		redLedCurrent = (LEDCurrent)p->value().toInt();
	}
	sensor.setLedsCurrent(irLedCurrent, redLedCurrent);
	request->send(200, "application/json", "{\"status\":\"OK\"}");
}

void dataRequest(AsyncWebServerRequest * request) {
	String str = "[";
	for (uint8_t i = 0; i < BUFF_SIZE; ++i) {
		str += "{";
		str += "\"ms\":" + String(buffMs[i]) + ",";
		str += "\"ir\":" + String(buffIrVals[i]) + ",";
		str += "\"red\":" + String(buffRedVals[i]) + "}";
		if(i != (BUFF_SIZE-1)) str += ",";
	}
	str += "]";
	request->send(200, "application/json", str.c_str());
}

void setup(){
	Serial.begin(115200);
 
	WiFi.hostname("HRSensor");
	WiFi.softAP(ssid, password);

	sensor.begin();
	sensor.setMode(MAX30100_MODE_SPO2_HR);
	sensor.setLedsCurrent(irLedCurrent, redLedCurrent);
	sensor.setLedsPulseWidth(PULSE_WIDTH);
	sensor.setSamplingRate(SAMPLING_RATE);
	sensor.setHighresModeEnabled(HIGHRES_MODE);

	server.on("/", HTTP_GET, dataRequest);
	server.on("/set_led_current", HTTP_GET, setLedCurrentRequest);
	server.begin();
}
 
void loop(){  
	uint16_t ir, red;
	sensor.update();
	if (sensor.getRawValues(&ir, &red)) {

		buffMs[buffCurrId] = millis();
		buffIrVals[buffCurrId] = ir;
		buffRedVals[buffCurrId] = red;
		buffCurrId = (++buffCurrId % BUFF_SIZE);
	}
	delay(5);
}