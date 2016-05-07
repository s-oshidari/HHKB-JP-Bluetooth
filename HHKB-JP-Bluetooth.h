//------------------------------------------------------------------------------
// RN42は日本語拡張のUSAGE IDを送信できないため、使用していない以下のIDで送信
// Windows側のマッピング変換ソフト(KeySwap Ver.4.02)で変換する
// 送れないID     代替ID
// ========== ==========
//   (\,|) 89   () 
//   (\,_) 87   () 
//  (無変) 8B  (Home) 4A
//  (変換) 8A   (End) 4D
//------------------------------------------------------------------------------

#define KEY_ROLL_OVER 6
#define MAX_ROWS 16
#define MAX_COLS  8
#define KEY_PRESSED LOW
#define COL_ENABLE LOW
#define COL_DISABLE HIGH
#define UNUSED 0
#define HID_KEYCODE_MODIFIER_MIN 0xE0
#define HID_KEYCODE_MODIFIER_MAX 0xE7
#define STATE_ON 0x01
#define STATE_OFF 0x00
#define SCAN_RATE 8
#define BT_MODE // Bluetooth Mode
//#define DEBUG

/* Arduino Pins */
int muxRowControlPin[] = {3, 4, 5};
int muxColControlPin[] = {6, 7, 8};
int enableColPin = 9;
int switchRawPin[] = {11, 12};
int keyReadPin = 2;

//SoftwareSerial softSerial(0, 1); // RX, TX

/* Multiplexer channel */
int muxChannel[16][3] = {
    {0, 0, 0}, // channel 0
    {1, 0, 0}, // channel 1
    {0, 1, 0}, // channel 2
    {1, 1, 0}, // channel 3
    {0, 0, 1}, // channel 4
    {1, 0, 1}, // channel 5
    {0, 1, 1}, // channel 6
    {1, 1, 1}, // channel 7
    {0, 0, 0}, // channel 8
    {1, 0, 0}, // channel 9
    {0, 1, 0}, // channel A
    {1, 1, 0}, // channel B
    {0, 0, 1}, // channel C
    {1, 0, 1}, // channel D
    {0, 1, 1}, // channel E
    {1, 1, 1}, // channel F
};

uint8_t zeroState[MAX_ROWS][MAX_COLS] = {0};
uint8_t prevState[MAX_ROWS][MAX_COLS];
uint8_t currentState[MAX_ROWS][MAX_COLS];

/* KEYMAP KEY TO HID KEYCODE*/
uint8_t KEYMAP_NORMAL_MODE[MAX_ROWS][MAX_COLS] = {
    {UNUSED, UNUSED, 0x29   /*ESC    */, 0x2B   /*TAB    */, UNUSED /*Fn     */, 0xE1   /*Lshift */, 0xE0   /*LCtrl  */, UNUSED},
    {UNUSED, UNUSED, 0x21   /*4      */, 0x08   /*E      */, 0x4A   /*MuHKN  */, 0x06   /*C      */, 0x07   /*D      */, UNUSED},
    {UNUSED, UNUSED, 0x20   /*3      */, 0x1A   /*W      */, 0xE2   /*Lalt   */, 0x1B   /*X      */, 0x16   /*S      */, UNUSED},
    {UNUSED, UNUSED, 0x1E   /*1      */, UNUSED /*       */, 0x35   /*HHK    */, UNUSED /*       */, UNUSED /*       */, UNUSED},
    {UNUSED, UNUSED, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED},
    {UNUSED, UNUSED, 0x22   /*5      */, 0x15   /*R      */, UNUSED /*       */, 0x19   /*V      */, 0x09   /*F      */, UNUSED},
    {UNUSED, UNUSED, 0x1F   /*2      */, 0x14   /*Q      */, 0xE3   /*LGui   */, 0x1D   /*Z      */, 0x04   /*A      */, UNUSED},
    {UNUSED, UNUSED, 0x23   /*6      */, 0x17   /*T      */, 0x2C   /*Space  */, 0x05   /*B      */, 0x0A   /*G      */, UNUSED},
    {UNUSED, UNUSED, 0x26   /*9      */, 0x0C   /*I      */, 0x88   /*Kana   */, 0x36   /*,      */, 0x0E   /*K      */, UNUSED},
    {UNUSED, UNUSED, 0x25   /*8      */, 0x18   /*U      */, 0x4D   /*Henkan */, 0x10   /*M      */, 0x0D   /*J      */, UNUSED},
    {UNUSED, UNUSED, 0x24   /*7      */, 0x1C   /*Y      */, UNUSED /*       */, 0x11   /*N      */, 0x0B   /*H      */, UNUSED},
    {UNUSED, UNUSED, 0x27   /*0      */, 0x12   /*O      */, 0xE6   /*Ralt   */, 0x37   /*.      */, 0x0F   /*L      */, UNUSED},
    {UNUSED, UNUSED, 0x2A   /*BS     */, UNUSED /*       */, 0x4F   /*Right  */, 0xE5   /*Rshift */, 0x28   /*Enter  */, UNUSED},
    {UNUSED, UNUSED, 0x55   /*\      */, 0x30   /*[      */, 0x51   /*Down   */, 0x52   /*Up     */, 0x32   /*]      */, UNUSED},
    {UNUSED, UNUSED, 0x2D   /*-      */, 0x13   /*P      */, UNUSED /*Fn     */, 0x38   /*/      */, 0x33   /*;      */, UNUSED},
    {UNUSED, UNUSED, 0x2E   /*~      */, 0x2F   /*@      */, 0x50   /*Left   */, 0x56   /*Ro     */, 0x34   /*:      */, UNUSED}
};

/* KEYMAP KEY TO HID KEYCODE(Function Mode)*/
uint8_t KEYMAP_FN_MODE[MAX_ROWS][MAX_COLS] = {
    {UNUSED, UNUSED, UNUSED /*       */, 0x39   /*Caps   */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED},
    {UNUSED, UNUSED, 0x3D   /*F4     */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED},
    {UNUSED, UNUSED, 0x3C   /*F3     */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED},
    {UNUSED, UNUSED, 0x3A   /*F1     */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED},
    {UNUSED, UNUSED, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED},
    {UNUSED, UNUSED, 0x3E   /*F5     */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED},
    {UNUSED, UNUSED, 0x3B   /*F2     */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED},
    {UNUSED, UNUSED, 0x3F   /*F6     */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED /*       */, UNUSED},
    {UNUSED, UNUSED, 0x42   /*F9     */, 0x46   /*PSc/SRq*/, UNUSED /*       */, 0x4D   /*End    */, 0x4A   /*Home   */, UNUSED},
    {UNUSED, UNUSED, 0x41   /*F8     */, UNUSED /*       */, UNUSED /*       */, 0x2D   /*-      */, 0x38   /*/      */, UNUSED},
    {UNUSED, UNUSED, 0x40   /*F7     */, UNUSED /*       */, UNUSED /*       */, 0x57   /*+      */, 0x35   /**      */, UNUSED},
    {UNUSED, UNUSED, 0x43   /*F10    */, 0x47   /*ScrLk  */, UNUSED /*       */, 0x4E   /*PgDn   */, 0x4B   /*PgUp   */, UNUSED},
    {UNUSED, UNUSED, 0x4C   /*Del    */, UNUSED /*       */, 0xE4   /*RCtrl  */, UNUSED /*       */, UNUSED /*       */, UNUSED},
    {UNUSED, UNUSED, 0x49   /*Ins    */, UNUSED /*       */, 0xE7   /*Rgui   */, 0xE5   /*RShift */, UNUSED /*       */, UNUSED},
    {UNUSED, UNUSED, 0x44   /*F11    */, 0x48   /*Pus/Brk*/, UNUSED /*       */, 0x51   /*Down   */, 0x50   /*Left   */, UNUSED},
    {UNUSED, UNUSED, 0x45   /*F12    */, 0x52   /*Up     */, 0x4C   /*Del    */, UNUSED /*       */, 0x4F   /*Right  */, UNUSED}
};

int isSleeping = 0;


ISR(TIMER2_OVF_vect);
void enterSleep();
void initializeState(uint8_t state[MAX_ROWS][MAX_COLS]);
void readKey(int row, int col, uint8_t state[MAX_ROWS][MAX_COLS]);
void switchRowEnable(int row);
void selectMux(int channel, int* controlPin);
void enableSelectedColumn();
void disableSelectedColumn();
int copyKeyState(uint8_t from[MAX_ROWS][MAX_COLS], uint8_t to[MAX_ROWS][MAX_COLS]);
int isAnyKeyPressed(uint8_t state[MAX_ROWS][MAX_COLS]);
uint8_t keymapKeyToHidKeycode(int row, int col);
uint8_t normalKeyToFnKey(int row, int col);
void sendKeyCodes(uint8_t state[MAX_ROWS][MAX_COLS]);
void sendKeyCodesBySerial(uint8_t modifiers,
                          uint8_t keycode0,
                          uint8_t keycode1,
                          uint8_t keycode2,
                          uint8_t keycode3,
                          uint8_t keycode4,
                          uint8_t keycode5);
void showSendingKeyCodes(uint8_t modifiers, uint8_t keyCodes[KEY_ROLL_OVER]);

