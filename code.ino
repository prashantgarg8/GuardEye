#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <base64.h>
#include "esp_timer.h"
#include "esp32-hal-ledc.h"

#define PIR_SENSOR_PIN 13
#define FLASH_LED_PIN 4
#define DOOR_SENSOR_PIN 12
#define DEBUG_MODE false
#define MOTION_COOLDOWN_MS 15000

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* TG_BOT_TOKEN = "YOUR_BOT_TOKEN";
const char* TG_CHAT_ID = "YOUR_CHAT_ID";

String IMAGGA_USER = "YOUR_IMAGGA_USER";
String IMAGGA_SECRET = "YOUR_IMAGGA_SECRET";

WiFiClientSecure SecureClient;
unsigned long lastMotion = 0;

void initGuardEyeCamera();
void initGuardEyeWiFi();
void sendToTelegram(camera_fb_t* fb, const String& caption);
String analyzeWithImagga(camera_fb_t* fb);
void guardLog(const String& msg);

void setup() {
  Serial.begin(115200);
  pinMode(PIR_SENSOR_PIN, INPUT);
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);
  initGuardEyeWiFi();
  initGuardEyeCamera();
  guardLog("GuardEye Ready");
}

void loop() {
  static bool lastDoorState = HIGH;
  bool currentDoorState = digitalRead(DOOR_SENSOR_PIN);
  if (currentDoorState != lastDoorState) {
    lastDoorState = currentDoorState;
    if (currentDoorState == HIGH) {
      guardLog("Door or Window OPENED");
      camera_fb_t* fb = esp_camera_fb_get();
      if (fb) {
        sendToTelegram(fb, "Door/Window OPENED");
        esp_camera_fb_return(fb);
      }
    } else {
      guardLog("Door or Window CLOSED");
    }
    delay(150);
  }
  if (digitalRead(PIR_SENSOR_PIN) == HIGH && millis() - lastMotion > MOTION_COOLDOWN_MS) {
    lastMotion = millis();
    guardLog("Motion detected");
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(300);
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      guardLog("Camera capture failed");
      digitalWrite(FLASH_LED_PIN, LOW);
      return;
    }
    digitalWrite(FLASH_LED_PIN, LOW);
    String label = analyzeWithImagga(fb);
    String caption;
    if (label.startsWith("person:")) {
      caption = "Person Detected (" + label.substring(label.indexOf(":") + 1) + "%)";
    } else if (label.startsWith("animal:")) {
      caption = "Animal Detected (" + label.substring(label.indexOf(":") + 1) + "%)";
    } else if (label == "imagga_fail" || label == "wifi_fail" || label == "json_fail") {
      caption = "Classification failed";
    } else {
      caption = "Motion Detected (Unknown)";
    }
    sendToTelegram(fb, caption);
    esp_camera_fb_return(fb);
  }
}

void initGuardEyeWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setSleep(false);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi Connected: " + WiFi.localIP().toString());
}

void initGuardEyeCamera() {
  camera_config_t camConfig;
  camConfig.ledc_channel = LEDC_CHANNEL_0;
  camConfig.ledc_timer = LEDC_TIMER_0;
  camConfig.pin_d0 = 5;
  camConfig.pin_d1 = 18;
  camConfig.pin_d2 = 19;
  camConfig.pin_d3 = 21;
  camConfig.pin_d4 = 36;
  camConfig.pin_d5 = 39;
  camConfig.pin_d6 = 34;
  camConfig.pin_d7 = 35;
  camConfig.pin_xclk = 0;
  camConfig.pin_pclk = 22;
  camConfig.pin_vsync = 25;
  camConfig.pin_href = 23;
  camConfig.pin_sscb_sda = 26;
  camConfig.pin_sscb_scl = 27;
  camConfig.pin_pwdn = 32;
  camConfig.pin_reset = -1;
  camConfig.xclk_freq_hz = 20000000;
  camConfig.pixel_format = PIXFORMAT_JPEG;
  camConfig.frame_size = FRAMESIZE_QVGA;
  camConfig.jpeg_quality = 12;
  camConfig.fb_count = 2;
  camConfig.fb_location = CAMERA_FB_IN_PSRAM;
  if (esp_camera_init(&camConfig) != ESP_OK) {
    guardLog("Camera initialization failed");
    return;
  }
  guardLog("Camera initialized");
}

void sendToTelegram(camera_fb_t* fb, const String& caption) {
  if (WiFi.status() != WL_CONNECTED || !fb) return;
  SecureClient.stop();
  SecureClient.setInsecure();
  if (!SecureClient.connect("api.telegram.org", 443)) {
    Serial.println("Telegram connection failed");
    return;
  }
  String boundary = "GuardEyeBoundary";
  String start =
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" +
    String(TG_CHAT_ID) + "\r\n--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"caption\"\r\n\r\n" +
    caption + "\r\n--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"photo\"; filename=\"guardeye.jpg\"\r\n"
    "Content-Type: image/jpeg\r\n\r\n";
  String end = "\r\n--" + boundary + "--\r\n";
  int totalLen = start.length() + fb->len + end.length();
  String header =
    "POST /bot" + String(TG_BOT_TOKEN) + "/sendPhoto HTTP/1.1\r\n"
    "Host: api.telegram.org\r\n"
    "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n"
    "Content-Length: " + String(totalLen) + "\r\n\r\n";
  SecureClient.print(header);
  SecureClient.print(start);
  SecureClient.write(fb->buf, fb->len);
  SecureClient.print(end);
  delay(500);
  SecureClient.stop();
  Serial.println("Image sent to Telegram");
}

String analyzeWithImagga(camera_fb_t* fb) {
  if (!fb || fb->len == 0) return "no_image";
  if (WiFi.status() != WL_CONNECTED) return "wifi_fail";
  WiFiClientSecure imaggaClient;
  imaggaClient.setInsecure();
  if (!imaggaClient.connect("api.imagga.com", 443)) {
    guardLog("Failed to connect to Imagga");
    return "imagga_fail";
  }
  String boundary = "GuardEyeUploadBoundary";
  String contentType = "multipart/form-data; boundary=" + boundary;
  String bodyStart =
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"image\"; filename=\"capture.jpg\"\r\n"
    "Content-Type: image/jpeg\r\n\r\n";
  String bodyEnd = "\r\n--" + boundary + "--\r\n";
  int contentLen = bodyStart.length() + fb->len + bodyEnd.length();
  String auth = base64::encode(IMAGGA_USER + ":" + IMAGGA_SECRET);
  imaggaClient.printf(
    "POST /v2/uploads HTTP/1.1\r\n"
    "Host: api.imagga.com\r\n"
    "Authorization: Basic %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n\r\n",
    auth.c_str(), contentType.c_str(), contentLen
  );
  imaggaClient.print(bodyStart);
  imaggaClient.write(fb->buf, fb->len);
  imaggaClient.print(bodyEnd);
  String response;
  long timeout = millis() + 7000;
  while (imaggaClient.connected() && millis() < timeout) {
    while (imaggaClient.available()) response += char(imaggaClient.read());
  }
  imaggaClient.stop();
  if (DEBUG_MODE) {
    Serial.println("[UPLOAD RESPONSE]");
    Serial.println(response);
  }
  int jsonStart = response.indexOf('{');
  if (jsonStart == -1) return "json_fail";
  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, response.substring(jsonStart))) return "json_fail";
  String uploadId = doc["result"]["upload_id"] | "";
  if (uploadId == "") return "imagga_fail";
  if (DEBUG_MODE) Serial.println("Upload ID: " + uploadId);
  HTTPClient http;
  WiFiClientSecure tagClient;
  tagClient.setInsecure();
  String tagUrl = "https://api.imagga.com/v2/tags?image_upload_id=" + uploadId;
  http.begin(tagClient, tagUrl);
  http.addHeader("Authorization", "Basic " + auth);
  int code = http.GET();
  String tagResponse = http.getString();
  http.end();
  if (DEBUG_MODE) {
    Serial.println("[TAGS CODE] " + String(code));
    Serial.println("[TAGS RESPONSE] " + tagResponse);
  }
  if (code != 200) return "imagga_fail";
  DynamicJsonDocument tagDoc(2048);
  if (deserializeJson(tagDoc, tagResponse)) return "json_fail";
  JsonArray tags = tagDoc["result"]["tags"];
  for (JsonObject tag : tags) {
    String name = tag["tag"]["en"];
    float conf = tag["confidence"];
    name.toLowerCase();
    if (name.indexOf("person") != -1 || name.indexOf("human") != -1 ||
        name.indexOf("man") != -1 || name.indexOf("woman") != -1)
      return "person:" + String(conf, 1);
    if (name.indexOf("dog") != -1 || name.indexOf("cat") != -1 ||
        name.indexOf("animal") != -1 || name.indexOf("puppy") != -1 ||
        name.indexOf("kitten") != -1)
      return "animal:" + String(conf, 1);
  }
  return "no_person";
}

void guardLog(const String& msg) {
  Serial.println(msg);
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://api.telegram.org/bot" + String(TG_BOT_TOKEN) + "/sendMessage");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String body = "chat_id=" + String(TG_CHAT_ID) + "&text=" + msg;
    http.POST(body);
    http.end();
  }
}
