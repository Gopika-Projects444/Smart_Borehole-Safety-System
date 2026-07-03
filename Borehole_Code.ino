#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP32Servo.h>

// -------- WiFi --------
const char* ssid = "Gopika";
const char* password = "Aurelia20";
String apiKey = "KRWMY6MFB7YFCK33";

// -------- Telegram --------
String botToken = "8660513754:AAFumXmlDZtNwBRVSa4yz2x5Z3_rFNUA5tw";
String chatID = "6645473603";

// -------- Pins --------
int pirPins[4] = {13, 12, 14, 27};
int redLED = 25;
int greenLED = 33;
int buzzer = 32;

Servo gate;

// -------- Variables --------
int motion = 0;
unsigned long lastTime = 0;
unsigned long interval = 15000;

bool sent = false;

// 🔥 HOLD VARIABLE
unsigned long redStartTime = 0;
bool redHold = false;

// 🔥 BLINK TIMER
unsigned long blinkTime = 0;
bool ledState = false;

void setup() {
  Serial.begin(115200);

  for(int i = 0; i < 4; i++){
    pinMode(pirPins[i], INPUT);
  }

  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(buzzer, OUTPUT);

  gate.attach(18);

  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");

  delay(30000); // PIR stabilize
}

void loop() {

  int count = 0;

  for(int i = 0; i < 4; i++){
    if(digitalRead(pirPins[i]) == HIGH){
      count++;
    }
  }

  if(count >= 2){

    int stableCount = 0;

    for(int j = 0; j < 3; j++){
      int tempCount = 0;

      for(int i = 0; i < 4; i++){
        if(digitalRead(pirPins[i]) == HIGH){
          tempCount++;
        }
      }

      if(tempCount >= 2){
        stableCount++;
      }

      delay(50);
    }

    if(stableCount >= 2){
      motion = 1;
    } else {
      motion = 0;
    }

  } else {
    motion = 0;
  }

  int gateStatus;

  // 🔥 START TIMER
  if (motion == 1 && !redHold) {
    redStartTime = millis();
    redHold = true;
  }

  // 🔴 HUMAN → RED BLINK 10 sec
  if (redHold && (millis() - redStartTime <= 10000)) {

    if (millis() - blinkTime > 300) {
      blinkTime = millis();
      ledState = !ledState;
      digitalWrite(redLED, ledState);
    }

    digitalWrite(greenLED, LOW);
    digitalWrite(buzzer, HIGH);

    gate.write(180);
    gateStatus = 0;

    Serial.println("Human Detected (Blinking)");

    if (!sent) {
      sendMsg();
      sent = true;
    }

  } else {

    redHold = false;

    // 🟢 NO HUMAN → GREEN BLINK
    if (millis() - blinkTime > 300) {
      blinkTime = millis();
      ledState = !ledState;
      digitalWrite(greenLED, ledState);
    }

    digitalWrite(redLED, LOW);
    digitalWrite(buzzer, LOW);

    gate.write(0);
    gateStatus = 1;

    Serial.println("No Human");

    sent = false;
  }

  Serial.print("Gate: ");
  Serial.println(gateStatus);

  if (millis() - lastTime > interval) {
    lastTime = millis();
    sendToThingSpeak(motion, gateStatus);
  }

  delay(100);
}

// -------- THINGSPEAK --------
void sendToThingSpeak(int motion, int gate) {

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    String url = "http://api.thingspeak.com/update?api_key=" + apiKey +
                 "&field1=" + String(motion) +
                 "&field2=" + String(gate);

    http.begin(url);
    int code = http.GET();

    Serial.print("ThingSpeak: ");
    Serial.println(code);

    http.end();
  }
}

// -------- TELEGRAM --------
void sendMsg() {

  if (WiFi.status() == WL_CONNECTED) {

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;

    String message = "Motion Detected Gate Closed";

    String url = "https://api.telegram.org/bot" + botToken +
                 "/sendMessage?chat_id=" + chatID +
                 "&text=" + message;

    http.begin(client, url);
    int code = http.GET();

    Serial.print("Telegram: ");
    Serial.println(code);

    http.end();
  }
}