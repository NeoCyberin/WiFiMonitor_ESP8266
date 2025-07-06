#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266Ping.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64

#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
#define OLED_SDA 14
#define OLED_SCL 12

#define BUTTON_PIN D4

volatile bool buttonPressed = false;

const int totalPages = 2;
volatile int currentPage = 0;

int pingGateway = -1;
int pingGoogle = -1;

unsigned long lastPingUpdate = 0;
const unsigned long pingUpdateInterval = 5000;

const char* espSSID = "HW-364A";
const char* espPassword = "1234567890";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
ESP8266WebServer server(80);
bool serverStarted = false;

unsigned long lastPressTime = 0;
const unsigned long debounceDelay = 500;
volatile unsigned long lastInterruptTime = 0;

unsigned long buttonDownTime = 0;
bool buttonHeld = false;
const unsigned long longPressDuration = 3000;

unsigned long startTime = 0;
const unsigned long sleepDelay = 60000;
volatile bool sleepCancelled = false;

inline const String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Wi-Fi Configuration</title>
  <style>
    body {
      margin: 0;
      font-family: Arial, sans-serif;
      background: #f4f4f4;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
    }

    .container {
      background: #fff;
      padding: 2rem;
      border-radius: 12px;
      box-shadow: 0 0 20px rgba(0,0,0,0.1);
      max-width: 400px;
      width: 90%;
    }

    h2 {
      text-align: center;
      margin-bottom: 1.5rem;
      color: #333;
    }

    label {
      display: block;
      margin-bottom: 0.5rem;
      font-weight: bold;
      color: #555;
    }

    input[type="text"],
    input[type="password"] {
      width: 100%;
      padding: 0.75rem;
      margin-bottom: 1rem;
      border: 1px solid #ccc;
      border-radius: 8px;
      box-sizing: border-box;
    }

    button {
      width: 100%;
      padding: 0.75rem;
      background-color: #28a745;
      border: none;
      border-radius: 8px;
      color: white;
      font-size: 1rem;
      cursor: pointer;
    }

    button:hover {
      background-color: #218838;
    }

    @media (max-width: 400px) {
      .container {
        padding: 1rem;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>Wi-Fi Configuration</h2>
    <form id="wifiConfigForm">
      <label for="ssid">SSID</label>
      <input type="text" id="ssid" name="ssid" placeholder="Enter SSID" required>

      <label for="password">Password</label>
      <input type="password" id="password" name="password" placeholder="Enter Password" required>

      <button type="submit">Save</button>
    </form>
    <script>
      document.getElementById('wifiConfigForm').addEventListener('submit', function(event) {
        event.preventDefault(); // Prevent the form from submitting normally
        const ssid = document.getElementById('ssid').value;
        const password = document.getElementById('password').value;

        fetch('/save', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
          },
          body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`
        })
        .then(response => response.text())
        .then(data => {
          alert(data); // Show success message
        })
        .catch(error => {
          console.error('Error:', error);
          alert('Failed to save configuration!');
        });
      });
    </script>
  </div>
</body>
</html>
)rawliteral";

void displayMessage(const String& message) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(message);
  display.display();
}

void displayInit() {
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (true);
  }
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  displayMessage("WiFi Monitor started");
}

void initializeLittleFS() {
  if (!LittleFS.begin()) {
    displayMessage("Failed to mount FS");
    return;
  }
}

void startAP() {
  IPAddress local_ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(espSSID, espPassword);
}

bool saveConfig (const String& ssid, const String& password) {
  JsonDocument doc;
  doc["ssid"] = ssid;
  doc["password"] = password;

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    displayMessage("Failed to open config file for writing");
    return false;
  }

  serializeJson(doc, configFile);
  configFile.close();
  return true;
}

bool loadConfig(String& ssid, String& password) {
  if (!LittleFS.exists("/config.json")) {
    displayMessage("Config file does not exist");
    return false;
  }
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    displayMessage("Failed to open config file for reading");
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    displayMessage("Failed to parse config file");
    return false;
  }

  ssid = doc["ssid"].as<String>();
  password = doc["password"].as<String>();
  return true;
}

void serverHandleRoot() {   
  server.send(200, "text/html", html);
}

void serverHandleSave() {
  if (server.method() == HTTP_POST) {
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    if (saveConfig(ssid, password)) {
      displayMessage("Configuration saved successfully!");
      server.send(200, "text/plain", "Configuration saved successfully!");
      delay(200);
      yield();
      delay(1800);
    } else {
      displayMessage("Save failed!\nCheck FS or JSON.");
      server.send(500, "text/plain", "Save failed!\nCheck FS or JSON.");
      delay(3000);
    }
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
  ESP.restart();
}

void serverStart() {
  server.on("/", serverHandleRoot);
  server.on("/save", serverHandleSave);
  server.begin();
  serverStarted = true;

  displayMessage("Web Server Started!\n\nWiFi SSID: " + String(espSSID) + "\nPassword: " + String(espPassword) + "\n\nVisit url: \nhttp://" + WiFi.softAPIP().toString() + "/");
}
void handleClient() {
  server.handleClient();
}

void IRAM_ATTR buttonISR() {
  unsigned long currentInterruptTime = millis();
  if (currentInterruptTime - lastInterruptTime > debounceDelay) {
    buttonPressed = true;
    buttonDownTime = millis();
    buttonHeld = true;
    lastInterruptTime = currentInterruptTime;

    sleepCancelled = true;
  }
}

void displayInfo() {
  if (currentPage == 0) {
    displayMessage("Network info, p.1\n\nSSID: " + WiFi.SSID() + "\nBSSID:\n" + WiFi.BSSIDstr() + "\nGateway: " + WiFi.gatewayIP().toString() + "\nRSSI: " + String(WiFi.RSSI()) + " dBm");
  } else {
    String statusStr = WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected";
    String gatewayPingStr = pingGateway >= 0 ? String(pingGateway) + "ms" : "Fail";
    String googlePingStr = pingGoogle >= 0 ? String(pingGoogle) + "ms" : "Fail";

    displayMessage("Network info, p.2\n\nChannel: " + String(WiFi.channel()) + "\nStatus: " + statusStr + "\nGW Ping: " + gatewayPingStr + "\n8.8.8.8: " + googlePingStr);
  }
  if (buttonPressed) {
    buttonPressed = false;
    currentPage = (currentPage + 1) % totalPages;
  }
}

void updatePing() {
  if (WiFi.status() == WL_CONNECTED) {
    IPAddress gateway = WiFi.gatewayIP();
    pingGateway = Ping.ping(gateway, 1) ? Ping.averageTime() : -1;
    pingGoogle = Ping.ping(IPAddress(8,8,8,8), 1) ? Ping.averageTime() : -1;
  } else {
    pingGateway = -1;
    pingGoogle = -1;
  }
}

void goToSleep() {
  displayMessage("Going to sleep...");
  delay(1000);
  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF);

  if (serverStarted) {
    server.stop();
    WiFi.softAPdisconnect(true);
  }
  ESP.deepSleep(0);
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  displayInit();
  initializeLittleFS();

  String wifiSSID, wifiPassword;
  if (!loadConfig(wifiSSID, wifiPassword)) {
    startAP();
    serverHandleRoot();
    serverStart();
  } else {
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(1000);
      displayMessage("Connecting to WiFi...\nAttempt " + String(attempts + 1));
      attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
      displayMessage("WiFi Failed!\nResetting config...");
      delay(500);
      LittleFS.remove("/config.json");
      delay(500);
      ESP.restart();
    }
  }

  startTime = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  if (serverStarted) {
    handleClient();

    if (sleepCancelled) {
      sleepCancelled = false;
      startTime = millis();
    }

    if (!sleepCancelled && (currentMillis - startTime >= sleepDelay)) {
      goToSleep();
    }
    return;
  }
  
  if (currentMillis - lastPingUpdate >= pingUpdateInterval) {
    lastPingUpdate = currentMillis;
    updatePing();
  }

  if (sleepCancelled) {
    sleepCancelled = false;
    startTime = currentMillis;
  }

  displayInfo();

  if (buttonHeld && digitalRead(BUTTON_PIN) == HIGH) {
    buttonHeld = false;
    unsigned long pressDuration = millis() - buttonDownTime;

    if (pressDuration >= longPressDuration) {
      displayMessage("Resetting config...");
      delay(500);

      LittleFS.remove("/config.json");
      delay(500);
      ESP.restart();
    }
  }

  if (!sleepCancelled && (currentMillis - startTime >= sleepDelay)) {
    goToSleep();
  }
}