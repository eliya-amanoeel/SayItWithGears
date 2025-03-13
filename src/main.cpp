#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <time.h>

// Network credentials
const char* ssid = "";
const char* password = "";

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
void displayTime();
void handleRoot();
void handleSet();
void handleSwitchMode();
void handleGetTime();

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.begin(9600);

    setupWiFi();
    setupServer();
}

void loop() {
    server.handleClient();
    if (currentMode == Clock) {
        displayTime();
    }
}

// Function to connect to WiFi
void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    configTime(0, 0, "pool.ntp.org");
    setenv("TZ", timezone, 0);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

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

// Function to handle the root endpoint
void handleRoot() {
    String html = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<link rel=\"icon\" href=\"data:,\">";
    html += "<style>";
    html += "html { font-family: 'Comic Sans MS', cursive, sans-serif; display: inline-block; margin: 0px auto; text-align: center; background-color: #f0f0f0;}";
    html += "h1 { color: #333; }";
    html += ".button { border: 2px solid transparent; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer; background-color: #FF0000; border-radius: 12px; }";
    html += ".highlight { border: 2px solid yellow; }";
    html += ".hidden { display: none; }";
    html += "#clock { font-size: 40px; font-weight: bold; color: #333; }";
    html += ".switch { position: relative; display: inline-block; width: 60px; height: 34px; }";
    html += ".switch input { opacity: 0; width: 0; height: 0; }";
    html += ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px; }";
    html += ".slider:before { position: absolute; content: \"\"; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }";
    html += "input:checked + .slider { background-color: #2196F3; }";
    html += "input:checked + .slider:before { transform: translateX(26px); }";
    html += "label { font-size: 20px; color: #333; }";
    html += "input[type=range] { -webkit-appearance: none; width: 100%; margin: 10.8px 0; }";
    html += "input[type=range]:focus { outline: none; }";
    html += "input[type=range]::-webkit-slider-runnable-track { width: 100%; height: 8.4px; cursor: pointer; animate: 0.2s; background: #ddd; border-radius: 1.3px; }";
    html += "input[type=range]::-webkit-slider-thumb { border: 1px solid #000000; height: 36px; width: 16px; border-radius: 3px; background: #FFFF00; cursor: pointer; -webkit-appearance: none; margin-top: -14px; }";
    html += "@keyframes rotate { from { transform: rotate(0deg); } to { transform: rotate(360deg); } }";
    html += ".rotate { display: inline-block; animation: rotate 2s linear infinite; }";
    html += "</style>";
    html += "<script>";
    html += "function updateNumber() {";
    html += "  const digit1 = document.getElementById('digit1').value;";
    html += "  const digit2 = document.getElementById('digit2').value;";
    html += "  const digit3 = document.getElementById('digit3').value;";
    html += "  const digit4 = document.getElementById('digit4').value;";
    html += "  const number = digit1 + digit2 + digit3 + digit4;";
    html += "  document.getElementById('chosenNumber').innerText = number;";
    html += "}";
    html += "function sendNumber() {";
    html += "  const number = document.getElementById('chosenNumber').innerText;";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/set?num=' + number, true);";
    html += "  xhr.send();";
    html += "}";
    html += "function switchMode() {";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/switchMode', true);";
    html += "  xhr.send();";
    html += "  xhr.onload = function() {";
    html += "    document.getElementById('currentMode').innerText = xhr.responseText;";
    html += "    if (xhr.responseText === 'Clock') {";
    html += "      document.getElementById('number-section').style.display = 'none';";
    html += "      document.getElementById('clock').style.display = 'block';";
    html += "      document.getElementById('animation').style.display = 'inline-block';";
    html += "    } else {";
    html += "      document.getElementById('number-section').style.display = 'block';";
    html += "      document.getElementById('clock').style.display = 'none';";
    html += "      document.getElementById('animation').style.display = 'none';";
    html += "    }";
    html += "  }";
    html += "}";
    html += "function updateClock() {";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/getTime', true);";
    html += "  xhr.send();";
    html += "  xhr.onload = function() {";
    html += "    document.getElementById('clock').innerText = xhr.responseText;";
    html += "  }";
    html += "}";
    html += "setInterval(updateClock, 1000);"; // Update clock every second
    html += "</script></head>";
    html += "<body><h1>ESP8266 Web Server</h1>";
    html += "<label class='switch'><input type='checkbox' onclick='switchMode()'><span class='slider'></span></label>";
    html += "<p>Mode: <span id='currentMode'>Tuning</span></p>";
    html += "<div id='number-section'>";
    html += "<label for='digit1'>Box nr 1</label>";
    html += "<input type='range' id='digit1' min='0' max='9' value='0' oninput='updateNumber()'><br>";
    html += "<label for='digit2'>Box nr 2</label>";
    html += "<input type='range' id='digit2' min='0' max='9' value='0' oninput='updateNumber()'><br>";
    html += "<label for='digit3'>Box nr 3</label>";
    html += "<input type='range' id='digit3' min='0' max='9' value='0' oninput='updateNumber()'><br>";
    html += "<label for='digit4'>Box nr 4</label>";
    html += "<input type='range' id='digit4' min='0' max='9' value='0' oninput='updateNumber()'><br>";
    html += "<p>Chosen Number: <span id='chosenNumber'>0000</span></p>";
    html += "<button class='button' onclick='sendNumber()'>Send</button>";
    html += "</div>";
    html += "<div id='clock' style='display:none;'>00:00:00</div>";
    html += "<div id='animation' class='rotate' style='display:none;'>&#9881;</div>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

// Function to handle the set endpoint
void handleSet() {
    if (currentMode == Tuning) {
        if (server.hasArg("num")) {
            String number = server.arg("num");
            if (number.length() == 4 && number.toInt() != 0) {
                int digit1 = number.substring(0, 1).toInt();
                int digit2 = number.substring(1, 2).toInt();
                int digit3 = number.substring(2, 3).toInt();
                int digit4 = number.substring(3, 4).toInt();
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
    if (currentMode == Tuning) {
        currentMode = Clock;
        server.send(200, "text/plain", "Clock");
    } else {
        currentMode = Tuning;
        server.send(200, "text/plain", "Tuning");
    }
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