/*
    Copyright 2023-2025 Zaden Ruggiero-Bouné

    This file is part of OpenGCC.

    OpenGCC is free software: you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 of the License, or (at your option) any later
   version.

    OpenGCC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with
   OpenGCC If not, see http://www.gnu.org/licenses/.
*/

#ifndef JOYBUS_H_
#define JOYBUS_H_

#include <array>

#include "hardware/pio.h"

/** \file joybus.hpp
 * \brief Joybus protocol implementation
 *
 * <details>
 * <summary>Joybus protocol description</summary>
 *
 * <h2>Electrical Details</h2>
 *
 * The Joybus protocol operates on 3.3V logic. The data line is an open
 * collector circuit – it should be pulled up via a resistor, then driven low
 * and let high as needed. The console and controller both have pull-up
 * resistors on the data line, and either device can drive it low at any time
 * (though both doing so simultaneously indicates a protocol error).
 *
 * <h3>Bits</h3>
 *
 * * Console transmits at a 200kHz bit rate, 5µs per bit.
 *   * Console stop bit is only 3µs
 * * Controller transmits at a 250kHz bit rate, 4µs per bit.
 * * Data bit structure
 *   1. 25% of the bit time logic low
 *   2. 50% of the bit time logic high for 1, logic low for 0
 *   3. 25% of the bit time logic high
 * * Console stop bit structure
 *   1. 1µs logic low
 *   2. 2µs logic high
 * * Controller stop bit structure
 *   1. 2µs logic low
 *   2. 2µs logic high
 *
 * <h2>Protocol</h2>
 *
 * Any individual communication via the protocol works as follows:
 * 1. Console sends command and request bytes (if # request bytes > 0)
 * 2. Console sends stop bit
 * 3. Controller sends response bytes
 * 4. Controller sends stop bit
 *
 * \note If the controller does not recognize the command, it does not respond
 *
 * The full protocol sequence is as follows:
 * 1. Console repeatedly sends 0x00/0xFF to scan for controllers
 * 2. Once a controller responds to probe, console sends 0x41
 * 3. Console polls controller with 0x40/0x43 based on game
 *
 * <h3>Gamecube Joybus Commands</h3>
 *
 * The following summarizes each command and response. Further details on each
 * command is provided below the table.
 *
 * | Command   | Byte | # Request Bytes | # Response Bytes |
 * |-----------|------|-----------------|------------------|
 * | Identify  | 0x00 | 0               | 3                |
 * | Reset     | 0xFF | 0               | 3                |
 * | Poll      | 0x40 | 2               | 8-10             |
 * | Origin    | 0x41 | 0               | 10               |
 * | Calibrate | 0x42 | 2               | 10               |
 * | Long Poll | 0x43 | 2               | 10               |
 *
 * <details>
 * <summary>Identify (0x00)</summary>
 * The first command sent from the console to a controller. The console sends
 * identify and occasionally the reset command to probe for controllers. The
 * controller responds with a 3-byte identifier – 0x090003 for a gamecube
 * controller.
 * </details>
 *
 * <details>
 * <summary>Reset (0xFF)</summary>
 * Functions the same as the identify command, but additionally signals the
 * controller to reset its internal state.
 * </details>
 *
 * <details>
 * <summary>Poll (0x40)</summary>
 * A request for the controller's current state. Has 2 additional request bytes.
 * The first request byte specifies the polling mode, which determines the
 * format of the state that is sent. Mode 3 is what is typically used. Modes 0-4
 * are valid, any mode above 4 will result in mode 0 being used. The second
 * request byte is used to set rumble state.
 * </details>
 *
 * <details>
 * <summary>Origin (0x41)</summary>
 * A request for the origin state of the controller. The values returned for
 * analog inputs will be considered their center, so future inputs will be
 * offset by the value returned in response to this command. OEM controllers
 * respond with the current controller state, which makes trigger-tricking
 * possible. The origin command is sent after the console receives a response
 * to the identify/reset command, before any regular polls take place. Receiving
 * this command sets the origin bit to 0 in future responses. The origin state
 * is sent using mode 5.
 * </details>
 *
 * <details>
 * <summary>Calibrate (0x42)</summary>
 * A request for the controller to recalibrate. Has 2 additional request bytes.
 * The response to this command is set as the new origin for the controller. The
 * first request byte is ignored, and the second is used to set rumble state.
 * The new origin state is sent using mode 5.
 * </details>
 *
 * <details>
 * <summary>Long Poll (0x43)</summary>
 * A request for the full current controller state. Has 2 additional request
 * bytes. The response to this command is set as the new origin for the
 * controller. The first request byte is ignored, and the second is used to
 * set rumble state. The current controller state is sent using mode 5.
 * </details>
 * 
 * <details>
 * <summary>Poll modes</summary>
 * Mode 0
 * | Byte # | Bit 0          | Bit 1          | Bit 2          | Bit 3          | Bit 4          | Bit 5          | Bit 6          | Bit 7          |
 * |--------|----------------|----------------|----------------|----------------|----------------|----------------|----------------|----------------|
 * | 0      | 0              | 0              | Origin         | Start          | Y              | X              | B              | A              |
 * | 1      | 1              | LT             | RT             | Z              | D-Pad Up       | D-Pad Down     | D-Pad Right    | D-Pad Left     |
 * | 2      | LStick X Bit 7 | LStick X Bit 6 | LStick X Bit 5 | LStick X Bit 4 | LStick X Bit 3 | LStick X Bit 2 | LStick X Bit 1 | LStick X Bit 0 |
 * | 3      | LStick Y Bit 7 | LStick Y Bit 6 | LStick Y Bit 5 | LStick Y Bit 4 | LStick Y Bit 3 | LStick Y Bit 2 | LStick Y Bit 1 | LStick Y Bit 0 |
 * | 4      | RStick X Bit 7 | RStick X Bit 6 | RStick X Bit 5 | RStick X Bit 4 | RStick X Bit 3 | RStick X Bit 2 | RStick X Bit 1 | RStick X Bit 0 |
 * | 5      | RStick Y Bit 7 | RStick Y Bit 6 | RStick Y Bit 5 | RStick Y Bit 4 | RStick Y Bit 3 | RStick Y Bit 2 | RStick Y Bit 1 | RStick Y Bit 0 |
 * | 6      | L Analog Bit 7 | L Analog Bit 6 | L Analog Bit 5 | L Analog Bit 4 | R Analog Bit 7 | R Analog Bit 6 | R Analog Bit 5 | R Analog Bit 4 |
 * | 7      | A Analog Bit 7 | A Analog Bit 6 | A Analog Bit 5 | A Analog Bit 4 | B Analog Bit 7 | B Analog Bit 6 | B Analog Bit 5 | B Analog Bit 4 |
 *
 * Mode 1
 * | Byte # | Bit 0          | Bit 1          | Bit 2          | Bit 3          | Bit 4          | Bit 5          | Bit 6          | Bit 7          |
 * |--------|----------------|----------------|----------------|----------------|----------------|----------------|----------------|----------------|
 * | 0      | 0              | 0              | Origin         | Start          | Y              | X              | B              | A              |
 * | 1      | 1              | LT             | RT             | Z              | D-Pad Up       | D-Pad Down     | D-Pad Right    | D-Pad Left     |
 * | 2      | LStick X Bit 7 | LStick X Bit 6 | LStick X Bit 5 | LStick X Bit 4 | LStick X Bit 3 | LStick X Bit 2 | LStick X Bit 1 | LStick X Bit 0 |
 * | 3      | LStick Y Bit 7 | LStick Y Bit 6 | LStick Y Bit 5 | LStick Y Bit 4 | LStick Y Bit 3 | LStick Y Bit 2 | LStick Y Bit 1 | LStick Y Bit 0 |
 * | 4      | RStick X Bit 7 | RStick X Bit 6 | RStick X Bit 5 | RStick X Bit 4 | RStick Y Bit 7 | RStick Y Bit 6 | RStick Y Bit 5 | RStick Y Bit 4 |
 * | 5      | L Analog Bit 7 | L Analog Bit 6 | L Analog Bit 5 | L Analog Bit 4 | L Analog Bit 3 | L Analog Bit 2 | L Analog Bit 1 | L Analog Bit 0 |
 * | 6      | R Analog Bit 7 | R Analog Bit 6 | R Analog Bit 5 | R Analog Bit 4 | R Analog Bit 3 | R Analog Bit 2 | R Analog Bit 1 | R Analog Bit 0 |
 * | 7      | A Analog Bit 7 | A Analog Bit 6 | A Analog Bit 5 | A Analog Bit 4 | B Analog Bit 7 | B Analog Bit 6 | B Analog Bit 5 | B Analog Bit 4 |
 *
 * Mode 2
 * | Byte # | Bit 0          | Bit 1          | Bit 2          | Bit 3          | Bit 4          | Bit 5          | Bit 6          | Bit 7          |
 * |--------|----------------|----------------|----------------|----------------|----------------|----------------|----------------|----------------|
 * | 0      | 0              | 0              | Origin         | Start          | Y              | X              | B              | A              |
 * | 1      | 1              | LT             | RT             | Z              | D-Pad Up       | D-Pad Down     | D-Pad Right    | D-Pad Left     |
 * | 2      | LStick X Bit 7 | LStick X Bit 6 | LStick X Bit 5 | LStick X Bit 4 | LStick X Bit 3 | LStick X Bit 2 | LStick X Bit 1 | LStick X Bit 0 |
 * | 3      | LStick Y Bit 7 | LStick Y Bit 6 | LStick Y Bit 5 | LStick Y Bit 4 | LStick Y Bit 3 | LStick Y Bit 2 | LStick Y Bit 1 | LStick Y Bit 0 |
 * | 4      | RStick X Bit 7 | RStick X Bit 6 | RStick X Bit 5 | RStick X Bit 4 | RStick Y Bit 7 | RStick Y Bit 6 | RStick Y Bit 5 | RStick Y Bit 4 |
 * | 5      | L Analog Bit 7 | L Analog Bit 6 | L Analog Bit 5 | L Analog Bit 4 | R Analog Bit 7 | R Analog Bit 6 | R Analog Bit 5 | R Analog Bit 4 |
 * | 6      | A Analog Bit 7 | A Analog Bit 6 | A Analog Bit 5 | A Analog Bit 4 | A Analog Bit 3 | A Analog Bit 2 | A Analog Bit 1 | A Analog Bit 0 |
 * | 7      | B Analog Bit 7 | B Analog Bit 6 | B Analog Bit 5 | B Analog Bit 4 | B Analog Bit 3 | B Analog Bit 2 | B Analog Bit 1 | B Analog Bit 0 |
 *
 * Mode 3
 * | Byte # | Bit 0          | Bit 1          | Bit 2          | Bit 3          | Bit 4          | Bit 5          | Bit 6          | Bit 7          |
 * |--------|----------------|----------------|----------------|----------------|----------------|----------------|----------------|----------------|
 * | 0      | 0              | 0              | Origin         | Start          | Y              | X              | B              | A              |
 * | 1      | 1              | LT             | RT             | Z              | D-Pad Up       | D-Pad Down     | D-Pad Right    | D-Pad Left     |
 * | 2      | LStick X Bit 7 | LStick X Bit 6 | LStick X Bit 5 | LStick X Bit 4 | LStick X Bit 3 | LStick X Bit 2 | LStick X Bit 1 | LStick X Bit 0 |
 * | 3      | LStick Y Bit 7 | LStick Y Bit 6 | LStick Y Bit 5 | LStick Y Bit 4 | LStick Y Bit 3 | LStick Y Bit 2 | LStick Y Bit 1 | LStick Y Bit 0 |
 * | 4      | RStick X Bit 7 | RStick X Bit 6 | RStick X Bit 5 | RStick X Bit 4 | RStick X Bit 3 | RStick X Bit 2 | RStick X Bit 1 | RStick X Bit 0 |
 * | 5      | RStick Y Bit 7 | RStick Y Bit 6 | RStick Y Bit 5 | RStick Y Bit 4 | RStick Y Bit 3 | RStick Y Bit 2 | RStick Y Bit 1 | RStick Y Bit 0 |
 * | 6      | L Analog Bit 7 | L Analog Bit 6 | L Analog Bit 5 | L Analog Bit 4 | L Analog Bit 3 | L Analog Bit 2 | L Analog Bit 1 | L Analog Bit 0 |
 * | 7      | R Analog Bit 7 | R Analog Bit 6 | R Analog Bit 5 | R Analog Bit 4 | R Analog Bit 3 | R Analog Bit 2 | R Analog Bit 1 | R Analog Bit 0 |
 *
 * Mode 4
 * | Byte # |  Bit 0          | Bit 1          | Bit 2         | Bit 3          | Bit 4          | Bit 5          | Bit 6          | Bit 7          |
 * |--------|----------------|----------------|----------------|----------------|----------------|----------------|----------------|----------------|
 * | 0      | 0              | 0              | Origin         | Start          | Y              | X              | B              | A              |
 * | 1      | 1              | LT             | RT             | Z              | D-Pad Up       | D-Pad Down     | D-Pad Right    | D-Pad Left     |
 * | 2      | LStick X Bit 7 | LStick X Bit 6 | LStick X Bit 5 | LStick X Bit 4 | LStick X Bit 3 | LStick X Bit 2 | LStick X Bit 1 | LStick X Bit 0 |
 * | 3      | LStick Y Bit 7 | LStick Y Bit 6 | LStick Y Bit 5 | LStick Y Bit 4 | LStick Y Bit 3 | LStick Y Bit 2 | LStick Y Bit 1 | LStick Y Bit 0 |
 * | 4      | RStick X Bit 7 | RStick X Bit 6 | RStick X Bit 5 | RStick X Bit 4 | RStick X Bit 3 | RStick X Bit 2 | RStick X Bit 1 | RStick X Bit 0 |
 * | 5      | RStick Y Bit 7 | RStick Y Bit 6 | RStick Y Bit 5 | RStick Y Bit 4 | RStick Y Bit 3 | RStick Y Bit 2 | RStick Y Bit 1 | RStick Y Bit 0 |
 * | 6      | A Analog Bit 7 | A Analog Bit 6 | A Analog Bit 5 | A Analog Bit 4 | A Analog Bit 3 | A Analog Bit 2 | A Analog Bit 1 | A Analog Bit 0 |
 * | 7      | B Analog Bit 7 | B Analog Bit 6 | B Analog Bit 5 | B Analog Bit 4 | B Analog Bit 3 | B Analog Bit 2 | B Analog Bit 1 | B Analog Bit 0 |
 *
 * Mode 5 (Only used by origin, calibrate, and long poll)
 * | Byte # | Bit 0          | Bit 1          | Bit 2          | Bit 3          | Bit 4          | Bit 5          | Bit 6          | Bit 7          |
 * |--------|----------------|----------------|----------------|----------------|----------------|----------------|----------------|----------------|
 * | 0      | 0              | 0              | Origin         | Start          | Y              | X              | B              | A              |
 * | 1      | 1              | LT             | RT             | Z              | D-Pad Up       | D-Pad Down     | D-Pad Right    | D-Pad Left     |
 * | 2      | LStick X Bit 7 | LStick X Bit 6 | LStick X Bit 5 | LStick X Bit 4 | LStick X Bit 3 | LStick X Bit 2 | LStick X Bit 1 | LStick X Bit 0 |
 * | 3      | LStick Y Bit 7 | LStick Y Bit 6 | LStick Y Bit 5 | LStick Y Bit 4 | LStick Y Bit 3 | LStick Y Bit 2 | LStick Y Bit 1 | LStick Y Bit 0 |
 * | 4      | RStick X Bit 7 | RStick X Bit 6 | RStick X Bit 5 | RStick X Bit 4 | RStick X Bit 3 | RStick X Bit 2 | RStick X Bit 1 | RStick X Bit 0 |
 * | 5      | RStick Y Bit 7 | RStick Y Bit 6 | RStick Y Bit 5 | RStick Y Bit 4 | RStick Y Bit 3 | RStick Y Bit 2 | RStick Y Bit 1 | RStick Y Bit 0 |
 * | 6      | L Analog Bit 7 | L Analog Bit 6 | L Analog Bit 5 | L Analog Bit 4 | L Analog Bit 3 | L Analog Bit 2 | L Analog Bit 1 | L Analog Bit 0 |
 * | 7      | R Analog Bit 7 | R Analog Bit 6 | R Analog Bit 5 | R Analog Bit 4 | R Analog Bit 3 | R Analog Bit 2 | R Analog Bit 1 | R Analog Bit 0 |
 * | 8      | A Analog Bit 7 | A Analog Bit 6 | A Analog Bit 5 | A Analog Bit 4 | A Analog Bit 3 | A Analog Bit 2 | A Analog Bit 1 | A Analog Bit 0 |
 * | 9      | B Analog Bit 7 | B Analog Bit 6 | B Analog Bit 5 | B Analog Bit 4 | B Analog Bit 3 | B Analog Bit 2 | B Analog Bit 1 | B Analog Bit 0 |
 * </details>
 *
 * <details>
 * <summary>Rumble bytes</summary>
 * | Value | Meaning            |
 * |-------|--------------------|
 * | 0x00  | Stop rumble        |
 * | 0x01  | Start rumble       |
 * | 0x02  | Apply rumble brake |
 * | 0x03  | Continue rumble    |
 * </details>
 *
 * <h2>Additional Sources</h2>
 * * [Joybus Protocol](https://sites.google.com/site/consoleprotocols/home/nintendo-joy-bus-documentation)
 * * [Simple Controller's Gamecube Protocol](https://simplecontrollers.com/blogs/resources/gamecube-protocol)
 * * [Nintendo Gamecube Controller Protocol](http://www.int03.co.uk/crema/hardware/gamecube/gc-control.htm)
 * * [Extrems' gba-as-controller](https://github.com/extremscorner/gba-as-controller)
 * </details>
 *
 * <details>
 * <summary>OpenGCC Joybus implementation</summary>
 * <h2>Overview</h2>
 * OpenGCC takes advantages of the RP2040's PIO and DMA to implement the
 * Joybus protocol with as little intervention from the main processor as
 * possible. After initial setup, the processor is only involved when a
 * command is received to process the command, and prepare the response.
 * 
 * <h2>PIO</h2>
 * The RP2040's programmable I/O (PIO) converts bits to and from electrical
 * signals.
 * 
 * The RX state machine reads the logic level of the data line – when it
 * drops low, the machine waits 2µs and then reads the logic level of the data
 * line. That value is shifted into the input shift register, which is pushed
 * into its RX FIFO at each byte. The main processor forces this state machine
 * into a specific sequence to handle the console stop bit, enable transmission,
 * and wait for transmission to complete before resuming reading the data line.
 * 
 * The TX state machine waits for the main processor to indicate that data is
 * ready to be transmitted, as well as waiting for the RX state machine to
 * signal that the console stop bit is done transmitting. It then pulls one
 * byte at a time from its TX FIFO, converting each bit into the proper high
 * and low logic levels. When the TX FIFO is empty, it sends the stop bit and
 * signals to the RX state machine that transmission is complete.
 * 
 * <h2>DMA</h2>
 * The RP2040's DMA is used to transfer data to the TX FIFO for the TX state
 * machine asynchronously from the main processor. The main processor triggers a
 * transfer of the appropriate length from the transaction buffer, and the DMA
 * moves the data as needed into the FIFO.
 * 
 * <h2>Interrupt Handler</h2>
 * The main processor's execution is interrupted when the first byte is pushed
 * into the RX state machine's RX FIFO. Based on the command's request length,
 * the processor either waits for the request bytes to be pushed into the FIFO
 * or proceeds immediately. From the time the last byte is received, the
 * processor has the remaining 3µs of the last bit and the 3µs of the stop bit to
 * process the command and get the state machine ready to transmit.
 * 
 * \note The 3+3µs is not entirely accurate – while USB adapters have fairly
 * precise timing, console transmission timings can be significantly different.
 * </details>
 */

/** \brief Initialize Joybus functionality
 *
 * \param pio The PIO instance to use for Joybus
 * \param in_pin The pin to use for Joybus receive
 * \param out_pin The pin to use for Joybus transmit
*/
void joybus_init(PIO pio, uint in_pin, uint out_pin);

/** \brief Interrupt handler that reads commands and starts response
 * transmission
 */
void handle_console_request();

/** \brief Triggers a transmission of the specified length from the transmission
 * buffer
 *
 * \note Transmissions are handled asynchronously. This function simply
 * configures the DMA to transfer the specified length from the transmission
 * buffer to the Joybus TX state machine, so other controller processes can
 * continue to run in the background as data is fed to the state machine.
 *
 * \param length Number of bytes from data to send
 */
void send_data(uint32_t length);

/** \brief Sends controller state with appropriate data based on a specific poll
 * mode
 *
 * \param mode The poll mode which determines how controller state is mapped
 * for transmission
 */
void send_mode(uint8_t mode);

#endif  // JOYBUS_H_
