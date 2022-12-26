/*
    Copyright 2022 Zaden Ruggiero-Bouné

    This file is part of NobGCC-SW.

    NobGCC-SW is free software: you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 of the License, or (at your option) any later
   version.

    NobGCC-SW is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with
   NobGCC. If not, see http://www.gnu.org/licenses/.
*/

#include "console_communication.hpp"

#include "common.h"
#include "hardware/dma.h"
#include "joybus.pio.h"

// Send buffer of data
void send_data(uint8_t buf[], uint32_t length) {
    dma_channel_config tx_config = dma_get_channel_config(tx_dma);
    dma_channel_configure(tx_dma, &tx_config, &joybus_pio->txf[tx_sm], buf,
                          length, true);
    pio_interrupt_clear(joybus_pio, TX_WAIT_IRQ);
}