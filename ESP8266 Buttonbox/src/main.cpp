#include <Arduino.h>
#include <esp8266wifi.h>
#include <esp8266httpclient.h>
// #include <asyncHTTPrequest.h>

void httpGETRequest(const char* serverName);
// asyncHTTPrequest request;
HTTPClient http;

const char *ssid = "X";
const char *password = "Y";

const int pin1 = 15;
const int pin2 = 13;
const int pin3 = 12;
const int pin4 = 14;
uint8_t val1 = 0;
uint8_t val2 = 0;
uint8_t val3 = 0;
uint8_t val4 = 0;

void setup() {
  pinMode(pin1, INPUT_PULLDOWN_16);
  pinMode(pin2, INPUT_PULLDOWN_16);
  pinMode(pin3, INPUT_PULLDOWN_16);
  pinMode(pin4, INPUT_PULLDOWN_16);
  Serial.begin(9600);

  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting..");
  }
  Serial.println("CONNECTED");
}

void loop() {
  bool tmp1 = digitalRead(pin1);
  bool tmp2 = digitalRead(pin2);
  if (val1 != tmp1) {
    val1 = digitalRead(pin1);
    if (val1 == 1) {
      Serial.println("Button 1 Pressed");
      httpGETRequest("http://192.168.0.94/klingel");
    } else {
      // httpGETRequest("http://192.168.0.94/klingelOff");
    }
  }
  if (val2 != tmp2) {
    val2 = digitalRead(pin2);
    if (val2 == 1) {
      Serial.println("Button 2 Pressed");
      httpGETRequest("http://192.168.0.94/tor");
    } else {
      // httpGETRequest("http://192.168.0.94/tor");
    }
  }
  // Serial.print("1: " + String(val1));
  // Serial.println(" 2: " + String(val2));
  
  
}


// void httpGETRequest(const char* serverName){
//     if(request.readyState() == 0 || request.readyState() == 4){
//         request.open("GET", serverName);
//         request.send();
//     }
// }

void httpGETRequest(const char* serverName) {
  // Your IP address with path or Domain name with URL path 
  http.begin(serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  // //String payload = "{}"; 

  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    //payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return;
}