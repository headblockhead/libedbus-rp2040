#pragma once

#include "hardware/pio.h"

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

  void *new_message[256];
} edbus_config_t;

edbus_config_t edbus_autogenerate_config(uint pin_ebd, uint pin_ebc);

void edbus_init(const edbus_config_t *config);
void edbus_deinit(const edbus_config_t *config);

typedef uint32_t edbus_message_t[8];

void edbus_construct_message(uint8_t channel, uint8_t data[29],
                             edbus_message_t *new_message);
void edbus_send_message(const edbus_config_t *config, edbus_message_t message);
