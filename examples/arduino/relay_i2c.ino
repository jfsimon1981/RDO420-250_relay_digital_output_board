 /*
  * BSD 3-Clause License
  * 
  * Copyright (c) 2023, Jean-François Simon
  * 
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions are met:
  * 
  * 1. Redistributions of source code must retain the above copyright notice, this
  *    list of conditions and the following disclaimer.
  * 
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 
  * 3. Neither the name of the copyright holder nor the names of its
  *    contributors may be used to endorse or promote products derived from
  *    this software without specific prior written permission.
  * 
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

// Configuration

#define LCD_ENABLE 1

// Pins for Arduino board

#define UEXT_PWR_E      8 // External power to UEXT
#define USER_LED_GREEN  7 // User led: green
#define USER_LED_YELLOW 9 // User led: yellow

#include <SoftwareSerial.h>
#include <LCD1x9.h>
#include <Wire.h>

uint8_t crc4(const uint8_t data, int len); // Make 2 CRC4 quadruplets from uint8_t[]
uint8_t crc4_from_frame(uint8_t command, uint8_t relay, uint8_t optional = 0);

#if LCD_ENABLE == 1
LCD1x9 lcd;
#endif
// SoftwareSerial serial (0, 1); // RX, TX

void interrupt_setup() {
  cli(); // Stop interrupts
  // Set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  // OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  OCR1A = 624; // 25 Hz
  TCCR1B |= (1 << WGM12); // turn on CTC mode
  TCCR1B |= (1 << CS12) | (1 << CS10); // Set CS10 and CS12 bits for 1024 prescaler
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei(); // Allow interrupts
}

long unsigned int t1 {0};

ISR(TIMER1_COMPA_vect) {
  t1++;
  Serial.print(".");
}

// the setup function runs once when you press reset or power the board
void setup() {
  // Digital
  pinMode(UEXT_PWR_E, OUTPUT);
  digitalWrite(UEXT_PWR_E, LOW); // Active low
  pinMode(USER_LED_GREEN, OUTPUT);
  pinMode(USER_LED_YELLOW, OUTPUT);

  // Demo boards addressed configuration
  // We close D0 and/or D1 to ground to send I2C open/close commands
  // to more than 1 board.

  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);

  // I2C, SDA,SCL
  Wire.begin(1);
  delay(3);
#if LCD_ENABLE == 1
  lcd.initialize();
#endif

  // Serial
  Serial.begin(115200);
//Serial1.begin(115200);
  Serial.setTimeout(100); // set new value to 100 milliseconds
  interrupt_setup();
}

/*
 * Set Relay address by DIP switch 1-2-3:
 * I2C addresses (8) 0x41/43/45/47/49/4b/4d/4f
 * Control via local push-buttons or I2C commands
 */

// Relay addresses

#define RELAY_BOARD_1_ADDRESS 0x41
#define RELAY_BOARD_2_ADDRESS 0x43
#define RELAY_BOARD_3_ADDRESS 0x45
#define RELAY_BOARD_4_ADDRESS 0x47
#define RELAY_BOARD_5_ADDRESS 0x49
#define RELAY_BOARD_6_ADDRESS 0x4B
#define RELAY_BOARD_7_ADDRESS 0x4D
#define RELAY_BOARD_8_ADDRESS 0x4F

// Relays number

#define RELAY_K1                0x40 // Designator for relay K1
#define RELAY_K2                0x41 // Designator for relay K2
#define RELAY_K3                0x42 // Designator for relay K3
#define RELAY_K4                0x43 // Designator for relay K4 

// Main control commands

#define CMD_OPEN                1    // Open a relay. pass relay number in param. No option.
#define CMD_CLOSE               2    // Close a relay. pass relay number in param. No option.
#define CMD_TOGGLE              3    // Toggle a relay. pass relay number in param. No option.
#define CMD_CLOSE_PULSE         4    // Close a relay. pass relay number in param. Pulse duration in opt or leave 0 for default.
#define CMD_OPEN_PULSE          5    // Open a relay. pass relay number in param. Pulse duration in opt or leave 0 for default.
#define CMD_CLOSE_ALL_RELAYS    6    // Close all relays. No option.
#define CMD_OPEN_ALL_RELAYS     7    // Open all relays. No option.

// Configuration and emergency off

#define SET_PULSE_DURATION      8    // Set pulse duration in seconds (1 ... 4294967296)
#define SET_ENABLE_LOCAL_CTRL   9    // Enable coil open/close from board. No option.
#define SET_DISABLE_LOCAL_CTRL  10   // Disable coil open/close from board. No option.
#define SET_ENABLE_REMOTE_CTRL  11   // Enable coil open/close from remote. No option.
#define SET_DISABLE_REMOTE_CTRL 12   // Disable coil open/close from remote. No option.
#define SET_EMERGENCY_OFF       13   // Open all coils and locks board. Requires local reset to restart operation.

// Read-back

#define READ_RELAY_POSITION     23   // Reads relay position (1 for open, 2 for close)
#define READ_STATUS             24   // Reads board status
#define READ_PORT               25   // Reads board port (PA7-0 k1 k2 k3 k4 s1 s2 s3 s4)

#define RELAY_IS_OPEN           1    // Returned value if coil is open.
#define RELAY_IS_CLOSED         2    // Returned value if coil is closed.

// Status

#define STATUS_BITS_AVAILABLE   0    // Board in available
#define STATUS_BITS_COM_ERROR   1    // A serial communication error occured
#define STATUS_BITS_LOC_CTRL_EN 2    // Local control enabled
#define STATUS_BITS_REM_CTRL_EN 3    // Remote control enabled
#define STATUS_BITS_IS_EOFF     4    // Is in Emergency Off state (local reset required)

// Send a command to relay. Option data (set_pulse_duration)
void i2c_test_relay(uint8_t board_address, uint8_t relay_num) {
  Serial.print(relay_num, HEX);

  // Close relay
  {
    uint8_t command = CMD_CLOSE;
    uint8_t relay = relay_num;
    uint8_t optional = 0; // Leave optional to 0 if unused
    uint8_t crc = crc4_from_frame(command, relay, optional);

    Wire.beginTransmission(board_address); // 0100 xxx1
    Wire.write(byte(command));  // Command (open, close, toggle, ...)
    Wire.write(byte(relay));    // Relays Kn
    Wire.write(byte(optional)); // Some commands have an optional data
    Wire.write(byte(crc));      // CRC
    Wire.endTransmission();
  }
  delay(370);

  // Open relay
  {
    uint8_t command = CMD_OPEN;
    uint8_t relay = relay_num;
    uint8_t optional = 0; // Leave optional to 0 if unused
    uint8_t crc = crc4_from_frame(command, relay, optional);

    Wire.beginTransmission(board_address); // 0100 xxx1
    Wire.write(byte(command));  // Command (open, close, toggle, ...)
    Wire.write(byte(relay));    // Relays Kn
    Wire.write(byte(optional)); // Some commands have an optional data
    Wire.write(byte(crc));      // CRC
    Wire.endTransmission();
  }
  delay(370);
}

//void demo_slow_motion_single_relay();
//void demo_fast_trains();

void loop() {

  /* 
   * This demo program sends requests to 1, 2, 4 or 8 boards.
   * Configuration with pints D0-D1
   * To send open/close commands to only 1 board, leave D0-D1 open
   * D0-D1 Number of boards requests
   * 0  0  1 (0x41)
   * 0  1  2 (0x41 and 0x43)
   * 1  0  4 (0x41 to  0x47)
   * 1  1  8 (0x41 to  0x4F)
   */

  static uint8_t boards_addr_tested = 1;

  // Check how many boards to send I2C test packets to
  // Configure 1, 2, 4 or 8 by D0-D1
  {
    int d0 = digitalRead(0);
    int d1 = digitalRead(1);

    if (d1 && d0) boards_addr_tested = 1;
    else if (d1 && !d0) boards_addr_tested = 2;
    else if (!d1 && d0) boards_addr_tested = 4;
    else if (!d1 && !d0) boards_addr_tested = 8;
  }

  // Send Demo program coil requests to 1 or more boards

  for (uint8_t boards_addr = 0; boards_addr < boards_addr_tested; boards_addr++) {
    uint8_t boards_i2c_addr = 0;
    if (boards_addr == 0) boards_i2c_addr = RELAY_BOARD_1_ADDRESS;
    else if (boards_addr == 1) boards_i2c_addr = RELAY_BOARD_2_ADDRESS;
    else if (boards_addr == 2) boards_i2c_addr = RELAY_BOARD_3_ADDRESS;
    else if (boards_addr == 3) boards_i2c_addr = RELAY_BOARD_4_ADDRESS;
    else if (boards_addr == 4) boards_i2c_addr = RELAY_BOARD_5_ADDRESS;
    else if (boards_addr == 5) boards_i2c_addr = RELAY_BOARD_6_ADDRESS;
    else if (boards_addr == 6) boards_i2c_addr = RELAY_BOARD_7_ADDRESS;
    else if (boards_addr == 7) boards_i2c_addr = RELAY_BOARD_8_ADDRESS;

    // demo_slow_motion_single_relay();
    // demo_fast_trains();

    i2c_test_relay(boards_i2c_addr, RELAY_K1); // Toggle K1
    i2c_test_relay(boards_i2c_addr, RELAY_K2); // Toggle K2
    i2c_test_relay(boards_i2c_addr, RELAY_K3); // Toggle K3
    i2c_test_relay(boards_i2c_addr, RELAY_K4); // Toggle K4
  }

  // Green
  digitalWrite(USER_LED_GREEN, HIGH);
  digitalWrite(USER_LED_YELLOW, LOW);
#if LCD_ENABLE == 1
  lcd.write((char*)("Demo"));
#endif
  Serial.println("Demo");
//Serial1.println("Green");
  delay(1319);
  // Yellow
  digitalWrite(USER_LED_GREEN, LOW);
  digitalWrite(USER_LED_YELLOW, HIGH);
#if LCD_ENABLE == 1
  lcd.write((char*)("Program"));
#endif
  Serial.print("Program (timer ");
//Serial1.println("Yellow");
  Serial.print(t1, DEC);
  Serial.println(")");

  delay(809);
  // LCD
}

// ************* CRC4 util *************

static const uint8_t crc4_lookup[16] = {
		0x00, 0x03, 0x06, 0x05, 0x0c, 0x0f, 0x0a, 0x09,
		0x08, 0x0b, 0x0e, 0x0d, 0x04, 0x07, 0x02, 0x01
};

uint8_t crc4(const uint8_t *d, int l) {
	uint8_t crc_h = 0, crc_l = 0;
	for (int i = 0; i < l; i++) {
		crc_l = crc4_lookup[(crc_l ^ d[i]) & 0x0f] ^ (crc_l >> 4);
		crc_h = crc4_lookup[(crc_h ^ (d[i] >> 4)) & 0x0f] ^ (crc_h >> 4);
	}
	return ((crc_h << 4) | crc_l);
}

uint8_t crc4_from_frame(uint8_t command, uint8_t relay, uint8_t optional) {
  uint8_t crc4_d[3]; // Every frame to the board is 3 data packets length + crc
  crc4_d[0] = command;
  crc4_d[1] = relay;
  crc4_d[2] = optional;
  uint8_t rv = crc4(crc4_d, 3);
  return rv;
}