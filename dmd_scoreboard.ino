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


//#define DEBUG true

#ifdef DEBUG
#define DEBUG_PRINT(x)      Serial.print (x)
#define DEBUG_PRINTDEC(x)   Serial.print (x, DEC)
#define DEBUG_PRINTLN(x)    Serial.println (x)
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


struct button {
  byte id;
  unsigned int mask;
  bool high;
  unsigned int state;
};

struct button buttons[] = {
  {TIME_UP_BUTTON_PIN, 0xFFE0, true, 0},
  {TIME_DOWN_BUTTON_PIN, 0xFFE0, true, 0},
  {HOME_UP_BUTTON_PIN, 0xFFE0, true, 0},
  {HOME_DOWN_BUTTON_PIN, 0xFFE0, true, 0},
  {AWAY_UP_BUTTON_PIN, 0xFFE0, true, 0},
  {AWAY_DOWN_BUTTON_PIN, 0xFFE0, true, 0},
  {PAUSE_BUTTON_PIN, 0xFFE0, true, 0},
  {REMOTE_BUTTON_A, 0xFFFC, false, 0},
  {REMOTE_BUTTON_B, 0xFFFC, false, 0},
  {REMOTE_BUTTON_C, 0xFFFC, false, 0},
  {REMOTE_BUTTON_D, 0xFFFC, false, 0}
};

#define DOWN   (-1)
#define UP     1

#define CLOCK_START_POS  32
#define AWAY_START_POS   0
#define HOME_START_POS  64

#define TRUE    1
#define FALSE   (!TRUE)

int home_score, away_score;

char time_updated;
bool home_updated;
bool away_updated;

#define CLOCK_UPDATE_DELAY_MILLIS  1000
unsigned int current_time; // in seconds
long last_time_millis = 0;
char colon = 1;
char pause = 0;


//Fire up the DMD library as dmd
#define DISPLAYS_ACROSS 3
#define DISPLAYS_DOWN 1
SPIDMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);

void processButtons() {

  static unsigned long lastMillis = millis();

  unsigned long now = millis();
  if (now - lastMillis < 20) {
    return; // no need to debounce less than 50ms steps
  }
#ifdef DEBUG
  if (now - lastMillis > 40) {
    DEBUG_PRINT("*************************missed cycles: ");
    DEBUG_PRINTLN((now - lastMillis) / 20);
  }
#endif

  lastMillis = now;

  byte qty = sizeof(buttons) / sizeof(button);
  int i;

  for (i = 0; i < qty; i++) {
    if (debounce(&buttons[i])) {
      DEBUG_PRINTLN(buttons[i].id);
      switch (buttons[i].id) {
        case HOME_UP_BUTTON_PIN:
        case REMOTE_BUTTON_A:
          home_updated = true;
          home_score++;
          break;
        case HOME_DOWN_BUTTON_PIN:
        case REMOTE_BUTTON_C:
          if (home_score) {
            home_updated = true;
            home_score--;
          }
          break;
        case AWAY_UP_BUTTON_PIN:
        case REMOTE_BUTTON_B:
          away_updated = true;
          away_score++;
          break;
        case AWAY_DOWN_BUTTON_PIN:
        case REMOTE_BUTTON_D:
          if (away_score) {
            away_updated = true;
            away_score--;
          }
          break;
        case TIME_UP_BUTTON_PIN:
          if (pause) {
            if (current_time > 100) {
              current_time += 100;
              current_time -= current_time % 100;
              time_updated = TRUE;
            }
            else {
              current_time = 100;
            }
          }
          break;
        case TIME_DOWN_BUTTON_PIN:
          if (pause) {
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
  home_updated = TRUE;
  away_updated = TRUE;
  pause = FALSE;

  last_time_millis = millis();

  dmd.clearScreen();
  dmd.selectFont(fixednums8x16);
  dmd.setBrightness(255);
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
    if (!pause && digitalRead(PAUSE_BUTTON_PIN) == LOW) {
      DEBUG_PRINTLN("PAUSED...");
      digitalWrite(PAUSE_BUTTON_LIGHT, LOW);
      dmd.setBrightness(32);
      pause = true;
    }
    if (pause && digitalRead(PAUSE_BUTTON_PIN) == HIGH) {
      DEBUG_PRINTLN("UNPAUSED...");
      digitalWrite(PAUSE_BUTTON_LIGHT, HIGH);
      last_time_millis = millis(); // // stop double-decrement on unpause // not - 1000;
      dmd.setBrightness(255);
      pause = false;
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

  if (home_updated) {
    dmd.drawFilledBox(HOME_START_POS, 0, HOME_START_POS + 31, 15, GRAPHICS_OFF); // clear the box
    show_number(home_score, HOME_START_POS, 1, 1);
    home_updated = false;
  }
  if (away_updated) {
    dmd.drawFilledBox(AWAY_START_POS, 0, AWAY_START_POS + 31, 15, GRAPHICS_OFF); // clear the box
    show_number(away_score, AWAY_START_POS, 1, 1);
    away_updated = false;
  }

}

void show_countdown(unsigned int time, char base_pos) {
  //dmd.selectFont(fixednums7x15);
  if (time > 60 || time == 0) {
    show_clock(time, CLOCK_START_POS);
  }
  else {
    show_number(time, CLOCK_START_POS, 1, 1);
  }
}

/*--------------------------------------------------------------------------------------
 Show clock numerals on the screen from a 4 digit time value
 --------------------------------------------------------------------------------------*/
void show_clock(unsigned int time, byte pos) {

  unsigned char ntt, nhh, ntn, nun;
  byte npos;


  dmd.selectFont(fixednums7x15);

  ntt = '0' + ((time % 10000) / 1000);
  nhh = '0' + ((time % 1000) / 100);
  ntn = '0' + ((time % 100)  / 10);
  nun = '0' + (time % 10);

  npos = centered_pos(time);
  //dmd.drawFilledBox(pos, 1, pos + 31, 15, false); // clear the box, redraw the character.
  if (ntt > 0) {
    dmd.drawChar(pos + npos, 1, ntt, GRAPHICS_NORMAL, fixednums7x15);
    pos += dmd.charWidth(ntt) + 1;
  }
  dmd.drawChar(pos + npos, 1, nhh, GRAPHICS_NORMAL, fixednums7x15);
  pos += dmd.charWidth(ntt); // allow for colon
  dmd.drawChar(pos + npos, -1, ':', GRAPHICS_OR, fixednums7x15);
  pos += 2;
  dmd.drawChar(pos + npos, 1, ntn, GRAPHICS_NORMAL, fixednums7x15);
  pos += dmd.charWidth(ntt) + 1;
  dmd.drawChar(pos + npos, 1, nun, GRAPHICS_NORMAL, fixednums7x15);
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
  dmd.drawString(pos + dpos, 1, sNumber, GRAPHICS_NORMAL, fixednums8x16);
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


char debounce(struct button *b) {
  byte result = digitalRead(b->id);
  if (b->high == true) {
    result = !result;
  }
  b->state = ((b->state << 1) | result) | b->mask; // cut off top bits - set them to 1's

#ifdef DEBUG
  if (b->id == 19 && result) {
    DEBUG_PRINTLN(result);
    DEBUG_PRINT("remote up button debounce buffer: ");
    DEBUG_PRINTLN(b->state);
  }
#endif
  if (b->state == 0xFFFE) { // we want a trailing 0 to prevent button repeats. Change to 0xFFFF to permit repeats
    DEBUG_PRINT(b->id);
    DEBUG_PRINTLN("BUTTON PRESSED");
    b->state = 0;
    return 1;
  }

  return 0;
}






