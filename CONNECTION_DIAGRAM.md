# Sơ đồ kết nối Arduino UNO và ESP32-CAM

## Các thành phần cần thiết
1. Arduino UNO
2. ESP32-CAM
3. Cảm biến siêu âm HC-SR04
4. Servo
5. Dây jumper
6. Nguồn điện cho ESP32-CAM (3.3V)
7. USB-to-TTL (để nạp code cho ESP32-CAM)

## Kết nối cảm biến siêu âm với Arduino UNO
- **VCC** của HC-SR04 -> **5V** của Arduino UNO
- **GND** của HC-SR04 -> **GND** của Arduino UNO
- **Trig** của HC-SR04 -> **Chân 10** (Digital) của Arduino UNO
- **Echo** của HC-SR04 -> **Chân 11** (Digital) của Arduino UNO

## Kết nối Servo với Arduino UNO
- **Dây đỏ** (VCC) của Servo -> **5V** của Arduino UNO
- **Dây nâu/đen** (GND) của Servo -> **GND** của Arduino UNO
- **Dây cam/vàng** (Signal) của Servo -> **Chân 12** (Digital) của Arduino UNO

## Kết nối tín hiệu từ Arduino UNO đến ESP32-CAM
- **Chân 8** (Digital) của Arduino UNO -> **GPIO13** của ESP32-CAM
- **GND** của Arduino UNO -> **GND** của ESP32-CAM

## Cấp nguồn cho ESP32-CAM
ESP32-CAM cần nguồn điện 3.3V ổn định. Có thể sử dụng một trong các cách sau:
- Sử dụng bộ nguồn ngoài 3.3V
- Sử dụng chân 3.3V của Arduino (lưu ý: có thể không đủ dòng cho ESP32-CAM khi chụp ảnh, nên dùng nguồn ngoài).

## Lưu ý quan trọng
1. **Không kết nối ESP32-CAM trực tiếp với chân 5V** - ESP32-CAM hoạt động ở 3.3V.
2. Khi nạp code cho ESP32-CAM, cần:
   - Kết nối GPIO0 với GND trong quá trình nạp
   - Ngắt kết nối GPIO0 từ GND sau khi nạp xong
   - Reset ESP32-CAM để chạy chương trình
3. Đảm bảo rằng khi chạy cả hai thiết bị cùng lúc, Arduino UNO và ESP32-CAM nên dùng nguồn riêng biệt để tránh vấn đề về nguồn điện.

## Quy trình hoạt động
1. Arduino UNO điều khiển servo để quét không gian
2. Cảm biến siêu âm phát hiện vật thể trong phạm vi (<30cm)
3. Khi phát hiện vật thể, servo dừng quay
4. Arduino gửi tín hiệu HIGH đến GPIO13 của ESP32-CAM
5. ESP32-CAM nhận tín hiệu, chụp ảnh và tải lên server
6. Arduino đợi 5 giây, sau đó tiếp tục quét 