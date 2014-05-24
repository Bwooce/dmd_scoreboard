/*


*/

/*--------------------------------------------------------------------------------------
 Includes
 --------------------------------------------------------------------------------------*/
#include <SPI.h>        //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#define uint8_t byte
#include <DMD2.h>        //
#include "fonts/Arial_Black_16.h"
#include "fonts/fixednums8x16.h"
#include "fonts/fixednums7x15.h"


#define DEBUG true

#ifdef DEBUG
#define DEBUG_PRINT(x)     Serial.print (x)
#define DEBUG_PRINTDEC(x)     Serial.print (x, DEC)
#define DEBUG_PRINTLN(x)  Serial.println (x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTDEC(x)
#define DEBUG_PRINTLN(x)
#endif

#define TIME_UP_BUTTON_PIN 5
#define TIME_DOWN_BUTTON_PIN 4
#define HOME_UP_BUTTON_PIN 0
#define HOME_DOWN_BUTTON_PIN 1
#define AWAY_UP_BUTTON_PIN 2
#define AWAY_DOWN_BUTTON_PIN 3
#define PAUSE_BUTTON_PIN A0
#define PAUSE_BUTTON_LIGHT A1
#define REMOTE_BUTTON_B  A5
#define REMOTE_BUTTON_D  A4
#define REMOTE_BUTTON_A  A3
#define REMOTE_BUTTON_C  A2

#define DOWN   (-1)
#define UP     1

#define CLOCK_START_POS  32
#define AWAY_START_POS   0
#define HOME_START_POS  64

#define TRUE    1
#define FALSE   (!TRUE)

int home_score, away_score;

char time_updated;

#define CLOCK_UPDATE_DELAY_MILLIS  1000
unsigned int current_time; // in seconds
long last_time_millis = 0;
char colon = 1;
char pause = 0;


//Fire up the DMD library as dmd
#define DISPLAYS_ACROSS 3
#define DISPLAYS_DOWN 1
SPIDMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);

//#define ISRTIME 1000

void processButtons() {
  remote_buttons();

  if (debounce_home_up()) {
    home_score++;
  }
  if (debounce_home_down()) {
    home_score--;
  }

  if (debounce_away_up()) {
    away_score++;
  }
  if (debounce_away_down()) {
    away_score--;
  }

  if (pause) {
    if (debounce_time_up()) {
      if (current_time > 100) {
        current_time += 100;
        current_time -= current_time % 100;
        time_updated = TRUE;
      }
      else {
        current_time = 100;
      }
    }
    if (debounce_time_down()) {
      if (current_time < 100) {
        current_time = 0;
        time_updated = TRUE;
      }
      else {
        current_time -= current_time % 100;
        current_time -= 100;
        time_updated = TRUE;
      }
    }
  }

}


void setup() {

  // setup the button pins
  pinMode(TIME_UP_BUTTON_PIN, INPUT);
  digitalWrite(TIME_UP_BUTTON_PIN, HIGH); // enage pullup resistor
  pinMode(TIME_DOWN_BUTTON_PIN, INPUT);
  digitalWrite(TIME_DOWN_BUTTON_PIN, HIGH);

  pinMode(HOME_UP_BUTTON_PIN, INPUT);
  digitalWrite(HOME_UP_BUTTON_PIN, HIGH); // enage pullup resistor
  pinMode(HOME_DOWN_BUTTON_PIN, INPUT);
  digitalWrite(HOME_DOWN_BUTTON_PIN, HIGH);

  pinMode(AWAY_UP_BUTTON_PIN, INPUT);
  digitalWrite(AWAY_UP_BUTTON_PIN, HIGH);
  pinMode(AWAY_DOWN_BUTTON_PIN, INPUT);
  digitalWrite(AWAY_DOWN_BUTTON_PIN, HIGH);

  pinMode(PAUSE_BUTTON_PIN, INPUT);
  digitalWrite(PAUSE_BUTTON_PIN, HIGH);

  pinMode(PAUSE_BUTTON_LIGHT, OUTPUT);
  digitalWrite(PAUSE_BUTTON_LIGHT, LOW);
  
  pinMode(REMOTE_BUTTON_A, INPUT);
  digitalWrite(REMOTE_BUTTON_A, LOW);

  pinMode(REMOTE_BUTTON_B, INPUT);
  digitalWrite(REMOTE_BUTTON_B, LOW);
 
  pinMode(REMOTE_BUTTON_D, INPUT);
  digitalWrite(REMOTE_BUTTON_C, LOW);
 
  pinMode(REMOTE_BUTTON_D, INPUT);
  digitalWrite(REMOTE_BUTTON_D, LOW); 

  home_score = 0;
  away_score = 0;
  time_updated = TRUE;

  last_time_millis = millis();

  dmd.clearScreen();
  dmd.selectFont(fixednums7x15);
  dmd.begin();

  current_time = 4500;

#ifdef DEBUG
  Serial.begin(9600);
#endif
  DEBUG_PRINTLN("Scoreboard starting");
}

void loop() {
  unsigned char i;
  static unsigned long last = millis();

  processButtons();

  if (last_time_millis + CLOCK_UPDATE_DELAY_MILLIS < millis() && current_time > 0) {
    if (!pause && digitalRead(PAUSE_BUTTON_PIN) == HIGH) {
      DEBUG_PRINTLN("PAUSED...");
      pause = 1;
      digitalWrite(PAUSE_BUTTON_LIGHT, LOW);
    }
    if (pause && digitalRead(PAUSE_BUTTON_PIN) == LOW) {
      DEBUG_PRINTLN("UNPAUSED...");
      digitalWrite(PAUSE_BUTTON_LIGHT, HIGH);
      last_time_millis = millis(); // // stop double-decrement on unpause // not - 1000;
      pause = 0;
    }

    if (!pause) {
      DEBUG_PRINTLN("NOT PAUSED");
      for (i = 0; i < (millis() - last_time_millis) / 1000; i++)
        sub_second();
      DEBUG_PRINT("current_time is :");
      DEBUG_PRINTLN(current_time);
      time_updated = TRUE;
    }

    if (millis() - last_time_millis > 1000) {
      last_time_millis = millis();
    }
  }
  if (time_updated) {
    show_countdown(current_time, CLOCK_START_POS);
    time_updated = FALSE;
  }

  if (home_score > 999 || home_score < 0) {
    home_score = 0;
  }

  if (away_score > 999 || away_score < 0) {
    away_score = 0;
  }

  show_number(home_score,HOME_START_POS, 1,1);
  show_number(away_score,AWAY_START_POS,1,1);

}


void show_countdown(unsigned int time, char base_pos) {
  dmd.selectFont(fixednums7x15);
  if (time > 60 || time == 0) {
    show_clock(time, CLOCK_START_POS);
  }
  else {
    show_number(time,CLOCK_START_POS, 1,1);
  }
}

/*--------------------------------------------------------------------------------------
 Show clock numerals on the screen from a 4 digit time value
 --------------------------------------------------------------------------------------*/
void show_clock(unsigned int time, byte pos) {

  unsigned char ntt, nhh, ntn, nun;
  byte npos;

  ntt = '0' + ((time % 10000) / 1000);
  nhh = '0' + ((time % 1000) / 100);
  ntn = '0' + ((time % 100)  / 10);
  nun = '0' + (time % 10);

  npos = centered_pos(time);
  String sNumber = String(time);
  dmd.drawString(pos+npos, 1, sNumber);
}

void add_second() {
  unsigned int sec, min;

  sec = current_time % 100;
  min = current_time - sec;

  if (++sec > 59) {
    sec = 0;
    min += 100;
  }
  if (min > 5900) {
    min = 0;
  }
  current_time = min + sec;
}

void sub_second() {
  unsigned int sec, min;

  sec = current_time % 100;
  min = current_time - sec;

  DEBUG_PRINT("Min is:");
  DEBUG_PRINT(min);
  DEBUG_PRINT(" Sec is:");
  DEBUG_PRINTLN(sec);

  if (sec < 1) {
    sec = 0;
    if (min >= 100) {
      min = min - 100;
      sec = 59;
    }
    else {
      min = 0;
    }
  }
  else {
    sec--;
  }
  current_time = min + sec;
}

void show_number(unsigned int number, byte pos, byte show, byte centered) {

  char dpos = 0;
  byte hh;

  dmd.selectFont(fixednums8x16);

  if (centered) {
    dpos = centered_pos(number);
  }
  else {
    dpos = 0;
  }

  String sNumber = String(number);
  dmd.drawString(pos+dpos, 1, sNumber);
}

byte centered_pos(unsigned int number) {
  if (number > 999) {
    return 0;
  }
  if (number > 99) {
    return 3;
  }
  if (number > 9) {
    return 8;
  }
  return 12;
}


char debounce_home_up() {
  static unsigned int state = 0;

  state = (state << 1) | digitalRead(HOME_UP_BUTTON_PIN) | 0xe000;
  if (state == 0xF000) {
    DEBUG_PRINTLN("HOME UP");
    return 1;
  }
  return 0;
}

char debounce_home_down() {
  static unsigned int state = 1;

  state = (state << 1) | digitalRead(HOME_DOWN_BUTTON_PIN) | 0xe000;
  if (state == 0xF000) {
    DEBUG_PRINTLN("HOME DOWN");
    return 1;
  }
  return 0;
}

char debounce_away_up() {
  static unsigned int state = 0;

  state = (state << 1) | digitalRead(AWAY_UP_BUTTON_PIN) | 0xe000;
  if (state == 0xF000) {
    DEBUG_PRINTLN("AWAY UP");
    return 1;
  }
  return 0;
}

char debounce_away_down() {
  static unsigned int state = 0;

  state = (state << 1) | digitalRead(AWAY_DOWN_BUTTON_PIN) | 0xe000;
  if (state == 0xF000) {
    DEBUG_PRINTLN("AWAY DOWN");
    return 1;
  }
  return 0;
}

char debounce_time_up() {
  static unsigned int state = 0;

  state = (state << 1) | digitalRead(TIME_UP_BUTTON_PIN) | 0xe000;
  if (state == 0xF000) {
    DEBUG_PRINTLN("TIME UP");
    return 1;
  }
  return 0;
}

char debounce_time_down() {
  static unsigned int state = 0;

  state = (state << 1) | digitalRead(TIME_DOWN_BUTTON_PIN) | 0xe000;
  if (state == 0xF000) {
    DEBUG_PRINTLN("TIME DOWN");
    return 1;
  }
  return 0;
}

char remote_buttons() {
  if(digitalRead(REMOTE_BUTTON_A) ) {
      DEBUG_PRINTLN("REMOTE_BUTTON_A");
  }
  if(digitalRead(REMOTE_BUTTON_B) ) {
      DEBUG_PRINTLN("REMOTE_BUTTON_B");
  }
    if(digitalRead(REMOTE_BUTTON_C) ) {
      DEBUG_PRINTLN("REMOTE_BUTTON_C");
  }
    if(digitalRead(REMOTE_BUTTON_D) ) {
      DEBUG_PRINTLN("REMOTE_BUTTON_D");
  }
  return 1;
}






