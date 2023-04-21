// ------------------------------------------------------------------
// ------------------------------------------------------------------
// GCU Capstone Project - Group 28 - 2023
// Written by Sandra M.
// ------------------------------------------------------------------
// ------------------------------------------------------------------


// Digital pin that will create pulses to send to the Portenta H7
int pulse_pin = 7;


void setup() {

  // Initiating Serial communication
  Serial.begin(9600);

  // Define pinmode
  pinMode(pulse_pin, OUTPUT);

}    


void loop() {

  // 0-1023 I/P
  int Pin;

  // Analog pin recieving the signal from the proximity sensor
  Pin = analogRead(A0);
     
  // The "if" statements is EDITABLE
  if (Pin > 388) {
    // Pin state HIGH
    digitalWrite(pulse_pin, HIGH);
    // analogWrite(pulse_pin, 400);
  } 
  else{
    // Pin state LOW
    digitalWrite(pulse_pin, LOW);
    // analogWrite(pulse_pin, 0);
  }

}