#pragma once

typedef enum {
  EDBUS_INIT_OK = 0,
  EDBUS_INIT_RX_ERROR,
  EDBUS_INIT_TX_ERROR,
} edbus_init_result_t;

edbus_init_result_t edbus_init(void);
