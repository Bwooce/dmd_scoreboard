/*

 */

/*--------------------------------------------------------------------------------------
 Includes
 --------------------------------------------------------------------------------------*/
#include <SPI.h>        //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#include <DMD.h>        //
#include <TimerOne.h>   //
#include "Arial_black_16.h"
#include "fixednums7x15.h"
#include "fixednums8x16.h"

//#define DEBUG

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


#define SPI_CLOCK_DIV128 0x02 // actually SPI_CLOCK_DIV64's code..

//Fire up the DMD library as dmd
#define DISPLAYS_ACROSS 3
#define DISPLAYS_DOWN 1
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);

#define ISRTIME 1000

/*--------------------------------------------------------------------------------------
 Interrupt handler for Timer1 (TimerOne) driven DMD refresh scanning, this gets
 called at the period set in Timer1.initialize();
 --------------------------------------------------------------------------------------*/
void processISR() {

  if(debounce_home_up()) {
    home_score++;
  }
  if(debounce_home_down()) {
    home_score--;
  }

  if(debounce_away_up()) {
    away_score++;
  }
  if(debounce_away_down()) {
    away_score--;
  }  

  if(pause) {
    if(debounce_time_up()) {
      current_time +=100;
      current_time -= current_time % 100;
      time_updated = TRUE;
    }
    if(debounce_time_down()) {
      if(current_time < 100) {
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

  home_score = 0;
  away_score = 0;
  time_updated = TRUE;

  last_time_millis = millis();

  //initialize TimerOne's interrupt/CPU usage used to scan and refresh the display
  Timer1.initialize(ISRTIME);           //period in microseconds to call ScanDMD. Anything longer than 5000 (5ms) and you can see flicker.
  Timer1.attachInterrupt(processISR);   //attach the Timer1 interrupt to ScanDMD which goes to dmd.scanDisplayBySPI()

  //clear/init the DMD pixels held in RAM
  dmd.clearScreen(true);   //true is normal (all pixels off), false is negative (all pixels on)
  dmd.selectFont(fixednums7x15);

  current_time = 4500;

#ifdef DEBUG
  Serial.begin(9600);
#endif
  DEBUG_PRINTLN("Scoreboard starting");
}

void loop() {
  unsigned char i;
  static unsigned long last = millis();

  if(millis() - last > 1) {
    dmd.scanDisplayBySPI();
  }

  if(last_time_millis + CLOCK_UPDATE_DELAY_MILLIS < millis() && current_time > 0) {
    if(!pause && digitalRead(PAUSE_BUTTON_PIN) == HIGH) {
      DEBUG_PRINTLN("PAUSED...");
      pause = 1;
      digitalWrite(PAUSE_BUTTON_LIGHT, LOW);
    }
    if(pause && digitalRead(PAUSE_BUTTON_PIN) == LOW) {
      DEBUG_PRINTLN("UNPAUSED...");
      digitalWrite(PAUSE_BUTTON_LIGHT, HIGH);
      last_time_millis = millis() - 1000;
      pause = 0;
    }

    if(!pause) {
      DEBUG_PRINTLN("NOT PAUSED");
      for(i=0;i<(millis()-last_time_millis)/1000; i++)
        sub_second();
      DEBUG_PRINT("current_time is :");
      DEBUG_PRINTLN(current_time);
      time_updated = TRUE;
    }

    if(millis()-last_time_millis > 1000) {
      last_time_millis = millis();
    }
  }  
  if(time_updated) {
    show_countdown(current_time,CLOCK_START_POS);
    time_updated = FALSE; 
  }

  if(home_score > 999 || home_score < 0) {
    home_score = 0;
  }

  if(away_score > 999 || away_score < 0) {
    away_score = 0;
  }

  show_number_p1(home_score, 1,1, HOME_START_POS);
  show_number_p3(away_score, 1,1, AWAY_START_POS);

}


void show_countdown(unsigned int time, char base_pos) {
  dmd.selectFont(fixednums7x15);
  if(time > 60) {
    show_clock(time, 1, base_pos);
  } 
  else {
    show_clock(time, 0, base_pos);
    show_number_p2(time, 1, 1, base_pos);
  }
} 

/*--------------------------------------------------------------------------------------
 Show clock numerals on the screen from a 4 digit time value
 --------------------------------------------------------------------------------------*/
void show_clock(unsigned int time, byte show, char pos) {

  static unsigned int last_time = 0;
  static unsigned char last_pos, first = 1;
  unsigned char tt, hh, tn, un;
  unsigned char ntt, nhh, ntn, nun;
  char npos, override;

  ntt = '0'+((time%10000)/1000);
  nhh = '0'+((time%1000) /100);
  ntn = '0'+((time%100)  /10);
  nun = '0'+ (time%10);

  override = 0;

  if(first) {
    first = 0;
    last_time = 0;
    last_pos = 0;
    tt = hh = tn = un = 255;
  } 
  else {
    if(last_pos != pos+centered_pos(time)) {
      DEBUG_PRINTLN("OVERRIDE SET*****");
      override = 1;
    }
    npos = pos+centered_pos(last_time);
    tt = '0'+((last_time%10000)/1000);
    hh = '0'+((last_time%1000) /100);
    tn = '0'+((last_time%100)  /10);
    un = '0'+ (last_time%10);
    if(tt > '0' && (override || tt!=ntt || show == 0)) {
      DEBUG_PRINT("Override=");
      DEBUG_PRINT(override);
      DEBUG_PRINT(" tt is:");
      DEBUG_PRINT(tt);
      DEBUG_PRINT(" ntt is:");
      DEBUG_PRINT(ntt);
      DEBUG_PRINT(" show is:");
      DEBUG_PRINT(show);
      DEBUG_PRINTLN("Removing thousands");
      dmd.drawChar(npos,  1,   tt, GRAPHICS_NOR );   // thousands
    }
    if(tt > '0') {
      npos+=dmd.charWidth(tt);
    }
    if(override || hh!=nhh || show == 0) {
      dmd.drawChar(npos,  1, hh, GRAPHICS_NOR );   // hundreds
    }
    npos+=dmd.charWidth(hh);
    npos-=2;
    if(override || show == 0) {
      dmd.drawChar(npos,  -1, ':', GRAPHICS_NOR); // remove even the colon
    }
    npos+=6;
    if(override || tn!=ntn || show == 0) {
      dmd.drawChar(npos,  1, tn, GRAPHICS_NOR );   // tens
    }
    npos+=dmd.charWidth(tn);
    if(override || un!=nun || show == 0)
      dmd.drawChar(npos,  1, un, GRAPHICS_NOR );   // units
  }  

  if(!show) {
    first = 1; // if we come back, treat it as though it was clear
    return;
  }

  npos = pos+centered_pos(time);
  if(ntt > '0' && (override || tt!=ntt)) {  
    dmd.drawChar(npos,  1,   ntt, GRAPHICS_NORMAL );   // thousands
  }
  if(ntt > '0')
    npos+=dmd.charWidth(ntt);
  if(override || hh!=nhh) {
    dmd.drawChar(npos,  1, nhh, GRAPHICS_NORMAL );   // hundreds
  }
  npos += dmd.charWidth(nhh);
  npos -=2;
  dmd.drawChar(npos,  -1, ':', GRAPHICS_OR);   // clock colon overlay on
  npos+=6;
  if(override || tn!=ntn) {
    dmd.drawChar(npos,  1, ntn, GRAPHICS_NORMAL );   // tens
  }
  npos += dmd.charWidth(ntn);
  if(override || un!=nun) {
    dmd.drawChar(npos,  1, nun, GRAPHICS_NORMAL );   // units
  }

  tt = ntt;
  hh = nhh;
  tn = ntn;
  un = nun;
  last_pos = pos+centered_pos(time);
  last_time = time;

}

void add_second() {
  unsigned int sec, min;

  sec = current_time % 100;
  min = current_time - sec;

  if(++sec > 59) {
    sec = 0;
    min +=100;
  }
  if(min > 5900) {
    min = 0;
  }
  current_time=min+sec;
}

void sub_second() {
  unsigned int sec, min;

  sec = current_time % 100;
  min = current_time - sec;

  DEBUG_PRINT("Min is:");
  DEBUG_PRINT(min);
  DEBUG_PRINT(" Sec is:");
  DEBUG_PRINTLN(sec);

  if(sec < 1) {
    sec = 0;
    if(min >= 100) {
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
  current_time=min+sec;
}

void show_number_p1(unsigned int number, byte show, byte centered, char pos) {
  static unsigned int last_number = 0;
  static byte was_centered = 0;
  static byte first = 1;
  char dpos = 0;
  unsigned char display = 0;
  byte hh;

  dmd.selectFont(fixednums8x16);
  if(first) {
    first = 0;
  } 
  else {
    if(number == last_number)
      return;
    // erase the old number
    if(was_centered) {
      dpos = centered_pos(last_number);
    } 
    else {   
      dpos = 0;
    }
    display = ((last_number%1000) / 100); // hundreds
    if(display) {
      dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NOR); 
      dpos+=dmd.charWidth('0'+display);
      dpos++;
    }
    hh = display;
    display = ((last_number%100) / 10); // tens
    if(display || hh) {
      dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NOR); 
      dpos+=dmd.charWidth('0'+display);
      dpos++;
    }
    display = (last_number%10); // units
    dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NOR); 
  }

  last_number = number;
  was_centered = centered;

  if(!show){
    return;
  }

  if(centered) {
    dpos = centered_pos(number);
  } 
  else {   
    dpos = 0;
  }

  display = ((number%1000) / 100); // hundreds
  if(display) {
    dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NORMAL); 
    dpos+=dmd.charWidth('0'+display);
    dpos++;
  }
  hh = display;
  display = ((number%100) / 10); // tens
  if(display || hh) {
    dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NORMAL); 
    dpos+=dmd.charWidth('0'+display);
    dpos++;
  }
  display = (number%10); // units
  dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NORMAL); 

}

void show_number_p2(unsigned int number, byte show, byte centered, char pos) {
  static unsigned int last_number = 0;
  static byte was_centered = 0;
  static byte first = 1;
  char dpos = 0;
  unsigned char display = 0;
  byte hh;

  if(first) {
    first = 0;
  } 
  else {
    // erase the old number
    if(was_centered) {
      dpos = centered_pos(last_number);
    } 
    else {   
      dpos = 0;
    }
    display = ((last_number%1000) / 100); // hundreds
    if(display) {
      dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NOR); 
      dpos+=dmd.charWidth('0'+display);
      dpos++;
    }
    hh = display;
    display = ((last_number%100) / 10); // tens
    if(display || hh) {
      dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NOR); 
      dpos+=dmd.charWidth('0'+display);
      dpos++;
    }
    display = (last_number%10); // units
    dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NOR); 
  }

  last_number = number;
  was_centered = centered;

  if(!show){
    return;
  }

  if(centered) {
    dpos = centered_pos(number);
  } 
  else {   
    dpos = 0;
  }

  display = ((number%1000) / 100); // hundreds
  if(display) {
    dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NORMAL); 
    dpos+=dmd.charWidth('0'+display);
    dpos++;
  }
  hh = display;
  display = ((number%100) / 10); // tens
  if(display || hh) {
    dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NORMAL); 
    dpos+=dmd.charWidth('0'+display);
    dpos++;
  }
  display = (number%10); // units
  dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NORMAL); 

}

void show_number_p3(unsigned int number, byte show, byte centered, char pos) {
  static unsigned int last_number = 0;
  static byte was_centered = 0;
  static byte first = 1;
  char dpos = 0;
  unsigned char display = 0;
  byte hh;

  dmd.selectFont(fixednums8x16);
  if(first) {
    first = 0;
  } 
  else {
    if(number == last_number)
      return;
    // erase the old number
    if(was_centered) {
      dpos = centered_pos(last_number);
    } 
    else {   
      dpos = 0;
    }
    display = ((last_number%1000) / 100); // hundreds
    if(display) {
      dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NOR); 
      dpos+=dmd.charWidth('0'+display);
      dpos++;
    }
    hh = display;
    display = ((last_number%100) / 10); // tens
    if(display || hh) {
      dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NOR); 
      dpos+=dmd.charWidth('0'+display);
      dpos++;
    }
    display = (last_number%10); // units
    dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NOR); 
  }

  last_number = number;
  was_centered = centered;

  if(!show){
    return;
  }

  if(centered) {
    dpos = centered_pos(number);
  } 
  else {   
    dpos = 0;
  }

  display = ((number%1000) / 100); // hundreds
  if(display) {
    dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NORMAL); 
    dpos+=dmd.charWidth('0'+display);
    dpos++;
  }
  hh = display;
  display = ((number%100) / 10); // tens
  if(display || hh) {
    dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NORMAL); 
    dpos+=dmd.charWidth('0'+display);
    dpos++;
  }
  display = (number%10); // units
  dmd.drawChar(pos+dpos, 1, '0'+display, GRAPHICS_NORMAL); 

}


byte centered_pos(unsigned int number) {
  if(number > 999) {
    return 0;
  }
  if(number > 99) {
    return 3;
  }
  if(number > 9) {
    return 8;
  }
  return 12;
}


char debounce_home_up() {
  static unsigned int state = 0;

  state = (state << 1) | digitalRead(HOME_UP_BUTTON_PIN) | 0xe000;
  if(state == 0xF000) {
    DEBUG_PRINTLN("HOME UP");
    return 1;
  }
  return 0;
}

char debounce_home_down() {
  static unsigned int state = 1;

  state = (state << 1) | digitalRead(HOME_DOWN_BUTTON_PIN) | 0xe000;
  if(state == 0xF000) {
    DEBUG_PRINTLN("HOME DOWN");
    return 1;
  }
  return 0;
}

char debounce_away_up() {
  static unsigned int state = 0;

  state = (state << 1) | digitalRead(AWAY_UP_BUTTON_PIN) | 0xe000;
  if(state == 0xF000) {
    DEBUG_PRINTLN("AWAY UP");
    return 1;
  }
  return 0;
}

char debounce_away_down() {
  static unsigned int state = 0;

  state = (state << 1) | digitalRead(AWAY_DOWN_BUTTON_PIN) | 0xe000;
  if(state == 0xF000) {
    DEBUG_PRINTLN("AWAY DOWN");
    return 1;
  }
  return 0;
}

char debounce_time_up() {
  static unsigned int state = 0;

  state = (state << 1) | digitalRead(TIME_UP_BUTTON_PIN) | 0xe000;
  if(state == 0xF000) {
    DEBUG_PRINTLN("TIME UP");
    return 1;
  }
  return 0;
}

char debounce_time_down() {
  static unsigned int state = 0;

  state = (state << 1) | digitalRead(TIME_DOWN_BUTTON_PIN) | 0xe000;
  if(state == 0xF000) {
    DEBUG_PRINTLN("TIME DOWN");
    return 1;
  }
  return 0;
}





