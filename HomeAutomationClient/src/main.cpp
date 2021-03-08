#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Credentials.h"

const char *ssid = esp_config::ssid;         //YOUR SSID HERE
const char *password = esp_config::password; //YOUR PASSWORD HERE

AsyncWebServer server(80);

const int buttonPin = 15;
const int relaisPin = 21;
int buttonState = 0;
int newState = 0;

String httpGETRequest(String serverName);

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.setHostname("HomeClient");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if(!MDNS.begin("homeclient")) {
     Serial.println("Error starting mDNS");
     return;
  }
  MDNS.addService("http", "tcp", 80);
  MDNS.addServiceTxt("http", "tcp", "Client", "test123");

  server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Hello World");
  });
  
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(relaisPin, HIGH);
    request->send(200, "text/plain", "Ist an");
  });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(relaisPin, LOW);
    request->send(200, "text/plain", "Ist aus");
  });

  server.on(
    "/init",
    HTTP_POST,
    [](AsyncWebServerRequest * request){},
    NULL,
    [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
 
      //Handling function implementation
      for (size_t i = 0; i < len; i++) {
        Serial.write(data[i]);
      }
 
      Serial.println();
 
      request->send(200);
  });

  server.on("/getapi", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(512);

    doc["name"] = "Generic Client";
    doc["description"] = "Generischer Client, testweise mit einem Relais und einem Button";
    JsonArray methods = doc.createNestedArray("methods");
      JsonObject on = methods.createNestedObject();
        on["name"] = "on";
        on["description"] = "Schaltet Relais an";
        on["HTTP-Type"] = "GET";
      JsonObject off = methods.createNestedObject();
        off["name"] = "off";
        off["description"] = "Schaltet Relais aus";
        off["HTTP-Type"] = "GET";
      JsonObject init = methods.createNestedObject();
        init["name"] = "init";
        init["description"] = "Initialisiert Client";
        init["HTTP-Type"] = "POST";
        JsonArray array = init.createNestedArray("POST-Parameter");
          array.add("INT");
          array.add("INT");
          array.add("STR");

    String buf;
    serializeJson(doc, buf);
    request->send(200, "application/json", buf);
  });

  server.begin();

  pinMode(buttonPin, INPUT);
  pinMode(relaisPin, OUTPUT);
  digitalWrite(relaisPin, LOW);
}

void loop()
{
  ArduinoOTA.handle();
  newState = digitalRead(buttonPin);
  if (newState != buttonState) {
    buttonState = newState;
    if (buttonState == HIGH) {
      Serial.println(httpGETRequest("http://homeserver/button_on"));
    } else {
      Serial.println(httpGETRequest("http://homeserver/button_off"));
    }
  }
}

String httpGETRequest(String serverName)
{
  HTTPClient http;
  Serial.println("Starting GET request");
  // Your IP address with path or Domain name with URL path
  http.begin(serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
  Serial.println("Ended Get Request");
  return payload;
}
