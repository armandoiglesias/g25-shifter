#include <Joystick.h>
Joystick_ Joystick;


//
//  G25 shifter to USB interface
//  based on a Teensy 2.0
//
//  3 operating modes
//  - H-pattern shifter
//  - Sequential shifter
//  - Analog handrake
//

//
//  G25 shifter pinout
//
//  DB9  Color    Shifter Description       Teensy
//  1    Black    1       +5V               +5V
//  2    Grey     7       Button Data       pin 17
//  3    Yellow   5       Button !CS & !PL  pin 18
//  4    Orange   3       Shifter X axis    pin 21 (A0)
//  5    Red      2       SPI input 
//  6    White    8       GND               GND
//  7    Purple   6       Button Clock      pin 19
//  8    Green    4       Shifter Y axis    pin 20 (A1)
//  9    Black    1       +5V

// Teensy pin definitions
#define LED_PIN            0
#define DATA_IN_PIN        4
#define MODE_PIN           2
#define CLOCK_PIN          3
#define X_AXIS_PIN         A1
#define Y_AXIS_PIN         A0

// H-shifter mode analog axis thresholds
#define HS_XAXIS_12        400
#define HS_XAXIS_56        600
#define HS_YAXIS_135       800
#define HS_YAXIS_246       300

// Sequential shifter mode analog axis thresholds
#define SS_UPSHIFT_BEGIN   670
#define SS_UPSHIFT_END     600
#define SS_DOWNSHIFT_BEGIN 430
#define SS_DOWNSHIFT_END   500

// Handbrake mode analog axis limits
#define HB_MAXIMUM         530
#define HB_MINIMUM         400
#define HB_RANGE           (HB_MAXIMUM-HB_MINIMUM)

// Digital inputs definitions
#define DI_REVERSE         1
#define DI_MODE            3
#define DI_RED_CENTERRIGHT 4
#define DI_RED_CENTERLEFT  5
#define DI_RED_RIGHT       6
#define DI_RED_LEFT        7
#define DI_BLACK_TOP       8
#define DI_BLACK_RIGHT     9
#define DI_BLACK_LEFT      10
#define DI_BLACK_BOTTOM    11
#define DI_DPAD_RIGHT      12
#define DI_DPAD_LEFT       13
#define DI_DPAD_BOTTOM     14
#define DI_DPAD_TOP        15

// Shifter state
#define DOWN_SHIFT         -1
#define NO_SHIFT           0
#define UP_SHIFT           1

// Shifter mode
#define SHIFTER_MODE       0
#define HANDBRAKE_MODE     1

// LED blink counter
int led=0;

// Shifter state
int shift=NO_SHIFT;

// Handbrake mode
int mode=SHIFTER_MODE;

// Called at startup
// Must initialize hardware and software modules
void setup()
{
  // G25 shifter analog inputs configuration 
  pinMode(X_AXIS_PIN, INPUT_PULLUP);   // X axis
  pinMode(Y_AXIS_PIN, INPUT_PULLUP);   // Y axis
  
  // G25 shift register interface configuration 
  pinMode(DATA_IN_PIN, INPUT);         // Data in
  pinMode(MODE_PIN, OUTPUT);           // Parallel/serial mode
  pinMode(CLOCK_PIN, OUTPUT);          // Clock
  
  // LED output mode configuration 
  pinMode(LED_PIN, OUTPUT);            // LED
  
  // Virtual joystick configuration 
  Joystick.begin(true);        // Joystick output is synchronized
  
  // Virtual serial interface configuration 
  Serial.begin(38400);
  
  // Virtual joystick initialization 
  ///Joystick.setXAxis(0);
  //Joystick.setYAxis(0);
  //Joystick.setZAxis(0);
  //Joystick.Zrotate(0);
  //Joystick.sliderLeft(0);
  //Joystick.sliderRight(0);
  
  // Digital outputs initialization
  digitalWrite(LED_PIN, LOW);
  digitalWrite(MODE_PIN, HIGH);
  digitalWrite(CLOCK_PIN, HIGH); 
}

// Called in a loop after initialization
void loop() 
{
  // Reading of button states from G25 shift register
  int b[16];
  
  digitalWrite(MODE_PIN, LOW);         // Parallel mode: inputs are read into shift register
  delayMicroseconds(10);               // Wait for signal to settle
  digitalWrite(MODE_PIN, HIGH);        // Serial mode: data bits are output on clock falling edge
  
  for(int i=0; i<16; i++)              // Iteration over both 8 bit registers
  {
    digitalWrite(CLOCK_PIN, LOW);      // Generate clock falling edge
    delayMicroseconds(10);             // Wait for signal to settle
  
    b[i]=digitalRead(DATA_IN_PIN);     // Read data bit and store it into bit array
    
    digitalWrite(CLOCK_PIN, HIGH);     // Generate clock rising edge
    delayMicroseconds(10);             // Wait for signal to settle
  }

  // Reading of shifter position
  int x=analogRead(A0);                 // X axis
  int y=analogRead(A1);                 // Y axis
  
  // Handbrake mode logic
  if(b[DI_RED_CENTERLEFT]!=0)          // Is left center red button depressed?
  {
    if(b[DI_RED_CENTERRIGHT]!=0)       // Is right center red button also depressed?
    {
      if(b[DI_RED_RIGHT]!=0)           // Is rightmost red button also depressed?
      {
        mode=HANDBRAKE_MODE;           // Handbrake mode is activated if the 3 rightmost 
      }                                // red buttons are depressed
      if(b[DI_RED_LEFT]!=0)            // Is leftmost red button also depressed?
      {
        mode=SHIFTER_MODE;             // Handbrake mode is deactivated if the 3 leftmost
      }                                // red buttons are depressed
    }
  }
  
  // Current gear calculation
  int gear=0;                          // Default value is neutral

  if(b[DI_MODE]==0)                    // H-shifter mode?
  {
    if(x<HS_XAXIS_12)                  // Shifter on the left?
    {
      if(y>HS_YAXIS_135) gear=1;       // 1st gear
      if(y<HS_YAXIS_246) gear=2;       // 2nd gear
    }
    else if(x>HS_XAXIS_56)             // Shifter on the right?
    {
      if(y>HS_YAXIS_135) gear=5;       // 5th gear
      if(y<HS_YAXIS_246) gear=6;       // 6th gear
    }
    else                               // Shifter is in the middle
    {
      if(y>HS_YAXIS_135) gear=3;       // 3rd gear
      if(y<HS_YAXIS_246) gear=4;       // 4th gear
    }
  }
  else                                 // Sequential mode 
  {
    if(mode==SHIFTER_MODE)             // Shifter mode?
    {
      Joystick.setXAxis(0);
      if(shift==NO_SHIFT)              // Current state: no shift
      {
        if(y>SS_UPSHIFT_BEGIN)         // Shifter to the front?
        {
          gear=1;                      // Shift-up
          shift=UP_SHIFT;              // New state: up-shift
        }
        if(y<SS_DOWNSHIFT_BEGIN)       // Shifter to the rear?
        {
          gear=2;                      // Shift-down
          shift=DOWN_SHIFT;            // New state: down-shift
        }
      }
      if(shift==UP_SHIFT)              // Current state: up-shift?
      {
        if(y>SS_UPSHIFT_END) gear=1;   // Beyond lower up-shift threshold: up-shift
        else shift=NO_SHIFT;           // Else new state: no shift
      }
      if(shift==DOWN_SHIFT)            // Current state: down-shift
      {
        if(y<SS_DOWNSHIFT_END) gear=2; // Below higher down-shift threshold: down-shift
        else shift=0;                  // Else new state: no shift
      }
    }
    else                               // Handbrake mode
    {
      if(y<HB_MINIMUM) y=HB_MINIMUM;   // Clip minimum position
      if(y>HB_MAXIMUM) y=HB_MAXIMUM;   // Clip maximum position
      
      long offset=(long)y-HB_MINIMUM;
      y=(int)(offset*1023/HB_RANGE);   // Scale input between minimum and maximum clip position
      
      //Joystick.setXAxis(y);                   // Set handbrake analog output
      gear=0;                          // No gear engaged: neutral
    }
  }
  
  if(gear!=6) b[DI_REVERSE]=0;         // Reverse gear is allowed only on 6th gear position
  if(b[DI_REVERSE]==1) gear=0;         // 6th gear is deactivated if reverse gear is engaged
  
  // Release virtual buttons for all gears
  Joystick.setButton(1, LOW);
  Joystick.setButton(2, LOW);
  Joystick.setButton(3, LOW);
  Joystick.setButton(4, LOW);
  Joystick.setButton(5, LOW);
  Joystick.setButton(6, LOW);
  
  // Depress virtual button for current gear
  if(gear>0) Joystick.setButton(gear, HIGH);
  
  // Set state of virtual buttons for all the physical buttons (including reverse)
  for(int i=0; i<16; i++) Joystick.setButton(7+i, b[i]);
  
  // Write new virtual joystick state
  Joystick.sendState();
  
  // Write inputs and outputs (remove comments to debug)
  
  Serial.print(" X axis: ");
  Serial.print(x);
  Serial.print(" Y axis: ");
  Serial.print(y);
  Serial.print(" Digital inputs: ");
  for(int i=0; i<16; i++)Serial.print(b[i]);
  Serial.print(" ");
  Serial.print(" Gear: ");
  Serial.print(gear);
  Serial.print(" Mode: ");
  Serial.print(mode);
  Serial.print(" Shift: ");
  Serial.println(shift);
  

  // Blink the on-board LED
  if(++led==100) led=0;                     // Period is 100 cycles * 10ms = 1s
  if(led==0) digitalWrite(LED_PIN, LOW);    // LED is off for 50 cycles
  if(led==50) digitalWrite(LED_PIN, HIGH);  // LED is on for 50 cycles 
  
  // Wait
  delay(10);                                // Wait for 10ms
}
