void setup() {
  Serial.begin(250000);

}

void loop() {
  delay(1000);
  unsigned long uLongMaxVal = 4294967295;

  unsigned long ticksOfSpanSet = uLongMaxVal - 2000;
  unsigned long now = uLongMaxVal + 1500;
  
  unsigned long ticksOfSpan = 3000;

  unsigned long result = ticksOfSpanSet + ticksOfSpan - now;


  Serial.print("result: ");
  Serial.println((double) result);
}