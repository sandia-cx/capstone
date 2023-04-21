#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD
#include <Arduino_PortentaBreakout.h> 
 
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------


// ----------- NOTES --------------------
// Reference point is BDC -- pulse count equals 0 (60) at BDC
// To find editable variable values search: "EDITABLE VARIABLE"


// ----------- STOP CODES --------------------
// 1 = Normal stop - start_function() - no new pulses sensed in 60 sec
// 2 = Normal stop - engine was powered off - main_function()
// 21 = Reference point (BDC) was not found
// 22 = Emercency Stop - count was off by more than the set amount


// -----------------------------------------------
// ---------- PIN SETUP & PIN VARIABLES ----------
// -----------------------------------------------

// ----- Sensor 1 -----
breakoutPin sensor_1_pin = GPIO_1;     // prox. sensor signal -> GPIO_1    
bool sensor_1_start;
bool sensor_1_main;
unsigned long loop_time_1;     // interrupt timer
unsigned long loop_time_1_prev;
volatile int engine_cycle_counter;         // keeps track of the cycle
bool call_main_func;
bool min_rpm_reached;
long min_rpm = 100;     // EDITABLE VARIABLE 


breakoutPin my_pulse_pin = GPIO_3;  // TEST - SANDRA



// ----- Driver 1 - Intake -----
breakoutPin intake_valve_driver = GPIO_0;
volatile int intake_pin_state;         // variable for reading the pin status -- 0 = closed - 1 = open


// ----- Driver 2 - Exhaust -----
breakoutPin exhaust_valve_driver = GPIO_2;
volatile int exhaust_pin_state;         // variable for reading the pin status -- 0 = closed - 1 = open


// ----- Output for Start Sequence -----
breakoutPin start_sequence_driver = GPIO_4;
volatile int start_seq_pin_state;         // variable for reading the pin status -- 0 = closed - 1 = open


// ----- LCD Display -----
breakoutPin sda = I2C_SDA_0;
breakoutPin scl = I2C_SCL_0;
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2); // Change to (0x27,20,4) for 20x4 LCD.


// -----------------------------------------------
// -------------- TIMING VARIABLES ---------------
// -----------------------------------------------
bool bdc_found;         // bool indicating if BDC has been found
volatile int pulse_count;         // variable for tracking the pulse count
volatile int pulse_check;         // variable for tracking if the pulse count is off 
volatile int pulse_check_prev; 


// -----------------------------------------------
// --------------- GEAR VARIABLES ----------------
// -----------------------------------------------
int gear_teeth_cnt = 60;  // EDITABLE VARIABLE 


// -----------------------------------------------
// ---------------- RPM VARIABLES ----------------
// -----------------------------------------------
volatile int rpm_pulses;
long rpm;

// -----------------------------------------------
// --------- SERVOMOTOR SIGNAL VARIABLES --------- 
// -----------------------------------------------

// const int IVO = 33; // intake valve open during cycle 1 -- EDITABLE VARIABLE
// const int IVC = 3; // intake valve close after BDC during cycle 2 -- intake valve close before BDC during cycle 1 -- EDITABLE VARIABLE

// const int EVO = 56; // exhaust valve close before BDC during cycle 2 -- exhaust valve close after BDC during cycle 1 -- EDITABLE VARIABLE
// const int EVC = 28; // exhaust valve close during cycle 1 -- EDITABLE VARIABLE


//POPPY
// const int IVO = 33 - 5; // intake valve open during cycle 1 -- EDITABLE VARIABLE
// const int IVC = 56; // intake valve close after BDC during cycle 2 -- intake valve close before BDC during cycle 1 -- EDITABLE VARIABLE

// const int EVO = 56 - 5; // exhaust valve close before BDC during cycle 2 -- exhaust valve close after BDC during cycle 1 -- EDITABLE VARIABLE
// const int EVC = 28 - 5; // exhaust valve close during cycle 1 -- EDITABLE VARIABLE
// - 6 was good, -7 was too much


const int IVO = 33 - 5; // intake valve open during cycle 1 -- EDITABLE VARIABLE
const int IVC = 56 ; // intake valve close after BDC during cycle 2 -- intake valve close before BDC during cycle 1 -- EDITABLE VARIABLE
//37 pulses between open and close.
// 200 between opens =34 pulses dr

const int EVO = 56 - 5; // exhaust valve close before BDC during cycle 2 -- exhaust valve close after BDC during cycle 1 -- EDITABLE VARIABLE
const int EVC = 28 - 5; // exhaust valve close during cycle 1 -- EDITABLE VARIABLE


// -----------------------------------------------
// ------------ VARIABLES FOR TIMERS -------------
// -----------------------------------------------
unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long currentMillis;
const unsigned long lcd_update_period_start = 500000;  //Period for how often the LCD screen should be updated in (micros) - EDITABLE VARIABLE
const unsigned long lcd_update_period_main = 50000;  //Period for how often the LCD screen should be updated in (micros) - EDITABLE VARIABLE

unsigned long interrupt_time = micros();
unsigned long last_interrupt_time = interrupt_time;


// -----------------------------------------------
// ----------- VARIABLES FOR STOPPING ------------
// -----------------------------------------------
bool emergency_stop;
bool normal_stop;
int stop_code_str;


// -----------------------------------------------
// ----------- LOOP Variables ------------
// -----------------------------------------------
bool start_reset = true;


// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------


void setup() {

  // put your setup code here, to run once:
  Serial.begin(9600);


  // initialize digital output pins
  Breakout.pinMode(intake_valve_driver, OUTPUT);
  Breakout.pinMode(exhaust_valve_driver, OUTPUT);
  Breakout.pinMode(start_sequence_driver, OUTPUT);
  

  // initialize the sensor_1 pin as an input:
  Breakout.pinMode(sensor_1_pin, INPUT_PULLUP);
  Breakout.pinMode(my_pulse_pin, OUTPUT);  // TEST - SANDRA


  // initialize the lcd display
  lcd.init();
  lcd.clear();         
  lcd.backlight();      // Make sure backlight is on

  // Update LCD
  lcd.setCursor(0,0);   //Set cursor to character 0 on line 0
  lcd.print("Group 28 - Start");

}



void loop(){    

  if (start_reset) {
    start_reset = false;
    start_sequence();
  }
}


void start_sequence() {

  sensor_1_start = false;
  sensor_1_main = false;
  loop_time_1 = 1000000000000;        // time difference between interrupts for sensor1 -- start engine within X sec of powering up or BDC won't be detected accurately
  loop_time_1_prev = 0;
  engine_cycle_counter = 0;         // keeps track of the cycle
  call_main_func = false;
  min_rpm_reached = false;


  bdc_found = false;         // bool indicating if BDC has been found
  pulse_count = 0;         // variable for tracking the pulse count
  pulse_check = 0;         // variable for tracking if the pulse count is off 
  pulse_check_prev = 1; 


  rpm_pulses = 0;
  rpm = 0;
  
  emergency_stop = false;
  normal_stop = false;
  stop_code_str = 0;


  // Start sequence - Valve initial positions - Intake valve slightly open
  digitalWrite(exhaust_valve_driver, HIGH);
  delay (200);
  digitalWrite(start_sequence_driver, HIGH);
  delay (200);
  digitalWrite(start_sequence_driver, LOW);


  // de-attach interrupt to sensor & E-Stop & Reset 
  detachInterrupt(digitalPinToInterrupt(sensor_1_pin));


  // Call start function  
  interrupt_time = micros();
  last_interrupt_time = interrupt_time; 
  start_function();

}



void start_function(){

  // attachInterrupt(digitalPinToInterrupt(sensor_1_pin), pin_ISR_1_start, RISING);
  attachInterrupt(digitalPinToInterrupt(sensor_1_pin), pin_ISR_1_start, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(sensor_1_pin), pin_ISR_1_start, HIGH);

  while ((!emergency_stop) && (!normal_stop) && (!call_main_func)) {

    currentMillis = micros();


    if (sensor_1_start) {
      detachInterrupt(digitalPinToInterrupt(sensor_1_pin));
      sensor_1_start = false;

      
      
      // Serial.print("Time btw interrupts S1 START (us): ");
      // Serial.println(loop_time_1);
      // Serial.print("Pulse Count: ");
      // Serial.println(pulse_count);
      // Serial.print("Cycle counter: ");
      // Serial.println(engine_cycle_counter);
      // Serial.println(" ");


      if (pulse_count == 57 && (bdc_found) && (min_rpm_reached)){
        emergency_stop = true;
        stop_code_str = 21;
      }

      if ((bdc_found) && (!emergency_stop) && (!normal_stop) && (min_rpm_reached)) {
        call_main_func = true;
      } 
      else {
        if ((!emergency_stop) && (!normal_stop)){
          // attachInterrupt(digitalPinToInterrupt(sensor_1_pin), pin_ISR_1_start, RISING);
          attachInterrupt(digitalPinToInterrupt(sensor_1_pin), pin_ISR_1_start, CHANGE);
          // attachInterrupt(digitalPinToInterrupt(sensor_1_pin), pin_ISR_1_start, HIGH);
        }
      }
    }

    rpm = rpm_pulses * 1000000 / (currentMillis - startMillis);
    if ((rpm > min_rpm) && (!min_rpm_reached)){
      min_rpm_reached = true;
    }

    if (currentMillis - startMillis >= lcd_update_period_start) {
      startMillis = currentMillis;
      rpm_pulses = 0; 

      if ((micros() - interrupt_time) > 300000000){
        normal_stop = true;
        stop_code_str = 1;
        detachInterrupt(digitalPinToInterrupt(sensor_1_pin));
      }
    }

  }

  if(call_main_func) {
    digitalWrite(exhaust_valve_driver, LOW);
    main_function();
  } 
  else{

    // CLOSE VALVES
    digitalWrite(intake_valve_driver, LOW);
    digitalWrite(exhaust_valve_driver, LOW);

    lcd.setCursor(0,1);   //Move cursor to character 0 on line 0
    lcd.print("Prev. code: ");
    lcd.setCursor(12,1);
    lcd.print(stop_code_str);

    delayMicroseconds(250000);
    start_reset = true;
  } 
}


void main_function(){
  
  // attachInterrupt(digitalPinToInterrupt(sensor_1_pin), pin_ISR_1_main, RISING);
  attachInterrupt(digitalPinToInterrupt(sensor_1_pin), pin_ISR_1_main, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(sensor_1_pin), pin_ISR_1_main, HIGH);

  while ((!emergency_stop) && (!normal_stop)) {

    currentMillis = micros();

    if (sensor_1_main) {

      detachInterrupt(digitalPinToInterrupt(sensor_1_pin));
      sensor_1_main = false;

      
      // Serial.print("Time btw interrupts S1 MAIN (us): ");
      // Serial.println(loop_time_1);
      // Serial.print("Pulse Count: ");
      // Serial.println(pulse_count);
      // Serial.print("Cycle counter: ");
      // Serial.println(engine_cycle_counter);
      // Serial.println(" ");
      
      if (pulse_count == 0){
         

        // if (pulse_check_prev != pulse_check){
        //   if (abs(pulse_check) > 10) {
        //     detachInterrupt(digitalPinToInterrupt(sensor_1_pin));
        //     emergency_stop = true;
        //     stop_code_str = 22;
        //   }
        // }

        pulse_check_prev = pulse_check;
        pulse_check = 0;
      }


      intake_pin_state = digitalRead(intake_valve_driver);
      exhaust_pin_state = digitalRead(exhaust_valve_driver);

      switch (pulse_count) {
        case IVO:
          if (intake_pin_state == 0 && engine_cycle_counter == 1){
            digitalWrite(intake_valve_driver, HIGH);                      
            intake_pin_state = digitalRead(intake_valve_driver);
          }
          break;

        case IVC:
          if (intake_pin_state == 1 && engine_cycle_counter == 1){
            digitalWrite(intake_valve_driver, LOW);
            intake_pin_state = digitalRead(intake_valve_driver);
          }
          break;

        case EVO:
          if (exhaust_pin_state == 0 && engine_cycle_counter == 2){
            digitalWrite(exhaust_valve_driver, HIGH);
            exhaust_pin_state = digitalRead(exhaust_valve_driver);
          }
          break;

        case EVC:
          if (exhaust_pin_state == 1 && engine_cycle_counter == 1){
            digitalWrite(exhaust_valve_driver, LOW);
            exhaust_pin_state = digitalRead(exhaust_valve_driver);
          }
          break;
      } 
      
      
      if ((!emergency_stop) && (!normal_stop)){
        // attachInterrupt(digitalPinToInterrupt(sensor_1_pin), pin_ISR_1_main, RISING);
        attachInterrupt(digitalPinToInterrupt(sensor_1_pin), pin_ISR_1_main, CHANGE);
        // attachInterrupt(digitalPinToInterrupt(sensor_1_pin), pin_ISR_1_main, HIGH);
      }
    }


    if (currentMillis - startMillis >= lcd_update_period_main) {

      startMillis = currentMillis;

      if ((micros() - interrupt_time) > 500000){
        normal_stop = true;
        stop_code_str = 2;
        detachInterrupt(digitalPinToInterrupt(sensor_1_pin));
      } 
    }

  }

  // CLOSE VALVES
  digitalWrite(intake_valve_driver, LOW);
  digitalWrite(exhaust_valve_driver, LOW);

  lcd.setCursor(0,1);   //Move cursor to character 0 on line 0
  lcd.print("Prev. code: ");
  lcd.setCursor(12,1);
  lcd.print(stop_code_str);

  delayMicroseconds(250000);
  start_reset = true;
}



void pin_ISR_1_start() {

  interrupt_time = micros();

  
  // If interrupts come faster than ### micros, assume it's a bounce and ignore
  // 0.25ms - 4000 rpm ==> 250 micros
  // 1ms - 1000 rpm
  // 2ms - 500 rpm


  if (interrupt_time - last_interrupt_time > 245) {


    digitalWrite(my_pulse_pin, HIGH);  // TEST - SANDRA

    loop_time_1_prev = loop_time_1;   

    loop_time_1 = interrupt_time - last_interrupt_time;
    last_interrupt_time = interrupt_time;

    if ((loop_time_1 > loop_time_1_prev * 3.5) && (loop_time_1 < loop_time_1_prev * 7)){
      pulse_count = 0;
      rpm_pulses = rpm_pulses + 3;

      if (min_rpm_reached){
        bdc_found = true;
        engine_cycle_counter = 1;
      }
      
    } else {
      rpm_pulses ++;
      pulse_count++;

      if (pulse_count == 113){
        pulse_count = 0;
      }

    }

    sensor_1_start = true;
    
  }

  delayMicroseconds(100);
  digitalWrite(my_pulse_pin, LOW); // TEST - SANDRA
  

}



void pin_ISR_1_main() {

  interrupt_time = micros();

  
  // If interrupts come faster than ### micros, assume it's a bounce and ignore
  // 0.25ms - 4000 rpm ==> 250 micros
  // 1ms - 1000 rpm
  // 2ms - 500 rpm
  if (interrupt_time - last_interrupt_time > 245) 
  {

    digitalWrite(my_pulse_pin, HIGH);  // TEST - SANDRA


    loop_time_1_prev = loop_time_1;

    loop_time_1 = interrupt_time - last_interrupt_time;
    last_interrupt_time = interrupt_time;

    if (loop_time_1 > loop_time_1_prev * 3.5) { 

      if (engine_cycle_counter == 1){
        engine_cycle_counter = 2;
      } else{
        engine_cycle_counter = 1;
      }

      pulse_check = pulse_count - 113;
      pulse_count = 0;

    } else {

      pulse_count++;

    }

    sensor_1_main = true;
    
  }

  delayMicroseconds(100);
  digitalWrite(my_pulse_pin, LOW); // TEST - SANDRA
  

}

