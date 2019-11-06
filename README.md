# UNO_CLOCK

This experimental sketch continuously displays the time on an 128x32 SSD1306 oled display. The starting time is hardcoded in the _global_seconds_ variable.

Disclaimer: Probably doesn't work on an UNO because there's a 16 MHz crystal on the pins.

# timer2 asynchronous mode showcase

I was looking for a way to time sequences with ms resolution using as little power as possible. ATmega328p's timer2 in asynchronous mode with 32 kHz crystal seems to be the perfect solution. In my tests the current in SLEEP_MODE_PWR_SAVE was around 2 uA.

For a future project idea I have in mind this means I don't need to use an external RTC modul for it anymore. This saves some space.

# timer2 asynchronous mode info

- https://www.gammon.com.au/forum/?id=11504&reply=10#reply10
- https://www.mikrocontroller.net/articles/Sleep_Mode (under "Quarzgenaue Zeitbasis", German)
- http://ww1.microchip.com/downloads/en/Appnotes/Atmel-1259-Real-Time-Clock-RTC-Using-the-Asynchronous-Timer_AP-Note_AVR134.pdf

# Hardware setup
- ATmega328p on breadboard, 8 MHz internal clock [MiniCore](https://github.com/MCUdude/MiniCore)
- 32 kHz watch crystal
- SSD1306 oled display

# Software
-  my fork of [Tiny4kOLED](https://github.com/chocotov1/Tiny4kOLED) (contains 16x32 digits only font)
