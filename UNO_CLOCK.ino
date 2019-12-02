//
// UNO_CLOCK: timer2 asynchronous mode with 32 khz watch crystal
//
// 2019-11-05: first version
//
// current measurements at 3.8 V:
// atmega328p on breadboard, 8 mhz, 32 khz watch crystal:
// - sleeping: 2 uA, spikes around 45 uA during updates (once a second, taking between 12 ms and 65 ms)
// - atmega328p + ssd1306 128x32 display showing time with big numbers: near 2 mA
//

#include <avr/sleep.h>
#include <avr/power.h>

#include <Tiny4kOLED_Wire.h>

byte timer2_ovf_counter;
volatile bool timer2_ovf;
volatile uint32_t global_seconds = 60900;
volatile bool seconds_changed = false;

//byte time_display_buffer[2][6];
byte time_display_buffer[2][6] = { {9,9,9,9,9,9}, {9,9,9,9,9,9} };
byte time_digits[6];

void setup() {
   ACSR = 1<<ACD;

   // https://www.mikrocontroller.net/articles/Sleep_Mode 
   // Timer2 konfigurieren
   // 17.9 Asynchronous Operation of Timer/Counter2
   // 17.11.8 ASSR – Asynchronous Status Register
   //ASSR = (1<<EXCLK);            // external clock, no crystal
   ASSR |= (1<<AS2);               // Timer2 asynchron takten
   //long_delay(1000);             // Einschwingzeit des 32kHz Quarzes
   //TCCR2  = 6;                   // Vorteiler 256 -> 2s Überlaufperiode ... TCCR2 bestaat niet..

   // 17.11.1 TCCR2A – Timer/Counter Control Register A
   TCCR2A = 0;
   // 17.11.2 TCCR2B – Timer/Counter Control Register B
   TCCR2B = 0;
   //TCCR2B = 1<<CS20;   // no prescaler
   //TCCR2B = 1<<CS21;   // prescaler: 8
   TCCR2B = 1<<CS21 | 1<<CS20;   // prescaler: 32   
   //TCCR2B = 1<<CS22 | 1<<CS21 | 1<<CS20; // prescaler: 1024
   
   //while((ASSR & (1<<TCR2UB)));    // Warte auf das Ende des Zugriffs ... TCR2UB bestaat niet..
   //TIFR   = (1<<TOV2);             // Interrupts löschen (*)
   TIMSK2 |= (1<<TOIE2);             // Timer overflow Interrupt freischalten
   // (*) "Alternatively, TOV2 is cleared by writing a logic one to the flag."

   // initialize digital pin LED_BUILTIN as an output.
   pinMode(LED_BUILTIN, OUTPUT);

   // disable ADC: saves 120 uA
   ADCSRA = 0;
   set_sleep_mode(SLEEP_MODE_PWR_SAVE);

   oled.begin();
   oled.setContrast(25); // 0: dim, 255: bright
   //oled.setFont(FONT6X8);
   //oled.setFont(FONT8X16);
   // big numbers
   oled.setFont(FONT16X32DIGITS);

   oled.setVcomhDeselectLevel(0); // darker screen, bigger contrast range 
   oled.setPrechargePeriod(0,0);  // darker screen, bigger contrast range

   // toggle DisplayFrame to not be the RenderFrame, otherwise updates are visible 
   oled.switchDisplayFrame();
   clear_screen();

   prepare_screen();

   // Turn on the display.
   oled.on();
   oled.switchFrame();
}

void clear_screen(){
    oled.clear();
    oled.switchFrame();
    oled.clear();
}

void prepare_screen(){
   // colons in both buffers
   paint_colons();
   oled.switchFrame();
   paint_colons();

   // font doesn't have spaces
   // clear the filled in spaces
   clear_area(0, 32, 3);
   clear_area(48, 32, 3);
}

void paint_colons(){
   oled.setCursor(0, 0);
   oled.print("  :  :");
}

void clear_area(byte x, byte width, byte columns){
   clear_area_one_frame(x, width, columns);
   oled.switchFrame();
   clear_area_one_frame(x, width, columns);
}

void clear_area_one_frame(byte x, byte width, byte columns){
   //oled.switchDisplayFrame(); // see updates without switching buffers

   for (byte i = 0; i <= columns; i++){
      oled.setCursor(x, i);
      oled.fillLength(0, width);
   }
}

void go_to_sleep() {
   /* 17.9 Asynchronous Operation of Timer/Counter2
    *  
     If Timer/Counter2 is used to wake the device up from power-save or ADC noise reduction mode, precautions must be
     taken if the user wants to re-enter one of these modes: If re-entering sleep mode within the TOSC1 cycle, the interrupt
     will immediately occur and the device wake up again. The result is multiple interrupts and wake-ups within one
     TOSC1 cycle from the first interrupt. If the user is in doubt whether the time before re-entering power-save or ADC
     noise reduction mode is sufficient, the following algorithm can be used to ensure that one TOSC1 cycle has elapsed:
     a. Write a value to TCCR2x, TCNT2, or OCR2x.
     b. Wait until the corresponding update busy flag in ASSR returns to zero.
     c. Enter power-save or ADC noise reduction mode
   */
  
   //17.11.8 ASSR – Asynchronous Status Register
   // dummy update of OCR2A to prevent immediate wake up if sleep was within one timer cycle
   // current went up a little bit: 2.2 uA vs 1.6 uA (tested when there was only a seconds incrementer in the main loop)
   OCR2A = 0;
   while((ASSR & (1<<OCR2AUB)));
   //sleep_mode: Put the device into sleep mode, taking care of setting the SE bit before, and clearing it afterwards.

   // benchmark awake time:
   // - one character display update            : 12 ms
   // - timer2_ovf interrupt / no display update: 51 us
   //
   //PORTB &= ~(1<<PB5); // turn off led (also to measure time it takes)
   sleep_mode();
   //PORTB |= 1<<PB5; // turn on led (also to measure time it takes)
}

ISR(TIMER2_OVF_vect){
  if (++timer2_ovf_counter == 4){
     global_seconds++;
     seconds_changed    = true;
     timer2_ovf_counter = 0;
  }

  timer2_ovf = true;
}

void process_time() {
   // benchmark: function takes 330 us measured on led pin (8 mhz)
   //PORTB |= 1<<PB5; // turn on led (also to measure time it takes)
   
   // probably not the most efficient way
   byte hours   = global_seconds / 3600;
   byte minutes = global_seconds / 60 % 60;
   byte seconds = global_seconds % 60;

   time_digits[0] = hours / 10;
   time_digits[1] = hours % 10;

   time_digits[2] = minutes / 10;  
   time_digits[3] = minutes % 10;  

   time_digits[4] = seconds / 10;
   time_digits[5] = seconds % 10;
   
   //PORTB &= ~(1<<PB5); // turn off led
}

void update_time_display() {
   // benchmark: i2c signal time: 12 ms per changed character

   // updating the whole screen takes more time than only updating one digit
   // experiment: only update time digits on the display that have changed values

   // there are two display buffers: keep track of the displayed digits of each buffer
   static bool current_buffer = 0;
   current_buffer = !current_buffer; // toggle the current buffer

   if (time_display_buffer[current_buffer][0] != time_digits[0]){
      if (time_digits[0]){
         // normal case: first hour digit is not 0
         oled.setCursor(0, 0);
         oled.print(time_digits[0]);
      } else {
         // first hour digit is zero: clear the digit position instead of printing a 0
         clear_area_one_frame(0, 16, 3);
      }
   }

   if (time_display_buffer[current_buffer][1] != time_digits[1]){
      oled.setCursor(16, 0);
      oled.print(time_digits[1]);
   }

   if (time_display_buffer[current_buffer][2] != time_digits[2]){
      oled.setCursor(48, 0);
      oled.print(time_digits[2]);
   }

   if (time_display_buffer[current_buffer][3] != time_digits[3]){
      oled.setCursor(64, 0);
      oled.print(time_digits[3]);
   }

   if (time_display_buffer[current_buffer][4] != time_digits[4]){
      oled.setCursor(96, 0);
      oled.print(time_digits[4]);
   }

   if (time_display_buffer[current_buffer][5] != time_digits[5]){
      oled.setCursor(112, 0);
      oled.print(time_digits[5]);
   }

   // display the updated frame
   oled.switchFrame();
   
   // store the current digits for later comparison
   for (byte i = 0; i < 6; i++){
      time_display_buffer[current_buffer][i] = time_digits[i]; 
   }
}

void show_time() {
   // benchmark: i2c signal time: 65 ms
   oled.setCursor(0, 0);
   oled.print(time_digits[0]);
   oled.print(time_digits[1]);
   oled.setCursor(48, 0);
   oled.print(time_digits[2]);
   oled.print(time_digits[3]);
   oled.setCursor(96, 0);
   oled.print(time_digits[4]);
   oled.print(time_digits[5]);

   // display the updated frame
   oled.switchFrame();
}

void loop() {
   if (timer2_ovf){
      //PORTB ^= 1<<PB5; // toggle led
      timer2_ovf = false;

      if (global_seconds >= 86400){
         global_seconds = 0;
      }
   }

   if (seconds_changed){
      process_time();
      update_time_display();
      //show_time();
      seconds_changed = false;
   }

   go_to_sleep();
}
