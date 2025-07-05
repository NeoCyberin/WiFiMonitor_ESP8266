#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64

#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
#define OLED_SDA 14
#define OLED_SCL 12

#define BUTTON_PIN D4 // GPIO2

volatile bool buttonPressed = false;

const int totalPages = 2;
volatile int currentPage = 0;

const char* espSSID = "HW-364A";
const char* espPassword = "1234567890";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
ESP8266WebServer server(80);
bool serverStarted = false;

unsigned long lastPressTime = 0;
const unsigned long debounceDelay = 500;
volatile unsigned long lastInterruptTime = 0;

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

void displayMessage(const char* message) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(message);
  display.display();
}

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
    } else {
      displayMessage("Failed to save configuration!");
      server.send(500, "text/plain", "Failed to save configuration!");
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
  if (serverStarted) {
    server.handleClient();
  }
}

void IRAM_ATTR buttonISR() {
  unsigned long currentInterruptTime = millis();
  if (currentInterruptTime - lastInterruptTime > debounceDelay) {
    buttonPressed = true;
    lastInterruptTime = currentInterruptTime;
  }
}

void updateDisplay() {
  if (currentPage == 0) {
    displayMessage("Network info, p.1 \n\n" + WiFi.SSID() + "\n" + WiFi.BSSIDstr() + "\n" + WiFi.gatewayIP().toString() + "\nRSSI: " + String(WiFi.RSSI()) + " dBm");
  } else {
    displayMessage("Network info, p.2 \n\nChannel: " + String(WiFi.channel()));
  }
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
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      displayMessage("Connecting to WiFi...");
    }
  }
}

void loop() {
  handleClient();
  if (WiFi.status() == WL_CONNECTED) {
    // Update display with WiFi information
    updateDisplay();
    if (buttonPressed) {
      buttonPressed = false;
      currentPage = (currentPage + 1) % totalPages;
    }
  }
}