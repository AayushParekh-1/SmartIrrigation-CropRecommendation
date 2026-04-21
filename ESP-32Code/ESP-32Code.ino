#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// --- Config ---
const char* ssid = "OnePlus 9R";
const char* password = "87651234";
const char* serverURL = "http://10.44.31.60:5001/data";  // ← replace with your actual IP

#define DHTPIN 4
#define DHTTYPE DHT11
#define SOIL_PIN 34
#define LED_PIN 2
#define MOISTURE_THRESHOLD 40

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=============================");
  Serial.println("  Smart Irrigation Starting  ");
  Serial.println("=============================");

  dht.begin();
  pinMode(LED_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Sending data to: ");
  Serial.println(serverURL);
  Serial.println("-----------------------------");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!");
  delay(1500);
  lcd.clear();
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // Check if DHT11 is working
  if (isnan(temp) || isnan(hum)) {
    Serial.println("ERROR: DHT11 read failed! Check wiring on GPIO4.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("DHT11 ERROR!");
    delay(2000);
    return;  // skip this loop cycle, try again next second
  }

  // Soil moisture: dry = high ADC, wet = low ADC
  int rawSoil = analogRead(SOIL_PIN);
  int moisture = map(rawSoil, 4095, 1500, 0, 100);
  moisture = constrain(moisture, 0, 100);

  bool pumpOn = moisture < MOISTURE_THRESHOLD;
  digitalWrite(LED_PIN, pumpOn ? HIGH : LOW);

  // --- Serial Monitor output ---
  Serial.println("-----------------------------");
  Serial.print("Temperature : ");
  Serial.print(temp);
  Serial.println(" °C");
  Serial.print("Humidity    : ");
  Serial.print(hum);
  Serial.println(" %");
  Serial.print("Soil Raw ADC: ");
  Serial.println(rawSoil);
  Serial.print("Moisture    : ");
  Serial.print(moisture);
  Serial.println(" %");
  Serial.print("Pump/LED    : ");
  Serial.println(pumpOn ? "ON" : "OFF");

  // --- LCD display ---
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("S:");
  lcd.print(moisture);
  lcd.print("% T:");
  lcd.print((int)temp);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("H:");
  lcd.print((int)hum);
  lcd.print("% W:");
  lcd.print(pumpOn ? "ON " : "OFF");

  // --- Send to Flask ---
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<128> doc;
    doc["moisture"] = moisture;
    doc["temperature"] = temp;
    doc["humidity"] = hum;
    doc["pump"] = pumpOn ? "ON" : "OFF";

    String body;
    serializeJson(doc, body);

    Serial.print("Sending to Flask... ");
    int code = http.POST(body);

    if (code == 200) {
      Serial.println("Success (200 OK)");
    } else if (code < 0) {
      Serial.println("FAILED — is Flask running? Is the IP correct?");
    } else {
      Serial.print("Got HTTP code: ");
      Serial.println(code);
    }

    http.end();
  } else {
    Serial.println("WiFi disconnected! Skipping POST.");
  }

  delay(1000);
}
