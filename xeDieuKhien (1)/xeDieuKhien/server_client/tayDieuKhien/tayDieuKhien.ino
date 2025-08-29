#include <WiFi.h>
#include <Wire.h>
#include <ADS1115_WE.h>
#include <LiquidCrystal_I2C.h>

#define MODE_BUTTON_PIN 19
bool lastButtonState = HIGH;
bool autoMode = false;

#define ADS_ADDR 0x48
ADS1115_WE adc(ADS_ADDR);
WiFiClient client;

const char* ssid = "ESP32_SERVER";
const char* password = "12345678";
#define LED_PIN 32

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Sửa nếu cần (0x3F)

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);

  // LCD
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Dang ket noi...");

  // WiFi
  WiFi.begin(ssid, password);
  int blinkState = LOW;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    blinkState = !blinkState;
    digitalWrite(LED_PIN, blinkState);
    Serial.print(".");
  }

  Serial.println("\nWiFi da ket noi!");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Da ket noi WiFi");
  digitalWrite(LED_PIN, HIGH);

  // ADC
  Wire.begin();
  if (!adc.init()) {
    Serial.println("ADS1115 not connected!");
  }
  adc.setVoltageRange_mV(ADS1115_RANGE_6144);
  adc.setMeasureMode(ADS1115_CONTINUOUS);
}

int16_t readChannel(ADS1115_MUX channel) {
  adc.setCompareChannels(channel);
  delayMicroseconds(50);
  return adc.getRawResult();
}

bool isConnected = false;
unsigned long lastSend = 0;
unsigned long lastBlink = 0;
bool ledState = false;
int16_t adcLeft_prev = 0;
int16_t adcRight_prev = 0;

void loop() {
  if (!client.connected()) {
    Serial.println("Dang ket noi Server...");
    lcd.setCursor(0, 0);
    lcd.print("Dang ket noi SV ");
    isConnected = client.connect("192.168.4.1", 1234);
    if (!isConnected) {
      Serial.println("Connect failed.");
      delay(1000);
      return;
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Da ket noi SV   ");
    }
  }

  if (millis() - lastBlink >= 500) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    lastBlink = millis();
  }

  // ==== Đọc joystick ====
  int16_t adcLeft = readChannel(ADS1115_COMP_0_GND);
  int16_t adcRight = readChannel(ADS1115_COMP_3_GND);

  // Chỉ gửi lệnh nếu thay đổi lớn hơn 100
  if (abs(adcLeft - adcLeft_prev) >= 100 || abs(adcRight - adcRight_prev) >= 100) {
    int speedL = map(adcLeft, 0, 16380, 100, -100);
    int speedR = map(adcRight, 0, 16380, 100, -100);
    if (abs(speedL) < 10) speedL = 0;
    if (abs(speedR) < 10) speedR = 0;

    String motorCmd = "CMD:L:" + String(speedL) + ",R:" + String(speedR) + "\n";
    client.print(motorCmd);
    delay(50);
    Serial.print("[SEND] "); Serial.println(motorCmd);
    // Lưu lại giá trị ADC cũ
    adcLeft_prev = adcLeft;
    adcRight_prev = adcRight;
  }

  // ==== Nhận dữ liệu tốc độ/quãng đường ====
  static float speed = 0.0;
  static float distance = 0.0;

  if (client.available()) {
    String data = client.readStringUntil('\n');
    Serial.print("[RECV] "); Serial.println(data);

    int spdIndex = data.indexOf("SPD:");
    int dstIndex = data.indexOf("DST:");
    if (spdIndex != -1 && dstIndex != -1) {
      speed = data.substring(spdIndex + 4, data.indexOf(',', spdIndex)).toFloat();
      distance = data.substring(dstIndex + 4).toFloat();
    }
  }

  // ==== Xác định hướng di chuyển ====
  String direction = "Dung Yen";
  int speedL = map(adcLeft, 0, 16380, 100, -100);
  int speedR = map(adcRight, 0, 16380, 100, -100);
  if (abs(speedL) < 10) speedL = 0;
  if (abs(speedR) < 10) speedR = 0;

  if (speedL == 0 && speedR == 0) {
    direction = "Dung Yen";
  } else if (speedL > 0 && speedR > 0) {
    direction = "Tien";
  } else if (speedL < 0 && speedR < 0) {
    direction = "Lui";
  } else if (speedL < speedR) {
    direction = "Re Trai";
  } else if (speedR < speedL) {
    direction = "Re Phai";
  }

  // ==== Hiển thị LCD ====
  lcd.setCursor(0, 0);
  lcd.print("V:");
  lcd.print(speed, 1);
  lcd.print("km D:");
  lcd.print(distance, 1);
  lcd.print("m   ");

  lcd.setCursor(0, 1);
  lcd.print("                ");  // clear dòng

  lcd.setCursor(0, 1);
  if (autoMode) {
    lcd.print("Che do: AUTO");
  } else {
    lcd.print(direction);
  }


  // ==== Kiểm tra nút nhấn chuyển chế độ ====
  bool currentButtonState = digitalRead(MODE_BUTTON_PIN);
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    autoMode = !autoMode;
    String modeCmd = autoMode ? "MODE:AUTO\n" : "MODE:MANUAL\n";
    client.print(modeCmd);
    Serial.print("[SEND] Chuyen che do: "); Serial.println(autoMode ? "AUTO" : "MANUAL");
    lcd.setCursor(0, 1);
    lcd.print("Che do: ");
    lcd.print(autoMode ? "AUTO   " : "MANUAL ");
    delay(200); // tránh nhấn đúp
  }
  lastButtonState = currentButtonState;


  delay(50);
}
