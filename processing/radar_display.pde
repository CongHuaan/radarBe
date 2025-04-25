import processing.serial.*; // imports library for serial communication
import java.awt.event.KeyEvent; // imports library for reading the data from the serial port
import java.io.IOException;

Serial myPort; // defines Object Serial
// defines variables
String angle="";
String distance="";
String data="";
String noObject;
float pixsDistance;
int iAngle, iDistance;
int index1=0;
int index2=0;
PFont orcFont;
long lastDataReceived = 0;
boolean radarActive = true;
String statusMessage = "";
boolean isFullScreen = true;  // Bắt đầu với full màn hình

// Để tránh settings() duplicated, dùng biến toàn cục này trước setup()
// và đặt size() trực tiếp trong setup()

void setup() {
  // Processing sẽ tự tạo settings() ngầm định với lệnh này
  if (isFullScreen) {
    // Không gọi fullScreen() ở đây, nhưng giữ comment để biết thiết lập
    // fullScreen() sẽ được gọi tự động bởi cờ ở dòng lệnh
    
    // Thử điều chỉnh kích thước để phù hợp với màn hình đầy đủ
    surface.setSize(displayWidth, displayHeight);
  } else {
    // size(1200, 700) được gọi ngầm định
    surface.setSize(1200, 700);
  }
  
  smooth();
  
  // Đặt vị trí cửa sổ về góc trên bên trái
  surface.setLocation(0, 0);
  
  // Danh sách tất cả các cổng Serial khả dụng
  println("Available serial ports:");
  for (int i = 0; i < Serial.list().length; i++) {
    println(i + " : " + Serial.list()[i]);
  }
  
  // Thử kết nối với cổng COM3, thay đổi thành cổng của bạn nếu cần
  try {
    myPort = new Serial(this,"COM3", 9600); // starts the serial communication
    myPort.bufferUntil('.'); // reads the data from the serial port up to the character '.'
    println("Connected to COM3");
  } catch (Exception e) {
    println("Error connecting to COM3. Please check port or try another port.");
    println(e.getMessage());
    radarActive = false;
  }
  
  // Hiển thị thông tin điều khiển
  println("=== RADAR CONTROL ===");
  println("W - Windowed mode (1200x700)");
  println("F - Full screen mode");
  println("ESC - Exit fullscreen (standard in Processing)");
  println("0-9 - Select COM port");
}

void draw() {
  // Kiểm tra xem có đang nhận dữ liệu không
  if (millis() - lastDataReceived > 2000) {
    radarActive = false;
  } else {
    radarActive = true;
    statusMessage = "";
  }
  
  // Vẽ nền và hiệu ứng làm mờ
  fill(98,245,31);
  noStroke();
  fill(0,4); 
  rect(0, 0, width, height-height*0.065); 
  
  fill(98,245,31); // green color
  // calls the functions for drawing the radar
  drawRadar(); 
  drawLine();
  drawObject();
  drawText();
  
  // Hiển thị thông báo trạng thái nếu có
  if (statusMessage != "") {
    fill(255, 0, 0);
    textSize(24);
    text(statusMessage, width/2 - 200, 30);
  }
  
  // Hiển thị hướng dẫn điều khiển ở góc trên bên phải
  fill(200, 200, 200);
  textSize(16);
  String controlHelp = "F: Fullscreen | W: Windowed | 0-9: Select COM port";
  text(controlHelp, width - textWidth(controlHelp) - 20, 25);
  
  // Hiển thị thông báo nếu không có dữ liệu
  if (!radarActive) {
    fill(255, 0, 0);
    textSize(24);
    text("No data received! Check connection to Arduino.", width/2 - 250, 60);
  }
}

void serialEvent (Serial myPort) { // starts reading data from the Serial Port
  try {
    // reads the data from the Serial Port up to the character '.'
    data = myPort.readStringUntil('.');
    
    if (data != null && data.length() > 2) {
      data = data.substring(0,data.length()-1);
      
      // Kiểm tra xem dữ liệu có đúng định dạng không
      if (data.indexOf(",") != -1) {
        index1 = data.indexOf(","); // find the character ','
        angle = data.substring(0, index1); // extract angle
        distance = data.substring(index1+1, data.length()); // extract distance
        
        // converts the String variables into Integer
        try {
          iAngle = int(angle);
          iDistance = int(distance);
          
          // Cập nhật thời gian nhận dữ liệu cuối cùng
          lastDataReceived = millis();
        } catch (Exception e) {
          println("Error parsing data: " + data);
        }
      }
    }
  } catch (Exception e) {
    println("Error in serialEvent: " + e.getMessage());
  }
}

void drawRadar() {
  pushMatrix();
  translate(width/2,height-height*0.074); // moves the starting coordinats to new location
  noFill();
  strokeWeight(2);
  stroke(98,245,31);
  // draws the arc lines
  arc(0,0,(width-width*0.0625),(width-width*0.0625),PI,TWO_PI);
  arc(0,0,(width-width*0.27),(width-width*0.27),PI,TWO_PI);
  arc(0,0,(width-width*0.479),(width-width*0.479),PI,TWO_PI);
  arc(0,0,(width-width*0.687),(width-width*0.687),PI,TWO_PI);
  // draws the angle lines
  line(-width/2,0,width/2,0);
  line(0,0,(-width/2)*cos(radians(30)),(-width/2)*sin(radians(30)));
  line(0,0,(-width/2)*cos(radians(60)),(-width/2)*sin(radians(60)));
  line(0,0,(-width/2)*cos(radians(90)),(-width/2)*sin(radians(90)));
  line(0,0,(-width/2)*cos(radians(120)),(-width/2)*sin(radians(120)));
  line(0,0,(-width/2)*cos(radians(150)),(-width/2)*sin(radians(150)));
  line((-width/2)*cos(radians(30)),0,width/2,0);
  popMatrix();
}

void drawObject() {
  pushMatrix();
  translate(width/2,height-height*0.074); // moves the starting coordinats to new location
  strokeWeight(9);
  stroke(255,10,10); // red color
  pixsDistance = iDistance*((height-height*0.1666)*0.025); // covers the distance from the sensor from cm to pixels
  // limiting the range to 40 cms
  if(iDistance < 40){
    // draws the object according to the angle and the distance
    line(pixsDistance*cos(radians(iAngle)),-pixsDistance*sin(radians(iAngle)),(width-width*0.505)*cos(radians(iAngle)),-(width-width*0.505)*sin(radians(iAngle)));
  }
  popMatrix();
}

void drawLine() {
  pushMatrix();
  strokeWeight(9);
  
  // Nếu radar không hoạt động, vẽ dòng quét màu xám
  if (!radarActive) {
    stroke(100, 100, 100);
  } else {
    stroke(30,250,60);
  }
  
  translate(width/2,height-height*0.074); // moves the starting coordinats to new location
  line(0,0,(height-height*0.12)*cos(radians(iAngle)),-(height-height*0.12)*sin(radians(iAngle))); // draws the line according to the angle
  popMatrix();
}

void drawText() { // draws the texts on the screen
  pushMatrix();
  if(iDistance>40) {
    noObject = "Out of Range";
  }
  else {
    noObject = "In Range";
  }
  fill(0,0,0);
  noStroke();
  rect(0, height-height*0.0648, width, height);
  fill(98,245,31);
  textSize(25);
  
  text("10cm",width-width*0.3854,height-height*0.0833);
  text("20cm",width-width*0.281,height-height*0.0833);
  text("30cm",width-width*0.177,height-height*0.0833);
  text("40cm",width-width*0.0729,height-height*0.0833);
  textSize(40);
  
  // Hiển thị trạng thái radar
  if (radarActive) {
    text("Object Radar - Active", width-width*0.875, height-height*0.0277);
  } else {
    fill(255, 0, 0);
    text("Object Radar - NOT ACTIVE", width-width*0.875, height-height*0.0277);
    fill(98,245,31);
  }
  
  // Hiển thị góc
  text("Angle: " + iAngle +" °", width-width*0.48, height-height*0.0277);
  
  // Hiển thị khoảng cách - tách riêng nhãn và giá trị
  text("Distance:", width-width*0.26, height-height*0.0277);
  
  // Tạo khoảng cách phù hợp cho giá trị
  if(iDistance<40) {
    text(iDistance + " cm", width-width*0.14, height-height*0.0277);
  } else {
    fill(255, 140, 0);  // Màu cam cho "Out of range"
    text("Out of range", width-width*0.14, height-height*0.0277);
    fill(98,245,31);    // Trở lại màu xanh
  }
  
  textSize(25);
  fill(98,245,60);
  translate((width-width*0.4994)+width/2*cos(radians(30)),(height-height*0.0907)-width/2*sin(radians(30)));
  rotate(-radians(-60));
  text("30°",0,0);
  resetMatrix();
  translate((width-width*0.503)+width/2*cos(radians(60)),(height-height*0.0888)-width/2*sin(radians(60)));
  rotate(-radians(-30));
  text("60°",0,0);
  resetMatrix();
  translate((width-width*0.507)+width/2*cos(radians(90)),(height-height*0.0833)-width/2*sin(radians(90)));
  rotate(radians(0));
  text("90°",0,0);
  resetMatrix();
  translate(width-width*0.513+width/2*cos(radians(120)),(height-height*0.07129)-width/2*sin(radians(120)));
  rotate(radians(-30));
  text("120°",0,0);
  resetMatrix();
  translate((width-width*0.5104)+width/2*cos(radians(150)),(height-height*0.0574)-width/2*sin(radians(150)));
  rotate(radians(-60));
  text("150°",0,0);
  popMatrix(); 
}

// Xử lý phím bấm
void keyPressed() {
  // F để chuyển sang chế độ toàn màn hình
  if (key == 'f' || key == 'F') {
    try {
      surface.setSize(displayWidth, displayHeight);
      surface.setLocation(0, 0);
      isFullScreen = true;
      statusMessage = "Switched to fullscreen mode";
      println("Switched to fullscreen mode");
    } catch (Exception e) {
      println("Error switching to fullscreen: " + e.getMessage());
    }
  }
  
  // W để chuyển sang chế độ cửa sổ
  if (key == 'w' || key == 'W') {
    try {
      surface.setSize(1200, 700);
      surface.setLocation(displayWidth/2 - 600, displayHeight/2 - 350);
      isFullScreen = false;
      statusMessage = "Switched to windowed mode";
      println("Switched to windowed mode");
    } catch (Exception e) {
      println("Error switching to windowed mode: " + e.getMessage());
    }
  }
  
  // Chọn cổng COM bằng phím số
  if (key >= '0' && key <= '9') {
    int portIndex = key - '0';
    if (portIndex < Serial.list().length) {
      if (myPort != null) {
        myPort.stop();
      }
      try {
        myPort = new Serial(this, Serial.list()[portIndex], 9600);
        myPort.bufferUntil('.');
        println("Connected to " + Serial.list()[portIndex]);
      } catch (Exception e) {
        println("Error connecting to port " + Serial.list()[portIndex]);
      }
    }
  }
} 