# ESP32-CAM with NestJS Backend

This project consists of two parts:
1. **ESP32-CAM code**: Captures images periodically and uploads them to a NestJS backend
2. **NestJS backend**: Receives and stores images from the ESP32-CAM

## ESP32-CAM Setup

### Requirements
- ESP32-CAM (AI-Thinker model)
- Arduino IDE with ESP32 board support installed
- USB-to-TTL converter for programming

### Configuration
1. Open the `ESP32_Camera_Upload.ino` file in Arduino IDE
2. Update the WiFi credentials:
   ```c
   const char* ssid = "Your_WiFi_SSID";
   const char* password = "Your_WiFi_Password";
   ```
3. Update the server URL to point to your NestJS backend:
   ```c
   const char* serverUrl = "http://your-server-ip:3000/images/upload";
   ```
4. Adjust the capture interval if needed (default is 30 seconds):
   ```c
   const int captureInterval = 30000; // 30 seconds
   ```
5. Connect ESP32-CAM to the USB-to-TTL converter:
   - ESP32 GND -> USB-TTL GND
   - ESP32 5V -> USB-TTL VCC (5V)
   - ESP32 U0R -> USB-TTL TX
   - ESP32 U0T -> USB-TTL RX
   - Connect GPIO 0 to GND during upload (disconnect after upload)
6. Select "AI Thinker ESP32-CAM" board in Arduino IDE
7. Upload the code
8. Reset the ESP32-CAM (disconnect and reconnect power)

## NestJS Backend Setup

### Requirements
- Node.js (v14 or later)
- npm

### Installation and Setup
1. Navigate to the backend directory:
   ```bash
   cd backend
   ```

2. Install dependencies:
   ```bash
   npm install
   ```

3. Start the server:
   ```bash
   npm start
   ```
   The server will run on http://localhost:3000

### API Endpoints
- **POST /images/upload**: Upload an image (used by ESP32-CAM)
- **GET /images/list**: Get list of all uploaded images
- **GET /images/:filename**: Get a specific image by filename

## How It Works
1. The ESP32-CAM will connect to your WiFi network on startup
2. Every 30 seconds (or as configured), it will:
   - Capture a photo
   - Upload it to the NestJS backend server
3. The NestJS backend will:
   - Receive the image
   - Save it to the uploads directory
   - Store metadata about the image
   - Make it available through the API

## Troubleshooting
- If ESP32-CAM fails to connect to WiFi, check your credentials
- If image upload fails, ensure your server IP is correct and that the server is running
- Check the serial monitor at 115200 baud for debugging information from ESP32-CAM #   r a d a r B e  
 