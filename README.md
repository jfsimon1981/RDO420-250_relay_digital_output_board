# Firmware for relay board RDO420-250 RDO420-250

RCO 8/80-28-MCP is a modular Relay Digital Output board
with ATTiny461 20-pin Atmel microcontroller.

More info on product page [LCElectronics/RDO420-250](http://lecomptoirelectronique.fr/store/index.php?route=product/product&path=66&product_id=65).

The board has 4 auxiliary relays with 2 3-terminal contacts per relay,
Digital input and resial input ports, local command push-buttons.

It can drive auxiliary and small power devices.

Please see product page and ressources for more informations.

## Operations

The board

  - Mode 1 Momentary push-button toggle
  - Mode 2 Copy push-button
  - Mode 3 Timer operation

Configuration can be done via I2C serial bus and push-buttons

  - Set mode and time-out by I2C for individual relays
    - Arduino example
    - C sources example for controlling via our MCUs
    - See programming manual
  - Set mode by switches
    - Keep SW1, SW2 or SW3 pressed to change mode/configure corresponding relay and reset board
    - Realease switch and press again 1, 2, 3 (to select mode 1, 2, 3 respectively (note 1)
  - If mode 3 is selected, release and press again any of SW1 to SW4 for selecting time-out respectively:
    - 1) 1s pulse
    - 2) 30s pulse
    - 3) 3 minutes pulse
    - 4) 5 minutes pulse

Additional features are available through I2C configuration: 
  - Enable/disable local control for individual or all relays
  - Fine grained time-out (1s-24h)

Note 1: configurations are persistent across reset/power up

Default is mode 2.

Mode 3 comes with additional features, timer, and custom watchdog timouts
to enable safe use in Serial configuration and default to Open relays on
com loss.

Mode 3 firmware, documentation and instructions with additional cost in Pro version
only.

## Serial protocol

The I2C serial protocol is used to transfer orders from a MCU to the device.
Protocol supports multiple devices, orders, read-back and similar to CRC in order
to ensure executed commands are valid MCU requests.

MCU side has to implement the full protocol for sending packets to devices which
includes the node address (1 byte), relays+command (2 bytes) and a CRC (1 byte). Frames are 4 bytes long.

MCU doesn't require but can implement the read-back protocol which ensures a
device (like relay board) has properly received and processed the request). Actions
taken upon transmission failure such as timing-out retries are up to the host
application code.

## Ressources

LCElectronics portal
  - [Home](http://lecomptoirelectronique.fr)
  - [Product page RDO420-250](http://lecomptoirelectronique.fr/relay-board-rdo420-250)

## Updates
  - V1.0 Thursday 9/3/23
    - Pre-release
  - V1.1 Friday 10/3/23
    - Features
      - I2C on fixed address firmware programmed to 0x41
      - Remote relay control via I2C
      - Local push-buttons relay control in toggle mode
      - Ports also available for hardwire push-buttons
    - Fixes
      - Introduce a timeout on I2C states to reset after approx 5ms if not further commands are received.
      - Ensure disambiguation between commands and relay numbers
