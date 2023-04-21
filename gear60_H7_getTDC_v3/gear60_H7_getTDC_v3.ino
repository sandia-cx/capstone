#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD
#include <Arduino_PortentaBreakout.h> 
 
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------

// -----------------------------------------------
// ---------- PIN SETUP & PIN VARIABLES ----------
// -----------------------------------------------

// ----- Sensor 1 -----
breakoutPin sensor_1_pin = GPIO_1;    
volatile int sensor_1_state;         // variable for reading the prox. sensor status
bool sensor_1_start = false;
float loop_time_1;              // time difference between intterupts for sensor1

// ----- Sensor 2 -----
breakoutPin sensor_2_pin = GPIO_5;     // Sensor2
volatile int sensor_2_state;         // variable for reading the prox. sensor status
bool sensor_2_start = false;
float loop_time_2;              // time difference between intterupts for sensor2
volatile int sensor_2_interrupt_counter;         // counts how many times the interrupt has been hit


// ----- STOP BUTTON -----


// ----- Driver 1 - Intake - Analog Outputs 1 & 2 -----


// ----- Driver 2 - Exhaust - Analog Outputs 3 & 4 -----


// ----- Analog Output 5 for Start Sequence -----


// ----- LCD Display -----
breakoutPin sda = I2C_SDA_0;
breakoutPin scl = I2C_SCL_0;
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2); // Change to (0x27,20,4) for 20x4 LCD.


// -----------------------------------------------
// -------------- TIMING VARIABLES ---------------
// -----------------------------------------------
bool tdc_found = false;         // bool indicating if TDC has been found
volatile int pulse_count = 0;         // variable for tracking the pulse count
volatile int deg_disp = 0;         // variable for tracking the gear displacement in degrees
volatile int pulse_check = 0;         // variable for tracking if the pulse count is off 


// -----------------------------------------------
// --------------- GEAR VARIABLES ----------------
// -----------------------------------------------
int gear_teeth_cnt = 60;
int deg_per_tooth = 6;


// -----------------------------------------------
// ---------------- RPM VARIABLES ----------------
// -----------------------------------------------
volatile float rpm_pulses = 0;
float rpm = 0;

// -----------------------------------------------
// --------- SERVOMOTOR SIGNAL VARIABLES --------- 
// -----------------------------------------------
const int IVO = 708; // intake valve open 12 deg before TDC
const int IVC = 210; // intake valve close 30 deg after BDC
int intake_valve_flag = 0; // 0 = closed - 1 = open

const int EVO = 510; // exhaust valve open 30 deg before BDC
const int EVC = 12; // exhaust valve close 12 deg after TDC
int exhaust_valve_flag = 0; // 0 = closed - 1 = open


// -----------------------------------------------
// ------------ VARIABLES FOR TIMERS -------------
// -----------------------------------------------
unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long currentMillis;
const unsigned long period = 5000;  //Period for how often the LCD screen should be updated in ms



// -----------------------------------------------
// ----------- VARIABLES FOR STOPPING ------------
// -----------------------------------------------
bool emergency_stop = false;
bool normal_stop = false;


// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------


void setup() {

  // put your setup code here, to run once:
  Serial.begin(9600);


  // initialize analog output pins


  // function that sets the valves to their starting positions


  // initialize Stop Button


  // initialize the sensor_1 pin as an input:
  Breakout.pinMode(sensor_1_pin, INPUT_PULLUP);
  detachInterrupt(digitalPinToInterrupt(sensor_1_pin));
  attachInterrupt(digitalPinToInterrupt(sensor_1_pin), pin_ISR_1_start, FALLING);

  // initialize the sensor_1 pin as an input:
  Breakout.pinMode(sensor_2_pin, INPUT_PULLUP);
  detachInterrupt(digitalPinToInterrupt(sensor_2_pin));
  attachInterrupt(digitalPinToInterrupt(sensor_2_pin), pin_ISR_2_start, FALLING);


  // initialize the lcd display
  lcd.init();
  lcd.clear();         
  lcd.backlight();      // Make sure backlight is on
  delay (1000);

  lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
  lcd.print("Group 28 Project");
  delay (2500);
  lcd.setCursor(0,1);   //Move cursor to character 2 on line 1
  lcd.print("Initializing..");
  delay (2500);

  lcd.clear(); 
  lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
  lcd.print("READY");
  
  
  //main function   
  main_function();

}

void loop(){

}


void main_function(){

  while ((!emergency_stop) || (!normal_stop)) {


    currentMillis = millis();
  
    if (sensor_1_start || sensor_2_start) {

      // Serial.println(pulse_count);
      // Serial.println(deg_disp);

      if (sensor_1_start){
        sensor_1_start = false;
        // Serial.print("Time btw interrupts S1: ");
        // Serial.println(loop_time_1);
        
      }
      
      if (sensor_2_start){

        sensor_2_start = false;
        // Serial.print("Time btw interrupts S2: ");
        // Serial.println(loop_time_2);

        if (pulse_count == 58){
          Serial.print("S2 timing check: ");
          Serial.println(pulse_check);

          lcd.setCursor(0,0);   //Move cursor to character 2 on line 1
          lcd.print("Off Time:     ");
          lcd.setCursor(10,0);   //Move cursor to character 2 on line 1
          lcd.print(pulse_check);
        }
      }
      // Serial.println(" ");
    }

    
    if (tdc_found) {
      switch (deg_disp) {
        case IVO:
          if (intake_valve_flag == 0){
            Serial.print("IVO: Open intake valve at deg disp: ");
            Serial.println(deg_disp);
            Serial.println(" ");
            intake_valve_flag = 1;
          }
          break;

        case IVC:
          if (intake_valve_flag == 1){
            Serial.print("IVC: Close intake valve at deg disp: ");
            Serial.println(deg_disp);
            Serial.println(" ");
            intake_valve_flag = 0;
          }
          break;

        case EVO:
          if (exhaust_valve_flag == 0){
            Serial.print("EVO: Open exhaust valve at deg disp: ");
            Serial.println(deg_disp);
            Serial.println(" ");
            exhaust_valve_flag = 1;
          }
          break;

        case EVC:
          if (exhaust_valve_flag == 1){
            Serial.print("EVC: Close exhaust valve at deg disp: ");
            Serial.println(deg_disp);
            Serial.println(" ");
            exhaust_valve_flag = 0;
          }
          break;
      }
      

      if (currentMillis - startMillis >= period) {
        // Serial.print("RPM PULSES: ");
        // Serial.println(rpm_pulses, 0);
        rpm = rpm_pulses * 1000 / (currentMillis - startMillis);
        startMillis = currentMillis;
        rpm_pulses = 0; 
        // Serial.print("RPM: ");
        // Serial.print(rpm, 2);
        // Serial.println(" ");

        lcd.setCursor(0,1);   //Move cursor to character 2 on line 1
        lcd.print("RPM:           ");
        lcd.setCursor(5,1);   //Move cursor to character 2 on line 1
        lcd.print(rpm, 2);
      }

    }
  
  }

}



void pin_ISR_1_start() {

  sensor_1_state = digitalRead(sensor_1_pin);

  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();

  // If interrupts come faster than .0005 ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 1.1 && sensor_1_state == 0) 
  {
    loop_time_1 = interrupt_time - last_interrupt_time;
    last_interrupt_time = interrupt_time;
    
    rpm_pulses ++;

    if (pulse_count == gear_teeth_cnt - 1)
    {
      pulse_count = 0;

      if (deg_disp < 720 - deg_per_tooth){
        deg_disp = deg_disp + deg_per_tooth;
      } else{
        deg_disp = 0;
      }
      
    } else {
      pulse_count++;
      deg_disp = deg_disp + deg_per_tooth;
    }

    sensor_1_start = true;
  
  }
}


void pin_ISR_2_start() {
  
  sensor_1_state = digitalRead(sensor_1_pin);
  sensor_2_state = digitalRead(sensor_2_pin);

  static unsigned long last_interrupt_time_2 = 0;
  unsigned long interrupt_time_2 = millis();

  // If interrupts come faster than .0005 ms, assume it's a bounce and ignore
  if (interrupt_time_2 - last_interrupt_time_2 > 1.1 && sensor_2_state == 0 && sensor_1_state == 1) 
  {
    loop_time_2 = interrupt_time_2 - last_interrupt_time_2;
    last_interrupt_time_2 = interrupt_time_2;

    rpm_pulses ++;

    if (tdc_found) {
      // if (pulse_count <= 57)
      if (sensor_2_interrupt_counter == 1)
      {
        pulse_check = pulse_count - 57;
        pulse_count = 58;
        sensor_2_interrupt_counter = 2;

        if (deg_disp < 360){
          deg_disp = 360 - (deg_per_tooth*2);
        } else {
          deg_disp = 720 - (deg_per_tooth*2);
        }
        
      } else {

        pulse_check = 0;
        pulse_count = 59;
        sensor_2_interrupt_counter = 1;

        if (deg_disp < 360){
          deg_disp = 360 - deg_per_tooth;
        } else {
          deg_disp = 720 - deg_per_tooth;
        }
      }

    } else {
      if (pulse_count <= 57) {
        pulse_count = 58;
        deg_disp = 708;
        tdc_found = true;
        sensor_2_interrupt_counter = 2;
      }
    }

    sensor_2_start = true;

  }
}
