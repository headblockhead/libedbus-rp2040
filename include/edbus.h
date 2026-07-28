#pragma once

#include "hardware/pio.h"

// edbus_message_consumer_t is a type of function which is called whenever a
// valid message is recieved on the bus.
typedef void (*edbus_message_consumer_t)(uint32_t identifier,
                                         const uint8_t data[24]);

// edbus_message_t is a complete message ready for transmission.
typedef uint32_t edbus_message_t[8];

typedef struct edbus_config {
  PIO pio;
  uint rx_sm;
  uint tx_sm;
  uint pio_irq_index;

  uint dma_irq_index;
  uint dma_channel_rx;
  uint dma_channel_tx;

  uint pin_ebd;
  uint pin_ebc;
  edbus_message_consumer_t message_consumer;
} edbus_config_t;

void edbus_configure(const edbus_config_t *c);
void edbus_enable();

void edbus_construct_message(uint32_t identifier, const uint8_t data[24],
                             edbus_message_t message_out);
void edbus_send_message(edbus_message_t message);
