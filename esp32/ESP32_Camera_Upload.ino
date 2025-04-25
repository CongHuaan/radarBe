#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_timer.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Camera model selection
#define CAMERA_MODEL_AI_THINKER // ESP32-CAM
#include "camera_pins.h"

// WiFi credentials - Cập nhật thông tin WiFi của bạn
const char* ssid = "Abc";
const char* password = "11111111";

// NestJS backend server details - Thay IP bằng địa chỉ IP của máy tính chạy server
const char* serverUrl = "http://192.168.1.5:3000/images/upload"; // Thay địa chỉ IP này bằng IP máy tính của bạn
const int captureInterval = 30000; // Chụp ảnh mỗi 30 giây

unsigned long lastCaptureTime = 0;

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Tắt brownout detector
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

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
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Check if it's time to capture and upload an image
  if (currentMillis - lastCaptureTime >= captureInterval) {
    lastCaptureTime = currentMillis;
    
    // Capture image
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
  
  // Small delay
  delay(100);
}

camera_fb_t* captureImage() {
  Serial.println("Taking a photo...");
  
  // Flash LED if available
  #if defined(LED_GPIO_NUM)
  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, HIGH);
  delay(100);
  #endif
  
  // Take a photo
  camera_fb_t* fb = esp_camera_fb_get();
  
  // Turn off LED
  #if defined(LED_GPIO_NUM)
  digitalWrite(LED_GPIO_NUM, LOW);
  #endif
  
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
  
  Serial.println("Uploading image to server...");
  
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
    String response = http.getString();
    Serial.println(response);
    http.end();
    return false;
  }
} 