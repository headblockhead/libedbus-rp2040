#include "edbus.h"
#include "edbus.pio.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"

struct edbus_state {
  PIO tx_pio;
  uint tx_sm;
  PIO rx_pio;
  uint rx_sm;
} state;

edbus_init_result_t edbus_init(void) {
  uint rx_offset;
  bool rx_success = pio_claim_free_sm_and_add_program_for_gpio_range(
      &edbus_rx_program, &state.rx_pio, &state.rx_sm, &rx_offset, 6, 2, true);
  if (!rx_success) {
    return EDBUS_INIT_RX_ERROR;
  }
  edbus_rx_program_init(state.rx_pio, state.rx_sm, rx_offset, 6, 7);

  uint tx_offset;
  bool tx_success = pio_claim_free_sm_and_add_program_for_gpio_range(
      &edbus_tx_program, &state.tx_pio, &state.tx_sm, &tx_offset, 6, 2, true);
  if (!tx_success) {
    return EDBUS_INIT_TX_ERROR;
  }
  edbus_tx_program_init(state.tx_pio, state.tx_sm, tx_offset, 6, 7);

  return EDBUS_INIT_OK;
}
