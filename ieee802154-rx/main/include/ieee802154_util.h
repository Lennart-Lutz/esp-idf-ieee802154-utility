#pragma once

#include <stdint.h>

#define FRAME_VERSION_STD_2003 0
#define FRAME_VERSION_STD_2006 1
#define FRAME_VERSION_STD_2015 2

#define FRAME_TYPE_BEACON       (0)
#define FRAME_TYPE_DATA         (1)
#define FRAME_TYPE_ACK          (2)
#define FRAME_TYPE_MAC_COMMAND  (3)
#define FRAME_TYPE_RESERVED     (4)
#define FRAME_TYPE_MULTIPURPOSE (5)
#define FRAME_TYPE_FRAGMENT     (6)
#define FRAME_TYPE_EXTENDED     (7)

#define ADDR_MODE_NONE     (0)  // PAN ID and address fields are not present
#define ADDR_MODE_RESERVED (1)  // Reseved
#define ADDR_MODE_SHORT    (2)  // Short address (16-bit)
#define ADDR_MODE_LONG     (3)  // Extended address (64-bit)

typedef struct ieee802154_fcf {
    uint8_t frame_type                      : 3;
    uint8_t secure                          : 1;
    uint8_t frame_pending                   : 1;
    uint8_t ack_request                     : 1;
    uint8_t pan_id_compression              : 1;
    uint8_t reserved                        : 1;
    uint8_t sequence_number_suppression     : 1;
    uint8_t information_elements_present    : 1;
    uint8_t dst_addr_mode                   : 2;
    uint8_t frame_ver                       : 2;
    uint8_t src_addr_mode                   : 2;
} ieee802154_fcf_t;

typedef struct {
    uint8_t mode; // ADDR_MODE_NONE || ADDR_MODE_SHORT || ADDR_MODE_LONG
    union {
        uint16_t short_address;
        uint8_t long_address[8]; // Needs to be in reversed byte order
    };
} ieee802154_address_t;

/**
 * Function to create a header for a 2003 ieee802154 data frame.
 * 
 * @param[in]  dst_pan_id   Destination pan id.
 * @param[in]  dst_addr     Pointer to the destination address ieee802154 struct.
 * @param[in]  src_pan_id   Source pan id.
 * @param[in]  src_addr     Pointer to the source address ieee802154 struct.
 * @param[in]  seq_nr       Sequence number of this data frame.
 * @param[in]  ack          Bool to set whether an ACK frame is required or not.
 * @param[in]  header       Pointer to the buffer which should store the frame.
 * 
 * Note: The seq_nr cannot be NULL!
 * 
 */
uint8_t esp_ieee802154_create_2003_data_header(uint16_t *dst_pan_id, ieee802154_address_t *dst_addr, uint16_t *src_pan_id, ieee802154_address_t *src_addr, uint8_t *seq_nr, bool ack, uint8_t *header);

/**
 * Function to create a header for a 2015 ieee802154 data frame.
 * 
 * @param[in]  dst_pan_id   Destination pan id.
 * @param[in]  dst_addr     Pointer to the destination address ieee802154 struct.
 * @param[in]  src_pan_id   Source pan id.
 * @param[in]  src_addr     Pointer to the source address ieee802154 struct.
 * @param[in]  seq_nr       Sequence number of this data frame.
 * @param[in]  ack          Bool to set whether an ACK frame is required or not.
 * @param[in]  header       Pointer to the buffer which should store the frame.
 * 
 */
uint8_t esp_ieee802154_create_2015_data_header(uint16_t *dst_pan_id, ieee802154_address_t *dst_addr, uint16_t *src_pan_id, ieee802154_address_t *src_addr, uint8_t *seq_nr, bool ack, uint8_t *header);

/**
 * Function to send a 2003 ieee802154 data frame with payload.
 * 
 * The source pan id and short/extended address need to be set via esp_ieee802154_set_panid() and
 * esp_ieee802154_set_short/extended_address().
 * If a short source address is available (different from 0xfff) the short address will be used.
 * 
 * @param[in]  dst_pan_id   Destination pan id.
 * @param[in]  dst_addr     Pointer to the destination address ieee802154 struct.
 * @param[in]  data         Pointer to the data.
 * @param[in]  data_length  Length of the data.
 * @param[in]  seq_nr       Sequence number of this data frame.
 * @param[in]  ack          Bool to set whether an ACK frame is required or not.
 * 
 * Note: The seq_nr cannot be NULL!
 * 
 */
void esp_ieee802154_send_2003_l2_data_frame(uint16_t dst_pan_id, ieee802154_address_t *dst_addr, uint8_t *data, uint8_t data_length, uint8_t *seq_nr, bool ack);

/**
 * Function to send a 2015 ieee802154 data frame with payload.
 * 
 * The source pan id and short/extended address need to be set via esp_ieee802154_set_panid() and
 * esp_ieee802154_set_short/extended_address().
 * If a short source address is available (different from 0xfff) the short address will be used.
 * 
 * @param[in]  dst_pan_id   Destination pan id.
 * @param[in]  dst_addr     Pointer to the destination address ieee802154 struct.
 * @param[in]  data         Pointer to the data.
 * @param[in]  data_length  Length of the data.
 * @param[in]  seq_nr       Sequence number of this data frame.
 * @param[in]  ack          Bool to set whether an ACK frame is required or not.
 * 
 */
void esp_ieee802154_send_2015_l2_data_frame(uint16_t dst_pan_id, ieee802154_address_t *dst_addr, uint8_t *data, uint8_t data_length, uint8_t *seq_nr, bool ack);

/**
 * Function to create a 2015 ieee802154 ack frame from a received frame.
 * 
 * This function uses loop unrolling to be more efficient.
 * 
 * @param[in]  frame            Pointer to the received frame.
 * @param[in]  enhack_frame     Pointer to the to the buffer to store the Enh-ACK frame.
 * 
 * Note: This function should be called in the esp_ieee802154_enh_ack_generator() function.
 * 
 */
void esp_ieee802154_create_2015_ack_frame(uint8_t *frame, uint8_t *enhack_frame);

/**
 * Print the contents of a packet.
 * 
 * Currently supported frames are
 * - 2003/2006 Data and ACK frames
 * - 2015 Data frames
 * 
 * Note: Security and Information Elements are not supported.
 * 
 * @param[in]  packet  The package for which the information is to be printed.
 * 
 */
void esp_ieee802154_print_packet(uint8_t *packet);