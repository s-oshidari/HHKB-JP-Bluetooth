#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "HHKB-JP-Bluetooth.h"

void setup()
{
  Serial.begin(115200);

  // set pin mode
  for (int i = 0; i < 3; i++) {
    pinMode(muxRowControlPin[i], OUTPUT);
    pinMode(muxColControlPin[i], OUTPUT);
  }
  pinMode(enableColPin, OUTPUT);
  for (int i = 0; i < 2; i++) {
    pinMode(switchRawPin[i], OUTPUT);
  }
  pinMode(keyReadPin, INPUT);

  // initialize pin status
  for (int i = 0; i < 3; i++) {
    digitalWrite(muxRowControlPin[i], LOW);
    digitalWrite(muxColControlPin[i], LOW);
  }
  digitalWrite(enableColPin, COL_DISABLE);
  for (int i = 0; i < 2; i++) {
    digitalWrite(switchRawPin[i], LOW);
  }

  initializeState(prevState);
  initializeState(currentState);
}

void loop()
{
  unsigned long scanStart = millis();

  for (int row = 0; row < MAX_ROWS; row++) {
    for (int col = 0; col < MAX_COLS; col++) {
      readKey(row, col, currentState);
    }
  }
  
  // Check if some keyes are pressed currently
  if (isAnyKeyPressed(currentState)) {
    sendKeyCodes(currentState);

  // Check if some keyes were pressed previously
  // and no key is pressed currently
  } else if (isAnyKeyPressed(prevState)) {
    sendKeyCodes(zeroState);
  }

  copyKeyState(currentState, prevState);

  unsigned long scanEnd = millis();

  // adjust scan rate to SCAN_RATE
  if ((scanEnd - scanStart) < SCAN_RATE) {
    enterSleep();
  }
}

void enterSleep()
{
}

void initializeState(uint8_t state[MAX_ROWS][MAX_COLS])
{
  for (int row = 0; row < MAX_ROWS; row++) {
    for (int col = 0; col < MAX_COLS; col++) {
      state[row][col] = STATE_OFF;
    }
  }
}

void readKey(int row, int col, uint8_t state[MAX_ROWS][MAX_COLS])
{
  // select row enable
  switchRowEnable(row);
  
  // select row
  selectMux(row, muxRowControlPin);

  // select and enable column
  selectMux(col, muxColControlPin);
  enableSelectedColumn();

  if (digitalRead(keyReadPin) == KEY_PRESSED) {
#ifdef DEBUG
    Serial.print("(");
    Serial.print(row);
    Serial.print(",");
    Serial.print(col);
    Serial.println(") ON");
#endif
    state[row][col] = STATE_ON;
  } else {
    state[row][col] = STATE_OFF;
  }

  disableSelectedColumn();
}

void switchRowEnable(int row)
{
  if (row < 8) {
    digitalWrite(switchRawPin[0], 0);
    digitalWrite(switchRawPin[1], 1);
  } else {
    digitalWrite(switchRawPin[0], 1);
    digitalWrite(switchRawPin[1], 0);
  }
}

void selectMux(int channel, int* controlPin)
{
  for (int i = 0; i < 3; i++) {
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }
  delayMicroseconds(15);
}

void enableSelectedColumn()
{
  digitalWrite(enableColPin, COL_ENABLE);
}

void disableSelectedColumn()
{
  digitalWrite(enableColPin, COL_DISABLE);
  delayMicroseconds(5);
}

int copyKeyState(uint8_t from[MAX_ROWS][MAX_COLS],
         uint8_t to[MAX_ROWS][MAX_COLS])
{
  for (int row = 0; row < MAX_ROWS; row++) {
    for (int col = 0; col < MAX_COLS; col++) {
      to[row][col] = from[row][col];
    }
  }
}

int isAnyKeyPressed(uint8_t state[MAX_ROWS][MAX_COLS])
{
  for (int row = 0; row < MAX_ROWS; row++) {
    for (int col = 0; col < MAX_COLS; col++) {
      if (state[row][col] == STATE_ON) return 1;
    }
  }
  return 0;
}

uint8_t keymapKeyToHidKeycode(int row, int col)
{
  return KEYMAP_NORMAL_MODE[row][col];
}

uint8_t normalKeyToFnKey(int row, int col)
{
  return KEYMAP_FN_MODE[row][col];
}

void sendKeyCodes(uint8_t state[MAX_ROWS][MAX_COLS])
{
  uint8_t modifiers = 0x00;
  uint8_t keyCodes[KEY_ROLL_OVER] = {0};
  int fnFlag = 0;
  int numOfKeyDown = 0;
  int hidKeycode;

  // check FnKey is pressed
  if (state[0][4] == STATE_ON || state[14][4] == STATE_ON) {
    fnFlag = 1;
  }

  for (int row = 0; row < MAX_ROWS; row++) {
    for (int col = 0; col < MAX_COLS; col++) {
      if (state[row][col] == STATE_ON) {
        hidKeycode = keymapKeyToHidKeycode(row, col);

        // modifier key is pressed
        if (HID_KEYCODE_MODIFIER_MIN <= hidKeycode && 
          hidKeycode <= HID_KEYCODE_MODIFIER_MAX) {
          modifiers |= (1 << (hidKeycode - HID_KEYCODE_MODIFIER_MIN));

        // ordinary key is pressed
        } else if (numOfKeyDown < KEY_ROLL_OVER) {
          if (fnFlag) {
            hidKeycode = normalKeyToFnKey(row, col);
          }
          if (hidKeycode != UNUSED) {
            keyCodes[numOfKeyDown++] = hidKeycode;
          }
        }
      }
    }
  }
#ifdef DEBUG
  showSendingKeyCodes(modifiers, keyCodes);
#endif
  sendKeyCodesBySerial(modifiers, keyCodes[0], keyCodes[1], keyCodes[2], keyCodes[3], keyCodes[4], keyCodes[5]);
}

void sendKeyCodesBySerial(uint8_t modifiers, uint8_t keycode0, uint8_t keycode1, uint8_t keycode2, uint8_t keycode3, uint8_t keycode4, uint8_t keycode5)
{
  Serial.write(0xFD); // Raw Report Mode
  Serial.write(0x09); // Length
  Serial.write(0x01); // Descriptor 0x01=Keyboard

  /* send key codes(8 bytes all) */
  Serial.write(modifiers);  // modifier keys
  Serial.write((byte)0x00); // reserved
  Serial.write(keycode0);   // keycode0
  Serial.write(keycode1);   // keycode1
  Serial.write(keycode2);   // keycode2
  Serial.write(keycode3);   // keycode3
  Serial.write(keycode4);   // keycode4
  Serial.write(keycode5);   // keycode5
  delay(5);
}

void showSendingKeyCodes(uint8_t modifiers, uint8_t keyCodes[KEY_ROLL_OVER])
{
  Serial.print("(");
  Serial.print(modifiers);
  Serial.print(", ");
  Serial.print(keyCodes[0]);
  Serial.print(", ");
  Serial.print(keyCodes[1]);
  Serial.print(", ");
  Serial.print(keyCodes[2]);
  Serial.print(", ");
  Serial.print(keyCodes[3]);
  Serial.print(", ");
  Serial.print(keyCodes[4]);
  Serial.print(", ");
  Serial.print(keyCodes[5]);
  Serial.println(")");
}

