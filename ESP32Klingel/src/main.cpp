#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "Credentials.h"
#include <ZMPT101B.h>
#include <SimpleKalmanFilter.h>

const int RELAY_GREEN = GPIO_NUM_16;
const int RELAY_BLUE = GPIO_NUM_17;

const int VOLTM_GREEN = GPIO_NUM_34; //Klingel
const int VOLTM_BLUE = GPIO_NUM_35;  //Tor

ZMPT101B zmpt_green = ZMPT101B(VOLTM_GREEN);
ZMPT101B zmpt_blue = ZMPT101B(VOLTM_BLUE);

SimpleKalmanFilter kalman_green = SimpleKalmanFilter(5, 5, 0.1);
SimpleKalmanFilter kalman_blue = SimpleKalmanFilter(5, 5, 0.1);

const long SERIAL_REFRESH_TIME = 10;
long refresh_time;

const char *ssid = esp_config::ssid;         //YOUR SSID HERE
const char *password = esp_config::password; //YOUR PASSWORD HERE

//Global variableshttps://github.com/Abdurraziq/ZMPT101B-arduino
String global_log_string = "";
//global forward declarations
String httpGETRequest(String serverName);
bool update_services(void *);
void displayServices(AsyncWebServerRequest *request);
void doKlingel(AsyncWebServerRequest *request);
void doTor(AsyncWebServerRequest *request);

//Used variables
AsyncWebServer server(80);
HTTPClient http;

void setup()
{
  Serial.begin(115200);

  pinMode(RELAY_BLUE, OUTPUT);
  pinMode(RELAY_GREEN, OUTPUT);
  pinMode(VOLTM_BLUE, INPUT);
  pinMode(VOLTM_GREEN, INPUT);
  zmpt_green.calibrate();
  zmpt_blue.calibrate();
  /*
  digitalWrite(RELAY_BLUE, HIGH);
  delay(100);
  digitalWrite(RELAY_GREEN, HIGH);
  delay(1000);
  digitalWrite(RELAY_BLUE, LOW);
  delay(100);
  digitalWrite(RELAY_GREEN, LOW);
  */

  Serial.println("Booting");
  WiFi.setHostname("HomeServer");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
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

  // Custom Code Here

  //Intialization
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin("homeklingel"))
  {
    Serial.println("Error starting mDNS");
    return;
  }

  server.on("/", HTTP_GET, displayServices);

  server.on("/klingel", HTTP_GET, doKlingel);
  server.on("/tor", HTTP_GET, doTor);

  //Task Scheduling
  //timer.every(5000, update_services);

  server.begin();
}

void loop()
{
  ArduinoOTA.handle();
  float green_raw = zmpt_blue.getVoltageAC();
  float blue_raw = zmpt_green.getVoltageAC();
  float blue_v = kalman_blue.updateEstimate(blue_raw);
  float green_v = kalman_green.updateEstimate(green_raw);

  if (millis() > refresh_time)
  {
    //Serial.print("blue:" + String(green_raw * 1000));
    //Serial.print(", green:" + String(blue_raw * 1000));
    Serial.print(", blue_kf:" + String(blue_v * 1000));
    Serial.println(", green_kf:" + String(green_v * 1000));
    refresh_time = millis() + SERIAL_REFRESH_TIME;
  }
}

/*
  Update Tasks for background
*/

void displayServices(AsyncWebServerRequest *request)
{
  //nrOfServices = MDNS.queryService("http", "tcp");
  String s = "Klingel Client ESP32";
  s += "<hr>";
  s += global_log_string;
  request->send(200, "text/html", s);
}

/*
  Update Tasks for background
*/

/*
 Predefined functions
*/

void doKlingel(AsyncWebServerRequest *request)
{
  digitalWrite(RELAY_GREEN, HIGH);
  delay(500);
  digitalWrite(RELAY_GREEN, LOW);
  request->send(200, "text/html", "Klingel An");
}

void doTor(AsyncWebServerRequest *request)
{
  digitalWrite(RELAY_BLUE, HIGH);
  delay(500);
  digitalWrite(RELAY_BLUE, LOW);
  request->send(200, "text/html", "Klingel An");
}

String httpGETRequest(String serverName)
{
  HTTPClient http;

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

  return payload;
}