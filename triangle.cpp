#include <simple2d.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

const uint8_t clockLED = 4;     // Connected to 74HC595 SRCLK   pin 11 (I11) gray
const uint8_t latchLED = 3;     // Connected to 74HC595 RCLK    pin 12 (I10) white
const uint8_t dataLED = 2;      // Connected to 74HC595 SER     pin 14 (I8)  black

const uint8_t dataSwitch = 12;    // 74HC165 SER_OUT pin 9  (D19) data in                    orange
const uint8_t latchSwitch = 11;   // 74HC165 SH/!LD  pin 1  (D20) shift or active low load   yellow
const uint8_t clockSwitch = 10;   // 74HC165 CLK     pin 2  (D21) clock that times shifting  green
const uint8_t doorSwitch = 9;

#define COLUMNS 18
#define SHELVES 5
#define COL_HEIGHT 180
#define BK_WIDTH 20
#define BOOKS (SHELVES * COLUMNS)
#define BOARDS ((BOOKS + 7) >> 3)         // one board controls 8 books (1 LED, 1 button each book)
#define BLINK_DURATION 50
#define BUTTON_CLICK_COUNT 50
#define BUTTON_DEBOUNCE 2
#define FOR_ARDUINO 0

#if !FOR_ARDUINO

#define LOW 0
#define HIGH 1
#define MSBFIRST 0
#define INPUT 0
#define OUTPUT 1

uint8_t shiftRegisterCounter;
uint8_t shiftRegisterContents[BOOKS];

int16_t digitalRead(uint8_t pin) { 
    if (dataSwitch == pin) {
        return (int16_t) shiftRegisterContents[shiftRegisterCounter];
    }
    return 0; 
}

void digitalWrite(uint8_t pin, uint8_t data) {
    if (clockSwitch == pin && HIGH == data) {
        ++shiftRegisterCounter;
    } else if (latchSwitch == pin && LOW == data) {
        shiftRegisterCounter = 0;
    }
 }

// stubs ignored by emulator

void delayMicroseconds(uint16_t ) {}
void delay(uint32_t ) {}
void shiftOut(uint8_t , uint8_t , uint8_t , uint8_t ) {}
void pinMode(uint8_t , uint8_t ) {}

#endif // !FOR_ARDUINO

#define BUTTON_CLICKED 255
#define LONG_PRESS 0
#define BUTTON_TIMER_STOPPED 0

const int8_t bookX[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17
}; 
const int8_t bookY[] = {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 
    40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40
};

uint8_t leds[BOARDS];
// chorded buttons are ignored
uint8_t buttonStart;      // button detected (255 if none detected)
uint8_t buttonPressed;    // button clicked or long pressed; (255 if neither)
uint8_t buttonClicked;    // 0: long press; 255: click; other: timer countdown

// !!! could save a few bytes by looking up bit mask in 8 uint8_t table
void clearLed(uint8_t index) {
    leds[index >> 3] &= ~(1 << (index & 7));
}

// !!! could save a few bytes by looking up bit mask in 8 uint8_t table
void setLed(uint8_t index) {
    leds[index >> 3] |= 1 << (index & 7);
}

// !!! could save a few bytes by looking up bit mask in 8 uint8_t table
void writeLed(uint8_t index, bool value) {
    uint8_t* addr = &leds[index >> 3];
    uint8_t setBit = 1 << (index & 7);
    if (value) {
        *addr |= setBit;
    } else {
        *addr &= ~setBit;
    }
}


// code for Simple2d

int button(uint8_t index) {
    return buttonStart == index;
}

int led(uint8_t index) {
    return (leds[index >> 3] >> (index & 7)) & 1;
}

// 80" high, 27" wide -- ~3 (5) 16" to 1 (18) 1.5" -- 10 to 1
void render() {
  for (int j = 0; j < SHELVES; ++j) {
    int s = j * COLUMNS;
    for (int i = 0; i < COLUMNS; i++) {
        int t = s + i;
        S2D_DrawQuad(
            i * BK_WIDTH + 20, j * COL_HEIGHT + COL_HEIGHT / 2 + 20, led(t), led(t), 1, 1,
            i * BK_WIDTH + 30, j * COL_HEIGHT + COL_HEIGHT / 2 + 20, led(t), led(t), 1, 1,
            i * BK_WIDTH + 30, j * COL_HEIGHT + COL_HEIGHT / 2 + 30, led(t), led(t), 1, 1,
            i * BK_WIDTH + 20, j * COL_HEIGHT + COL_HEIGHT / 2 + 30, led(t), led(t), 1, 1
        );
        S2D_DrawQuad(
            i * BK_WIDTH + 20, j * COL_HEIGHT + COL_HEIGHT / 2 + 40, .5 + button(t) * .5, 0, 0, 1,
            i * BK_WIDTH + 30, j * COL_HEIGHT + COL_HEIGHT / 2 + 40, .5 + button(t) * .5, 0, 0, 1,
            i * BK_WIDTH + 30, j * COL_HEIGHT + COL_HEIGHT / 2 + 50, .5 + button(t) * .5, 0, 0, 1,
            i * BK_WIDTH + 20, j * COL_HEIGHT + COL_HEIGHT / 2 + 50, .5 + button(t) * .5, 0, 0, 1
        );
    }
  }
  for (int j = 1; j < SHELVES; ++j) {
        S2D_DrawQuad(
              0, j * COL_HEIGHT + 20, 1, 1, 1, 1,
            400, j * COL_HEIGHT + 20, 1, 1, 1, 1,
            400, j * COL_HEIGHT + 30, 1, 1, 1, 1,
              0, j * COL_HEIGHT + 30, 1, 1, 1, 1
        );
  }
  S2D_Image *img = S2D_CreateImage("start.jpg");
  img->x = 25;
  img->y = 30;
  img->width = 40;
  img->height = COL_HEIGHT;
  S2D_DrawImage(img);
  S2D_FreeImage(img);
  img = S2D_CreateImage("igiveup.jpg");
  img->x = 125;
  img->y = 30;
  img->width = 40;
  img->height = COL_HEIGHT;
  S2D_DrawImage(img);
  S2D_FreeImage(img);
}

// !!! replace with table lookup since books are not equal width
void on_mouse(S2D_Event e) {
  int b, c;
  switch (e.type) {
    case S2D_MOUSE_DOWN:
        b = (e.y - 25 - COL_HEIGHT / 2) / COL_HEIGHT;
        if (b < 0) b = 0;
        b *= COLUMNS;
        c = (e.x - 15) / BK_WIDTH;
        if (c < 0) c = 0;
        b += c;
        if (b >= BOOKS) b = BOOKS - 1;
        shiftRegisterContents[b] = 1;
      break;
    case S2D_MOUSE_UP:
        memset(shiftRegisterContents, 0, sizeof(shiftRegisterContents));
        break;
  }
}

// code for portability (built in to Arduino)

int32_t random(uint16_t max) {
    return rand() % max;
}

// code going to Arduino
// arduino global state

uint8_t currentGame;

//  state questions:
    // are some books (e.g. 'I Give Up') excluded from possible goal?
    // could require 'I Give Up' (and others) to take long press
uint8_t gameState[8];

enum Game {
    SelectGameInit,
    SelectGame,
    ButtonTestInit,
    ButtonTest,
    ColderWarmerInit,
    ColderWarmer,
    Loser,
    Winner,
};

enum Button {
    StartButton,
    ButtonTestButton,
    ColderWarmerButton,
    IGiveUpButton
};

enum BT_State {
    BT_Last,
    BT_Duration
};

enum CW_State {
    CW_Goal,
    CW_Guess,
    CW_Guess_Blink_Duration,
    CW_Guess_Blink_Count,
    CW_Distance,
    CW_Tries
};

void open_door() {
    digitalWrite(doorSwitch, HIGH);
    delay(100);
    digitalWrite(doorSwitch, LOW);
}

uint8_t sqrt_approx(int8_t dx, int8_t dy) {
    if (dx < dy) {
        dx >>= 1;
    } else {
        dy >>= 1;
    }
    return dx + dy;
}

void game_loop() {
    // !!! add:
    // if long press 'i give up' etc, handle that separately
    if (LONG_PRESS == buttonClicked) {
        switch(buttonPressed) {
            case StartButton:
                currentGame = SelectGameInit;
            break;
            case ButtonTestButton:
                currentGame = ButtonTestInit;
            break;
            case ColderWarmerButton:
                currentGame = ColderWarmerInit;
            break;
            case IGiveUpButton:
                switch(currentGame) {
                    case SelectGame:
                        open_door();
                    break;
                    case ButtonTest:
                        currentGame = ButtonTestInit;
                    break;
                    case ColderWarmer:
                        currentGame = ColderWarmerInit;
                    break;
                    default:
                        currentGame = SelectGameInit;
                    break;
                } 
            break;
        }
    }
    switch(currentGame) {
        case Winner:
            // run winner animation
            // to do : call routine to unlock the door
            // to do : animate leds on either side
            if (gameState[CW_Guess_Blink_Count] > 1 
                    && 0 == --gameState[CW_Guess_Blink_Duration]) {
                uint8_t blinkOn = --gameState[CW_Guess_Blink_Count] & 1;
                uint8_t idx = gameState[CW_Goal];
                if (idx > 0 && bookY[idx - 1] == bookY[idx]) {
                    writeLed(idx - 1, blinkOn);
                }
                if (idx < BOOKS - 1 && bookY[idx + 1] == bookY[idx]) {
                    writeLed(idx + 1, blinkOn);
                }
            }
            if (StartButton == buttonPressed) {
                currentGame = SelectGameInit;
            }
         break;
        case SelectGameInit:
            // !!! set up selection animation
            currentGame = SelectGame;
        break;
        case SelectGame:
            if (ButtonTestButton == buttonPressed) {
                currentGame = ButtonTestInit;
            } else if (ColderWarmerButton == buttonPressed) {
                currentGame = ColderWarmerInit;
            }
        break;
        case ButtonTestInit:
            gameState[BT_Last] = 255;
            currentGame = ButtonTest;
        break;
        case ButtonTest:
            if (0 == --gameState[BT_Duration] || 255 != buttonPressed) {
                if (255 != gameState[BT_Last]) {
                    clearLed(gameState[BT_Last]);
                    gameState[BT_Last] = 255;
                }
            }
            if (255 != buttonPressed) {
                setLed(buttonPressed);
                gameState[BT_Last] = buttonPressed;
                // show error (not 255, not 0) with rapid flashing
                gameState[BT_Duration] = BUTTON_TIMER_STOPPED != buttonClicked 
                    ? BUTTON_CLICKED == buttonClicked ? 20 : 1 : 255 /* long press */ ;
            }
        break;
        case ColderWarmerInit:
            gameState[CW_Goal] = (uint16_t) random(BOOKS);
            gameState[CW_Guess] = 0;
            gameState[CW_Tries] = 0;
            currentGame = ColderWarmer;
        break;
        case ColderWarmer:
            if (255 != buttonPressed) {
                ++gameState[CW_Tries];
                clearLed(gameState[CW_Guess]);
                gameState[CW_Guess_Blink_Duration] = 1;
                gameState[CW_Guess_Blink_Count] = 8;
                if (gameState[CW_Goal] == buttonPressed) {
                    gameState[CW_Distance] = 30;
                    setLed(gameState[CW_Goal]);
                    currentGame = Winner;
                } else {
                    gameState[CW_Guess] = buttonPressed;
                    int8_t dx = bookX[buttonPressed] - bookX[gameState[CW_Goal]];
                    int8_t dy = bookY[buttonPressed] - bookY[gameState[CW_Goal]];
                    gameState[CW_Distance] = sqrt_approx(dx, dy);
                }
            }
            if (gameState[CW_Guess_Blink_Count] > 1 
                    && 0 == --gameState[CW_Guess_Blink_Duration]) {
                uint8_t blinkOn = --gameState[CW_Guess_Blink_Count] & 1;
                writeLed(gameState[CW_Guess], blinkOn);
                gameState[CW_Guess_Blink_Duration] = BLINK_DURATION * gameState[CW_Distance] / 30;
            }
        break;
    }
}

void setup() {
  pinMode(latchLED, OUTPUT);
  pinMode(clockLED, OUTPUT);
  pinMode(dataLED, OUTPUT);
  pinMode(dataSwitch, INPUT);
  pinMode(latchSwitch, OUTPUT);
  pinMode(clockSwitch, OUTPUT);
  currentGame = SelectGameInit;
  buttonStart = 255;
  buttonClicked = BUTTON_TIMER_STOPPED;
  buttonPressed = 255;
}

// Compare read buttons to last button state. If button is newly pressed
// (i.e., it has been seen down for xx milliseconds) put it in a event queue
// for use by the game loop.
//  digitalWrite latchSwitch LOW resets internal index
void loop() {
    digitalWrite(latchSwitch, LOW);   // load the A-H data lines into the shift register
    delayMicroseconds(20); // Requires a delay here according to the datasheet timing diagram
    digitalWrite(latchSwitch, HIGH);
    uint8_t buttonDown = 255;
    for (uint8_t i = 0; i < BOOKS; ++i) {
        digitalWrite(clockSwitch, LOW);
        if (digitalRead(dataSwitch)) {
            buttonDown = i;
        }
        digitalWrite(clockSwitch, HIGH);
    }
    if (buttonStart != buttonDown) {
        if (255 == buttonDown) {
        // !!! bouncing switch contacts could turn long press into click
        // add minimum time for generating click
            if (BUTTON_TIMER_STOPPED != buttonClicked 
                    && --buttonClicked < BUTTON_CLICK_COUNT - BUTTON_DEBOUNCE) {
                buttonPressed = buttonStart;
                buttonClicked = BUTTON_CLICKED;
            }
        } else {
            buttonStart = buttonDown;
            buttonClicked = BUTTON_CLICK_COUNT;
        }
    } else if (255 != buttonDown && BUTTON_TIMER_STOPPED != buttonClicked 
            && LONG_PRESS == --buttonClicked) {
        buttonPressed = buttonDown;
    }
    digitalWrite(latchLED, LOW);  // don't change LEDs while sending data
    for (uint8_t i = BOARDS; i > 0; ) {
        shiftOut(dataLED, clockLED, MSBFIRST, leds[--i]);
    }
    digitalWrite(latchLED, HIGH);  // light LEDs
    game_loop();
    if (255 != buttonPressed) {
        buttonStart = 255;
        buttonClicked = BUTTON_TIMER_STOPPED;
        buttonPressed = 255;
    }
}

// code only for Simple2D

int main() {
    memset(leds, 0, sizeof(leds));
    memset(shiftRegisterContents, 0, sizeof(shiftRegisterContents));
    setup();
    S2D_Window *window = S2D_CreateWindow("", 400, 960, loop, render, S2D_RESIZABLE);
    window->on_mouse = on_mouse;
    S2D_Show(window);
    return 0;
}

