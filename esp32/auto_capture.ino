#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_timer.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Camera model
#define CAMERA_MODEL_AI_THINKER // ESP32-CAM
#include "camera_pins.h"

// WiFi credentials
const char* ssid = "Abc";
const char* password = "11111111";

// NestJS backend server details
const char* serverUrl = "http://172.20.10.10:3000/images/upload";
const char* serverHost = "172.20.10.10"; // Host for ping test
const int serverPort = 3000; // Port for connection test

// Flash LED pin for ESP32-CAM
#define LED_PIN 4 // GPIO 4 on ESP32-CAM

// Input pin from Arduino UNO to trigger photo capture
#define TRIGGER_PIN 12 // GPIO 12 (you can change to any available GPIO pin)
int lastTriggerState = LOW;

unsigned long lastCaptureTime = 0;
bool manualCapture = false;

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Tắt brownout detector
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  
  // Thiết lập LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Thiết lập pin nhận tín hiệu từ Arduino UNO
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  
  // Test LED để đảm bảo LED hoạt động
  Serial.println("Testing flash LED...");
  digitalWrite(LED_PIN, HIGH); // Bật LED
  delay(1000);               // Giữ sáng 1 giây
  digitalWrite(LED_PIN, LOW);  // Tắt LED
  delay(500);                // Đợi 0.5 giây
  Serial.println("Flash LED test completed");
  
  // Flash LED để báo hiệu khởi động
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }

  // Camera configuration
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Initialize with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA; // 800x600
    config.jpeg_quality = 10;           // 0-63, lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_VGA;  // 640x480
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera initialization failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  // Adjust camera settings
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 0);       // -2 to 2
  s->set_saturation(s, 0);     // -2 to 2
  s->set_gainceiling(s, GAINCEILING_2X);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  
  Serial.print("Connected to WiFi. IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("ESP32-CAM will capture photos when triggered by Arduino UNO signal on GPIO pin " + String(TRIGGER_PIN));
  
  // Flash LED để báo hiệu WiFi đã kết nối thành công
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
}

void loop() {
  // Kiểm tra tín hiệu từ Arduino UNO
  int triggerState = digitalRead(TRIGGER_PIN);
  
  // Phát hiện sự thay đổi trạng thái từ HIGH sang LOW (falling edge)
  if (triggerState == LOW && lastTriggerState == HIGH) {
    Serial.println("Trigger signal received from Arduino UNO!");
    
    Serial.println("Time to capture a new photo!");
    
    // Chụp ảnh
    camera_fb_t* fb = captureImage();
    
    if (fb) {
      // Upload image to backend
      bool uploadSuccess = uploadImage(fb);
      Serial.println(uploadSuccess ? "Image upload successful" : "Image upload failed");
      
      // Return the frame buffer to be reused
      esp_camera_fb_return(fb);
    } else {
      Serial.println("Failed to capture image");
    }
  }
  
  lastTriggerState = triggerState;
  
  // Delay nhỏ để giảm CPU usage
  delay(50);
}

camera_fb_t* captureImage() {
  Serial.println("Taking a photo...");
  
  // Flash LED if available
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  
  // Take a photo
  camera_fb_t* fb = esp_camera_fb_get();
  
  // Turn off LED
  digitalWrite(LED_PIN, LOW);
  
  if (!fb) {
    Serial.println("Camera capture failed");
    return NULL;
  }
  
  Serial.printf("Captured image: %dx%d, size: %d bytes\n", fb->width, fb->height, fb->len);
  return fb;
}

bool uploadImage(camera_fb_t* fb) {
  if (!fb) {
    return false;
  }
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Reconnecting...");
    WiFi.reconnect();
    
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
      delay(500);
      Serial.print(".");
      retries++;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Failed to reconnect to WiFi");
      return false;
    }
    
    Serial.println("Reconnected to WiFi");
  }
  
  // Test server connection before attempting upload
  Serial.print("Testing connection to server at ");
  Serial.println(serverHost);
  
  WiFiClient testClient;
  if (testClient.connect(serverHost, serverPort)) {
    Serial.println("Server connection test successful");
    testClient.stop();
  } else {
    Serial.println("Cannot connect to server. Check server IP and port");
    digitalWrite(LED_PIN, LOW);
    return false;
  }
  
  Serial.println("Uploading image to server...");
  
  // Flash LED để báo hiệu đang tải ảnh lên
  Serial.println("Turning ON flash LED for upload...");
  digitalWrite(LED_PIN, HIGH);
  // Đảm bảo LED được bật với mức logic HIGH
  int ledState = digitalRead(LED_PIN);
  Serial.print("LED state after turning on: ");
  Serial.println(ledState == HIGH ? "HIGH" : "LOW");
  
  WiFiClient wifiClient;
  HTTPClient http;
  
  // Khởi tạo HTTP client
  http.begin(wifiClient, serverUrl);
  
  // Chuẩn bị dữ liệu multipart form
  String boundary = "ESP32CameraBoundary";
  String contentType = "multipart/form-data; boundary=" + boundary;
  http.addHeader("Content-Type", contentType);
  
  // Tạo phần đầu và phần cuối của form
  String head = "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"image\"; filename=\"esp32cam.jpg\"\r\n";
  head += "Content-Type: image/jpeg\r\n\r\n";
  
  String tail = "\r\n--" + boundary + "--\r\n";
  
  // Tính toán tổng kích thước
  uint32_t imageLen = fb->len;
  uint32_t extraLen = head.length() + tail.length();
  uint32_t totalLen = imageLen + extraLen;
  
  // Thiết lập header Content-Length
  http.addHeader("Content-Length", String(totalLen));
  
  // Tạo buffer cho toàn bộ dữ liệu multipart
  uint8_t *buffer = (uint8_t*)malloc(totalLen);
  if (!buffer) {
    Serial.println("Not enough memory to allocate buffer");
    digitalWrite(LED_PIN, LOW);
    return false;
  }
  
  // Sao chép head vào buffer
  uint32_t pos = 0;
  memcpy(buffer, head.c_str(), head.length());
  pos += head.length();
  
  // Sao chép hình ảnh vào buffer
  memcpy(buffer + pos, fb->buf, fb->len);
  pos += fb->len;
  
  // Sao chép tail vào buffer
  memcpy(buffer + pos, tail.c_str(), tail.length());
  
  // Gửi POST request với dữ liệu buffer
  int httpResponseCode = http.POST(buffer, totalLen);
  
  // Giải phóng bộ nhớ
  free(buffer);
  
  // Tắt LED khi hoàn thành
  digitalWrite(LED_PIN, LOW);
  
  // Kiểm tra kết quả
  if (httpResponseCode >= 200 && httpResponseCode < 300) {
    String response = http.getString();
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.println(response);
    http.end();
    return true;
  } else {
    Serial.print("Error uploading image. HTTP Response code: ");
    Serial.println(httpResponseCode);
    
    // Chi tiết lỗi HTTP
    if (httpResponseCode == -1) {
      Serial.println("Failed to connect to server. Possible reasons:");
      Serial.println("1. Server is not running");
      Serial.println("2. Server IP or port is incorrect");
      Serial.println("3. Firewall is blocking the connection");
      Serial.println("4. Server and ESP32 are on different networks");
    } else if (httpResponseCode == 400) {
      Serial.println("Bad request - server couldn't understand the request");
    } else if (httpResponseCode == 404) {
      Serial.println("Endpoint not found - check your server URL");
    } else if (httpResponseCode >= 500) {
      Serial.println("Server error - the server failed to fulfill an apparently valid request");
    }
    
    String response = http.getString();
    Serial.println(response);
    http.end();
    return false;
  }
} 