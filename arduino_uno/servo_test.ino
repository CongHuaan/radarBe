#include <Servo.h>

// Định nghĩa chân cho servo
const int testServoPin = 9;  // Servo thứ hai ở pin 9

Servo testServo;  // Tạo đối tượng servo để test

void setup() {
  // Khởi động Serial để debug
  Serial.begin(9600);
  Serial.println("Bắt đầu test servo...");
  
  // Gắn servo vào pin
  testServo.attach(testServoPin);
  Serial.println("Đã kết nối servo vào pin 9");
  
  // Đợi 1 giây để ổn định
  delay(1000);
}

void loop() {
  // Di chuyển servo từ 0 đến 180 độ
  Serial.println("Di chuyển servo đến 0 độ");
  testServo.write(0);
  delay(2000);  // Đợi 2 giây
  
  Serial.println("Di chuyển servo đến 90 độ");
  testServo.write(90);
  delay(2000);  // Đợi 2 giây
  
  Serial.println("Di chuyển servo đến 180 độ");
  testServo.write(180);
  delay(2000);  // Đợi 2 giây
  
  Serial.println("Di chuyển servo về 90 độ");
  testServo.write(90);
  delay(2000);  // Đợi 2 giây
} 