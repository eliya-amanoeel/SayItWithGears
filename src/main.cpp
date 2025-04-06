#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ArduinoOTA.h>
#include <time.h>
#include "WiFiCredentials.h"
#include <LittleFS.h>

// Time zone configuration
const char* timezone = "CET-1CEST,M3.5.0,M10.5.0/3"; // Central Europe

// Web server instance
ESP8266WebServer server(80);

// Mode enumeration
enum Mode { Tuning, Clock };
Mode currentMode = Tuning; // Default mode

// Function prototypes
void setupWiFi();
void setupServer();
void setupOTA();
void displayTime();
void handleRoot();
void handleSet();
void handleSwitchMode();
void handleGetTime();

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.begin(9600);

    if (!LittleFS.begin()) {
        Serial.println("An error has occurred while mounting LittleFS");
        return;
    }

    setupWiFi();
    setupServer();
    setupOTA();
}

void loop() {
    server.handleClient();
    ArduinoOTA.handle();
    if (currentMode == Clock) {
        displayTime();
    }
}

// Function to connect to WiFi
void setupWiFi() {
    WiFi.mode(WIFI_STA);

    // Attempt to connect to primary WiFi network
    WiFi.begin(ssid, password);
    Serial.print("Connecting to primary WiFi");
    for (int i = 0; i < 20; i++) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected to primary WiFi");
            break;
        }
        delay(500);
        Serial.print(".");
    }

    // If still not connected, print an error message
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nFailed to connect to WiFi");
        return;
    }

    configTime(0, 0, "pool.ntp.org");
    setenv("TZ", timezone, 0);

    Serial.println("");
    Serial.println("\nWiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

// Function to configure server routes
void setupServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/set", HTTP_GET, handleSet);
    server.on("/switchMode", HTTP_GET, handleSwitchMode);
    server.on("/getTime", HTTP_GET, handleGetTime);

    server.begin();
}

// Function to set up OTA updates
void setupOTA() {
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
        }
        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });
    ArduinoOTA.begin();
    Serial.println("OTA Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

// Function to handle the root endpoint
void handleRoot() {
    if (!LittleFS.exists("/ui.html")) {
        server.send(404, "text/plain", "File not found");
        return;
    }

    File file = LittleFS.open("/ui.html", "r");
    if (!file) {
        server.send(500, "text/plain", "Failed to open file");
        return;
    }

    String html = file.readString();
    file.close();
    server.send(200, "text/html", html);
}

// Function to handle the set endpoint
void handleSet() {
    if (currentMode == Tuning) {
        if (server.hasArg("num")) {
            String number = server.arg("num");
            if (number.length() == 4) {
                int digit4 = number.substring(0, 1).toInt();
                int digit3 = number.substring(1, 2).toInt();
                int digit2 = number.substring(2, 3).toInt();
                int digit1 = number.substring(3, 4).toInt();
                int combined1 = 00; 
                int combined2 = 00; 
                // Ensure combined1 and combined2 are treated as valid even if they are zero
                if (number != 0000) {
                    combined1 = digit1 * 10 + digit2;
                    combined2 = digit3 * 10 + digit4;
                }
                char buf[32];
                sprintf(buf, "$D%02d%02d\n", combined1, combined2);
                Serial.print(buf);
                server.send(200, "text/plain", "Set display to: " + number);
            } else {
                server.send(400, "text/plain", "Invalid number! Send ?num=4-digit number");
            }
        } else {
            server.send(400, "text/plain", "Missing parameter: ?num=4-digit number");
        }
    } else {
        server.send(400, "text/plain", "Invalid operation in current mode.");
    }
}

// Function to handle the switch mode endpoint
void handleSwitchMode() {
    currentMode = (currentMode == Tuning) ? Clock : Tuning;
    server.send(200, "text/plain", (currentMode == Clock) ? "Clock" : "Tuning");
}

// Function to handle the get time endpoint
void handleGetTime() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char buf[9];
    sprintf(buf, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    server.send(200, "text/plain", buf);
}

// Function to display the current time
void displayTime() {
    static time_t lastv = 0;
    static uint8_t cnt = 0;
    char buf[32];

    time_t tnow = time(nullptr);
    tm* tm = localtime(&tnow);

    if (lastv != tm->tm_min) { // update time every minute
        lastv = tm->tm_min;
        sprintf(buf, "$D%02d%02d\n", tm->tm_hour, tm->tm_min); // The display expects a message like this: $D[number]\n
        Serial.print(buf);
    }

    delay(1000);

    if (cnt++ > 5) { // blink the LED to indicate we're still alive
        cnt = 0;
        digitalWrite(LED_BUILTIN, LOW); // Turn the LED on
        delay(50);
        digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off
    }
}