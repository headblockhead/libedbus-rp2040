#pragma once

#include "hardware/pio.h"

typedef enum {
  EDBUS_INIT_OK = 0,
  EDBUS_INIT_ERROR_GPIO_NOT_SEQUENTIAL,
  EDBUS_INIT_ERROR_INVALID_PIO,
  EDBUS_INIT_ERROR_INVALID_RX_SM,
  EDBUS_INIT_ERROR_INVALID_TX_SM,
  EDBUS_INIT_ERROR_RX,
  EDBUS_INIT_ERROR_TX,
} edbus_init_result_t;

edbus_init_result_t edbus_init(PIO pio, uint rx_sm, uint tx_sm, uint gpio_ebd,
                               uint gpio_ebc);
