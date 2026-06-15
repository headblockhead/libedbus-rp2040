#include "edbus.h"
#include "edbus.pio.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"

struct edbus_state {
  PIO pio;
  uint rx_sm;
  uint tx_sm;
} state;

void edbus_tx_irq0_callback(void) {
  // TODO
}

edbus_init_result_t edbus_init(PIO pio, uint rx_sm, uint tx_sm, uint gpio_ebd,
                               uint gpio_ebc) {
  if (pio != pio0 && pio != pio1) {
    return EDBUS_INIT_ERROR_INVALID_PIO;
  }
  if (rx_sm > 3) {
    return EDBUS_INIT_ERROR_INVALID_RX_SM;
  }
  if (tx_sm > 3) {
    return EDBUS_INIT_ERROR_INVALID_TX_SM;
  }
  if (gpio_ebd + 1 != gpio_ebc) {
    return EDBUS_INIT_ERROR_GPIO_NOT_SEQUENTIAL;
  }

  state.pio = pio;
  state.rx_sm = rx_sm;
  state.tx_sm = tx_sm;

  int rx_offset = pio_add_program(state.pio, &edbus_rx_program);
  if (rx_offset < 0) {
    return EDBUS_INIT_ERROR_RX;
  }
  edbus_rx_program_init(state.pio, state.rx_sm, rx_offset, gpio_ebd, gpio_ebc);

  int tx_offset = pio_add_program(state.pio, &edbus_tx_program);
  if (tx_offset < 0) {
    return EDBUS_INIT_ERROR_TX;
  }
  edbus_tx_program_init(state.pio, state.tx_sm, tx_offset, gpio_ebd, gpio_ebc);

  pio_set_irq0_source_enabled(state.pio, pis_interrupt0, true);
  uint irq_num = PIO_IRQ_NUM(state.pio, 0);
  irq_set_exclusive_handler(irq_num, edbus_tx_irq0_callback);
  irq_set_enabled(irq_num, true);

  return EDBUS_INIT_OK;
}
