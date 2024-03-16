/*
 * @file    savageRoller.ino
 * @author  Douglas P. Fields, Jr. <symbolics@lisp.engineer>
 * @copyright Copyright 2023 Douglas P. Fields, Jr.
 * @license Apache License, Version 2.0 - https://www.apache.org/licenses/LICENSE-2.0
 * @brief   Rolls dice for the Savage Worlds game
 * @version 0.7
 * @date    2024-03-14 (Created 2023-10-30)
 *
 * Targets the M5 Cardputer.
 *
 * If you roll more than one die, it adds them.
 * If you use a wild, it replaces the lowest rolled die if higher.
 * Each die can explode - adds another roll if it rolls the highest number.
 * 
 * NOTEs:
 * 1. String (Capital String) is an Arduino String not a C++ std::string
 *
 * TODO:
 * 1. Double buffer the display with "sprites" so it doesn't flicker
 *    when updating the display
 *
 * CHANGELOG
 * v0.7: Added CRITICAL FAIL message
 * v0.6: Added toggle for exploding dice (default=on)
 * v0.5: Allow subtracting a roll using the number key just
 *         before the one that adds it.
 *       Cap mods to +/- 99.
 *       Cap dice to 99 per die.
 *       Change draw order to show result last.
 * v0.4: Show roll details
 * v0.3: Add a splash page with instructions
 * v0.2: Colorized display and compacted it a little bit.
 *       Added previous rolls display.
 * v0.1: First version.
 */

#include "M5Cardputer.h"
#include <vector>

const uint8_t MAJOR_VERSION = 0;
const uint8_t MINOR_VERSION = 7;

enum class Page { Splash, Roller };
Page currentPage = Page::Splash; // What UI page are we displaying?

// User inputs
uint8_t numD4 = 0;
uint8_t numD6 = 0;
uint8_t numD8 = 0;
uint8_t numD10 = 0;
uint8_t numD12 = 0;
uint8_t includeWild = 0;
uint8_t allowExplode = 1;
int8_t plusOrMinus = 0;

const int8_t maxPlus = 99;
const int8_t minPlus = -99;
const uint8_t maxNum = 99;

int16_t fontHeight; // For advancing the display of text
int16_t displayHeight;
int16_t displayWidth;

uint8_t firstRun = 1; // So we display something the first time through
uint8_t stateChange = 0; // Update the display
uint8_t calcResult = 0; // Roll the dice!
long rollResult = 0; // The results of the last roll (fully exploded, wilded, adjusted)
uint8_t includeResult = 0; // Show the result in the output screen?

const int ROLLS_TO_SAVE = 5; // One more than the number displayed, as we save the current one too
const int WIDTH_BUFFER = 6; // Lessen screen width by a bit for nice right margin

std::vector<char> previouslyPressed; // Keys pressed last time we checked
std::vector<char> newlyPressed; // New keys pressed since last time we checked
std::vector<long> rolled; // Individual (exploded) rolls - to replace lowest with wild
std::vector<long> prevRolls; // The last n previous rolls
String rollDetails;
String wildDetails;


/* Main Arduino setup entry point.
 */
void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setTextDatum(textdatum_t::top_left);
  M5Cardputer.Display.setFont(&fonts::FreeSans9pt7b);
  M5Cardputer.Display.setTextSize(1);
  fontHeight = M5Cardputer.Display.fontHeight(&fonts::FreeSans9pt7b);
  displayHeight = M5Cardputer.Display.height();
  displayWidth = M5Cardputer.Display.width();

  // Initialize the random seed
  randomSeed(analogRead(0));
  prevRolls.reserve(ROLLS_TO_SAVE);
}

/* Look at all the currently pressed keys,
 * compare them to previously pressed keys,
 * and store which ones are new.
 */
void calculateNewlyPressed() {
  auto currentlyPressed = M5Cardputer.Keyboard.keysState().word;
  uint8_t found;

  newlyPressed.clear();

  // Find anything that wasn't previously pressed
  for (const auto key : currentlyPressed) {
    found = 0;
    for (const auto prev : previouslyPressed) {
      if (key == prev) {
        found = 1;
        break;
      }
    }
    if (!found) {
      newlyPressed.push_back(key);
    }
  }

  // Save our previously pressed again
  previouslyPressed.clear();
  for (const auto key : currentlyPressed) {
    previouslyPressed.push_back(key);
  }
}  // calculateNewlyPressed()


/* See if the specified key is in the newly pressed
 * key list.
 */
bool isNewlyPressed(char c) {
  for (const auto key : newlyPressed) {
    if (key == c) {
      return true;
    }
  }
  return false;
}

/*
 * If any keys are newly pressed, update state appropriately.
 */
void handleKeys() {
  if (M5Cardputer.Keyboard.isChange()) {
    calculateNewlyPressed();
    if (isNewlyPressed('4')) {
      if (numD4 < maxNum) {
        numD4++;
        stateChange = 1;
      }
    }
    if (isNewlyPressed('6')) {
      if (numD6 < maxNum) {
        numD6++;
        stateChange = 1;
      }
    }
    if (isNewlyPressed('8')) {
      if (numD8 < maxNum) {
        numD8++;
        stateChange = 1;
      }
    }
    if (isNewlyPressed('0')) {
      if (numD10 < maxNum) {
        numD10++;
        stateChange = 1;
      }
    }
    if (isNewlyPressed('2')) {
      if (numD12 < maxNum) {
        numD12++;
        stateChange = 1;
      }
    }

    // Remove dice
    if (isNewlyPressed('1')) {
      if (numD12 > 0) {
        numD12--;
        stateChange = 1;
      }
    }
    if (isNewlyPressed('3')) {
      if (numD4 > 0) {
        numD4--;
        stateChange = 1;
      }
    }
    if (isNewlyPressed('5')) {
      if (numD6 > 0) {
        numD6--;
        stateChange = 1;
      }
    }
    if (isNewlyPressed('7')) {
      if (numD8 > 0) {
        numD8--;
        stateChange = 1;
      }
    }
    if (isNewlyPressed('9')) {
      if (numD10 > 0) {
        numD10--;
        stateChange = 1;
      }
    }

    if (isNewlyPressed('w')) {
      includeWild = !includeWild;
      stateChange = 1;
    }
    if (isNewlyPressed('e')) {
      allowExplode = !allowExplode;
      stateChange = 1;
    }
    // + key requires a shift, so also do =
    if (isNewlyPressed('+') || isNewlyPressed('=')) {
      if (plusOrMinus < maxPlus) {
        plusOrMinus++;
        stateChange = 1;
      }
    }
    // - key requires a shift, so also do _
    if (isNewlyPressed('-') || isNewlyPressed('_')) {
      if (plusOrMinus > minPlus) {
        plusOrMinus--;
        stateChange = 1;
      }
    }
    // TODO: KEY_BACKSPACE needs to be handled specially, so for now use ESC/`/~ key
    if (isNewlyPressed('`') || isNewlyPressed('~')) {
      numD4 = numD6 = numD8 = numD10 = numD12 = 0;
      plusOrMinus = 0;
      includeResult = 0;
      stateChange = 1;
    }
    if (isNewlyPressed(' ') || M5Cardputer.Keyboard.keysState().enter) {
      calcResult = 1;
    }
    if (isNewlyPressed('/') || isNewlyPressed('?')) {
      // Show the instructions again
      firstRun = 1;
      currentPage = Page::Splash;
    }
  }
}

/* Rolls a specified die.
 * If allowExplode, then roll in the savage worlds way,
 * where a maximum roll is rolled again, additively.
 */
long rollWithExplode(long die) {
  long max = die + 1;
  long retval = 0;
  long roll;

  if (rollDetails.length() > 0) rollDetails += ", ";

  if (allowExplode) {
    bool rolled = false;
    do {
      roll = random(1, max);
      retval += roll;
      if (rolled) rollDetails += "+";
      rollDetails += String(roll);
      rolled = true;
    } while (roll == die);
  } else {
    roll = random(1, max);
    retval = roll;
    rollDetails += String(roll);
  }

  return retval;
}

/* Do the user's requested roll, with all explodings,
 * and handle the wild die appropriately to replace the
 * lowest exploded roll if it's higher.
 */
long doRoll() {
  long retval = 0;
  String temp;

  rollDetails = "";
  wildDetails = "";
  rolled.clear();
  for (uint8_t i = 0; i < numD4;  i++) rolled.push_back(rollWithExplode(4));
  for (uint8_t i = 0; i < numD6;  i++) rolled.push_back(rollWithExplode(6));
  for (uint8_t i = 0; i < numD8;  i++) rolled.push_back(rollWithExplode(8));
  for (uint8_t i = 0; i < numD10; i++) rolled.push_back(rollWithExplode(10));
  for (uint8_t i = 0; i < numD12; i++) rolled.push_back(rollWithExplode(12));

  if (includeWild && rolled.size() > 0) {
    std::sort(rolled.begin(), rolled.end()); // To sort in descending order, add: , std::greater<long>()
    // Save the details of the wild roll
    temp = rollDetails;
    rollDetails = "";
    long wild = rollWithExplode(6);
    wildDetails = rollDetails;
    rollDetails = temp;
    if (wild > rolled[0]) {
      rolled[0] = wild;
    }
  }
  
  for (const auto val : rolled) {
    retval += val;
  }
  return retval;
} // doRoll


/* If we have to do the roll, do it, and add the modifier,
 * and set it up for display.
 */
void updateState() {
  if (calcResult) {
    calcResult = 0;
    if ((numD4 | numD6 | numD8 | numD10 | numD12) != 0) {
      rollResult = doRoll() + plusOrMinus;
      // Save in our previous rolls (up to a specified limit)
      // TODO: Refactor to use std::queue or deque
      prevRolls.insert(prevRolls.begin(), rollResult);
      while (prevRolls.size() > ROLLS_TO_SAVE) {
        prevRolls.pop_back();
      }
      stateChange = 1;
      includeResult = 1;
    }
  }
}

/* Finds a position to split (preferably a space, comma or plus)
 * that maximally fits on a single line. We do it by brute force
 * for now. TODO: Linear search. Returns -1 if we don't need to split.
 */
int findSplitPos(String s) {
  if (M5Cardputer.Display.textWidth(s) <= displayWidth - WIDTH_BUFFER)
    return -1;

  int lastPos = s.length() - 1;
  char c;
  bool splitSpot;
  int32_t wid;
  do {
    do {
      c = s[lastPos];
      splitSpot = c == ',' || c == ' ' || c == '+';
      if (!splitSpot)
        lastPos--;
    } while (!splitSpot);
    wid = M5Cardputer.Display.textWidth(s.substring(0, lastPos));
    if (wid > displayWidth - WIDTH_BUFFER)
      lastPos--;
  } while (wid > displayWidth - WIDTH_BUFFER);

  return lastPos;
}

/* If any state has changed, then
 * update the display.
 */
void handleDisplay() {
  if (!stateChange) {
    return;
  }

  // TODO: Figure out how to double buffer so the display doesn't flash when
  // doing the clear() and drawing a new thing.
  // See: https://community.m5stack.com/topic/1480/double-buffer-in-m5stickc
  M5Cardputer.Display.clear();

  // Strings with \n do NOT advance to the next line
  // So what's the font height?
  // https://community.m5stack.com/topic/1773/determine-the-width-of-a-character
  // https://github.com/M5ez/M5ez
  String s;

  if ((numD4 | numD6 | numD8 | numD10 | numD12) == 0) {
    // | instead of + avoids overflow e.g. 255 + 1 == 0 for 8-bit unsigned
    s = " no dice yet";
  } else {
    s = " " +
           (numD4  > 0 ? String(numD4) + "d4  " : "") + 
           (numD6  > 0 ? String(numD6) + "d6  " : "") +
           (numD8  > 0 ? String(numD8) + "d8  " : "") +
           (numD10 > 0 ? String(numD10) + "d10  " : "") +
           (numD12 > 0 ? String(numD12) + "d12" : "");
  }
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setTextDatum(textdatum_t::top_left);
  M5Cardputer.Display.drawString(s, 0, 0);

  if (plusOrMinus != 0) {
    s = plusOrMinus > 0 ? " +" + String(plusOrMinus) : " -" + String(-plusOrMinus);
    M5Cardputer.Display.setTextColor(plusOrMinus > 0 ? CYAN : RED);
    M5Cardputer.Display.setTextDatum(textdatum_t::top_right);
    M5Cardputer.Display.drawString(s, displayWidth, 1 * fontHeight);
  }

  if (includeWild) {
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.setTextDatum(textdatum_t::top_left);
    M5Cardputer.Display.drawString(" wild", 0, 1 * fontHeight);
  }

  s = allowExplode ? "explode=on" : "explode=off";
  M5Cardputer.Display.setTextColor(allowExplode ? MAROON : DARKGREY);
  M5Cardputer.Display.setTextDatum(textdatum_t::top_center);
  M5Cardputer.Display.drawString(s, displayWidth / 2, 1 * fontHeight);

  // Display previous rolls
  // Display it before the result, so the result is on top of this, as it
  // is the more important
  if (prevRolls.size() > 0) {
    int num = 0;
    // If we're not showing the current result, show it in the old results,
    // but otherwise don't show the current result with the old results.
    // The current result is always the first entry.
    int start = includeResult ? 1 : 0;
    s = "";
    for (const long p : prevRolls) {
      num++;
      if (num == start) {
        // Skip first one
        continue;
      }
      if (num > start + 1) {
        s += ", ";
      }
      s += String(p);
    }
    M5Cardputer.Display.setTextColor(BLUE);
    M5Cardputer.Display.setTextDatum(textdatum_t::bottom_right);
    M5Cardputer.Display.drawString(s, displayWidth, displayHeight);
  }


  if (includeResult) {
    // TODO: Clear the area under the result in case it overwrites the history of
    // rolls, which makes it hard to read the white text with blue behind it
    if ((numD4 + numD6 + numD8 + numD10 + numD12) == 1 && includeWild && rollResult == (1 + plusOrMinus)) {
      s = " CRITICAL FAIL ";
      M5Cardputer.Display.setTextColor(WHITE, RED);
    } else {
      s = " Result:  " + String(rollResult);
      M5Cardputer.Display.setTextColor(WHITE);
    }
    M5Cardputer.Display.setTextDatum(textdatum_t::bottom_left);
    M5Cardputer.Display.drawString(s, 0, displayHeight);

    // Print 2 lines of details + wild line
    // We use textWidth to get the exact width and find the longest part
    // split on a + or a space that fits on a line.
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.setTextDatum(textdatum_t::top_left);
    int splitPos = findSplitPos(rollDetails);
    if (splitPos < 0) {
      M5Cardputer.Display.drawString(rollDetails, 0, 5 * fontHeight / 2);
    } else {
      M5Cardputer.Display.drawString(rollDetails.substring(0, splitPos + 1), 0, 5 * fontHeight / 2);
      String s = rollDetails.substring(splitPos + 1);
      if (s.length() > 0 && s[0] == ' ')
        s = s.substring(1);
      M5Cardputer.Display.drawString(rollDetails.substring(splitPos + 1), 0, 7 * fontHeight / 2);
    }

    if (wildDetails.length() > 0) {
      M5Cardputer.Display.setTextColor(RED);
      M5Cardputer.Display.setTextDatum(textdatum_t::top_left);
      M5Cardputer.Display.drawString("w: " + wildDetails, 0, 9 * fontHeight / 2);
    }
  }

}

/* Display the splash page and instructions.
 */
void splashHandleDisplay() {
  if (!stateChange) {
    return;
  }
  // stateChange is updated to 0 in the beginning of the main loop
  // (or 1 if it's the first time through)

  M5Cardputer.Display.clear();

  M5Cardputer.Display.setTextColor(RED);
  M5Cardputer.Display.setTextDatum(textdatum_t::top_center);
  M5Cardputer.Display.drawString("Savage Worlds Dice Roller", displayWidth / 2, 0 * fontHeight);

  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setTextDatum(textdatum_t::top_center);
  M5Cardputer.Display.drawString("by Douglas P. Fields, Jr.", displayWidth / 2, 3 * fontHeight / 2);

  M5Cardputer.Display.setTextColor(CYAN);
  M5Cardputer.Display.setTextDatum(textdatum_t::top_center);
  M5Cardputer.Display.drawString("24680 adds a die, odd subs", displayWidth / 2, 3 * fontHeight);
  M5Cardputer.Display.drawString("[w]ild, [e]xplode, +/- mod", displayWidth / 2, 4 * fontHeight);
  M5Cardputer.Display.drawString("space roll, esc reset, ? help", displayWidth / 2, 5 * fontHeight);

  M5Cardputer.Display.setTextColor(YELLOW);
  M5Cardputer.Display.setTextDatum(textdatum_t::bottom_left);
  M5Cardputer.Display.drawString(" press 'y' to continue", 0, displayHeight);

  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setTextDatum(textdatum_t::bottom_right);
  M5Cardputer.Display.drawString("v" + String(MAJOR_VERSION) + "." + String(MINOR_VERSION) + " ", displayWidth, displayHeight);
}

/* If any key is pressed and then all keys are released, then
 * go to the roller page. */
void splashHandleKeys() {
  if (M5Cardputer.Keyboard.isChange()) {
    calculateNewlyPressed();

    if (isNewlyPressed('y')) {
      currentPage = Page::Roller;
      // Display the Roller page when we first get to it
      firstRun = 1;
    }
  }
}


/* Main Arduino event loop entry.
 */
void loop() {
  M5Cardputer.update();

  stateChange = firstRun;
  firstRun = 0;

  if (currentPage == Page::Splash) {
    splashHandleDisplay();
    splashHandleKeys();
  } else if (currentPage == Page::Roller) {
    handleKeys();
    updateState();
    handleDisplay();
  }
}




/* Arduino CLI Notes
https://arduino.github.io/arduino-cli/0.21/getting-started/

Plug M5Cardputer in via USB-C

PS:2 C:\Users\Doug> arduino-cli config init
Config file written to: C:\Users\Doug\AppData\Local\Arduino15\arduino-cli.yaml

Edit this file to add:

board_manager:
  additional_urls:
    - https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json

Also change:

directories:
  user: D:\Doug\src\Arduino

PS:4 C:\Users\Doug> arduino-cli board list
Port Protocol Type              Board Name         FQBN                Core
COM3 serial   Serial Port (USB) ESP32C3 Dev Module esp32:esp32:esp32c3 esp32:esp32

PS:5 C:\Users\Doug> arduino-cli core install esp32:esp32
Tool arduino:dfu-util@0.11.0-arduino5 already installed
Downloading packages...
esp32:xtensa-esp32-elf-gcc@esp-2021r2-patch5-8.4.0 esp32:xtensa-esp32-elf-gcc@esp-2021r2-patch5-8.4.0 already downloaded
...
Installing esp32:mklittlefs@3.0.0-gnu12-dc7f933...
Configuring tool....
esp32:mklittlefs@3.0.0-gnu12-dc7f933 installed
Installing platform esp32:esp32@2.0.11...
Configuring platform....
Platform esp32:esp32@2.0.11 installed

PS:6 C:\Users\Doug> arduino-cli core list
ID            Installed Latest Name
arduino:avr   1.8.6     1.8.6  Arduino AVR Boards
esp32:esp32   2.0.11    2.0.11 esp32
m5stack:esp32 2.0.8     2.0.8  M5Stack

PS:41 D:\Doug\src\Arduino> arduino-cli core install m5stack:esp32
Platform m5stack:esp32@2.0.8 already installed

PS:12 D:\Doug\src\Arduino> arduino-cli sketch new M5C-CLI-Test-1
Sketch created in: D:\Doug\src\Arduino\M5C-CLI-Test-1

PS:17 D:\Doug\src\Arduino> arduino-cli core update-index
Downloading index: package_index.tar.bz2 downloaded
Downloading index: package_m5stack_index.json downloaded

PS:18 D:\Doug\src\Arduino> arduino-cli core search M5
ID            Version Name
m5stack:esp32 2.0.8   M5Stack

PS:28 D:\Doug\src\Arduino> arduino-cli lib list
Name        Installed Available    Location              Description
IRremote    4.2.0     -            LIBRARY_LOCATION_USER -
M5Cardputer 1.0.2     -            LIBRARY_LOCATION_USER -
M5GFX       0.1.10    0.1.11       LIBRARY_LOCATION_USER Library for M5Stack All Display
M5Unified   0.1.10    -            LIBRARY_LOCATION_USER -


NOTE: Add -v for verbose output

PS:43 D:\Doug\src\Arduino> arduino-cli compile --fqbn m5stack:esp32:cardputer M5C-CLI-Test-1
Sketch uses 600665 bytes (45%) of program storage space. Maximum is 1310720 bytes.
Global variables use 26964 bytes (8%) of dynamic memory, leaving 300716 bytes for local variables. Maximum is 327680 bytes.

Used library Version Path
M5Cardputer  1.0.2   D:\Doug\src\Arduino\libraries\M5Cardputer
M5Unified    0.1.10  D:\Doug\src\Arduino\libraries\M5Unified
M5GFX        0.1.10  D:\Doug\src\Arduino\libraries\M5GFX

Used platform Version Path
m5stack:esp32 2.0.8   C:\Users\Doug\AppData\Local\Arduino15\packages\m5stack\hardware\esp32\2.0.8


m5stack:esp32 2.0.8   C:\Users\Doug\AppData\Local\Arduino15\packages\m5stack\hardware\esp32\2.0.8
PS:44 D:\Doug\src\Arduino> arduino-cli upload -p COM3 --fqbn m5stack:esp32:cardputer M5C-CLI-Test-1
esptool.py v4.5.1
Serial port COM3
Connecting...
Chip is ESP32-S3 (revision v0.1)
Features: WiFi, BLE
Crystal is 40MHz
MAC: f4:12:fa:6c:42:9c
...
Writing at 0x0009d489... (100 %)
Wrote 601024 bytes (309745 compressed) at 0x00010000 in 4.0 seconds (effective 1216.9 kbit/s)...
Hash of data verified.

Leaving...
Hard resetting via RTS pin...
New upload port: COM3 (serial)


--------------------------------------------------
This project:

PS:45 D:\Doug\src\Arduino> arduino-cli compile --fqbn m5stack:esp32:cardputer .\savageRoller\
PS:46 D:\Doug\src\Arduino> arduino-cli upload -p COM3 --fqbn m5stack:esp32:cardputer .\savageRoller\

===> WORKS

---------------------------------------------------

VS Code instructions

First set up the CLI above

Install plug-in: https://github.com/microsoft/vscode-arduino
* Set the CLI directory and to use the CLI

Open a new workspace (new copy of VS Code) for the directory with the .ino file

Click the thing on the bottom
* Select board: M5Cardputer
  * F1: Arduino: Select Board Type
  * F1: Arduino: Select Serial Port

Fix this error:
Output path is not specified. Unable to reuse previously compiled files. Build will be slower. See README.
* Edit the file .vscode/arduino.json
* Add "output": "../build", 
  * or somesuch

To build and upload
* Click Arduino: Verify (Control-Alt-R) in the top right toolbar
* Click Arduino: Upload (Control-Alt-U) in the top right toolbar

At some point it builds an intellisense settings and you can click through to all the code.

TODO:
* Get detailed output during the run of verify/upload

*/