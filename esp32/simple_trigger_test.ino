#include "esp_camera.h"
#include <WiFi.h>

// Camera model
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// Chân GPIO nhận tín hiệu từ Arduino
#define TRIGGER_PIN 13
#define LED_PIN 33  // LED_GPIO_NUM cho AI Thinker

void setup() {
  Serial.begin(115200);
  delay(1000); // Đợi serial ổn định
  
  // Thiết lập chân
  pinMode(TRIGGER_PIN, INPUT_PULLDOWN);
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("ESP32-CAM Simple Trigger Test");
  Serial.println("-----------------------------");
  Serial.println("Waiting for signal from Arduino...");
  
  // Nhấp nháy LED báo hiệu khởi động
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
  
  // Camera initialization
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
  
  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  
  Serial.println("Camera initialized successfully");
}

void loop() {
  // Đọc trạng thái chân trigger
  int triggerValue = digitalRead(TRIGGER_PIN);
  
  // Nếu phát hiện tín hiệu HIGH
  if (triggerValue == HIGH) {
    Serial.println("Trigger signal detected! Taking photo...");
    
    // Bật đèn LED để báo hiệu
    digitalWrite(LED_PIN, HIGH);
    
    // Chụp ảnh
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
    } else {
      Serial.printf("Captured image: %dx%d, size: %d bytes\n", 
                   fb->width, fb->height, fb->len);
      
      // Trả lại bộ nhớ
      esp_camera_fb_return(fb);
    }
    
    // Tắt đèn LED
    digitalWrite(LED_PIN, LOW);
    
    // Đợi một chút để tránh trigger liên tục
    delay(1000);
  }
  
  // Delay nhỏ để giảm CPU usage
  delay(100);
} 