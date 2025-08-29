
void sendStm32(int8_t dutyMotorLeft, int8_t dutyMotorRight)
{
  // Tạo chuỗi dạng "@left[30],right[50]#"
  char frame[50];
  snprintf(frame, sizeof(frame), "@left[%d],right[%d]#", dutyMotorLeft, dutyMotorRight);

  // Gửi qua UART2 (Serial2)
  Serial2.print(frame);

  // (Tùy chọn) Gửi ra Serial monitor để debug
  Serial.print("Send to STM32: ");
  Serial.println(frame);
}


















