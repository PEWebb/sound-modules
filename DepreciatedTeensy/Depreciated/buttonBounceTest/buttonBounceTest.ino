#include <Bounce.h>

// Bounce objects to easily and reliably read the buttons
Bounce buttonRecord = Bounce(2, 8);
Bounce buttonPlay   = Bounce(3, 8);  // 8 = 8 ms debounce time

void setup() {
  // Configure the pushbutton pins
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
}

void loop()                     
{
  // First, read the buttons
  buttonRecord.update();
  buttonPlay.update();
  
  if (buttonRecord.risingEdge()) {
    Serial.println("Record Button Pressed");
  } else if (buttonRecord.fallingEdge()) {
    Serial.println("Record Button Release");
  } else if (buttonPlay.risingEdge()) {
    Serial.println("Play Button Pressed");
  } else if (buttonPlay.fallingEdge()) {
    Serial.println("Play Button Release");
  }
}
