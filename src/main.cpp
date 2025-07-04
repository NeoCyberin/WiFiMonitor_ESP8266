#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

#define SCREEN_WIDTH 128  // Define the width and height of the OLED display
#define SCREEN_HEIGHT 64 // Define the width and height of the OLED display

#define OLED_RESET     -1   // Reset pin for the OLED display, -1 means no reset pin is used
#define SCREEN_ADDRESS 0x3C // I2C address for the OLED display
#define OLED_SDA 14         // Define the SDA pin for I2C communication (D6 on NodeMCU)
#define OLED_SCL 12         // Define the SCL pin for I2C communication (D5 on NodeMCU)

// Create an instance of the OLED display
// This instance is created with the specified screen width, height, and I2C interface
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Wi-Fi credentials
// Replace with your network credentials
const char* ssid = "HW-364A";
const char* password = "1234567890";

ESP8266WebServer server(80); // Create a web server on port 80
bool serverStarted = false; // Flag to check if the server has started

// Function to display a message on the OLED screen
// This function clears the display, sets the cursor position, prints the message, and updates the
void displayMessage(const char* message) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(message);
  display.display();
}

// Overloaded function to display a message on the OLED screen
// This function takes a String object as an argument, clears the display, sets the cursor position
void displayMessage(const String& message) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(message);
  display.display();
}

// Function to initialize the OLED display
// This function initializes the display, clears it, sets the cursor position, and prints a message
// If the display initialization fails, it enters an infinite loop
void displayInit() {
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (true);
  }
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  displayMessage("\n\nWiFi Monitor started");
}

// Function to initialize the LittleFS file system
// This function attempts to mount the LittleFS file system and displays an error message if it fails
void initializeLittleFS() {
  if (!LittleFS.begin()) {
    displayMessage("Failed to mount FS");
    return;
  }
}

// Function to start the Access Point (AP) mode
// This function configures the AP with a local IP address, gateway, and subnet mask
void startAP() {
  IPAddress local_ip(192, 168, 4, 1); // Set the local IP address for the AP
  IPAddress gateway(192, 168, 4, 1);   // Set the gateway address
  IPAddress subnet(255, 255, 255, 0);  // Set the subnet mask
  
  WiFi.softAPConfig(local_ip, gateway, subnet); // Configure the AP with the specified IP, gateway, and subnet
  WiFi.softAP(ssid, password); // Start the AP with the specified SSID and password
}

// Function to handle the root URL ("/") request
// This function serves the index.html file from the LittleFS file system
void serverHandleRoot() {   
  if (LittleFS.exists("/wwwroot/index.html")) {
    File indexFile = LittleFS.open("/wwwroot/index.html", "r");
    server.streamFile(indexFile, "text/html");
    indexFile.close();
  } else {
    server.send(404, "text/plain", "404: File Not Found");
  }
}

void serverStart() {
  server.on("/", serverHandleRoot); // Handle requests to the root URL ("/")
  server.onNotFound([]() { // Handle requests to URLs that are not found
    server.send(404, "text/plain", "404: Not Found");
  });
  server.begin(); // Start the web server
  serverStarted = true; // Set the flag to indicate that the server has started

  displayMessage("Web Server Started!\n\nWiFi SSID: " + String(ssid) + "\nPassword: " + String(password) + "\n\nVisit url: \nhttp://" + WiFi.softAPIP().toString() + "/"); // Display the web server details on the OLED screen
}

void handleClient() {
  if (serverStarted) {
    server.handleClient();
  }
}

void setup() {
  displayInit(); // Initialize the OLED display

  initializeLittleFS(); // Initialize the LittleFS file system

  startAP(); // Start the Access Point (AP) mode
  serverHandleRoot(); // Handle requests to the root URL ("/")
  serverStart(); // Start the web server
}

void loop() {
  handleClient(); // Handle incoming client requests
}