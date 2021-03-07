#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "Credentials.h"

#include <arduino-timer.h>

const char *ssid = esp_config::ssid;         //YOUR SSID HERE
const char *password = esp_config::password; //YOUR PASSWORD HERE

//Global variables
int nrOfServices = 0;
String global_log_string = "";
//global forward declarations
bool update_services(void *);
void displayServices(AsyncWebServerRequest *request);
String httpGETRequest(String serverName);

//Used variables
AsyncWebServer server(80);
HTTPClient http;
auto timer = timer_create_default();

void setup()
{
  Serial.begin(115200);
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

  if (!MDNS.begin("homeserver"))
  {
    Serial.println("Error starting mDNS");
    return;
  }

  MDNS.addService("http", "tcp", 80);
  MDNS.addServiceTxt("http", "tcp", "prop1", "test");
  MDNS.addServiceTxt("http", "tcp", "prop2", "test2");

  server.on("/hello", HTTP_GET, displayServices);

  //Task Scheduling
  timer.every(5000, update_services);

  server.begin();
}

void loop()
{
  ArduinoOTA.handle();
  timer.tick();
}

/*
  Update Tasks for background
*/

void displayServices(AsyncWebServerRequest *request)
{
  //nrOfServices = MDNS.queryService("http", "tcp");
  String s = "Number of Services: " + String(nrOfServices);
  for (int i = 0; i < nrOfServices; i = i + 1)
  {
    String ip = MDNS.IP(i).toString();
    String servername = "http://" + ip + "/getapi";
    s += "<br><hr>";
    s += "<h1>Service: " + String(MDNS.hostname(i)) + "</h1>";
    s += "<br>";
    s += "IP: " + ip;
    s += "<br>";
    s += "Port:" + String(MDNS.port(i));
    s += "<br> Api: <br>";
    String payload = httpGETRequest(servername);
    s += payload;
  }
  s += "<hr>";
  s += global_log_string;
  request->send(200, "text/html", s);
}

bool update_services(void *)
{
  nrOfServices = MDNS.queryService("http", "tcp");
  //global_log_string += String(millis()) + ": Updated services to : " + String(nrOfServices) + "\n<br>";
  return true;
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

void handleButtonPress()
{
}