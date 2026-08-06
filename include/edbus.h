#ifndef EDBUS_H
#define EDBUS_H

#include <stdbool.h>
#include <stdint.h>

// edbus_message_t is a complete message ready for transmission.
typedef uint32_t edbus_message_t[8];

// edbus_message_handler_t is a type of function which is called from within an
// IRQ whenever a valid incoming message is received on the edbus.
typedef void (*edbus_message_handler_t)(uint32_t channel,
                                        const uint8_t data[static 24]);

typedef enum edbus_event {
  EDBUS_EVENT_RX_RECV,    // Occurs when an incoming message is received,
                          // regardless of its validity.
  EDBUS_EVENT_RX_BAD_CRC, // Occurs when an incoming message fails CRC checking,
                          // so is considered invalid and is discarded.
  EDBUS_EVENT_TX_ARBITRATION_LOSS_RETRY, // Occurs when an outgoing message's
                                         // transmission will be reattempted due
                                         // to a higher priority message taking
                                         // precedence during the current
                                         // transmission attempt.
  EDBUS_EVENT_TX_DONE, // Occurs when an outgoing message has been fully
                       // transmitted without any arbitration errors.
} edbus_event_t;

// edbus_event_handler_t is a type of function which is called from within an
// IRQ to alert to a notable event occurring related to the edbus.
typedef void (*edbus_event_handler_t)(edbus_event_t event);

// edbus_config_t is a type of struct containing edbus's internal configuration.
typedef struct edbus_config edbus_config_t;

// edbus_get_default_config returns an edbus_config struct with default values.
// pin_ebc must be sequentially after (+1 from) pin_ebd.
// By default, edbus uses PIO 0, running on state machines 0 and 1, and uses
// DMA channels 0 and 1.
edbus_config_t *edbus_get_default_config(unsigned int pin_ebd,
                                         unsigned int pin_ebc,
                                         edbus_callback_t callback);

// edbus_config_set_pio
void edbus_config_set_pio(edbus_config_t *c, unsigned int pio_num,
                          unsigned int rx_sm, unsigned int tx_sm);

void edbus_config_set_pio_irq(edbus_config_t *c, unsigned int pio_irq_num);

void edbus_config_set_dma(edbus_config *c, unsigned int dma_chan_rx,
                          unsigned int dma_chan_tx, unsigned int dma_irq_num);

void edbus_config_set_pins(edbus_config *c, unsigned int pin_ebd,
                           unsigned int pin_ebc);

void edbus_config_set_callback(edbus_config *c, edbus_callback callback);

void edbus_init(const edbus_config *c);

void edbus_make_message(uint32_t channel, const uint8_t data[24],
                        edbus_message message_out);

void edbus_send_blocking(edbus_message message);
bool edbus_send_timeout_ms(edbus_message message, uint32_t timeout_ms);
bool edbus_send_timeout_us(edbus_message message, uint32_t timeout_us);

#endif
