void setup()   {                
  Serial.begin(38400);
  pinMode(2, INPUT);
}

void loop()                     
{
  if (digitalRead(2) == HIGH) {
    Serial.println("Button is not pressed...");
  } else {
    Serial.println("Button pressed!!!");
  }
  delay(250);
}
