#include <Servo.h>

// Định nghĩa chân cho cảm biến siêu âm
const int trigPin = 10;
const int echoPin = 11;

// Chân gửi tín hiệu đến ESP32-CAM
const int triggerPin = 3;  // Chân 3 -> GPIO12 của ESP32-CAM

// Định nghĩa chân cho servo và laser
const int radarServoPin = 12;  // Servo chính cho radar (giữ nguyên)
const int laserServoPin = 9;   // Servo thứ hai điều khiển góc laser
const int laserPin = 8;        // Chân điều khiển module laser (HIGH để bật)

// Ngưỡng phát hiện đối tượng (cm)
const int detectionThreshold = 10;  // Phát hiện đối tượng trong khoảng 10cm

// Bộ lọc và đo khoảng cách chính xác
const int numReadings = 3;       // Số lần đọc để lấy trung bình
const int invalidDistance = 400; // Giá trị khoảng cách không hợp lệ

// Variables for the duration and the distance
long duration;
int distance;
int filteredDistance;
int distances[numReadings]; // Mảng lưu các giá trị đo
int readIndex = 0;          // Chỉ số đọc hiện tại
int totalDistance = 0;      // Tổng khoảng cách (để tính trung bình)
bool objectConfirmed = false; // Chỉ bắn khi đối tượng được xác nhận sau nhiều lần đo
bool objectDetected = false;
bool laserActive = false;   // Trạng thái laser

// Thông tin vị trí đối tượng đã phát hiện
int detectedAngle = 0;      // Góc của đối tượng đã phát hiện
int detectedDistance = 0;   // Khoảng cách của đối tượng đã phát hiện

Servo radarServo; // Servo điều khiển chuyển động radar
Servo laserServo; // Servo điều khiển hướng laser

void setup() {
  // Cài đặt chân
  pinMode(trigPin, OUTPUT);     // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);      // Sets the echoPin as an Input
  pinMode(triggerPin, OUTPUT);  // Chân gửi tín hiệu đến ESP32-CAM
  pinMode(laserPin, OUTPUT);    // Chân điều khiển laser
  
  // Khởi tạo trạng thái mặc định
  digitalWrite(triggerPin, HIGH); // Không kích hoạt
  digitalWrite(laserPin, LOW);    // Laser tắt
  
  // Khởi động Serial
  Serial.begin(9600);
  
  // Gắn servo
  radarServo.attach(radarServoPin); 
  laserServo.attach(laserServoPin);
  
  // Đặt servo về vị trí bắt đầu
  radarServo.write(90);
  laserServo.write(90);
  delay(1000);
  
  Serial.println("Dual Servo Laser Radar System Ready");
}

void loop() {
  // ============================
  // Quét với servo radar (15-165 độ)
  // ============================
  
  for (int i = 15; i <= 165; i++) {  
    // Di chuyển radar servo
    radarServo.write(i);
    delay(15); // Đợi radar servo di chuyển
    
    // Di chuyển laser servo riêng biệt
    if (!objectDetected) {
      laserServo.write(i);
      delay(15); // Đợi laser servo di chuyển
    }
    
    // Đo khoảng cách với bộ lọc
    filteredDistance = getFilteredDistance();
    
    // Gửi dữ liệu góc và khoảng cách qua Serial cho Processing
    Serial.print(i);
    Serial.print(",");
    Serial.print(filteredDistance < 400 ? filteredDistance : 401);
    Serial.print(".");
    delay(1);
    
    // Phát hiện đối tượng trong tầm
    if (filteredDistance > 0 && filteredDistance < detectionThreshold && !objectDetected) {
      if (confirmObject()) {
        objectDetected = true;
        detectedAngle = i;
        detectedDistance = filteredDistance;
        
        Serial.print("Object CONFIRMED at angle ");
        Serial.print(detectedAngle);
        Serial.print(" degrees, distance: ");
        Serial.print(detectedDistance);
        Serial.println("cm");
        
        activateLaser();
        triggerCamera();
        
        // Tiếp tục gửi dữ liệu trong khi đợi
        for (int keep = 0; keep < 40; keep++) {
          Serial.print(detectedAngle);
          Serial.print(",");
          Serial.print(filteredDistance < 400 ? filteredDistance : 401);
          Serial.print(".");
          delay(50);
        }
        
        deactivateLaser();
        objectDetected = false;
      }
    }
  }
  
  // ============================
  // Quét ngược lại từ 165 đến 15 độ
  // ============================
  
  for (int i = 165; i > 15; i--) {  
    radarServo.write(i);
    
    if (!objectDetected) {
      laserServo.write(i);
      delay(30);
    }
    
    filteredDistance = getFilteredDistance();
    
    Serial.print(i);
    Serial.print(",");
    Serial.print(filteredDistance < 400 ? filteredDistance : 401);
    Serial.print(".");
    delay(1);
    
    if (filteredDistance > 0 && filteredDistance < detectionThreshold && !objectDetected) {
      if (confirmObject()) {
        objectDetected = true;
        detectedAngle = i;
        detectedDistance = filteredDistance;
        
        Serial.print("Object CONFIRMED at angle ");
        Serial.print(detectedAngle);
        Serial.print(" degrees, distance: ");
        Serial.print(detectedDistance);
        Serial.println("cm");
        
        activateLaser();
        triggerCamera();
        
        for (int keep = 0; keep < 40; keep++) {
          Serial.print(detectedAngle);
          Serial.print(",");
          Serial.print(filteredDistance < 400 ? filteredDistance : 401);
          Serial.print(".");
          delay(50);
        }
        
        deactivateLaser();
        objectDetected = false;
      }
    }
  }
}

// Bật laser và điều chỉnh servo laser để trỏ vào đối tượng
void activateLaser() {
  Serial.println("Activating laser and pointing to object...");
  
  // Bật laser
  digitalWrite(laserPin, HIGH);
  laserActive = true;
  
  // Để servo radar tiếp tục theo dõi đối tượng
  radarServo.write(detectedAngle);
  
  // Quét laser servo qua lại xung quanh vị trí phát hiện
  const int sweepRange = 10;     // Độ rộng quét: 10 độ mỗi bên
  const int sweepDelay = 50;     // Thời gian delay giữa mỗi bước quét (ms)
  const int numSweeps = 10;      // Số lần quét qua lại
  const int sweepStep = 2;       // Bước nhảy mỗi lần quét (độ)
  
  // Quét qua lại trong khi chụp ảnh
  for(int sweep = 0; sweep < numSweeps; sweep++) {
    // Quét từ giữa sang phải
    for(int angle = detectedAngle; angle <= detectedAngle + sweepRange; angle += sweepStep) {
      laserServo.write(angle);
      delay(sweepDelay);
    }
    
    // Quét từ phải sang trái
    for(int angle = detectedAngle + sweepRange; angle >= detectedAngle - sweepRange; angle -= sweepStep) {
      laserServo.write(angle);
      delay(sweepDelay);
    }
    
    // Quét từ trái về giữa
    for(int angle = detectedAngle - sweepRange; angle <= detectedAngle; angle += sweepStep) {
      laserServo.write(angle);
      delay(sweepDelay);
    }
  }
  
  // Trở về vị trí phát hiện ban đầu
  laserServo.write(detectedAngle);
  delay(100);
  
  // Tắt laser
  digitalWrite(laserPin, LOW);
  laserActive = false;
  Serial.println("Laser sweep complete");
}

// Tắt laser
void deactivateLaser() {
  digitalWrite(laserPin, LOW);
  laserActive = false;
  Serial.println("Laser deactivated");
}

// Hàm xác nhận đối tượng bằng cách đo nhiều lần
bool confirmObject() {
  int confirmCount = 0;
  const int requiredConfirmations = 3;  // Số lần xác nhận cần thiết
  
  for (int i = 0; i < requiredConfirmations; i++) {
    int checkDistance = getRawDistance();
    
    // Nếu phát hiện đối tượng trong ngưỡng
    if (checkDistance > 0 && checkDistance < detectionThreshold) {
      confirmCount++;
    }
    delay(50);  // Đợi giữa các lần đo
  }
  
  return (confirmCount >= requiredConfirmations);
}

// Lấy khoảng cách thô (một lần đo)
int getRawDistance() {
  digitalWrite(trigPin, LOW); 
  delayMicroseconds(2);
  
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH); 
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Với timeout để tránh treo
  duration = pulseIn(echoPin, HIGH, 23529); // Timeout tương đương khoảng 4m
  
  // Kiểm tra nếu timeout
  if (duration == 0) {
    return invalidDistance;
  }
  
  // Tính toán khoảng cách
  int measuredDistance = duration * 0.034 / 2;
  
  // Kiểm tra phạm vi hợp lệ (3cm đến 300cm)
  if (measuredDistance < 3 || measuredDistance > 300) {
    return invalidDistance;
  }
  
  return measuredDistance;
}

// Lấy khoảng cách đã lọc (trung bình nhiều lần đo)
int getFilteredDistance() {
  // Loại bỏ giá trị cũ khỏi tổng
  totalDistance = totalDistance - distances[readIndex];
  
  // Đọc khoảng cách mới
  int newDistance = getRawDistance();
  
  // Lưu giá trị mới vào mảng
  distances[readIndex] = newDistance;
  
  // Cộng giá trị mới vào tổng
  totalDistance = totalDistance + newDistance;
  
  // Tăng chỉ số đọc và quay vòng nếu cần
  readIndex = (readIndex + 1) % numReadings;
  
  // Tính giá trị trung bình
  int averageDistance = totalDistance / numReadings;
  
  // Loại bỏ các giá trị ngoại lệ
  if (averageDistance >= invalidDistance) {
    return invalidDistance;
  }
  
  return averageDistance;
}

// Hàm gửi tín hiệu đến ESP32-CAM
void triggerCamera() {
  Serial.println("Triggering ESP32-CAM to take photo...");
  
  // Gửi xung LOW đến ESP32-CAM
  digitalWrite(triggerPin, LOW);
  delay(200);  // Giữ tín hiệu trong 200ms
  digitalWrite(triggerPin, HIGH);
  
  Serial.println("Trigger signal sent to ESP32-CAM!");
} 