
void setupUltrasonicAndServo() {
  scanServo.attach(SERVO_PIN);
  scanServo.write(90);  // Nhìn thẳng ban đầu
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // timeout 30ms
  return duration * 0.034 / 2;
}

void autoDriveLogic() {
  static unsigned long lastAutoCheck = 0;
  if (millis() - lastAutoCheck < 1000) return;
  lastAutoCheck = millis();

  scanServo.write(90);  // luôn nhìn thẳng khi di chuyển
  delay(200);
  long forwardDist = readDistanceCM();

  if (forwardDist > 20) {
    Serial.println("[AUTO] Di thang (khong co vat can)");
    Serial2.println("@left(dir[0],duty[10])-right(dir[0],duty[10])#"); // đi thẳng chậm
    return;
  }

  // Có vật cản phía trước → quét xung quanh
  Serial.println("[AUTO] Vat can phia truoc! Dang quet...");
  long distances[2];
  int scanAngles[] = {0, 180};

  for (int i = 0; i < 2; i++) {
    scanServo.write(scanAngles[i]);
    delay(400);
    distances[i] = readDistanceCM();
    Serial.print("Goc "); Serial.print(scanAngles[i]);
    Serial.print(": "); Serial.print(distances[i]); Serial.println(" cm");
  }

  // Chọn hướng không có vật cản
  if (distances[0] > 20) {
    Serial.println("[AUTO] Re phai");
    Serial2.println("@left(dir[0],duty[10])-right(dir[1],duty[10])#"); // quay phải
  } else if (distances[1] > 20) {
    Serial.println("[AUTO] Re trai");
    Serial2.println("@left(dir[1],duty[10])-right(dir[0],duty[10])#"); // quay trái
  } else {
    Serial.println("[AUTO] Tat ca huong bi chan - Lui xe");
    Serial2.println("@left(dir[1],duty[10])-right(dir[1],duty[10])#"); // lùi
    delay(500);
    Serial2.println("@left(dir[0],duty[0])-right(dir[0],duty[0])#");   // dừng lại sau lùi
  }

  // Sau khi chọn hướng/quét, đưa servo về giữa
  scanServo.write(90);
}

