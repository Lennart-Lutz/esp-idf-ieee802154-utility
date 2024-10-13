#include <string.h>
#include <stdbool.h>
#include <esp_ieee802154.h>

#include "esp_log.h"
#include "ieee802154_util.h"

#define TAG "ieee802154"

/* --- IEEE802154 utility functions --- */

static void reverse_memcpy(uint8_t *restrict dst, const uint8_t *restrict src, size_t n)
{
    size_t i;

    for (i = 0; i < n; ++i)
    {
        dst[n - 1 - i] = src[i];
    }
}

uint8_t esp_ieee802154_create_2003_data_header(uint16_t *dst_pan_id, ieee802154_address_t *dst_addr, uint16_t *src_pan_id, ieee802154_address_t *src_addr, uint8_t *seq_nr, bool ack, uint8_t *header)
{
    if (seq_nr == NULL)
    {
        ESP_LOGE(TAG, "Sequence number cant be NULL.");
    }

    /**
     * According to the IEEE802.15.4 standard 2003, the pan id can be compressed if the source and destination
     * pan id is the same.
     * In other words, if the pan id matches, only the destination pan id has to be present in the header and the
     * pan_id_compression bit in the frame control field should be set to 1.
     */
    bool pic = true; // intra network communication
    if (*dst_pan_id != *src_pan_id)
    {
        pic = false; // inter network communication
    }

    ieee802154_fcf_t frame_control_field = {
        .frame_type = FRAME_TYPE_DATA,
        .secure = false,
        .frame_pending = false,
        .ack_request = ack,
        .pan_id_compression = pic,
        .reserved = false,
        .sequence_number_suppression = false,
        .information_elements_present = false,
        .dst_addr_mode = dst_addr->mode,
        .frame_ver = FRAME_VERSION_STD_2003,
        .src_addr_mode = src_addr->mode};

    uint8_t position = 0; // The position in the header
    memcpy(&header[position], &frame_control_field, sizeof(frame_control_field));
    position = 2;

    memcpy(&header[position], seq_nr, sizeof(uint8_t));
    position += 1; // 3

    memcpy(&header[position], dst_pan_id, sizeof(uint16_t));
    position += 2; // 5

    if (frame_control_field.dst_addr_mode == ADDR_MODE_SHORT)
    {
        memcpy(&header[position], &dst_addr->short_address, sizeof(dst_addr->short_address));
        position += 2; // 7
    }
    else if (frame_control_field.dst_addr_mode == ADDR_MODE_LONG)
    {
        reverse_memcpy(&header[position], (uint8_t *)&dst_addr->long_address, sizeof(dst_addr->long_address));
        position += 8;
    }

    if (pic == false)
    {
        // Add the SRC PAN to perform an inter PAN communication
        memcpy(&header[position], src_pan_id, sizeof(uint16_t));
        position += 2; // 9
    }

    if (frame_control_field.src_addr_mode == ADDR_MODE_SHORT)
    {
        memcpy(&header[position], &src_addr->short_address, sizeof(src_addr->short_address));
        position += 2; // 9/11
    }
    else if (frame_control_field.src_addr_mode == ADDR_MODE_LONG)
    {
        /**
         * Since the promiscous mode needs the long mac address in reversed byte order,
         * we can use a normal memcpy here.
         */
        memcpy(&header[position], (uint8_t *)&src_addr->long_address, sizeof(src_addr->long_address));
        position += 8;
    }

    return position; // Length of the header
}

uint8_t esp_ieee802154_create_2015_data_header(uint16_t *dst_pan_id, ieee802154_address_t *dst_addr, uint16_t *src_pan_id, ieee802154_address_t *src_addr, uint8_t *seq_nr, bool ack, uint8_t *header)
{
    /**
     * According to the IEEE802.15.4 standard 2015, the sequence number can be suppressed.
     */
    bool sns = false;
    if (seq_nr == NULL)
    {
        sns = true;
    }

    /**
     * According to the IEEE802.15.4 standard 2015, the pan id can be compressed if the source and destination
     * pan id is the same.
     * In other words, if the pan id matches, only the destination pan id has to be present in the header and the
     * pan_id_compression bit in the frame control field should be set to 1.
     */
    bool pic = true;
    if (*dst_pan_id != *src_pan_id)
    {
        pic = false;
    }

    ieee802154_fcf_t frame_control_field = {
        .frame_type = FRAME_TYPE_DATA,
        .secure = false,
        .frame_pending = false,
        .ack_request = ack,
        .pan_id_compression = pic,
        .reserved = false,
        .sequence_number_suppression = sns,
        .information_elements_present = false,
        .dst_addr_mode = dst_addr->mode,
        .frame_ver = FRAME_VERSION_STD_2015,
        .src_addr_mode = src_addr->mode};

    uint8_t position = 0; // The position in the header
    memcpy(&header[position], &frame_control_field, sizeof(frame_control_field));
    position = 2;

    if (sns == false)
    {
        memcpy(&header[position], seq_nr, sizeof(uint8_t));
        position += 1; // 3
    }

    memcpy(&header[position], dst_pan_id, sizeof(uint16_t));
    position += 2; // 5

    if (frame_control_field.dst_addr_mode == ADDR_MODE_SHORT)
    {
        memcpy(&header[position], &dst_addr->short_address, sizeof(dst_addr->short_address));
        position += 2; // 7
    }
    else if (frame_control_field.dst_addr_mode == ADDR_MODE_LONG)
    {
        reverse_memcpy(&header[position], (uint8_t *)&dst_addr->long_address, sizeof(dst_addr->long_address));
        position += 8;
    }

    if (pic == false)
    {
        // Add the SRC PAN to perform an inter PAN communication
        memcpy(&header[position], src_pan_id, sizeof(uint16_t));
        position += 2; // 9
    }

    if (frame_control_field.src_addr_mode == ADDR_MODE_SHORT)
    {
        memcpy(&header[position], &src_addr->short_address, sizeof(src_addr->short_address));
        position += 2; // 9/11
    }
    else if (frame_control_field.src_addr_mode == ADDR_MODE_LONG)
    {
        /**
         * Since the promiscous mode needs the long mac address in reversed byte order,
         * we can use a normal memcpy here.
         */
        memcpy(&header[position], (uint8_t *)&src_addr->long_address, sizeof(src_addr->long_address));
        position += 8;
    }

    return position; // Length of the header
}

uint8_t frame[128];

void esp_ieee802154_send_2003_l2_data_frame(uint16_t dst_pan_id, ieee802154_address_t *dst_addr, uint8_t *data, uint8_t data_length, uint8_t *seq_nr, bool ack)
{
    ieee802154_address_t src_addr;

    uint16_t src_pan_id = esp_ieee802154_get_panid();
    uint16_t src_addr_short = esp_ieee802154_get_short_address();

    // Check if the short source address is available (0xffff is the value if the short adrress is not set)
    if (src_addr_short == 0xffff)
    {
        src_addr.mode = ADDR_MODE_LONG;
        esp_ieee802154_get_extended_address(src_addr.long_address); // In reversed byte order
    }
    else
    {
        src_addr.mode = ADDR_MODE_SHORT;
        src_addr.short_address = src_addr_short;
    }

    memset(frame, 0, 127);
    /* Create the header of the frame*/
    uint8_t hdr_len = esp_ieee802154_create_2003_data_header(&dst_pan_id, dst_addr, &src_pan_id, &src_addr, seq_nr, ack, &frame[1]);

    /* Copy the payload to the frame */
    memcpy(&frame[hdr_len + 1], data, data_length);

    /* Set the length of the frame */
    frame[0] = 1 + hdr_len + data_length + 2; // Length and FCS included

    esp_ieee802154_transmit(frame, true); // Always do CCA!
}

void esp_ieee802154_send_2015_l2_data_frame(uint16_t dst_pan_id, ieee802154_address_t *dst_addr, uint8_t *data, uint8_t data_length, uint8_t *seq_nr, bool ack)
{
    ieee802154_address_t src_addr;

    uint16_t src_pan_id = esp_ieee802154_get_panid();
    uint16_t src_addr_short = esp_ieee802154_get_short_address();

    // Check if the short source address is available (0xffff is the value if the short adrress is not set)
    if (src_addr_short == 0xffff)
    {
        src_addr.mode = ADDR_MODE_LONG;
        esp_ieee802154_get_extended_address(src_addr.long_address); // In reversed byte order
    }
    else
    {
        src_addr.mode = ADDR_MODE_SHORT;
        src_addr.short_address = src_addr_short;
    }

    memset(frame, 0, 127);
    /* Create the header of the frame*/
    uint8_t hdr_len = esp_ieee802154_create_2015_data_header(&dst_pan_id, dst_addr, &src_pan_id, &src_addr, seq_nr, ack, &frame[1]);

    /* Copy the payload to the frame */
    memcpy(&frame[hdr_len + 1], data, data_length);

    /* Set the length of the frame */
    frame[0] = 1 + hdr_len + data_length + 2; // Length and FCS included

    esp_ieee802154_transmit(frame, true); // Always do CCA!
}

void esp_ieee802154_create_2015_ack_frame(uint8_t *frame, uint8_t *enhack_frame)
{
    uint8_t position = 1; // Exclude the frame length

    /* Modify the frame control field for the ACK and copy it in the ACK frame */
    ieee802154_fcf_t *fcf = (ieee802154_fcf_t *)&frame[position];
    ieee802154_fcf_t ack_fcf = {
        .frame_type = FRAME_TYPE_ACK,
        .secure = false,
        .frame_pending = false,
        .ack_request = false,
        .pan_id_compression = fcf->pan_id_compression,
        .reserved = false,
        .sequence_number_suppression = fcf->sequence_number_suppression,
        .information_elements_present = false,
        .dst_addr_mode = fcf->src_addr_mode,
        .frame_ver = FRAME_VERSION_STD_2015,
        .src_addr_mode = fcf->dst_addr_mode
    };
    uint8_t *ack_byte_fcf = (uint8_t *)&ack_fcf;
    enhack_frame[position] = ack_byte_fcf[0];
    position += 1;
    enhack_frame[position] = ack_byte_fcf[1];
    position += 1;

    /* Copy the sequence number in the ACK frame if its set */
    if (!fcf->sequence_number_suppression)
    {
        enhack_frame[position] = frame[position];
        position += 1;
    }

    /* Copy the src pan id (if set) and adress to the destination fields of the ACK frame */
    if (fcf->pan_id_compression)
    {
        /* If pan id compression is set, the destination pan id is kept */
        enhack_frame[position] = frame[position];
        position += 1;
        enhack_frame[position] = frame[position];
        position += 1;

        /* Safe start positions */
        uint8_t frame_src_addr_position = position;
        uint8_t frame_dst_addr_position = position;

        /* Calculate start position of the source address */
        frame_src_addr_position += 2;
        if (fcf->dst_addr_mode == ADDR_MODE_LONG)
        {
            frame_src_addr_position += 6;
        }

        /* Copy the source address as the destination address in the ACK frame */
        enhack_frame[position] = frame[frame_src_addr_position]; // Copy short address
        position += 1;
        frame_src_addr_position += 1;
        enhack_frame[position] = frame[frame_src_addr_position];
        position += 1;
        frame_src_addr_position += 1;

        if (fcf->src_addr_mode == ADDR_MODE_LONG)
        {
            enhack_frame[position] = frame[frame_src_addr_position]; // Copy remaining long address
            position += 1;
            frame_src_addr_position += 1;
            enhack_frame[position] = frame[frame_src_addr_position];
            position += 1;
            frame_src_addr_position += 1;
            enhack_frame[position] = frame[frame_src_addr_position];
            position += 1;
            frame_src_addr_position += 1;
            enhack_frame[position] = frame[frame_src_addr_position];
            position += 1;
            frame_src_addr_position += 1;
            enhack_frame[position] = frame[frame_src_addr_position];
            position += 1;
            frame_src_addr_position += 1;
            enhack_frame[position] = frame[frame_src_addr_position];
            position += 1;
            frame_src_addr_position += 1;
        }

        /* Copy the destination address as the source address in the ACK frame */
        enhack_frame[position] = frame[frame_dst_addr_position]; // Copy short address
        position += 1;
        frame_dst_addr_position += 1;
        enhack_frame[position] = frame[frame_dst_addr_position];
        position += 1;
        frame_dst_addr_position += 1;

        if (fcf->dst_addr_mode == ADDR_MODE_LONG)
        {
            enhack_frame[position] = frame[frame_dst_addr_position]; // Copy remaining long address
            position += 1;
            frame_dst_addr_position += 1;
            enhack_frame[position] = frame[frame_dst_addr_position];
            position += 1;
            frame_dst_addr_position += 1;
            enhack_frame[position] = frame[frame_dst_addr_position];
            position += 1;
            frame_dst_addr_position += 1;
            enhack_frame[position] = frame[frame_dst_addr_position];
            position += 1;
            frame_dst_addr_position += 1;
            enhack_frame[position] = frame[frame_dst_addr_position];
            position += 1;
            frame_dst_addr_position += 1;
            enhack_frame[position] = frame[frame_dst_addr_position];
            position += 1;
            frame_dst_addr_position += 1;
        }
    }
    else
    {
        /* Safe start positions */
        uint8_t frame_src_addr_position = position;
        uint8_t frame_dst_addr_position = position;

        /* Calculate start position of the source address */
        frame_src_addr_position += 4;
        if (fcf->dst_addr_mode == ADDR_MODE_LONG)
        {
            frame_src_addr_position += 6;
        }

        /* Copy the source pan id and address as the destination in the ACK frame */
        enhack_frame[position] = frame[frame_src_addr_position]; // Copy pan id
        position += 1;
        frame_src_addr_position += 1;
        enhack_frame[position] = frame[frame_src_addr_position];
        position += 1;
        frame_src_addr_position += 1;
        enhack_frame[position] = frame[frame_src_addr_position]; // Copy short address
        position += 1;
        frame_src_addr_position += 1;
        enhack_frame[position] = frame[frame_src_addr_position];
        position += 1;
        frame_src_addr_position += 1;

        if (fcf->src_addr_mode == ADDR_MODE_LONG)
        {
            enhack_frame[position] = frame[frame_src_addr_position]; // Copy remaining long address
            position += 1;
            frame_src_addr_position += 1;
            enhack_frame[position] = frame[frame_src_addr_position];
            position += 1;
            frame_src_addr_position += 1;
            enhack_frame[position] = frame[frame_src_addr_position];
            position += 1;
            frame_src_addr_position += 1;
            enhack_frame[position] = frame[frame_src_addr_position];
            position += 1;
            frame_src_addr_position += 1;
            enhack_frame[position] = frame[frame_src_addr_position];
            position += 1;
            frame_src_addr_position += 1;
            enhack_frame[position] = frame[frame_src_addr_position];
            position += 1;
            frame_src_addr_position += 1;
        }

        /* Copy the destination pan id and address as the source in the ACK frame */
        enhack_frame[position] = frame[frame_dst_addr_position]; // Copy pan id
        position += 1;
        frame_dst_addr_position += 1;
        enhack_frame[position] = frame[frame_dst_addr_position];
        position += 1;
        frame_dst_addr_position += 1;
        enhack_frame[position] = frame[frame_dst_addr_position]; // Copy short address
        position += 1;
        frame_dst_addr_position += 1;
        enhack_frame[position] = frame[frame_dst_addr_position];
        position += 1;
        frame_dst_addr_position += 1;

        if (fcf->dst_addr_mode == ADDR_MODE_LONG)
        {
            enhack_frame[position] = frame[frame_dst_addr_position]; // Copy remaining long address
            position += 1;
            frame_dst_addr_position += 1;
            enhack_frame[position] = frame[frame_dst_addr_position];
            position += 1;
            frame_dst_addr_position += 1;
            enhack_frame[position] = frame[frame_dst_addr_position];
            position += 1;
            frame_dst_addr_position += 1;
            enhack_frame[position] = frame[frame_dst_addr_position];
            position += 1;
            frame_dst_addr_position += 1;
            enhack_frame[position] = frame[frame_dst_addr_position];
            position += 1;
            frame_dst_addr_position += 1;
            enhack_frame[position] = frame[frame_dst_addr_position];
            position += 1;
            frame_dst_addr_position += 1;
        }
    }
    
    /* Add some payload if needed */
    //enhack_frame[position] = 'A';
    //position += 1;
    //enhack_frame[position] = 'C';
    //position += 1;
    //enhack_frame[position] = 'K';
    //position += 1;

    /* Set the correct length of the ACK frame */
    enhack_frame[0] = position + 2; // Includes FCS
}

/* --- Analyze functions --- */

static char *frame_version_to_string(uint8_t frame_version)
{
    switch (frame_version)
    {
    case FRAME_VERSION_STD_2003:
        return "2003";
    case FRAME_VERSION_STD_2006:
        return "2006";
    case FRAME_VERSION_STD_2015:
        return "2015";
    default:
        return "Invalid";
    }
}

static char *addr_mode_to_string(uint8_t addr_mode)
{
    switch (addr_mode)
    {
    case ADDR_MODE_NONE:
        return "None";
    case ADDR_MODE_RESERVED:
        return "Reserved";
    case ADDR_MODE_SHORT:
        return "Short";
    case ADDR_MODE_LONG:
        return "Long";
    default:
        return "Invalid"; // Should never happen
    }
}

static char *frame_type_to_string(ieee802154_fcf_t *fcf)
{
    switch (fcf->frame_type)
    {
    case FRAME_TYPE_BEACON:
        return "Beacon";
    case FRAME_TYPE_DATA:
        return "Data";
    case FRAME_TYPE_ACK:
        if (fcf->frame_ver == FRAME_VERSION_STD_2015)
        {
            return "Enh-ACK";
        }
        return "Imm-ACK";
    case FRAME_TYPE_MAC_COMMAND:
        return "MAC CMD";
    case FRAME_TYPE_RESERVED:
        return "Reserved";
    case FRAME_TYPE_MULTIPURPOSE:
        return "Multipurpose (2015)";
    case FRAME_TYPE_FRAGMENT:
        return "Fragment (2015)";
    case FRAME_TYPE_EXTENDED:
        return "Extended (2015)";
    default:
        return "Invalid"; // Should never happen
    }
}

#define BYTES_PER_LINE 12

static void esp_ieee802154_data_hexdump(uint8_t *buffer, uint8_t buff_len)
{
    uint8_t line = 0;
    uint8_t offset = 0;
    uint8_t bytesRead;
    char hexPart[BYTES_PER_LINE * 3 + 1];
    char asciiPart[BYTES_PER_LINE + 1];

    while (offset < buff_len)
    {
        bytesRead = buff_len - offset < BYTES_PER_LINE ? buff_len - offset : BYTES_PER_LINE;

        for (uint8_t i = 0; i < BYTES_PER_LINE; i++)
        {
            if (i < bytesRead)
            {
                sprintf(&hexPart[i * 3], "%02x ", buffer[offset + i]);
                asciiPart[i] = (buffer[offset + i] >= 32 && buffer[offset + i] <= 126) ? buffer[offset + i] : '.';
            }
            else
            {
                sprintf(&hexPart[i * 3], "   ");
                asciiPart[i] = '\0';
            }
        }
        hexPart[BYTES_PER_LINE * 3] = '\0';
        asciiPart[BYTES_PER_LINE] = '\0';

        if (line == 0)
        {
            ESP_LOGI(TAG, "Data dump: %s|%s|", hexPart, asciiPart);
        }
        else
        {
            ESP_LOGI(TAG, "           %s|%s|", hexPart, asciiPart);
        }
        line += 1;
        offset += bytesRead;
    }
}

static void esp_ieee802154_print_address_information(uint8_t *packet, uint8_t *position)
{
    ieee802154_fcf_t *fcf = (ieee802154_fcf_t *)&packet[1];
    uint16_t dst_pan_id = 0;
    uint16_t src_pan_id = 0;
    uint8_t dst_addr[8] = {0};
    uint8_t src_addr[8] = {0};
    uint16_t short_dst_addr = 0;
    uint16_t short_src_addr = 0;

    if (fcf->dst_addr_mode == ADDR_MODE_SHORT || fcf->dst_addr_mode == ADDR_MODE_LONG)
    {
        dst_pan_id = *((uint16_t *)&packet[*position]);
        *position += 2;
        ESP_LOGI(TAG, "DST PAN: %02x:%02x", dst_pan_id >> 8, dst_pan_id & 0x00FF);
    }

    switch (fcf->dst_addr_mode)
    {
    case ADDR_MODE_SHORT:
    {
        short_dst_addr = *((uint16_t *)&packet[*position]);
        *position += 2;
        char broadcast_str[20] = "";
        if (short_dst_addr == 0xFFFF)
        {
            if (dst_pan_id == 0xFFFF)
            {
                char *globalb = "(global Broadcast)";
                memcpy(&broadcast_str, globalb, 19);
            }
            else
            {
                char *localb = "(local Broadcast)";
                memcpy(&broadcast_str, localb, 18);
            }
        }
        ESP_LOGI(TAG, "DST ADDR: %02x:%02x %s", short_dst_addr >> 8, short_dst_addr & 0x00FF, broadcast_str);
        break;
    }
    case ADDR_MODE_LONG:
    {
        for (uint8_t idx = 0; idx < sizeof(dst_addr); idx++)
        {
            dst_addr[idx] = packet[*position + sizeof(dst_addr) - 1 - idx];
        }
        *position += 8;
        ESP_LOGI(TAG, "DST ADDR: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dst_addr[0],
                 dst_addr[1], dst_addr[2], dst_addr[3], dst_addr[4], dst_addr[5], dst_addr[6], dst_addr[7]);
        break;
    }
    default:
    {
        // Typically not possible, because of hardware filtering
        ESP_LOGW(TAG, "No DST address information present");
        return;
    }
    }

    if (fcf->dst_addr_mode == ADDR_MODE_SHORT || fcf->dst_addr_mode == ADDR_MODE_LONG)
    {
        if (fcf->pan_id_compression)
        {
            // SRC PAN is the same as DST PAN -> intra PAN (same network)
            ESP_LOGI(TAG, "SRC PAN: %02x:%02x (intra PAN)", dst_pan_id >> 8, dst_pan_id & 0x00FF);
        }
        else
        {
            // SRC PAN is different from DST PAN -> inter PAN (across network)
            src_pan_id = *((uint16_t *)&packet[*position]);
            *position += 2;
            ESP_LOGI(TAG, "SRC PAN: %02x:%02x (inter PAN)", src_pan_id >> 8, src_pan_id & 0x00FF);
        }
    }

    switch (fcf->src_addr_mode)
    {
    case ADDR_MODE_SHORT:
    {
        short_src_addr = *((uint16_t *)&packet[*position]);
        *position += 2;
        ESP_LOGI(TAG, "SRC ADDR: %02x:%02x", short_src_addr >> 8, short_src_addr & 0x00FF);
        break;
    }
    case ADDR_MODE_LONG:
    {
        for (uint8_t idx = 0; idx < sizeof(src_addr); idx++)
        {
            src_addr[idx] = packet[*position + sizeof(src_addr) - 1 - idx];
        }
        *position += 8;
        ESP_LOGI(TAG, "SRC ADDR: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", src_addr[0], src_addr[1],
                 src_addr[2], src_addr[3], src_addr[4], src_addr[5], src_addr[6], src_addr[7]);
        break;
    }
    default:
    {
        ESP_LOGW(TAG, "No SRC address information present.");
        return;
    }
    }
}

void esp_ieee802154_print_packet(uint8_t *packet)
{
    uint8_t packet_length = packet[0];
    uint8_t position = 1; // Exclude the length

    ieee802154_fcf_t *fcf = (ieee802154_fcf_t *)&packet[position];
    position += 2;

    ESP_LOGI(TAG, "---------------------------------------------------------------------");

    ESP_LOGI(TAG, "------ Frame Control Field ------");
    ESP_LOGI(TAG, "Frame type:                   %s", frame_type_to_string(fcf));
    ESP_LOGI(TAG, "Security Enabled:             %s", fcf->secure ? "True" : "False");
    ESP_LOGI(TAG, "Frame pending:                %s", fcf->frame_pending ? "True" : "False");
    ESP_LOGI(TAG, "Acknowledge request:          %s", fcf->ack_request ? "True" : "False");
    ESP_LOGI(TAG, "PAN ID Compression:           %s", fcf->pan_id_compression ? "True" : "False");
    ESP_LOGI(TAG, "Reserved:                     %s", fcf->reserved ? "True" : "False");
    if (fcf->frame_ver == FRAME_VERSION_STD_2015)
    {
        ESP_LOGI(TAG, "Sequence Number Suppression:  %s", fcf->sequence_number_suppression ? "True" : "False");
        ESP_LOGI(TAG, "Information Elements Present: %s", fcf->information_elements_present ? "True" : "False");
    }
    ESP_LOGI(TAG, "Destination addressing mode:  %s", addr_mode_to_string(fcf->dst_addr_mode));
    ESP_LOGI(TAG, "Frame version:                %s", frame_version_to_string(fcf->frame_ver));
    ESP_LOGI(TAG, "Source addressing mode:       %s", addr_mode_to_string(fcf->src_addr_mode));

    if (fcf->secure || fcf->information_elements_present)
    {
        ESP_LOGE(TAG, "Security and Information Elements are currently not supported.");
        ESP_LOGI(TAG, "---------------------------------------------------------------------");
        return;
    }

    if (fcf->reserved)
    {
        ESP_LOGW(TAG, "Reserved bit is set...");
    }
    
    ESP_LOGI(TAG, "------ %s Packet ------", frame_type_to_string(fcf));

    switch (fcf->frame_type)
    {
    case FRAME_TYPE_DATA:

        if (fcf->sequence_number_suppression && fcf->frame_ver == FRAME_VERSION_STD_2015)
        {
            ESP_LOGI(TAG, "Sequence number suppressed.");
        }
        else // FRAME_VERSION_STD_2003/2006 or sequence_number_suppression == false
        {

            uint8_t sequence_number = packet[position];
            position += 1;
            ESP_LOGI(TAG, "Sequence number: %u", sequence_number);
        }

        esp_ieee802154_print_address_information(packet, &position);

        uint8_t *data = &packet[position];
        uint8_t data_length = packet_length - position - sizeof(uint16_t);
        position += data_length;
        position += 1; // The hardware adds a 0 before rssi/lqi

        ESP_LOGI(TAG, "Data length: %u", data_length);
        esp_ieee802154_data_hexdump(data, data_length);

        break;
    case FRAME_TYPE_ACK:
        if (fcf->frame_ver == FRAME_VERSION_STD_2015)
        {
            if (fcf->sequence_number_suppression)
            {
                ESP_LOGI(TAG, "Sequence number suppressed.");
            }
            else
            {
                uint8_t sequence_number = packet[position];
                position += 1;
                ESP_LOGI(TAG, "Sequence number: %u", sequence_number);
            }

            esp_ieee802154_print_address_information(packet, &position);

            uint8_t *data = &packet[position];
            uint8_t data_length = packet_length - position - sizeof(uint16_t);
            position += data_length;
            position += 1; // The hardware adds a 0 before rssi/lqi

            if (data_length > 0)
            {
                ESP_LOGI(TAG, "ACK contains data.");
                ESP_LOGI(TAG, "Data length: %u", data_length);
                esp_ieee802154_data_hexdump(data, data_length);
            }
            else
            {
                ESP_LOGI(TAG, "ACK contains no data."); 
            }
        }
        else
        {
            uint8_t sequence_number = packet[position];
            position += 1;
            ESP_LOGI(TAG, "Sequence number: %u", sequence_number);
        }
        break;
    default:
        ESP_LOGW(TAG, "Printing this packets is currently not supported.");
        break;
    }

    // Doesnt work if the type is not supported
    ESP_LOGI(TAG, "----- Transmission Info -----");

    // There is a 0 between the data and rssi/lqi values
    int8_t rssi = packet[position];
    uint8_t lqi = packet[position + 1];
    ESP_LOGI(TAG, "RSSI: %d", rssi);
    ESP_LOGI(TAG, "LQI: %d", lqi);

    ESP_LOGI(TAG, "---------------------------------------------------------------------");
}
