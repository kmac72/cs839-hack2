#include <WiFi.h>
#include <HTTPClient.h>

#define PRESSURE_PIN 25
#define DOOR_PIN 27
#define LED_PIN 12
#define POTENT_PIN 32

const char* ssid = "redacted";
const char* password = "redacted";

WiFiClient client;
bool changeState = false;

int potent_value;
int prev_state = LOW;

void setup() {

  Serial.begin(9600);

  pinMode(PRESSURE_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected"); 
}

void loop() {
  
  int pressure_state = digitalRead(PRESSURE_PIN);
  int door_state = digitalRead(DOOR_PIN);

  int potent_value = analogRead(POTENT_PIN);
  // Serial.print("Potent angle ");
  // Serial.println(potent_value);

  if(potent_value >= 200 && (pressure_state == LOW || door_state == HIGH)) {

    digitalWrite(LED_PIN, HIGH);

    if(door_state == HIGH && prev_state == LOW) {

      // String url = "http://maker.ifttt.com/trigger/fridge_open/with/key/b_qtMpsjjEK1WKy0oyDfu5";      
      String url = "http://maker.ifttt.com/trigger/out_of_door_notification/with/key/b_qtMpsjjEK1WKy0oyDfu5";
      Serial.println("Requesting URL");
      HTTPClient http;
      http.begin(client, url);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      int httpResponseCode = http.POST("value1=Fridge");
      Serial.println(httpResponseCode);
      http.end();

      prev_state = HIGH;
    }

  } else {

    digitalWrite(LED_PIN, LOW);

    prev_state = LOW;
  }
}
