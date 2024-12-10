#include "esp_camera.h"
#include <WiFi.h>
#include <ESP32Servo.h>
#include <HTTPClient.h>

// Ultrasonic Sensor Pins
#define TRIG_PIN 12
#define ECHO_PIN 13

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const char* ssid = "Usaamahwifi";
const char* password = "Usaamah2";
const char* apiKey = "UE2GBZ0DR1QNFS0G"; // Replace with your ThingSpeak API Key
const char* server = "https://api.thingspeak.com/update?api_key=UE2GBZ0DR1QNFS0G&field1=0";

WiFiServer httpServer(80);
WiFiClient live_client;

// Servo control
Servo myServo;
int servoPos = 90;  // Default servo position

// Ultrasonic Sensor Variables
int objectCount = 0;
unsigned long lastSendTime = 0;

String index_html = "<meta charset=\"utf-8\"/>\n" \
                    "<style>\n" \
                    "#content {\n" \
                    "display: flex;\n" \
                    "flex-direction: column;\n" \
                    "justify-content: center;\n" \
                    "align-items: center;\n" \
                    "text-align: center;\n" \
                    "min-height: 100vh;}\n" \
                    "</style>\n" \
                    "<body bgcolor=\"#000000\"><div id=\"content\"><h2 style=\"color:#ffffff\">CiferTech LIVE</h2><img src=\"video\"><br>" \
                    "<button onclick=\"moveServo('left')\">Move Left</button><br>" \
                    "<button onclick=\"moveServo('right')\">Move Right</button></div>" \
                    "<script>\n" \
                    "function moveServo(direction) {\n" \
                    "  var xhr = new XMLHttpRequest();\n" \
                    "  xhr.open('GET', '/' + direction, true);\n" \
                    "  xhr.send();\n" \
                    "}\n" \
                    "</script></body>";

void configCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 9;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_vflip(s, 1); 
    s->set_hmirror(s, 1); 
  } else {
    Serial.println("Failed to get sensor object");
  }
}

void sendToThingSpeak(int count) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "?api_key=" + apiKey + "&field1=" + String(count);
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.println("ThingSpeak Response: " + String(httpResponseCode));
    } else {
      Serial.println("Error sending data to ThingSpeak");
    }
    http.end();
  }
}

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return (duration * 0.034 / 2);
}

void checkDistance() {
  float distance = measureDistance();
  if (distance < 10.0) {
    objectCount++;
    Serial.println("Object detected! Count: " + String(objectCount));
    if (millis() - lastSendTime > 15000) { // Send data every 15 seconds
      sendToThingSpeak(objectCount);
      lastSendTime = millis();
    }
  }
}

void liveCam(WiFiClient &client) {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Frame buffer could not be acquired");
    return;
  }
  client.print("--frame\n");
  client.print("Content-Type: image/jpeg\n\n");
  client.flush();
  client.write(fb->buf, fb->len);
  client.flush();
  client.print("\n");
  esp_camera_fb_return(fb);
}

void http_resp() {
  WiFiClient client = httpServer.available();
  if (client.connected()) {
    String req = "";
    while (client.available()) {
      req += (char)client.read();
    }
    Serial.println("request " + req);
    int addr_start = req.indexOf("GET") + strlen("GET");
    int addr_end = req.indexOf("HTTP", addr_start);
    if (addr_start == -1 || addr_end == -1) {
      Serial.println("Invalid request " + req);
      return;
    }
    req = req.substring(addr_start, addr_end);
    req.trim();
    Serial.println("Request: " + req);
    client.flush();

    String s;
    if (req == "/") {
      s = "HTTP/1.1 200 OK\n";
      s += "Content-Type: text/html\n\n";
      s += index_html;
      s += "\n";
      client.print(s);
      client.stop();
    }
    else if (req == "/video") {
      live_client = client;
      live_client.print("HTTP/1.1 200 OK\n");
      live_client.print("Content-Type: multipart/x-mixed-replace; boundary=frame\n\n");
      live_client.flush();
    }
    else if (req == "/left") {
      servoPos = max(0, servoPos - 10);
      myServo.write(servoPos);
      client.print("HTTP/1.1 200 OK\n\n");
      client.stop();
    }
    else if (req == "/right") {
      servoPos = min(180, servoPos + 10);
      myServo.write(servoPos);
      client.print("HTTP/1.1 200 OK\n\n");
      client.stop();
    }
    else {
      s = "HTTP/1.1 404 Not Found\n\n";
      client.print(s);
      client.stop();
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi Connected. IP: " + WiFi.localIP().toString());

  httpServer.begin();
  configCamera();

  myServo.attach(14); 
  myServo.write(servoPos); 
}

void loop() {
  checkDistance();
  http_resp();
  if (live_client.connected()) {
    liveCam(live_client);
  }
}
