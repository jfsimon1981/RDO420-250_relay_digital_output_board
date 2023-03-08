#include <avr/io.h>
#include "program_io.h"
#include "program_time.h"
#include "program_util.h"

// Includes for I2C
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "usiTwiSlave.h"

#ifdef DEBUG
#define TIMER0_PULSE 1 // Use DEBUG and PA3

// Use PA3 to display blink led (MCU alive)
void init_led() {DDRA |= (1 << DDA3);}
void set_led() {PORTA |= (1 << PA3);}
void clr_led() {PORTA &= ((1 << PA3) ^ 0xff);}
void toggle_led() {PORTA ^= (1<<PA3);}
#endif

#if defined( __AVR_ATtiny261__ ) | \
     defined( __AVR_ATtiny461__ ) | \
     defined( __AVR_ATtiny861__ )
#  define DDR_USI             DDRB
#  define PORT_USI            PORTB
#  define PIN_USI             PINB
#  define PORT_USI_SDA        PB0
#  define PORT_USI_SCL        PB2
#  define PIN_USI_SDA         PINB0
#  define PIN_USI_SCL         PINB2
#  define USI_START_COND_INT  USISIF
#  define USI_START_VECTOR    USI_START_vect
#  define USI_OVERFLOW_VECTOR USI_OVF_vect
#endif

/*
 * Jumpers 1-3 define I2C address: 0x41/43/45/47
 * I2C exhange specs
 * Write relay number: 0x01 ... 0x04 to send a command to relay followed by
 * 0x00 for off or 0x01 for on. Terminate (0xff).
 * 
 */

void init_timer0() {
  TCCR0A = (1 << CTC0); // 8 bits width, CTC mode
  TCCR0B = (1 << CS02) | (0 << CS01) | (1 << CS00); // Prescaler 101 = 1:1024
  OCR0A = 0x40;
  TIMSK = (1 << OCIE0A); // CompA Interrupt Enable
}

ISR (TIMER0_COMPA_vect) {
#if TIMER0_PULSE == 1
  static unsigned int led = 0;
  static unsigned int i = 0;
  if (!led) {
    if (i == 20) {
      i = 0;
      set_led();
      led = 1;
    }
  } else {
    if (i == 2) {
      i = 0;
      clr_led();
      led = 0;
    }
  }
  i++;
#endif
}

void program_loop() {

#ifdef DEBUG
  init_led();
  set_led();
  display_4bits(0xff);
  _delay_ms(310);
  clr_led();
  display_4bits(0x00);
#endif

/*
  // Test time accuracy
  while (1) {
    PORTA ^= (1 << PA6);
    _delay_ms(500);
  }
*/

  const uint8_t slave_address = 0x41; // I2C address

  usiTwiSlaveInit(slave_address);
  init_timer0();
  sei(); // interrupt enable

  /*if(usiTwiDataInReceiveBuffer()) {
		  uint16_t v;
		  adc_code_t code = (adc_code_t)usiTwiReceiveByte();
		  if( code == RADCT ) {
			  PORTB &= ~(1<<PB6);
			  v = read_temp();
		  }
		  else {
			  v = read_adc((uint8_t)code);
		  }
		  usiTwiTransmitByte((uint8_t)v);
		  usiTwiTransmitByte((uint8_t)(v >> 8));
	  }*/

  // ************* Main program loop *************

  while (1) {

    // Read I2C
    char c;
    c++;
    while (usiTwiAmountDataInReceiveBuffer() > 0) {
      c = usiTwiReceiveByte();
      // usiTwiTransmitByte(c);
    }

    /*
      // Mode 1 Direct
      set_k1(!read_sw1());
      set_k2(!read_sw2());
      set_k3(!read_sw3());
      set_k4(!read_sw4());
    */
    // Mode 2 Toggle
    if (0) {
      bool sw1 = read_sw1();
      bool sw2 = read_sw2();
      bool sw3 = read_sw3();
      bool sw4 = read_sw4();

      if (!(sw1 & sw2 & sw3 & sw4)) {
        if (!sw1) {toggle_k1();}
        else if (!sw2) {toggle_k2();}
        else if (!sw3) {toggle_k3();}
        else if (!sw4) {toggle_k4();}
        delay_ms(50);
        while (!(sw1 & sw2 & sw3 & sw4)) {
          sw1 = read_sw1();
          sw2 = read_sw2();
          sw3 = read_sw3();
          sw4 = read_sw4();
        }
        delay_ms(150);
      }
    }
  }
}



