# Savage Worlds Dice Roller for M5 Cardputer

by [Douglas P. Fields, Jr.](mailto:symbolics@lisp.engineer)
w/ [Jared Pellegrini](https://github.com/jaredpellegrini)
and [Kris Zaragoza](https://github.com/kzaragoza)

license [Apache License, Version 2.0](https://www.apache.org/licenses/LICENSE-2.0)

Rolls dice for the [Savage Worlds](https://peginc.com/savage-settings/savage-worlds/) game.

Targets the [M5 Cardputer](https://docs.m5stack.com/en/core/Cardputer).

# ROLLING

* If you roll more than one die, it adds them.
* If you use a wild, it replaces the lowest rolled die if higher.
* Each die explodes - adds another roll if it rolls the highest number.

# DECK OF CARDS

* A card will only be drawn from the deck once per shuffle.
* Toggling jokers will force a re-shuffle.

# CHANGELOG
* v1.0: Display battery charge level on pressing the b key from the roller screen.
* v0.9: Added card deck
* v0.8: Roll when enter/return key pressed.
* v0.7: Added CRITICAL FAIL message
* v0.6: Added toggle for exploding dice (default=on)
* v0.5: Allow subtracting a roll using the number key just
  * before the one that adds it.
  * Cap mods to +/- 99.
  * Cap dice to 99 per die.
  * Change draw order to show result last.
* v0.4: Show roll details
* v0.3: Add a splash page with instructions
* v0.2: Colorized display and compacted it a little bit.
  * Added previous rolls display.
* v0.1: First version.

# References

* [Arduino Configuration for M5](https://docs.m5stack.com/en/quick_start/Cardputer/arduino)
* [M5 Cardputer Docs](https://docs.m5stack.com/en/core/Cardputer)
* [M5 StampS3 Docs](https://docs.m5stack.com/en/core/StampS3)
* [VS Code and Arduino](https://www.circuitstate.com/tutorials/how-to-use-vs-code-for-creating-and-uploading-arduino-sketches/)
* [Arduino IDE Download](https://www.arduino.cc/en/software)
* [Arduino CLI Download](https://arduino.github.io/arduino-cli/0.35/installation/)
* [M5Stack Reddit](https://www.reddit.com/r/M5Stack/)
* [M5 Cardputer Reddit](https://www.reddit.com/r/CardPuter/)

