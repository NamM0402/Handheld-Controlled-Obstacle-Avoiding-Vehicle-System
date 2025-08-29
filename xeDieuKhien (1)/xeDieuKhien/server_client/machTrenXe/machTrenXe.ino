#include <WiFi.h>
#include <Ticker.h>
#include <ESP32Servo.h> 

#define LED_PIN 2

#define TRIG_PIN 26
#define ECHO_PIN 27

#define SERVO_PIN 25
#define SERVO_PWM_CHANNEL 0
#define SERVO_PWM_FREQ 50
#define SERVO_PWM_RESOLUTION 16  // 16 bit = 65536 steps

WiFiServer server(1234);
WiFiClient client;

#define right_enA   22
#define right_enB   23
#define left_enA    34
#define left_enB    35

#define UART2_RX 16
#define UART2_TX 17

volatile long encoderLeft = 0;
volatile long encoderRight = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

const float wheelCircumference = 0.3016;
const float encoderPulsesPerMotorRev = 13.0;
const float gearRatio = 19.2;
const float totalPPR = encoderPulsesPerMotorRev * gearRatio * 2.0;
const float sampleInterval = 0.1;

long prevLeft = 0, prevRight = 0;

Ticker encoderTicker;

float speedAvg_kmh = 0;
float totalDistance = 0;

void IRAM_ATTR handleLeftEncoder() {
  bool A = digitalRead(left_enA);
  bool B = digitalRead(left_enB);
  portENTER_CRITICAL_ISR(&mux);
  encoderLeft += (A == B) ? 1 : -1;
  portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR handleRightEncoder() {
  bool A = digitalRead(right_enA);
  bool B = digitalRead(right_enB);
  portENTER_CRITICAL_ISR(&mux);
  encoderRight += (A == B) ? 1 : -1;
  portEXIT_CRITICAL_ISR(&mux);
}

void readEncoderSpeed() {
  portENTER_CRITICAL(&mux);
  long left = encoderLeft;
  long right = encoderRight;
  portEXIT_CRITICAL(&mux);

  long deltaLeft = left - prevLeft;
  long deltaRight = right - prevRight;
  prevLeft = left;
  prevRight = right;

  float speedLeft = (deltaLeft / totalPPR) * wheelCircumference / sampleInterval;
  float speedRight = (deltaRight / totalPPR) * wheelCircumference / sampleInterval;
  float speedAvg = (speedLeft + speedRight) / 2.0;

  totalDistance += speedAvg * sampleInterval;
  speedAvg_kmh = speedAvg * 3.6;

  //Serial.printf("[SPEED] %.2f km/h | Distance: %.2f m\n", speedAvg_kmh, totalDistance);
}

bool autoMode = false;
bool lastAutoMode = false;  // Dùng để phát hiện chuyển chế độ
Servo scanServo;

bool isConnected = false;
unsigned long lastSend = 0;
unsigned long lastBlink = 0;
bool ledState = false;

void setup() 
{
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, UART2_RX, UART2_TX);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("[BOOT] ESP32 khởi động...");
  WiFi.softAP("ESP32_SERVER", "12345678");
  Serial.print("[WIFI] IP: ");
  Serial.println(WiFi.softAPIP());

  server.begin();

  pinMode(left_enA, INPUT_PULLUP);
  pinMode(left_enB, INPUT_PULLUP);
  pinMode(right_enA, INPUT_PULLUP);
  pinMode(right_enB, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(left_enA), handleLeftEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(right_enA), handleRightEncoder, CHANGE);

  encoderTicker.attach(sampleInterval, readEncoderSpeed);

  setupUltrasonicAndServo();
  //setupServoPWM();
}

void loop() 
{
  if (!client || !client.connected()) {
    WiFiClient newClient = server.available();
    if (newClient && newClient.connected()) {
      client = newClient;
      isConnected = true;
      Serial.print("[CLIENT] Đã kết nối: ");
      Serial.println(client.remoteIP());
    } else if (isConnected) {
      Serial.println("[CLIENT] Mất kết nối.");
      isConnected = false;
    }
  }

  
  if (isConnected && millis() - lastSend >= 1000) {
    String frame = "SPD:" + String(speedAvg_kmh, 2) + ",DST:" + String(totalDistance, 2) + "\n";
    size_t bytesSent = client.print(frame);
    if (bytesSent == 0 || !client.connected()) {
      // Log gửi thất bại (giữ lại)
      Serial.println("[WARN] Gửi thất bại!");
      client.stop();
      isConnected = false;
    }
    lastSend = millis();  // cập nhật lại thời gian gửi
  }
  

  if (isConnected) {
    if (millis() - lastBlink >= 500) {
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
      lastBlink = millis();
    }
  } else {
    digitalWrite(LED_PIN, HIGH);
  }

  if (isConnected && client.available()) 
  {
    String data = client.readStringUntil('\n');
    data.trim();
    // TẮT log nhận dữ liệu để giảm nghẽn:
    // Serial.print("[RECV] "); Serial.println(data);

    if (data.startsWith("CMD:"))
    {
      int lIndex = data.indexOf("L:");
      int rIndex = data.indexOf("R:");
      if (lIndex != -1 && rIndex != -1) {
        int comma = data.indexOf(',', lIndex);
        int leftSpeed = data.substring(lIndex + 2, comma).toInt();
        int rightSpeed = data.substring(rIndex + 2).toInt();

        // ĐẢO CHIỀU: gạt lên quay lùi, gạt xuống quay tiến
        int leftDir = (leftSpeed < 0) ? 1 : 0;
        int rightDir = (rightSpeed < 0) ? 1 : 0;

        int leftDuty = constrain(abs(leftSpeed), 0, 100);
        int rightDuty = constrain(abs(rightSpeed), 0, 100);

        String cmdUART = "@left(dir[" + String(leftDir) + "],duty[" + String(leftDuty) + "])";
        cmdUART += "-right(dir[" + String(rightDir) + "],duty[" + String(rightDuty) + "])#";

        for(int i=0;i<3;i++)
        {
          Serial2.println(cmdUART);
          delay(50);
        }
        // TẮT log UART để giảm độ trễ:
         Serial.print("[UART2] Gửi: "); Serial.println(cmdUART);
      }
    }
    else if (data == "MODE:AUTO") {
      if (!autoMode) {
        autoMode = true;
        Serial.println("[MODE] Chuyen sang AUTO");
      }
    } 
    else if (data == "MODE:MANUAL") {
      if (autoMode) {
        autoMode = false;
        Serial.println("[MODE] Chuyen sang MANUAL - Dung xe ngay");
        Serial2.println("@left(dir[0],duty[0])-right(dir[0],duty[0])#");  // Gửi lệnh dừng xe
      }
    }
  }//if (isConnected && client.available())

    if (autoMode) {
    autoDriveLogic();
  }
  // Nếu vừa chuyển từ auto sang manual, thì dừng lại
  if (lastAutoMode && !autoMode) {
    Serial.println("[MODE] Chuyen tu AUTO -> MANUAL trong loop - Dung xe");
    Serial2.println("@left(dir[0],duty[0])-right(dir[0],duty[0])#");
  }
  lastAutoMode = autoMode;



}
