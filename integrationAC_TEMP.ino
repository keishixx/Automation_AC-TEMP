#include <WiFi.h>
#include <WebServer.h>
#include <IRremoteESP8266.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h> 
#include <IRsend.h> 

#define DHTPIN 18       // Change this to the actual GPIO pin where your DHT22 data pin is connected
#define DHTTYPE DHT22  // Define sensor type as DHT22 

#define IR_LED_PIN 14  // GPIO for IR LED

IRsend irsend(IR_LED_PIN);

// Wi-Fi Credentials (Replace with your actual Wi-Fi SSID & Password)
const char* ssid = "iPhone";
const char* password = "shimalakas";

WebServer server(80);  // Web server on port 80 
DHT dht(DHTPIN, DHTTYPE);

uint64_t irPowerToggle = 0x80FF48B7;
uint64_t irTempUp = 0x80FF58A7;
uint64_t irTempDown = 0x80FFC837;
uint64_t irTimer = 0x80FFD827;
uint64_t irMode = 0x80FF40BF;
uint64_t irFan = 0x80FF708F;
uint64_t irSwing = 0x80FFF00F;
uint64_t irSleep = 0x80FF50AF;

String deviceState = "OFF";
int timerHours = 0;  
unsigned long lastDHTReadTime = 0; // For non-blocking DHT read
float humidity, temperature;

bool autoMode = false; // Default: Automatic mode disabled

void sendIRSignal() {
    Serial.println("Sending IR Power Signal...");
    irsend.sendNEC(irPowerToggle, 32);
    deviceState = (deviceState == "OFF") ? "ON" : "OFF";
    // If AC is controlled via the web, disable auto mode
    autoMode = false;  

    server.send(200, "text/plain", deviceState);
}

void getSensorData() {
    String sensorMessage = "Temperature: " + String(temperature) + " °C\nHumidity: " + String(humidity) + " %";
    server.send(200, "text/plain", sensorMessage);
}


void disableAutoMode() {
    autoMode = false;
    server.send(200, "text/plain", "Automatic Mode Disabled");
}

void enableAutoMode() {
    autoMode = true;
    server.send(200, "text/plain", "Automatic Mode Enabled");
}

void sendTempUpSignal() {
    Serial.println("Sending IR Signal: Increase Temperature");
    irsend.sendNEC(irTempUp, 32);
    server.send(200, "text/plain", "Temperature Increased");
}

void sendTempDownSignal() {
    Serial.println("Sending IR Signal: Decrease Temperature");
    irsend.sendNEC(irTempDown, 32);
    server.send(200, "text/plain", "Temperature Decreased");
}

void sendTimerSignal() {
    Serial.println("Setting Timer...");
    irsend.sendNEC(irTimer, 32);
    timerHours++;
    if (timerHours > 12) {
        timerHours = 0;
    }
    server.send(200, "text/plain", "Timer Set: " + String(timerHours) + " hrs");
}

void sendModeSignal() {
    Serial.println("Changing Mode...");
    irsend.sendNEC(irMode, 32);
    server.send(200, "text/plain", "Mode Changed");
}

void sendFanSignal() {
    Serial.println("Adjusting Fan...");
    irsend.sendNEC(irFan, 32);
    server.send(200, "text/plain", "Fan Speed Adjusted");
}

void sendSwingSignal() {
    Serial.println("Toggling Swing...");
    irsend.sendNEC(irSwing, 32);
    server.send(200, "text/plain", "Swing Toggled");
}

void sendSleepSignal() {
    Serial.println("Activating Sleep Mode...");
    irsend.sendNEC(irSleep, 32);
    server.send(200, "text/plain", "Sleep Mode Activated");
}

void getStatus() {
    String statusMessage = "Device is " + deviceState + 
                           "\nTimer Set: " + String(timerHours) + "  hrs" +
                           "\nAuto Mode: " + (autoMode ? "Enabled" : "Disabled");
    server.send(200, "text/plain", statusMessage);
}


void handleRoot() {
    server.send(200, "text/html", R"rawliteral(
        <html>
        <head>
            <title>ESP32 IR Remote</title>
            <style>
                body { font-family: Arial, sans-serif; text-align: center; }
                button { font-size: 20px; padding: 10px; margin: 10px; }
                #status, #sensorData { font-size: 22px; color: green; margin-top: 10px; font-weight: bold; }
            </style>
            <script>
              function sendIR(command) {
                  fetch(`/${command}`)
                      .then(response => response.text())
                      .then(data => {
                          alert(data);
                          updateStatus();
                      });
              }

              function updateStatus() {
                  fetch('/status')
                      .then(response => response.text())
                      .then(data => {
                          document.getElementById('status').innerText = data;
                      });
              }

              function updateSensorData() {
                  fetch('/sensorData')
                      .then(response => response.text())
                      .then(data => {
                          document.getElementById('sensorData').innerText = data;
                      });
              }

              setInterval(updateStatus, 1000);
              setInterval(updateSensorData, 2000);
          </script>
        </head>
        <body>
            <h2>ESP32 IR Remote Control</h2>
            <button onclick="sendIR('send')">Toggle Power</button>
            <button onclick="sendIR('tempUp')">Increase Temperature</button>
            <button onclick="sendIR('tempDown')">Decrease Temperature</button>
            <button onclick="sendIR('timer')">Set Timer</button>
            <button onclick="sendIR('mode')">Change Mode</button>
            <button onclick="sendIR('fan')">Adjust Fan</button>
            <button onclick="sendIR('swing')">Toggle Swing</button>
            <button onclick="sendIR('sleep')">Activate Sleep Mode</button>
            <button onclick="sendIR('enableAutoMode')">Enable Auto Mode</button>
            <button onclick="sendIR('disableAutoMode')">Disable Auto Mode</button>
            <p id="status">Device is OFF</p>
            <p id="sensorData">Loading sensor data...</p>
        </body>
        </html>
    )rawliteral");
}

void setup() {  

    Serial.begin(115200);
    irsend.begin();
    dht.begin();

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi Connected!");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.on("/send", sendIRSignal);
    server.on("/status", getStatus);
    server.on("/tempUp", sendTempUpSignal);
    server.on("/tempDown", sendTempDownSignal);
    server.on("/timer", sendTimerSignal);
    server.on("/mode", sendModeSignal);
    server.on("/fan", sendFanSignal);
    server.on("/swing", sendSwingSignal);
    server.on("/sleep", sendSleepSignal);
    server.on("/disableAutoMode", disableAutoMode);
    server.on("/enableAutoMode", enableAutoMode);
    server.on("/sensorData", getSensorData);
    
    server.begin();
    Serial.println("Web server started.");
}


void loop() {
    server.handleClient();

    // Read DHT sensor every 5 seconds
    if (millis() - lastDHTReadTime > 5000) {
        lastDHTReadTime = millis();
        humidity = dht.readHumidity();
        temperature = dht.readTemperature(); // Default in Celsius

        if (isnan(humidity) || isnan(temperature)) {
            Serial.println("Failed to read from DHT sensor!");
        } else {
            Serial.print("Humidity: ");
            Serial.print(humidity);
            Serial.print(" %  |  Temperature: ");
            Serial.print(temperature);
            Serial.println(" °C");

            // AUTOMATIC CONTROL ONLY WORKS IF IN AUTO MODE
            if (autoMode) {
                if (temperature >= 38 && deviceState == "OFF") {
                    Serial.println("Temperature too high! Turning ON AC...");
                    irsend.sendNEC(irPowerToggle, 32);
                    deviceState = "ON";
                } 
                else if (temperature <= 36 && deviceState == "ON") {
                    Serial.println("Temperature dropped! Turning OFF AC...");
                    irsend.sendNEC(irPowerToggle, 32);
                    deviceState = "OFF";
                }
            }
        }
    }
}

