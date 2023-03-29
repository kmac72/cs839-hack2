#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"

#define PRESSURE_PIN 25
#define DOOR_PIN 27
#define LED_PIN 12
#define POTENT_PIN 32
#define TAG_PIN 13

const char* ntpServer = "pool.ntp.org";
unsigned long epochTime; 

const char* ssid = "";
const char* password = "";

WiFiClient client;

bool text_sent = false;
int user_tag = 1;

int pressure_state;
int door_prev;
int door_state;
int tag_prev;
int tag_state = LOW;

unsigned long prev_timestamp;

void setup() {

  Serial.begin(9600);

  pinMode(PRESSURE_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(TAG_PIN, INPUT_PULLUP);
  tag_state = digitalRead(TAG_PIN);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected"); 

    configTime(0, 0, ntpServer);
    prev_timestamp = millis();
}

float run_model(int user_tag, int pressure_state, int door_state) {

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return 0;
  }

  // Serial.print(timeinfo.tm_hour);
  // Serial.print(":");
  // Serial.print(timeinfo.tm_min);
  // Serial.println();

  float norm_time = (((float)timeinfo.tm_hour - 5) * 60 + (float)timeinfo.tm_min) / (23 * 60 + 59);
  Serial.print("Normalized time: ");
  Serial.println(norm_time);
  //0-6 Mon-Sun

  int pressure_inv = 1;
  if(pressure_state == 1) {
    pressure_inv = 0;
  }

  Serial.print("Wallet: ");
  Serial.println(pressure_inv);
  Serial.print("Fridge: ");
  Serial.println(door_state);
  
  int weekday = 0;  // Monday

  float result_1 = (-1.51901847e-01 * norm_time) + (1.11010576e-03 * weekday) + (1.53562454e+00 * pressure_inv) + (1.54808714e-01 * door_state) + (9.92987854e-01 * 0) + -0.56029411;
  float result_2 = (-0.66003516 * .48) + (0.01156815 * weekday) + 0.3077989;
  result_1 = 1 / (1 + exp(-result_1));
  result_2 = 1 / (1 + exp(-result_2));
  return ((result_1 + result_2) / 2);
}

void log_to_spreadsheet() {

  int pressure_log = 1;
  if(pressure_state == 1) {
    pressure_log = 0;
  }

  String url = "http://maker.ifttt.com/trigger/UserData/with/key/b_qtMpsjjEK1WKy0oyDfu5";
  // Serial.println("Logging data to spreadsheet");
  HTTPClient http;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String values = String("value1=" + String(user_tag) + "&value2=" + String(door_state) + "&value3=" + String(pressure_log));
  int httpResponseCode = http.POST(values);
  // Serial.println(httpResponseCode);
  http.end();
}

void loop() {
  
  pressure_state = digitalRead(PRESSURE_PIN);
  door_state = digitalRead(DOOR_PIN);
  tag_prev = tag_state;
  tag_state = digitalRead(TAG_PIN);
  int potent_value = analogRead(POTENT_PIN);

  if(tag_state == HIGH && tag_prev == LOW) {

    if(user_tag) {
      user_tag = 0;
    } else {
      user_tag = 1;
    }

    Serial.print("User tag changed: ");
    Serial.println(user_tag);
  }

  if(potent_value >= 200 /*&& (pressure_state == LOW || door_state == HIGH)*/) {

    digitalWrite(LED_PIN, HIGH);

    if(!text_sent) {

      // String url = "http://maker.ifttt.com/trigger/out_of_door_notification/with/key/b_qtMpsjjEK1WKy0oyDfu5";
      // Serial.println("Requesting URL");
      // HTTPClient http;
      // http.begin(client, url);
      // http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      // int httpResponseCode = http.POST("value1=Fridge");
      // Serial.println(httpResponseCode);
      // http.end();

      text_sent = true;
      delay(20);
      // Serial.println("Alert sent");
      log_to_spreadsheet();
      float model_state = run_model(user_tag, pressure_state, door_state);
      Serial.print("Model result: ");
      Serial.println(model_state);
      // 0 away, 1 home
      if(model_state < .5) {
        Serial.println("User away, send alert if needed");
      } else {
        Serial.println("User home, no alerts");
      }
    }

  } else {

    text_sent = false;
    delay(20);
    digitalWrite(LED_PIN, LOW);
  }

  unsigned long curr_timestamp = millis();
  if(curr_timestamp - prev_timestamp > 10000/*600000*/) {

    // 10 minutes elapsed, trigger log to spreadsheet
    // Serial.println("Logging to spreadsheet");
    log_to_spreadsheet();
    prev_timestamp = millis();
  }
}
