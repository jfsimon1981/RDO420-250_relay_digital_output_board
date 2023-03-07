#include <avr/io.h>
#include "program_io.h"
#include "program_time.h"

// Includes for I2C
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "usiTwiSlave.h"

#define TIMER0_PULSE 1

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

void blink_k4() {
  for (int j = 0; j < 10; j++) {
    toggle_k4();
    delay_ms(50);
  }
}

void fast_blinker(int);
void fast_blinker_pa6(int);

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
      PORTA |= (1 << PA6);
      led = 1;
    }
  } else {
    if (i == 2) {
      i = 0;
      PORTA &= (1 << PA6);
      led = 0;
    }
  }
  i++;
#endif
}

void fast_blinker(int n) {
  for (int i = 0; i < n; i++) {
      PORTA ^= (1 << PA7);
      _delay_ms(7);
  }
}

void fast_blinker_pa6(int n) {
  PORTA &= (0 << PA6);
  for (int i = 0; i < n; i++) {
      PORTA |= (1 << PA6);
      _delay_ms(100);
      PORTA &= (0 << PA6);
      _delay_ms(100);
  }
  PORTA &= (0 << PA6);
}

// #Interrupt test section

int test_i2c_rx = 0;
void program_loop() {

  fast_blinker_pa6(1);

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

  fast_blinker_pa6(1);

//  fast_blinker_pa6(3);

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
    // uint8_t state = 0;
    // I2C
/*
    if (i2c_buffer_index > 0) {
      i2c_buffer_index = 0;
      fast_blinker_pa6(3);
    }
*/

    // Read I2C
    char c;
    c++;
    while (usiTwiAmountDataInReceiveBuffer() > 0) {
      c = usiTwiReceiveByte();
      // usiTwiTransmitByte(c);
//      fast_blinker_pa6(1);
    }

    if (test_i2c_rx) {
      test_i2c_rx = 0;
      fast_blinker_pa6(1);
    }

    // Flash LED slowly
    /*{
      const uint32_t t_on = 130, t_per = 170000;
      static uint32_t t = 0;
      if (!t)
        PORTB |= (1 << PB6);  // Led on
      else if (t == t_on)
        PORTB &= ~(1 << PB6); // Led off
      if (t < t_per)
        t++;
      else
        t = 0;
    }*/

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



