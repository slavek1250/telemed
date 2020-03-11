#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <MAX30100.h>
#include <Wire.h>

// Sampling is tightly related to the dynamic range of the ADC.
// refer to the datasheet for further info
#define SAMPLING_RATE                       MAX30100_SAMPRATE_100HZ

// The LEDs currents must be set to a level that avoids clipping and maximises the
// dynamic range
#define IR_LED_CURRENT                      MAX30100_LED_CURR_50MA
#define RED_LED_CURRENT                     MAX30100_LED_CURR_27_1MA

// The pulse width of the LEDs driving determines the resolution of
// the ADC (which is a Sigma-Delta).
// set HIGHRES_MODE to true only when setting PULSE_WIDTH to MAX30100_SPC_PW_1600US_16BITS
#define PULSE_WIDTH                         MAX30100_SPC_PW_1600US_16BITS
#define HIGHRES_MODE                        true

#define BUFF_SIZE 100
//#include <Adafruit_Sensor.h>
//#include <DHT.h>

const char* ssid     = "ESP8266-Access-Point";
const char* password = "123456789";

AsyncWebServer server(80);
MAX30100 sensor;


uint8_t buffCurrId = 0;
unsigned long buffMs[BUFF_SIZE] = { 0 };
long buffVals[BUFF_SIZE] = { 0 };

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
  
  Serial.print("Setting AP (Access Point)…");
  
  //WiFi.softAP(ssid, password);

  WiFi.begin("tplink", "0987654321a123");

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  Serial.println(WiFi.localIP());

  sensor.begin();
  sensor.setMode(MAX30100_MODE_SPO2_HR);
  sensor.setLedsCurrent(IR_LED_CURRENT, RED_LED_CURRENT);
  sensor.setLedsPulseWidth(PULSE_WIDTH);
  sensor.setSamplingRate(SAMPLING_RATE);
  sensor.setHighresModeEnabled(HIGHRES_MODE);

  server.on("/", HTTP_GET, dataRequest);
  server.begin();

}
 
void loop(){  
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= 10000) {
    previousMillis = currentMillis;



  }
  uint16_t ir, red;
  sensor.update();
  while (sensor.getRawValues(&ir, &red)) {

	  buffMs[buffCurrId] = millis();
	  buffVals[buffCurrId] = red;
	  buffCurrId = (++buffCurrId % BUFF_SIZE);
  }

  //Serial.println(meanDiff(sensor.IR));
  delay(10);
}

long meanDiff(int M) {
#define LM_SIZE 15
	static int LM[LM_SIZE];      // LastMeasurements
	static byte index = 0;
	static long sum = 0;
	static byte count = 0;
	long avg = 0;

	// keep sum updated to improve speed.
	sum -= LM[index];
	LM[index] = M;
	sum += LM[index];
	index++;
	index = index % LM_SIZE;
	if (count < LM_SIZE) count++;

	avg = sum / count;
	return avg - M;
}