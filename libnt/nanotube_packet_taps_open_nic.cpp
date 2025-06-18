 /**************************************************************************\
*//*! \file nanotube_packet_taps_open_nic.h
** \author  Simone Mannarino
**  \brief  OpenNIC interface for Nanotube packet taps.
**   \date  2025-06-18
*//*
\**************************************************************************/

#include "nanotube_packet_taps.h"
#include "nanotube_packet_taps_open_nic.h"
#include "nanotube_packet_taps_core.h"

#include <cstring>

///////////////////////////////////////////////////////////////////////////

// Decode how many bytes in this word are valid packet data

static inline
uint16_t open_nic_valid_data_bytes(const open_nic::word* open_nic_word)
#if __clang__
    __attribute__((always_inline))
#endif
{
    // Get the tkeep bitmask
    unsigned long long tkeep_mask = open_nic_word->get_tkeep();

    // Count the valid bytes based on tkeep
    uint16_t byte_count = 0;
    for (int i = 0; i < 64; ++i) {
        if (tkeep_mask & (1ULL << i)) {
            byte_count++;
        }
    }

    return byte_count;
}



///////////////////////////////////////////////////////////////////////////

// OPEN_NIC bus wrapper for nanotube_tap_packet_length_core()

void nanotube_tap_packet_length_open_nic(
    /* Outputs. */
    struct nanotube_tap_packet_length_resp *resp_out,

    /* State. */
    struct nanotube_tap_packet_length_state *state_inout,

    /* Inputs. */
    const void *packet_word_in,
    const struct nanotube_tap_packet_length_req *req_in)
#if __clang__
    __attribute__((always_inline))
#endif
{
    check_type(nanotube_tap_packet_length, _open_nic);

    const auto* open_nic_w_in = (const open_nic::word*)packet_word_in;
    uint16_t packet_word_length;

    // Check if the packet's metadata indicates it's the end of the packet (EOP)
    bool packet_word_eop = open_nic_w_in->get_tlast();

    // Calculate the length of valid data bytes in the packet
    packet_word_length = open_nic_valid_data_bytes(open_nic_w_in);

    // Call core processing function with the computed packet length and EOP status
    nanotube_tap_packet_length_core(
        resp_out,
        state_inout,
        packet_word_eop,
        packet_word_length,
        req_in);
}


///////////////////////////////////////////////////////////////////////////

// OPEN_NIC bus wrapper for nanotube_tap_packet_read_core()

void nanotube_tap_packet_read_open_nic(
    /* Constant parameters */
    uint16_t result_buffer_length,
    uint8_t result_buffer_index_bits,

    /* Outputs. */
    struct nanotube_tap_packet_read_resp *resp_out,
    uint8_t *result_buffer_inout,

    /* State. */
    struct nanotube_tap_packet_read_state *state_inout,

    /* Inputs. */
    const void *packet_word_in,
    const struct nanotube_tap_packet_read_req *req_in)
#if __clang__
    __attribute__((always_inline))
#endif
{
    check_type(nanotube_tap_packet_read, _open_nic);

    const uint8_t word_index_bits = 6;
    static_assert(sizeof(open_nic::data_bytes) <= (1U << word_index_bits),
                  "Word index width is too small.");
    
    const auto* open_nic_w_in = (const open_nic::word*)packet_word_in;
    uint16_t packet_word_length;

    // Check if the packet's metadata indicates it's the end of the packet (EOP)
    bool packet_word_eop = open_nic_w_in->get_tlast();

    // Calculate the length of valid data bytes in the packet
    packet_word_length = open_nic_valid_data_bytes(open_nic_w_in);

    // Call the core processing function with the computed packet length and EOP status
    nanotube_tap_packet_read_core(
        result_buffer_length, 
        result_buffer_index_bits,
        open_nic::data_bytes, 
        word_index_bits,
        resp_out, 
        result_buffer_inout,
        state_inout,
        open_nic_w_in->data_ptr(0), // Pointer to the packet data
        packet_word_eop, 
        packet_word_length, 
        req_in);
}

///////////////////////////////////////////////////////////////////////////

// OPEN_NIC bus wrapper for nanotube_tap_packet_write_core()

void nanotube_tap_packet_write_open_nic(
    /* Constant parameters */
    uint16_t request_buffer_length,
    uint8_t request_buffer_index_bits,

    /* Outputs. */
    void *packet_word_out,

    /* State. */
    struct nanotube_tap_packet_write_state *state_inout,

    /* Inputs. */
    const void *packet_word_in,
    const struct nanotube_tap_packet_write_req *req_in,
    const uint8_t *request_bytes_in,
    const uint8_t *request_mask_in)
#if __clang__
    __attribute__((always_inline))
#endif
{
    check_type(nanotube_tap_packet_write, _open_nic);

    const uint8_t word_index_bits = 2;
    static_assert(sizeof(open_nic::data_bytes) <= (1U << word_index_bits),
                  "Word index width is too small.");

    const auto *open_nic_w_in = (const open_nic::word*)packet_word_in;
    auto *open_nic_w_out = (open_nic::word*)packet_word_out;
    uint16_t packet_word_length;

    bool packet_word_eop = open_nic_w_in->get_tlast();

    // Calculate the length of valid data bytes in the packet
    packet_word_length = open_nic_valid_data_bytes(open_nic_w_in);

    // Propagate any additional sideband bytes/signals
    if (open_nic::total_bytes > open_nic::data_bytes) {
        memcpy(open_nic_w_out->data_ptr(open_nic::data_bytes),
               open_nic_w_in->data_ptr(open_nic::data_bytes),
               open_nic::total_bytes - open_nic::data_bytes);
    }

    // Invoke the core of the tap.
    nanotube_tap_packet_write_core(
        /* Constant parameters */
        request_buffer_length,
        request_buffer_index_bits,
        open_nic::data_bytes,
        word_index_bits,

        /* Outputs. */
        open_nic_w_out->data_ptr(), // Pointer to the output data

        /* State. */
        state_inout,

        /* Inputs. */
        open_nic_w_in->data_ptr(), // Pointer to the input data
        packet_word_eop,
        packet_word_length,
        req_in,
        request_bytes_in,
        request_mask_in);
}



///////////////////////////////////////////////////////////////////////////

// OPEN_NIC bus wrapper for nanotube_tap_packet_resize_ingress_core()

void nanotube_tap_packet_resize_ingress_open_nic(
    /* Outputs. */
    bool *packet_done_out,
    nanotube_tap_packet_resize_cword_t *cword_out,
    nanotube_tap_offset_t *packet_length_out,

    /* State. */
    nanotube_tap_packet_resize_ingress_state_t *state,

    /* Inputs. */
    nanotube_tap_packet_resize_req_t *resize_req_in,
    void *packet_word_in)
#if __clang__
    __attribute__((always_inline))
#endif
{
    check_type(nanotube_tap_packet_resize_ingress, _open_nic);

    auto *open_nic_w_in = (open_nic::word*)packet_word_in; // Cast input to open_nic::word

    uint16_t packet_word_length;

    bool packet_word_eop = open_nic_w_in->get_tlast();

    // Calculate the length of valid data bytes in the packet
    packet_word_length = open_nic_valid_data_bytes(open_nic_w_in);

    nanotube_tap_packet_resize_ingress_core(
        open_nic::data_bytes,
        packet_done_out,
        cword_out,
        packet_length_out,
        state,
        resize_req_in,
        packet_word_length,
        packet_word_eop);
}


///////////////////////////////////////////////////////////////////////////

// OPEN_NIC bus wrapper for nanotube_tap_packet_resize_egress_core()

void nanotube_tap_packet_resize_egress_open_nic(
    /* Outputs. */
    bool *input_done_out,
    bool *packet_done_out,
    bool *word_valid_out,
    void *packet_word_out,

    /* State. */
    nanotube_tap_packet_resize_egress_state_t *state,
    void *state_packet_word,

    /* Inputs. */
    nanotube_tap_packet_resize_cword_t *cword,
    void *packet_word_in,
    nanotube_tap_offset_t new_packet_len)
#if __clang__
    __attribute__((always_inline))
#endif
{
    check_type(nanotube_tap_packet_resize_egress, _open_nic);

    auto *open_nic_w_in  = (open_nic::word*)packet_word_in; // Input packet
    auto *open_nic_w_out = (open_nic::word*)packet_word_out; // Output packet

    nanotube_tap_offset_t word_length_out;
    bool word_eop_out;

    // Determine the end of packet (EOP) status for input
    bool input_eop = open_nic_w_in->get_tlast();

    // Core processing for resizing egress packets
    nanotube_tap_packet_resize_egress_core(
        open_nic::data_bytes, open_nic::log_data_bytes,
        input_done_out,
        word_valid_out, &word_eop_out,
        &word_length_out, open_nic_w_out->data_ptr(),
        state, (uint8_t*)state_packet_word,
        cword, open_nic_w_in->data_ptr());

    // Determine if the packet is done based on input EOP
    *packet_done_out = input_eop && *input_done_out;

    // Set AXI4 signals for output
    if (*word_valid_out && open_nic::sideband_signals_bytes) {
        // Set tlast to indicate end of packet
        open_nic_w_out->set_tlast(word_eop_out);
        
        open_nic_w_out->set_tkeep(word_length_out);

        // Copy sideband signals from input to output
        open_nic_w_out->set_tuser_size(open_nic_w_in->get_tuser_size());
        open_nic_w_out->set_tuser_src(open_nic_w_in->get_tuser_src());
        open_nic_w_out->set_tuser_dst(open_nic_w_in->get_tuser_dst());
    }
}

///////////////////////////////////////////////////////////////////////////

bool nanotube_tap_packet_is_eop_open_nic(
    const void *packet_word_in,
    struct nanotube_tap_packet_eop_state *state_inout
)
#if __clang__
    __attribute__((always_inline))
#endif
{
    check_type(nanotube_tap_packet_is_eop, _open_nic);

    auto *pw = (open_nic::word*)packet_word_in;

    // The end of packet (EOP) can be determined from tlast
    // along with tvalid, ensuring it's the last valid word of the packet.
    // According to the Open-NIC specification, tlast indicates
    // if the current packet is finished.
    return pw->get_tlast();
}

///////////////////////////////////////////////////////////////////////////
