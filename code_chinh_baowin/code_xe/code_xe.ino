#include <WiFi.h>
#include <Ticker.h>
#include <ESP32Servo.h> 

#define LED_PIN 2
#define TRIG_PIN 26
#define ECHO_PIN 27
#define SERVO_PIN 25
#define SERVO_PWM_CHANNEL 0
#define SERVO_PWM_FREQ 50
#define SERVO_PWM_RESOLUTION 16

WiFiServer server(1234);
WiFiClient client;

#define right_enA 22
#define right_enB 23
#define left_enA 34
#define left_enB 35
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
Servo scanServo;

void setupUltrasonicAndServo() {
  scanServo.attach(SERVO_PIN);
  scanServo.write(90);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return duration * 0.034 / 2;
}

void autoDriveLogic() {
  static unsigned long lastAutoCheck = 0;
  if (millis() - lastAutoCheck < 500) return;
  lastAutoCheck = millis();

  scanServo.write(90);
  delay(200);
  long forwardDist = readDistanceCM();

  if (forwardDist > 20) {
    Serial2.println("@left(dir[0],duty[10])-right(dir[0],duty[10])#");
    return;
  }
  // nếu có vật cản trước mặt thì dùng lại
  Serial2.println("@left(dir[0],duty[0])-right(dir[0],duty[0])#");
  Serial.println("[AUTO] Vat can phia truoc! Dang quet...");
  long distances[2];
  int scanAngles[] = {0, 180};

  for (int i = 0; i < 2; i++) {
    scanServo.write(scanAngles[i]);
    delay(200);
    distances[i] = readDistanceCM();
    Serial.print("Goc "); Serial.print(scanAngles[i]);
    Serial.print(": "); Serial.print(distances[i]); Serial.println(" cm");
  }

  if (distances[0] > 20) {
    Serial.println("[AUTO] Re phai");
    Serial2.println("@left(dir[0],duty[10])-right(dir[1],duty[10])#");
  } else if (distances[1] > 20) {
    Serial.println("[AUTO] Re trai");
    Serial2.println("@left(dir[1],duty[10])-right(dir[0],duty[10])#");
  } else {
    Serial.println("[AUTO] Tat ca huong bi chan - Lui xe");
    Serial2.println("@left(dir[1],duty[10])-right(dir[1],duty[10])#");
    delay(500);
    Serial2.println("@left(dir[0],duty[0])-right(dir[0],duty[0])#");
  }

  scanServo.write(90);
}

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
  speedAvg_kmh = (speedLeft + speedRight) / 2.0 * 3.6;

  totalDistance += abs((speedLeft + speedRight) / 2.0 * sampleInterval);
}

bool autoMode = false;
bool lastAutoMode = false;
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
      Serial.println("[WARN] Gửi thất bại!");
      client.stop();
      isConnected = false;
    }
    lastSend = millis();
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

  if (isConnected && client.available()) {
    String data = client.readStringUntil('\n');
    data.trim();
    if (data.startsWith("CMD:") && !autoMode) { // Chỉ xử lý CMD ở chế độ thủ công
      int lIndex = data.indexOf("L:");
      int rIndex = data.indexOf("R:");
      if (lIndex != -1 && rIndex != -1) {
        int comma = data.indexOf(',', lIndex);
        int leftSpeed = data.substring(lIndex + 2, comma).toInt();
        int rightSpeed = data.substring(rIndex + 2).toInt();
        int leftDir = (leftSpeed < 0) ? 1 : 0;
        int rightDir = (rightSpeed < 0) ? 1 : 0;
        int leftDuty = constrain(abs(leftSpeed), 0, 20); // Giới hạn duty tối đa 30
        int rightDuty = constrain(abs(rightSpeed), 0, 20); // Giới hạn duty tối đa 30
        String cmdUART = "@left(dir[" + String(leftDir) + "],duty[" + String(leftDuty) + "])";
        cmdUART += "-right(dir[" + String(rightDir) + "],duty[" + String(rightDuty) + "])#";
        for (int i = 0; i < 3; i++) {
          Serial2.println(cmdUART);
          delay(50);
        }
        Serial.print("[UART2] Gửi: "); Serial.println(cmdUART);
      }
    } else if (data == "MODE:AUTO") {
      if (!autoMode) {
        autoMode = true;
        Serial.println("[MODE] Chuyen sang AUTO");
      }
    } else if (data == "MODE:MANUAL") {
      if (autoMode) {
        autoMode = false;
        Serial.println("[MODE] Chuyen sang MANUAL - Dung xe ngay");
        String cmdUART = "@left(dir[0],duty[0])-right(dir[0],duty[0])#";
        for (int i = 0; i < 3; i++) {
          Serial2.println(cmdUART);
          delay(50);
        }
      }
    }
  }

  if (autoMode) {
    autoDriveLogic();
  }
  if (lastAutoMode && !autoMode) {
    Serial.println("[MODE] Chuyen tu AUTO -> MANUAL trong loop - Dung xe");
    String cmdUART = "@left(dir[0],duty[0])-right(dir[0],duty[0])#";
    for (int i = 0; i < 3; i++) {
      Serial2.println(cmdUART);
      delay(50);
    }
  }
  lastAutoMode = autoMode;
}