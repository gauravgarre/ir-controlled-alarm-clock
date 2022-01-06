/*
  IR Controlled Alarm Clock

  An alarm clock controlled by an IR remote without
  an RTC. A 74HC595 shift register is used to drive
  the 4 digit 7 segment display.

  The circuit:
  (defined below)

  Created 3 January 2022
  By Gaurav Garre
  Modified 5 January 2022
  By Gaurav Garre

  https://github.com/gauravgarre/ir-controlled-alarm-clock

*/

#include "IRremote.h"

#define DATA 2      // 74HC595  pin 2 DS
#define LATCH 3     // 74HC595  pin 3 STCP
#define CLOCK 4     // 74HC595  pin 4 SHCP
#define RECEIVER 6  // IR receiver  pin 6
#define ALARM_LED 7 // ALARM led pin 7
#define SET_LED 8   // SET led  pin 8
#define D1 9        // Digit 1 of digit display
#define D2 10       // Digit 2 of digit display
#define D3 11       // Digit 3 of digit display
#define D4 12       // Digit 4 of digit display
#define BUZZER 13   // buzzer pin 13


IRrecv irrecv(RECEIVER);     // create instance of 'irrecv'
decode_results results;      // create instance of 'decode_results'

// initializes various global variables
int refresh = 1;
String time = "0000";
String alarm = "0000";
int initHour = 0;
int initMin = 0;
int hour = 0;
int min = 0;
int alarmOn = 0;

enum led_state { // FSM states
  TIME,
  SET,
  ALARM,
  OFF
};

enum led_state state = TIME;

int digitPins[4] = {D1, D2, D3, D4};

unsigned char table[] = // reference values for digit display
{0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x00};

// displays digit using shift register
void display(unsigned char num) { 
  digitalWrite(LATCH,LOW);
  shiftOut(DATA,CLOCK,MSBFIRST,table[num]);
  digitalWrite(LATCH,HIGH);
}

// displays some time to digit display
void displayTime(String time) {
  if (state == OFF) {
    clear();
    digitalWrite(SET_LED, LOW);
    return;
  }
  
  // individually writes each digit to the display
  // persistence of vision allows us to see all 4 digits
  for (int i = 0; i < 4; i++) {
    clear();
    display(time.substring(i, i+1).toInt());

    //setting a digit pin to LOW allows for writing
    digitalWrite(digitPins[i], LOW); 

    delay(refresh);
  }
}

// turns off display by blocking writing
// to all digits
void clear() {
  digitalWrite(D1, HIGH);
  digitalWrite(D2, HIGH);
  digitalWrite(D3, HIGH);
  digitalWrite(D4, HIGH);
}

// finds the time and updates time var
void checkTime() {
  // uses in-built timer and an offset to determine the time locally
  hour = (initHour + ((millis() + 60000 * initMin) / 3600000)) % 24;
  min = (initMin + (millis() / 60000)) % 60;

  // adds any missing leading zeroes
  String leading_hour = "";
  String leading_min = "";
  if (String(hour).length() == 1) {
    leading_hour = "0";
  }
  if (String(min).length() == 1) {
    leading_min = "0";
  }

  time = leading_hour + String(hour) + leading_min + String(min);
}

// finds 0-9 digit based on IR hex codes
int findIRDigit() {
  int temp = -1;
 
  switch (results.value) {
    case 0xFF6897: temp = 0;    break;
    case 0xFF30CF: temp = 1;    break;
    case 0xFF18E7: temp = 2;    break;
    case 0xFF7A85: temp = 3;    break;
    case 0xFF10EF: temp = 4;    break;
    case 0xFF38C7: temp = 5;    break;
    case 0xFF5AA5: temp = 6;    break;
    case 0xFF42BD: temp = 7;    break;
    case 0xFF4AB5: temp = 8;    break;
    case 0xFF52AD: temp = 9;    break;
  }

  return temp;
}

void setup() {
  pinMode(LATCH,OUTPUT); // initializes pins
  pinMode(CLOCK,OUTPUT);
  pinMode(DATA,OUTPUT);
  pinMode(SET_LED, OUTPUT);
  pinMode(ALARM_LED, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  irrecv.enableIRIn(); // start the receiver

  Serial.begin(9600);
}

void loop() {
  if (state == TIME) {
    checkTime();
  }

  // if alarm time is being selected,
  // display that instead of the real time
  if (state == ALARM) {
    displayTime(alarm);
  } else {
    displayTime(time);
  }

  // checks whether alarm needs to be played
  if (time.equals(alarm) && alarmOn) {
    digitalWrite(13, HIGH);
  }

  // checks for an IR signal
  if (irrecv.decode(&results)) {

    // inserts digit user entered at end of time
    if (state == SET) {
      int temp = findIRDigit();

      if (temp != -1) {
        time = time.substring(1, 4) + String(temp);
      }
    }

    // inserts digit user entered at end of alarm time
    if (state == ALARM) {
      int temp = findIRDigit();

      if (temp != -1) {
        alarm = alarm.substring(1, 4) + String(temp);
      }
    }

    // remaining IR button functionality
    switch (results.value) {
      case 0xFFA25D: // power button (display power)
        if (state != OFF) {
          state = OFF;
        } else {
          state = TIME;
        }
        break;

      case 0xFFE21D: // func button (set time)
        if (state == TIME) {
          digitalWrite(SET_LED, HIGH);
          state = SET;
        } else if (state == SET) {
          digitalWrite(SET_LED, LOW);
          state = TIME;
          initMin = time.substring(2, 4).toInt() - (millis() / 60000);
          initHour = time.substring(0, 2).toInt() - ((millis() + 60000 * initMin) / 3600000);
        }
        break;

      case 0xFF629D: // volume + button (alarm time)
        if (state == TIME) {
          state = ALARM;
          alarmOn = 1;
          digitalWrite(ALARM_LED, HIGH);
        } else if (state == ALARM) {
          state = TIME;
        }
        break;

      case 0xFFA857: // volume - button (snooze)
        digitalWrite(ALARM_LED, LOW);
        alarmOn = 0;
        digitalWrite(BUZZER, LOW);
    }

    irrecv.resume(); // receive the next value
  }  
}
