#include "edbus.h"
#include "crc32.h"
#include "edbus.pio.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/sem.h"
#include "pico/stdlib.h"
#include <string.h>

static void uint8_to_uint32(const uint8_t in[4], uint32_t *out) {
  *out = ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) |
         ((uint32_t)in[2] << 8) | ((uint32_t)in[3] << 0);
}

static void message_bytes_to_message(const uint8_t message_bytes[32],
                                     edbus_message_t message_out) {
  for (int i = 0; i < 8; i++) {
    uint8_to_uint32(&message_bytes[i * 4], &message_out[i]);
  }
}

static void uint32_to_uint8(uint32_t in, uint8_t out[4]) {
  out[0] = (uint8_t)((in >> 24) & 0xFF);
  out[1] = (uint8_t)((in >> 16) & 0xFF);
  out[2] = (uint8_t)((in >> 8) & 0xFF);
  out[3] = (uint8_t)((in >> 0) & 0xFF);
}

static void message_to_message_bytes(const edbus_message_t message,
                                     uint8_t message_bytes_out[32]) {
  for (int i = 0; i < 8; i++) {
    uint32_to_uint8(message[i], &message_bytes_out[i * 4]);
  }
}

static edbus_message_t message_outbound;
static semaphore_t message_outbound_semaphore;
static edbus_message_t message_inbound;
static edbus_config_t config;

static void edbus_check_config(const edbus_config_t *c) {
  check_pio_param(c->pio);
  check_sm_param(c->rx_sm);
  check_sm_param(c->tx_sm);
  invalid_params_if(HARDWARE_PIO, c->pio_irq_index >= NUM_PIO_IRQS);
  invalid_params_if(HARDWARE_DMA, c->dma_irq_index >= NUM_DMA_IRQS);
  check_dma_channel_param(c->dma_channel_rx);
  check_dma_channel_param(c->dma_channel_tx);
  check_pio_pin_param(c->pin_ebd);
  check_pio_pin_param(c->pin_ebc);
  invalid_params_if(HARDWARE_PIO, c->pin_ebd + 1 != c->pin_ebc);
}

void edbus_configure(const edbus_config_t *c) {
  edbus_check_config(c);
  config = *c;
}

static void init_pio_program_rx(void) {
  uint offset_rx = pio_add_program(config.pio, &edbus_rx_program);
  edbus_rx_program_init(config.pio, config.rx_sm, offset_rx, config.pin_ebd,
                        config.pin_ebc);
}

static void init_pio_program_tx(void) {
  uint offset_tx = pio_add_program(config.pio, &edbus_tx_program);
  edbus_tx_program_init(config.pio, config.tx_sm, offset_tx, config.pin_ebd,
                        config.pin_ebc);
}

static pio_interrupt_source_t pio_get_interrupt_source(uint interrupt) {
#if PICO_PIO_VERSION > 0
  invalid_params_if(HARDWARE_PIO, interrupt > 7);
#else
  invalid_params_if(HARDWARE_PIO, interrupt > 3);
#endif
  return (pio_interrupt_source_t)(pis_interrupt0 + interrupt);
}

static void edbus_pio_irq_handler(void) {
  if (pio_interrupt_get(config.pio, edbus_tx_error_irq)) {
    dma_channel_abort(config.dma_channel_tx);
    pio_sm_clear_fifos(config.pio, config.tx_sm);
    dma_channel_set_read_addr(config.dma_channel_tx, message_outbound, true);
    pio_interrupt_clear(config.pio, edbus_tx_error_irq);
  }
  if (pio_interrupt_get(config.pio, edbus_tx_done_irq)) {
    pio_interrupt_clear(config.pio, edbus_tx_done_irq);
    sem_release(&message_outbound_semaphore);
  }
}

static void init_pio_irq(void) {
  pio_interrupt_source_t irq_source_tx_error =
      pio_get_interrupt_source(edbus_tx_error_irq);
  pio_interrupt_source_t irq_source_tx_done =
      pio_get_interrupt_source(edbus_tx_done_irq);
  uint32_t irq_source_mask =
      (1 << irq_source_tx_error) | (1 << irq_source_tx_done);
  pio_set_irqn_source_mask_enabled(config.pio, config.pio_irq_index,
                                   irq_source_mask, true);

  uint nvic_irq_num_pio = pio_get_irq_num(config.pio, config.pio_irq_index);
  irq_add_shared_handler(nvic_irq_num_pio, edbus_pio_irq_handler,
                         PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
  irq_set_enabled(nvic_irq_num_pio, true);
}

static void init_dma_rx() {
  dma_channel_claim(config.dma_channel_rx);
  dma_channel_config dma_config_rx =
      dma_channel_get_default_config(config.dma_channel_rx);
  channel_config_set_transfer_data_size(&dma_config_rx, DMA_SIZE_32);
  channel_config_set_read_increment(&dma_config_rx, false);
  channel_config_set_write_increment(&dma_config_rx, true);
  uint dreq_rx = pio_get_dreq(config.pio, config.rx_sm, false);
  channel_config_set_dreq(&dma_config_rx, dreq_rx);
  channel_config_set_sniff_enable(&dma_config_rx, true);
  dma_sniffer_set_data_accumulator(0xFFFFFFFF);
  dma_sniffer_enable(config.dma_channel_rx, DMA_SNIFF_CTRL_CALC_VALUE_CRC32,
                     true);
  dma_channel_configure(config.dma_channel_rx, &dma_config_rx, message_inbound,
                        &config.pio->rxf[config.rx_sm],
                        dma_encode_transfer_count(8), false);
}

static void init_dma_tx() {
  dma_channel_claim(config.dma_channel_tx);
  dma_channel_config dma_config_tx =
      dma_channel_get_default_config(config.dma_channel_tx);
  channel_config_set_transfer_data_size(&dma_config_tx, DMA_SIZE_32);
  channel_config_set_read_increment(&dma_config_tx, true);
  channel_config_set_write_increment(&dma_config_tx, false);
  uint dreq_tx = pio_get_dreq(config.pio, config.tx_sm, true);
  channel_config_set_dreq(&dma_config_tx, dreq_tx);
  dma_channel_configure(config.dma_channel_tx, &dma_config_tx,
                        &config.pio->txf[config.tx_sm], message_outbound,
                        dma_encode_transfer_count(8), false);
}

static void edbus_dma_irq_handler(void) {
  if (dma_irqn_get_channel_status(config.dma_irq_index,
                                  config.dma_channel_rx)) {
    dma_irqn_acknowledge_channel(config.dma_irq_index, config.dma_channel_rx);
    uint32_t crc = dma_sniffer_get_data_accumulator();
    if (crc == 0) {
      uint8_t message_bytes[32];
      message_to_message_bytes(message_inbound, message_bytes);
      config.message_consumer(message_inbound[0], &message_bytes[4]);
    }
    dma_sniffer_set_data_accumulator(0xFFFFFFFF);
    dma_channel_set_write_addr(config.dma_channel_rx, message_inbound, true);
  };
  if (dma_irqn_get_channel_status(config.dma_irq_index,
                                  config.dma_channel_tx)) {
    dma_irqn_acknowledge_channel(config.dma_irq_index, config.dma_channel_tx);
  };
}

static void init_dma_irq() {
  dma_irqn_set_channel_enabled(config.dma_irq_index, config.dma_channel_rx,
                               true);
  dma_irqn_set_channel_enabled(config.dma_irq_index, config.dma_channel_tx,
                               true);

  uint nvic_irq_num_dma = dma_get_irq_num(config.dma_irq_index);
  irq_add_shared_handler(nvic_irq_num_dma, edbus_dma_irq_handler,
                         PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
  irq_set_enabled(nvic_irq_num_dma, true);
}

void edbus_enable() {
  sem_init(&message_outbound_semaphore, 1, 1);
  init_pio_program_rx();
  init_pio_program_tx();
  init_pio_irq();
  init_dma_rx();
  init_dma_tx();
  init_dma_irq();
  dma_channel_start(config.dma_channel_rx);
}

static uint32_t calculate_crc(uint8_t message_bytes[32]) {
  uint32_t crc = 0xFFFFFFFF;
  for (int i = 0; i < 28; i++) {
    uint8_t table_index = (crc >> 24) ^ message_bytes[i];
    crc = (crc << 8) ^ crc32_table[table_index];
  }
  return crc;
}

void edbus_construct_message(uint32_t identifier, const uint8_t data[24],
                             edbus_message_t message_out) {
  uint8_t message_bytes[32];
  uint32_to_uint8(identifier, message_bytes);
  memcpy(&message_bytes[4], data, 24);
  uint32_t crc = calculate_crc(message_bytes);
  uint32_to_uint8(crc, &message_bytes[28]);
  message_bytes_to_message(message_bytes, message_out);
}

void edbus_send_message(edbus_message_t message) {
  sem_acquire_blocking(&message_outbound_semaphore);
  memcpy(message_outbound, message, sizeof(edbus_message_t));
  dma_channel_set_read_addr(config.dma_channel_tx, message_outbound, true);
}
