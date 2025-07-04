#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

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

// Function to initialize the OLED display
// This function initializes the display, clears it, sets the cursor position, and prints a message
// If the display initialization fails, it enters an infinite loop
void displayInit() {
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (true);
  }
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.println("\n\nWiFi Monitor started");
  display.display();
}

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

void startAP() {
  IPAddress local_ip(192, 168, 4, 1); // Set the local IP address for the AP
  IPAddress gateway(192, 168, 4, 1);   // Set the gateway address
  IPAddress subnet(255, 255, 255, 0);  // Set the subnet mask
  WiFi.softAPConfig(local_ip, gateway, subnet); // Configure the AP with the specified IP, gateway, and subnet
  WiFi.softAP(ssid, password); // Start the AP with the specified SSID and password
  displayMessage("AP Started\n\nSSID: " + String(ssid) + "\nPassword: " + String(password) + 
                 "\nIP: " + local_ip.toString()); // Display the AP details on the OLED screen
}

void setup() {
  displayInit(); // Initialize the OLED display
  startAP(); // Start the Access Point (AP) mode
}

void loop() {
}