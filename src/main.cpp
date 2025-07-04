#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
#define OLED_SDA 14 // D6
#define OLED_SCL 12 // D5

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "";
const char* password = "";

void displayInit() {
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (true);
  }
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.println("\n\n   Initializing...");
  display.display();
}

void setup() {
  displayInit();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Connected!");
  display.display();
}

void loop() {
}