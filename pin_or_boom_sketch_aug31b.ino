/*
  LiquidCrystal 16x2 LCD display
  The circuit:
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K potentiometer:
 *   ends to +5V and ground
 *   wiper to LCD VO pin (pin 3)
 */
#include <LiquidCrystal.h>
// why don't I see enums more??
enum { RS=53, ENABLE=52, D4=44, D5=46, D6=48, D7=50, CLEAR_BUTTON=51};
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(RS, ENABLE, D4, D5, D6, D7);

#include <Keypad.h>
const byte ROWS = 4;
const byte COLS = 4;
byte rowPins[ROWS] = {9, 8, 7, 6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 4, 3, 2}; //connect to the column pinouts of the keypad
// adjusted map to what I was getting... easier than trying to swap pins around
char const * const layout = // (ignore this: const correctness woo! :)
    "DCBA"
    "#963" // (ab)using string literal concatenation instead of calling makeKeymap
    "0852"
    "*741";

Keypad elgooKeypad = Keypad(layout, rowPins, colPins, ROWS, COLS); 

void lcd_reset()
{
  lcd.clear();
  lcd.print("Enter your pin: ");
}

char* unlock = "1234"; // pin to achieve unlock/win state, up to 16 chars for 16x2 lcd
// delay between entering last pin character and when it assumes you've stopped
// that way if you intended to type 12345678 it doesn't stop at 1234 and win/unlock
#define PASSWORD_DELAY 3000
// delay before boom on incorrect pin, so it doesn't immediately boom on 1st pin
// and so that you have sufficient time upon a mistake to hit the reset button
#define BOOM_DELAY 7000
// pin number for active buzzer
#define BUZZER 49

// user's pin input (16 characters + nul terminator for C string / lcd print)
char input[17];
// time of last pin input (for delay calculation)
unsigned long last_key_press;
// pauses loop... could refactor to not use this tbh, just delay and setup() in endGame
int gameOver;
// current length of user's pin input (to avoid pointlessly calling strlen)
int len;

void setup() {
  // set every character to 0, could have used for loop but eh
  // every char instead of *input=0 to avoid strcpy of every input
  memcpy(input,"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16);
  last_key_press = millis();
  gameOver = len = 0;

  pinMode(CLEAR_BUTTON,INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER,LOW);

  lcd.begin(16, 2);
  lcd_reset();
}

void endGame(char* msg, bool buzz=false)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msg);
  if(buzz) digitalWrite(BUZZER,HIGH);
  gameOver = 1;
}

void loop() {
  if (gameOver) { delay(5000); setup(); return; }

  char key = elgooKeypad.getKey();
  if(key) {
    last_key_press = millis();
    // if filled 16x2 lcd then drop first character and append new one
    // could just do --len and drop the "else" keyword but I'm not saving without testing now!
    if(len == 16) { memmove(input,input+1,15); input[15] = key; }
    else input[len++] = key;
  }
  
  // on button press clear user input (reset without boom)
  // can just call setup() unless we have more state later...
  // or maybe endGame if a delay is acceptable
  // and/or refactor gameOver and add an argument
  if(digitalRead(CLEAR_BUTTON) == LOW) { lcd_reset(); len = 0; memcpy(input,"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 15); }

  // make sure we're at the start of the second line of the lcd
  lcd.setCursor(0, 1);
  // get time since last keypress
  unsigned long time = millis() - last_key_press;
  //lcd.print(time); // debugging an issue with button constantly signalling
  //lcd.print(" ");  // and later an issue with last_key_press being int and
  lcd.print(input);  // 'randomly' jumping to 60k and instantly exploding lol

  // check for delays having elapsed and win state, if necessary endGame
  if(time > PASSWORD_DELAY && strcmp(input, unlock) == 0) endGame("You've won!");
  else if(time > BOOM_DELAY && len > 0) endGame("BOOM", true);
}
