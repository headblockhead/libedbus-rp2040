#include "edbus.h"
#include "edbus.pio.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"

static edbus_message_t message_outbound;
static edbus_message_t message_inbound;

static void edbus_pio_irq_handler(void) {
  // TODO
}
static void edbus_dma_irq_handler(void) {
  // TODO
}

static pio_interrupt_source_t pio_get_interrupt_source(uint interrupt) {
#if PICO_PIO_VERSION > 0
  invalid_params_if(HARDWARE_PIO, interrupt > 7);
#else
  invalid_params_if(HARDWARE_PIO, interrupt > 3);
#endif
  return (pio_interrupt_source_t)(pis_interrupt0 + interrupt);
}

static void check_config(const edbus_config_t *config) {
  check_pio_param(config->pio);
  invalid_params_if(HARDWARE_PIO, config.pio_irq_index >= NUM_PIO_IRQS);
  invalid_params_if(HARDWARE_DMA, config.dma_irq_index >= NUM_DMA_IRQS);
  check_sm_param(config->rx_sm);
  check_sm_param(config->tx_sm);
  check_pio_pin_param(config->pin_ebd);
  check_pio_pin_param(config->pin_ebc);
  invalid_params_if(HARDWARE_PIO, config.pin_ebd + 1 != config.pin_ebc);
}

static void init_pio_programs(const edbus_config_t *config) {
  uint offset_rx = pio_add_program(config->pio, &edbus_rx_program);
  edbus_rx_program_init(config->pio, config->rx_sm, offset_rx, config->pin_ebd,
                        config->pin_ebc);

  uint offset_tx = pio_add_program(config->pio, &edbus_tx_program);
  edbus_tx_program_init(config->pio, config->tx_sm, offset_tx, config->pin_ebd,
                        config->pin_ebc);
}

static void init_pio_irq(const edbus_config_t *config) {
  pio_interrupt_source_t irq_source_rx_fifo_not_empty =
      pio_get_rx_fifo_not_empty_interrupt_source(config->rx_sm);
  pio_interrupt_source_t irq_source_tx_error =
      pio_get_interrupt_source(edbus_tx_error_irq);
  pio_set_irqn_source_mask_enabled(
      config->pio, config->pio_irq_index,
      (1 << irq_source_tx_error) | (1 << irq_source_rx_fifo_not_empty), true);

  uint nvic_irq_num_pio = pio_get_irq_num(config->pio, config->pio_irq_index);
  irq_add_shared_handler(nvic_irq_num_pio, edbus_pio_irq_handler,
                         PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
  irq_set_enabled(nvic_irq_num_pio, true);
}

static void init_dma_channels(const edbus_config_t *config) {
  dma_channel_claim(config->dma_channel_rx);
  dma_channel_config dma_config_rx =
      dma_channel_get_default_config(config->dma_channel_rx);
  channel_config_set_read_increment(&dma_config_rx, false);
  channel_config_set_write_increment(&dma_config_rx, true);
  uint dreq_rx = pio_get_dreq(config->pio, config->rx_sm, false);
  channel_config_set_dreq(&dma_config_rx, dreq_rx);
  dma_channel_configure(config->dma_channel_rx, &dma_config_rx, message_inbound,
                        &config->pio->rxf[config->rx_sm],
                        dma_encode_transfer_count(8), false);

  dma_channel_claim(config->dma_channel_tx);
  dma_channel_config dma_config_tx =
      dma_channel_get_default_config(config->dma_channel_tx);
  uint dreq_tx = pio_get_dreq(config->pio, config->tx_sm, true);
  channel_config_set_dreq(&dma_config_tx, dreq_tx);
  channel_config_set_read_increment(&dma_config_tx, true);
  channel_config_set_write_increment(&dma_config_tx, false);
  dma_channel_configure(config->dma_channel_tx, &dma_config_tx,
                        &config->pio->txf[config->tx_sm], message_outbound,
                        dma_encode_transfer_count(8), false);
}

static void init_dma_irq(const edbus_config_t *config) {
  dma_irqn_set_channel_enabled(config->dma_irq_index, config->dma_channel_rx,
                               true);
  dma_irqn_set_channel_enabled(config->dma_irq_index, config->dma_channel_tx,
                               true);

  uint nvic_irq_num_dma = dma_get_irq_num(config->dma_irq_index);
  irq_add_shared_handler(nvic_irq_num_dma, edbus_dma_irq_handler,
                         PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
  irq_set_enabled(nvic_irq_num_dma, true);
}

void edbus_init(const edbus_config_t *config) {
  check_config(config);
  init_pio_programs(config);
  init_pio_irq(config);
  init_dma_channels(config);
  init_dma_irq(config);
}

static uint16_t calculate_crc16_of(uint8_t data[29]) {}

void edbus_construct_message(uint8_t channel, uint8_t data[29],
                             edbus_message_t *new_message) {
  // TODO
}
void edbus_send_message(const edbus_config_t *config, edbus_message_t message) {
  // TODO
}

void edbus_deinit(const edbus_config_t *config) {
  // TODO
}
