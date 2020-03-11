#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <MAX30100.h>
#include <Wire.h>

#define SAMPLING_RATE		MAX30100_SAMPRATE_100HZ
#define IR_LED_CURRENT      MAX30100_LED_CURR_50MA
#define RED_LED_CURRENT     MAX30100_LED_CURR_27_1MA
#define PULSE_WIDTH         MAX30100_SPC_PW_1600US_16BITS
#define HIGHRES_MODE        true
#define INT_PIN				2
#define BUFF_SIZE			130

const char* ssid     = "ESP8266-Access-Point";
const char* password = "123456789";

AsyncWebServer server(80);
MAX30100 sensor;

uint8_t buffCurrId = 0;
unsigned long buffMs[BUFF_SIZE] = { 0 };
long buffVals[BUFF_SIZE] = { 0 };


void initMax30100SpO2Interrupt() {
	Wire.beginTransmission(MAX30100_I2C_ADDRESS);
	Wire.write(MAX30100_REG_INTERRUPT_ENABLE);
	Wire.write(MAX30100_IE_ENB_SPO2_RDY);
	Wire.endTransmission();
}

void ICACHE_RAM_ATTR handleInterrupt() {
	Serial.println("OK");
}

void dataRequest(AsyncWebServerRequest * request) {
	String str = "[";
	for (uint8_t i = 0; i < BUFF_SIZE; ++i) {
		str += "{";
		str += "\"T\":" + String(buffMs[i]) + ",";
		str += "\"V\":" + String(buffVals[i]) + "}";
		if(i != (BUFF_SIZE-1)) str += ",";
	}
	str += "]";
	request->send(200, "application/json", str.c_str());
}

void setup(){
	Serial.begin(115200);
  
	Serial.println("Setting AP (Access Point)…");
  
	//WiFi.softAP(ssid, password);

	WiFi.begin("tplink", "0987654321a123");

	//IPAddress IP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	//Serial.println(IP);
	Serial.println(WiFi.localIP());

	sensor.begin();
	sensor.setMode(MAX30100_MODE_SPO2_HR);
	sensor.setLedsCurrent(IR_LED_CURRENT, RED_LED_CURRENT);
	sensor.setLedsPulseWidth(PULSE_WIDTH);
	sensor.setSamplingRate(SAMPLING_RATE);
	sensor.setHighresModeEnabled(HIGHRES_MODE);

	server.on("/", HTTP_GET, dataRequest);
	server.begin();
	/*
	initMax30100SpO2Interrupt();
	attachInterrupt(digitalPinToInterrupt(INT_PIN), handleInterrupt, FALLING);
	pinMode(INT_PIN, INPUT_PULLUP);
	*/
}
 
void loop(){  
  uint16_t ir, red;
  sensor.update();
  while (sensor.getRawValues(&ir, &red)) {

	  buffMs[buffCurrId] = millis();
	  buffVals[buffCurrId] = red;
	  buffCurrId = (++buffCurrId % BUFF_SIZE);
  }
}