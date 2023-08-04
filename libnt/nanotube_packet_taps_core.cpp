/**************************************************************************\
*//*! \file nanotube_packet_taps_core.cpp
** \author  Neil Turton <neilt@amd.com>
**  \brief  The implementation of Nanotube packet taps.
**   \date  2020-09-23
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include "nanotube_packet_taps.h"
#include "nanotube_packet_taps_core.h"

#include <algorithm>
#include <cassert>
#include <cstring>

/* NOTE: Enabling these will break the HLS pass. Just disable it in
 * gen_test_list while debugging */
#define DEBUG_ROTATE_DOWN 0
#define DEBUG_CLASSIFY    0
#define DEBUG_DUPLICATE   0
#define DEBUG_PACKET_READ 0
#define DEBUG_PACKET_WRITE 0
#define DEBUG_RESIZE_IN_TAP  0
#define DEBUG_RESIZE_EG_TAP  0

#if ( DEBUG_ROTATE_DOWN || \
      DEBUG_CLASSIFY || \
      DEBUG_DUPLICATE || \
      DEBUG_PACKET_READ || \
      DEBUG_PACKET_WRITE || \
      DEBUG_RESIZE_IN_TAP || \
      DEBUG_RESIZE_EG_TAP )
#include <iomanip>
#include <iostream>
#endif

///////////////////////////////////////////////////////////////////////////

void nanotube_rotate_down(
  /* Constant parameters. */
  uint16_t buffer_out_length,
  uint16_t rot_buffer_length,
  uint16_t buffer_in_length,
  uint8_t rot_amount_bits,

  /* Outputs. */
  uint8_t *buffer_out,

  /* Inputs. */
  const uint8_t *buffer_in,
  uint16_t rot_amount)
#ifdef NANOTUBE_SYNTHESIS
  __attribute__((always_inline))
#endif
{
  int stage;
  uint16_t index;

  /* A temporary buffer for splitting the rotation into stages.  The
   * input values are in temp[0].  Stage 0 copies from temp[0] to
   * temp[1] and so on.  The result ends up in
   * temp[buffer_in_index_bits].
   */
  uint8_t temp[rot_amount_bits+1][rot_buffer_length];

  /* Copy the input into the temporary buffer. */
#if DEBUG_ROTATE_DOWN
  std::cout << "Rotate down: out_len=" << buffer_out_length
            << ", rot_len=" << rot_buffer_length
            << ", in_len=" << buffer_in_length
            << ", rot_amount=" << rot_amount << "\n";
  std::cout << "Input buffer:\n";
  for (index = 0; index < buffer_in_length; index++) {
    std::cout << "  Offset " << std::setw(2) << index
              << std::setw(0) << ": 0x"
              << std::setw(2) << std::setfill('0') << std::hex
              << int(buffer_in[index])
              << std::setw(0) << std::setfill(' ') << std::dec
              << "\n";
  }
  std::cout << "\n";
#endif

#ifdef NANOTUBE_SYNTHESIS
#pragma clang loop unroll(full)
#endif
  for (index = 0; index < rot_buffer_length; index++) {
    temp[0][index] = ( index < buffer_in_length ? buffer_in[index] : 0 );
  }

  /* Perform each stage of the rotation, each corresponding with a
   * single bit of the rotation amount and starting with the MSB. */
#ifdef NANOTUBE_SYNTHESIS
#pragma clang loop unroll(full)
#endif
  for (stage = 0; stage < rot_amount_bits; stage++) {
    /* Which bit of the rotation amount is being considered. */
    uint8_t bit_num = (rot_amount_bits-1)-stage;

    /* The amount by which to conditionally rotate in this stage. */
    uint16_t stage_rot_amount = uint16_t(1) << bit_num;

    /* Whether to rotate in this stage. */
    uint8_t sel = (rot_amount >> bit_num) & 1;

    /* The amount of rotation left to perform after this stage. */
    uint16_t max_remaining = (1<<bit_num)-1;

    /* The number of bytes to produce in this stage. */
    uint16_t stage_num_bytes =
      std::min(rot_buffer_length,
               uint16_t(buffer_out_length + max_remaining));

#if DEBUG_ROTATE_DOWN
    std::cout << "Stage " << stage
              << ", bit_num=" << int(bit_num)
              << ", stage_rot_amount=" << int(stage_rot_amount)
              << ", sel=" << int(sel)
              << " stage_num_bytes=" << int(stage_num_bytes)
              << "\n";
#endif

#ifdef NANOTUBE_SYNTHESIS
#pragma clang loop unroll(full)
#endif
    for (index = 0; index < rot_buffer_length; index++) {
      if (index >= stage_num_bytes)
        continue;
      uint16_t other_index = (stage_rot_amount+index) % rot_buffer_length;
      temp[stage+1][index] = ( sel
                               ? temp[stage][other_index]
                               : temp[stage][index] );
#if DEBUG_ROTATE_DOWN
      std::cout << "  Offset " << std::setw(2) << index
                << std::setw(0) << ": 0x"
                << std::setw(2) << std::setfill('0') << std::hex
                << int(temp[stage+1][index])
                << std::setw(0) << std::setfill(' ') << std::dec
                << "\n";
#endif
    }
#if DEBUG_ROTATE_DOWN
    std::cout << "\n";
#endif
  }

  /* Copy the output into the output buffer. */
#if DEBUG_ROTATE_DOWN
  std::cout << "Output buffer:\n";
#endif
#ifdef NANOTUBE_SYNTHESIS
#pragma clang loop unroll(full)
#endif
  for (index = 0; index < buffer_out_length; index++) {
    uint16_t src_index = index % rot_buffer_length;
    buffer_out[index] = temp[rot_amount_bits][src_index];
#if DEBUG_ROTATE_DOWN
    std::cout << "  Offset " << std::setw(2) << index
              << std::setw(0) << ": 0x"
              << std::setw(2) << std::setfill('0') << std::hex
              << int(buffer_out[index])
              << std::setw(0) << std::setfill(' ') << std::dec
              << "\n";
#endif
  }
#if DEBUG_ROTATE_DOWN
  std::cout << "\n";
#endif
}

///////////////////////////////////////////////////////////////////////////

enum byte_class: uint8_t {
  BYTE_CLASS_BEFORE,
  BYTE_CLASS_IN,
  BYTE_CLASS_AFTER,
};

/*! Classify the buffer entries relative to the specified region.
 *
 * \param result_buffer_length The number of entries in the buffer.
 *
 * \param result_buffer_index_bits The number of bits required to
 * represent an index into the buffer.
 *
 * \param buffer_out The output buffer.
 *
 * \param start_offset The offset of the first byte of the region.
 *
 * \param end_offset The offset after the last byte of the region.
 *
 * The buffer bytes are all set based on whether they are before, in
 * or after the specified region as follows:
 *   BYTE_CLASS_BEFORE: index < start_offset
 *   BYTE_CLASS_IN: index >= start_offset && index < end_offset
 *   BYTE_CLASS_AFTER: index >= end_offset
 *
 * The caller must make sure start_offset<=end_offset.
 */
static void nanotube_classify_entries(
  /* Constant parameters. */
  uint16_t result_buffer_length,
  uint8_t result_buffer_index_bits,

  /* Outputs. */
  enum byte_class *buffer_out,

  /* Inputs. */
  uint16_t start_offset,
  uint16_t end_offset)
#ifdef NANOTUBE_SYNTHESIS
  __attribute__((always_inline))
#endif
{
  /* Classify result buffer bytes as being before/in/after the current
   * word.  To make this efficient, the bytes are arranged in a
   * rectangular grid and the rows and columns of this grid are
   * classified in such a way that the classification of each byte can
   * be determined from the row classification and the column
   * classification.
   *
   * Bytes are classified using values:
   *   0 - Before the current word (retain value)
   *   1 - In the current word (set from rot_buffer)
   *   2 - After the current word (set to zero)
   *
   * Rows are classified using 3 bits:
   *   0 - Some bytes are classified as "before".
   *   1 - Some bytes are classified as "in".
   *   2 - Some bytes are classified as "after".
   *
   * NB: All rows have at least one bit set.  At most one row has bit
   * 0 and some other bit set.  At most one row has bit 2 and some
   * other bit set.
   *
   * Columns are classified using 2 bits:
   *
   *   0 - In the row containing the first byte of the word, the byte
   *       in this column is before the word.
   *
   *   1 - In the row containing the first byte after the word, the
   *       byte in this column is after the word.
   */

  /* Work out the grid size.  Try to make it close to being square,
   * but keep column count as a power of 2 so that division is
   * trivial. */
  uint16_t column_count = (1<<(result_buffer_index_bits/2));
  uint16_t row_count = ( (result_buffer_length + column_count - 1) /
                         column_count );

#if DEBUG_CLASSIFY
  std::cout << "  Grid size: " << column_count << " x " << row_count
            << "\n"
            << "  Row classes: ";
#endif

  /* Classify the rows. */
  enum {
    ROW_CLASS_BEFORE_BIT,
    ROW_CLASS_IN_BIT,
    ROW_CLASS_AFTER_BIT,
  };
  uint8_t row_class[row_count];
  uint16_t row_index;
#ifdef NANOTUBE_SYNTHESIS
#pragma clang loop unroll(full)
#endif
  for (row_index = 0; row_index < row_count; row_index++) {
    uint16_t row_start = row_index * column_count;
    uint16_t row_end = (row_index+1) * column_count;
    int some_before = (row_start < start_offset);
    int some_in = ( start_offset < row_end &&
                    end_offset > row_start );
    int some_after = (row_end > end_offset);
    row_class[row_index] = ( (some_before << ROW_CLASS_BEFORE_BIT) |
                             (some_in << ROW_CLASS_IN_BIT) |
                             (some_after << ROW_CLASS_AFTER_BIT) );
#if DEBUG_CLASSIFY
    std::cout << ' ' << uint32_t(row_class[row_index]);
#endif
  }

#if DEBUG_CLASSIFY
  std::cout << "\n  Col classes: ";
#endif

  /* Classify the columns. */
  enum {
    COL_CLASS_BEFORE_FIRST_BIT,
    COL_CLASS_AFTER_LAST_BIT,
  };
  uint8_t column_class[column_count];
  uint16_t start_column = (start_offset % column_count);
  uint16_t end_column = (end_offset % column_count);
  uint16_t column_index;
#ifdef NANOTUBE_SYNTHESIS
#pragma clang loop unroll(full)
#endif
  for (column_index = 0; column_index < column_count; column_index++) {
    int before_first = ( column_index < start_column );
    int after_last = ( column_index >= end_column );
    column_class[column_index] =
      ( (before_first << COL_CLASS_BEFORE_FIRST_BIT) |
        (after_last << COL_CLASS_AFTER_LAST_BIT) );
#if DEBUG_CLASSIFY
    std::cout << ' ' << uint32_t(column_class[column_index]);
#endif
  }

#if DEBUG_CLASSIFY
  std::cout << "\n  Byte classes:";
#endif

  int index;
#ifdef NANOTUBE_SYNTHESIS
#pragma clang loop unroll(full)
#endif
  for (index = 0; index < result_buffer_length; index++) {
    row_index = index / column_count;
    column_index = index % column_count;

    uint8_t this_row_class = row_class[row_index];
    int row_has_some_before =
      ( (this_row_class & (1<<ROW_CLASS_BEFORE_BIT)) != 0 );
    int row_has_some_in =
      ( (this_row_class & (1<<ROW_CLASS_IN_BIT)) != 0 );
    int row_has_some_after =
      ( (this_row_class & (1<<ROW_CLASS_AFTER_BIT)) != 0 );

    uint8_t this_col_class = column_class[column_index];
    int col_is_before_first =
      ( (this_col_class & (1<<COL_CLASS_BEFORE_FIRST_BIT)) != 0 );
    int col_is_after_last =
      ( (this_col_class & (1<<COL_CLASS_AFTER_LAST_BIT)) != 0 );

    enum byte_class this_byte_class = BYTE_CLASS_AFTER;

    // Check whether this row contains any bytes which are in the
    // current word.  If it does then there might be a transition from
    // "before" to "in" or "in" to "after".  If it doesn't then all
    // the bytes are "before" or all are "after" since there can be no
    // transition from "before" to "after" without being "in".
    if (row_has_some_in) {
      if (row_has_some_before && col_is_before_first) {
        this_byte_class = BYTE_CLASS_BEFORE;
      } else if (row_has_some_after && col_is_after_last) {
        this_byte_class = BYTE_CLASS_AFTER;
      } else {
        this_byte_class = BYTE_CLASS_IN;
      }
    } else {
      if (row_has_some_before) {
        this_byte_class = BYTE_CLASS_BEFORE;
      } else {
        this_byte_class = BYTE_CLASS_AFTER;
      }
    }

#if DEBUG_CLASSIFY
    std::cout << ' ' << uint32_t(this_byte_class);
#endif
    buffer_out[index] = this_byte_class;
  }

#if DEBUG_CLASSIFY
  std::cout << '\n';
#endif
}

///////////////////////////////////////////////////////////////////////////

void nanotube_shift_down_bits(uint32_t buffer_out_length,
                              uint8_t *buffer_out,
                              const uint8_t *buffer_in,
                              uint8_t shift_amount)
#ifdef NANOTUBE_SYNTHESIS
  __attribute__((always_inline))
#endif
{
#ifdef NANOTUBE_SYNTHESIS
#pragma clang loop unroll(full)
#endif
  for (uint32_t index = 0; index < buffer_out_length; index++) {
    uint16_t val_lo = buffer_in[index];
    uint16_t val_hi = buffer_in[index+1];
    uint16_t val = ( val_hi << 8 ) | val_lo;
    buffer_out[index] = val >> shift_amount;
  }
}

///////////////////////////////////////////////////////////////////////////

void nanotube_duplicate_bits(uint8_t *dup_bits_out,
                             const uint8_t *bit_vec_in,
                             uint32_t input_bit_length,
                             uint32_t padded_bit_length)
#ifdef NANOTUBE_SYNTHESIS
  __attribute__((always_inline))
#endif
{
  uint32_t input_byte_length = (input_bit_length+7)/8;
  uint32_t output_byte_length = (2*padded_bit_length+7)/8;
  uint32_t lshift = padded_bit_length % 8;
  uint32_t rshift = 8 - lshift;

#if DEBUG_DUPLICATE
  std::cout << "nanotube_duplicate_bits(input_bit_length="
            << input_bit_length
            << ", padded_bit_length=" << padded_bit_length
            << ")\n  input_byte_length=" << input_byte_length
            << ", output_byte_length=" << output_byte_length
            << ", lshift=" << lshift
            << ", rshift=" << rshift
            << '\n';
#endif

#ifdef NANOTUBE_SYNTHESIS
#pragma clang loop unroll(full)
#endif
  /* Write each output byte once. */
  for (uint32_t out_idx = 0; out_idx < output_byte_length; out_idx++) {
    /* Each output byte consists of at most three contiguous ranges of
     * bits.
     *
     *   - Unshifted bits in the first copy of the input.
     *   - Shifted left bits in the second copy of the input.
     *   - Shifted right bits in the second copy of the input.
     */
    uint8_t out_val = 0;
    uint32_t out_low_bit = 8 * out_idx;
    uint32_t out_high_bit = out_low_bit + 7;

#if DEBUG_DUPLICATE
    std::cout << "  Index " << out_idx << '\n';
    std::cout << std::hex << std::setfill('0');
#endif

    /* Include unshifted bits from the first copy. */
    if (out_low_bit < input_bit_length) {
      uint8_t byte = bit_vec_in[out_idx];
#if DEBUG_DUPLICATE
      std::cout << "    NoShift in[0x" << out_idx << "]=0x"
                << std::setw(2) << uint32_t(byte) << std::setw(0);
#endif
      if (out_high_bit >= input_bit_length) {
        uint32_t num_bits = (input_bit_length - out_low_bit);
        byte &= ( (1<<num_bits) - 1 );
#if DEBUG_DUPLICATE
        std::cout << ", num_bits=" << num_bits << " masked=0x"
                  << std::setw(2) << uint32_t(byte) << std::setw(0);
#endif
      }
#if DEBUG_DUPLICATE
      std::cout << "\n";
#endif
      out_val |= byte;
    }

    /* Include the shifted left bits in the second copy of the input.
     * We just calculate the input index to work out whether any bits
     * are in range.  This checks both the upper bound and lower bound
     * since the subtraction will wrap if out_high_bit is too low. */
    uint32_t lshift_in_idx = (out_high_bit - padded_bit_length) / 8;
    if (lshift_in_idx < input_byte_length) {
      uint8_t byte = bit_vec_in[lshift_in_idx];
#if DEBUG_DUPLICATE
      std::cout << "    LShift in[0x" << lshift_in_idx << "]=0x"
                << std::setw(2) << uint32_t(byte) << std::setw(0);
#endif

      uint32_t num_bits = input_bit_length - 8*lshift_in_idx;
      if (num_bits < 8) {
        byte &= (1<<num_bits)-1;
#if DEBUG_DUPLICATE
        std::cout << ", num_bits=" << num_bits << ", masked=0x"
                  << std::setw(2) << uint32_t(byte) << std::setw(0);
#endif
      }

      uint8_t sh_byte = (byte << lshift);
#if DEBUG_DUPLICATE
      std::cout << ", shifted=0x"
                << std::setw(2) << uint32_t(sh_byte) << std::setw(0)
                << '\n';
#endif

      out_val |= sh_byte;
    }

    /* Include the shifted right bits in the second copy of the input.
     * Again, we just calculate the input index to work out whether
     * any bits are in range. */
    uint32_t rshift_in_idx = lshift_in_idx - 1;
    if (rshift_in_idx < input_byte_length && rshift < 8) {
      uint8_t byte = bit_vec_in[rshift_in_idx];
#if DEBUG_DUPLICATE
      std::cout << "    RShift in[0x" << rshift_in_idx << "]=0x"
                << std::setw(2) << uint32_t(byte) << std::setw(0);
#endif

      uint32_t num_bits = input_bit_length - (8*rshift_in_idx);
      if (num_bits < 8) {
        byte &= (1<<num_bits)-1;
#if DEBUG_DUPLICATE
        std::cout << ", num_bits=" << num_bits << ", masked=0x"
                  << std::setw(2) << uint32_t(byte) << std::setw(0);
#endif
      }

      uint8_t shifted = byte >> rshift;
#if DEBUG_DUPLICATE
      std::cout << " shifted=0x"
                << std::setw(2) << uint32_t(shifted) << std::setw(0);
      std::cout << '\n';
#endif
      out_val |= shifted;
    }

#if DEBUG_DUPLICATE
    std::cout << "    Result 0x"
              << std::setw(2) << uint32_t(out_val) << std::setw(0)
              << '\n';
    std::cout << std::dec << std::setfill(' ');
#endif

    dup_bits_out[out_idx] = out_val;
  }
}


///////////////////////////////////////////////////////////////////////////

void nanotube_tap_packet_length_core(
  /* Outputs. */
  struct nanotube_tap_packet_length_resp *resp_out,

  /* State. */
  struct nanotube_tap_packet_length_state *state_inout,

  /* Inputs. */
  bool packet_word_eop,
  uint16_t packet_word_length,
  const struct nanotube_tap_packet_length_req *req_in)
#ifdef NANOTUBE_SYNTHESIS
  __attribute__((always_inline))
#endif
{
  // Calculate the offset at the end of this word.
  uint16_t end_offset = state_inout->packet_offset + packet_word_length;
  state_inout->packet_offset = (packet_word_eop ? 0 : end_offset);

  // Determine whether the request indicates that the response can be
  // sent.
  bool req_in_valid = (req_in->valid & 1) != 0;
  uint16_t req_in_max_length = req_in->max_length;
  bool req_done = ( req_in_valid && end_offset >= req_in_max_length );

  // Determine whether the response can be sent.
  bool valid = ( packet_word_eop || req_done );

  // Update the done bit.
  bool old_done = state_inout->done;
  state_inout->done = ( packet_word_eop ? 0 : (valid || old_done) );

  // Populate the response structure.
  resp_out->valid = valid && !old_done;
  resp_out->result_length = ( req_done ? req_in_max_length : end_offset );
}

///////////////////////////////////////////////////////////////////////////

void nanotube_tap_packet_read_core(
  /* Constant parameters */
  uint16_t result_buffer_length,
  uint8_t result_buffer_index_bits,
  uint16_t packet_buffer_length,
  uint8_t packet_buffer_index_bits,

  /* Outputs. */
  struct nanotube_tap_packet_read_resp *resp_out,
  uint8_t *result_buffer_inout,

  /* State. */
  struct nanotube_tap_packet_read_state *state_inout,

  /* Inputs. */
  const uint8_t *packet_buffer_in,
  bool packet_word_eop,
  uint16_t packet_word_length,
  const struct nanotube_tap_packet_read_req *req_in)
#ifdef NANOTUBE_SYNTHESIS
  __attribute__((always_inline))
#endif
{
  /* The read tap copies a contiguous region of the packet into the
   * result buffer.  It does this by first rotating the packet data so
   * that each result byte corresponds with a single byte of the
   * rotated data.  It then updates a selection of result bytes by
   * either copying the corresponding byte from the rotated data or
   * setting it to zero.
   */

  /* Determine the offset into the packet and update the packet offset
   * state. */
  uint16_t word_start_offset = state_inout->packet_offset;
  uint16_t word_end_offset = word_start_offset + packet_word_length;
  uint16_t new_packet_offset = ( packet_word_eop ? 0 : word_end_offset );
  state_inout->packet_offset = new_packet_offset;

#if DEBUG_PACKET_READ
  std::cout << "Tap packet read request: valid="
            << uint32_t(req_in->valid) << ", offset="
            << uint32_t(req_in->read_offset) << ", length="
            << uint32_t(req_in->read_length) << ", word_eop="
            << uint32_t(packet_word_eop) << ", word_len="
            << uint32_t(packet_word_length) << ".\n";
  std::cout << "  Word in:" << std::setfill('0') << std::hex;
#ifdef NANOTUBE_SYNTHESIS
#pragma clang loop unroll(full)
#endif
  for (uint32_t idx=0; idx<packet_buffer_length; idx++) {
    std::cout << " 0x" << std::setw(2) << uint32_t(packet_buffer_in[idx])
              << std::setw(0);
  }
  std::cout << std::dec << std::setfill(' ')
            << "\n  word_start_offset=" << uint32_t(word_start_offset)
            << ", word_end_offset=" << uint32_t(word_end_offset)
            << ", new_packet_offset=" << uint32_t(new_packet_offset)
            << "\n";
#endif

  /* Determine the rotation amount without performing a division.
   * When the word containing the first byte of the read arrives, the
   * rotation amount is the difference between the read offset and the
   * offset at the start of the word.  It does not change for later
   * words.
   */
  uint8_t started_read = ( req_in->valid &&
                           req_in->read_offset < word_end_offset );

  uint8_t first_read = ( started_read &&
                         req_in->read_offset >= word_start_offset );

  uint16_t rot_amount = ( first_read
                          ? req_in->read_offset - word_start_offset
                          : state_inout->rotate_amount );
  state_inout->rotate_amount = rot_amount;

#if DEBUG_PACKET_READ
  std::cout << "  started_read=" << int(started_read)
            << ", first_read=" << int(first_read)
            << ", rot_amount=" << int(rot_amount)
            << ".\n";
#endif

  /* Rotate the input packet word into the rotation buffer. */
  uint8_t rot_buffer[std::min(result_buffer_length,
                              packet_buffer_length)];
  nanotube_rotate_down(sizeof(rot_buffer),
                       packet_buffer_length,
                       packet_buffer_length,
                       packet_buffer_index_bits,
                       rot_buffer,
                       packet_buffer_in,
                       rot_amount);

#if DEBUG_PACKET_READ
  std::cout << "  Rotated: " << std::setfill('0') << std::hex;
#ifdef NANOTUBE_SYNTHESIS
#pragma clang loop unroll(full)
#endif
  for (uint32_t idx=0; idx<sizeof(rot_buffer); idx++) {
    std::cout << " 0x" << std::setw(2) << uint32_t(rot_buffer[idx])
              << std::setw(0);
  }
  std::cout << std::dec << std::setfill(' ') << '\n';
#endif

  /* Next work out how the current packet word relates to the result
   * buffer.  The result start index is the first byte of the buffer
   * which is in or after the packet word.  The result end index is
   * the first byte of the buffer which is after the packet word.
   */
  uint16_t max_frag_length =
    ( !started_read ? 0 :
      ( first_read
        ? word_end_offset - req_in->read_offset
        : packet_word_length ) );

  uint16_t result_start_offset = state_inout->result_offset;
  state_inout->result_offset =
    ( (packet_word_eop || !started_read) ? 0 :
      result_start_offset + max_frag_length );

  uint16_t result_end_offset =
    std::min(uint16_t(result_start_offset + max_frag_length),
             req_in->read_length);

  bool old_done = state_inout->done;
  bool is_done = ( ( req_in->valid &&
                     result_end_offset >= req_in->read_length ) ||
                   packet_word_eop );
  bool new_done = ( packet_word_eop ? 0 : is_done );
  state_inout->done = new_done;
  resp_out->valid = ( is_done && !old_done );
  resp_out->result_length = result_end_offset;

#if DEBUG_PACKET_READ
  std::cout << "  max_frag_length=" << uint32_t(max_frag_length)
            << ", result_start_offset=" << uint32_t(result_start_offset)
            << ", result_end_offset=" << uint32_t(result_end_offset)
            << ".\n"
            << "  old_done=" << uint32_t(old_done)
            << ", is_done=" << uint32_t(is_done)
            << ", new_done=" << uint32_t(new_done)
            << ".\n"
            << "  resp_out->valid=" << uint32_t(resp_out->valid)
            << ", resp_out->result_length=" << uint32_t(resp_out->result_length)
            << ".\n";
#endif

  enum byte_class byte_classes[result_buffer_length];
  nanotube_classify_entries(
    result_buffer_length,
    result_buffer_index_bits,
    byte_classes,
    result_start_offset,
    result_end_offset);

  int index;
#ifdef NANOTUBE_SYNTHESIS
#pragma clang loop unroll(full)
#endif
  for (index = 0; index < result_buffer_length; index++) {
    enum byte_class this_byte_class = byte_classes[index];

    uint8_t byte_val = 0;
    switch (this_byte_class) {
    case BYTE_CLASS_BEFORE:
      byte_val = result_buffer_inout[index];
      break;

    case BYTE_CLASS_IN:
      byte_val = rot_buffer[index % sizeof(rot_buffer)];
      break;

    case BYTE_CLASS_AFTER:
      byte_val = 0;
      break;
    }
    result_buffer_inout[index] = byte_val;
  }
}

///////////////////////////////////////////////////////////////////////////

void nanotube_tap_packet_write_core(
  /* Constant parameters */
  uint16_t request_buffer_length,
  uint8_t request_buffer_index_bits,
  uint16_t packet_buffer_length,
  uint8_t packet_buffer_index_bits,

  /* Outputs. */
  uint8_t *packet_buffer_out,

  /* State. */
  struct nanotube_tap_packet_write_state *state_inout,

  /* Inputs. */
  const uint8_t *packet_buffer_in,
  bool packet_word_eop,
  uint16_t packet_word_length,
  const struct nanotube_tap_packet_write_req *req_in,
  const uint8_t *request_bytes_in,
  const uint8_t *request_mask_in)
#ifdef NANOTUBE_SYNTHESIS
  __attribute__((always_inline))
#endif
{
  /* The write tap copies the request buffer into a contiguous region
   * of the packet.  It does this by first rotating the request data
   * so that each packet byte corresponds with a single byte of the
   * rotated data.  It does the same thing with the mask values.  It
   * then updates a selection of packet bytes by copying the
   * corresponding byte from the rotated data.
   */

  /* Determine the offset into the packet and update the packet offset
   * state. */
  uint16_t word_start_offset = state_inout->packet_offset;
  uint16_t word_end_offset = word_start_offset + packet_word_length;
  uint16_t new_packet_offset = ( packet_word_eop ? 0 : word_end_offset );
  state_inout->packet_offset = new_packet_offset;

  /* Determine the start and end offsets of the written region within
   * the packet. */
  uint16_t req_start_offset = req_in->write_offset;
  uint16_t req_length = ( req_in->write_length <= request_buffer_length
                          ? req_in->write_length
                          : request_buffer_length );
  uint16_t req_end_offset = req_start_offset + req_length;

  /* Determine whether this is the first write. */
  uint8_t first_write = ( req_start_offset >= word_start_offset );

  /* Determine the start offset of the written region within the
   * current word assuming this is the first word of the write. */
  uint16_t first_frag_start_word_offset = ( req_start_offset -
                                            word_start_offset );

  /* Next work out how the current packet word relates to the request
   * buffer.  The frag start offset is the first byte of the buffer
   * which is in or after the packet word.  The frag end offset is the
   * first byte of the buffer which is after the packet word.  If the
   * request is not valid, this code selects an empty range.  This
   * makes sure the tap has no effect until the valid bit is set.
   */
  uint16_t frag_start_word_offset =
    ( req_in->valid && (req_start_offset < word_end_offset)
      ? ( req_start_offset < word_start_offset
          ? 0
          : first_frag_start_word_offset )
      : packet_word_length );
  uint16_t frag_end_word_offset =
    ( req_in->valid && (req_end_offset < word_end_offset)
      ? ( req_end_offset < word_start_offset
          ? 0
          : req_end_offset - word_start_offset )
      : packet_word_length );

#if DEBUG_PACKET_WRITE
  std::cout << "Tap packet write request: valid="
            << uint32_t(req_in->valid) << ", offset="
            << uint32_t(req_in->write_offset) << ", length="
            << uint32_t(req_in->write_length) << ".\n";

  std::cout << "  word_start_offset=" << uint32_t(word_start_offset)
            << ", word_end_offset=" << uint32_t(word_end_offset)
            << ", new_packet_offset=" << uint32_t(new_packet_offset)
            << "\n"
            << ", first_write=" << int(first_write)
            << ", req_start_offset=" << req_start_offset
            << ", req_end_offset=" << req_end_offset
            << ", first_frag_swo=" << first_frag_start_word_offset
            << "\n";
  std::cout << "  frag_start_word_offset=" << uint32_t(frag_start_word_offset)
            << ", frag_end_word_offset=" << uint32_t(frag_end_word_offset)
            << ".\n";

  std::cout << "  Word in:  " << std::setfill('0') << std::hex;
  for (uint32_t idx=0; idx<packet_buffer_length; idx++) {
    std::cout << " 0x" << std::setw(2) << uint32_t(packet_buffer_in[idx])
              << std::setw(0);
  }

  std::cout << "\n  Data in:  ";
  for (uint32_t idx=0; idx<request_buffer_length; idx++) {
    uint32_t data_byte = request_bytes_in[idx];
    std::cout << " 0x" << std::setw(2) << data_byte << std::setw(0);
  }

  std::cout << "\n  Mask in:  ";
  for (uint32_t idx=0; idx<(request_buffer_length+7)/8U; idx++) {
    uint32_t mask_byte = request_mask_in[idx];
    std::cout << " 0x" << std::setw(2) << mask_byte << std::setw(0);
  }

  std::cout << "\n  MDat in:  ";
  for (uint32_t idx=0; idx<request_buffer_length; idx++) {
    uint32_t data_byte = request_bytes_in[idx];
    uint8_t mask_byte = request_mask_in[idx/8];
    uint8_t mask_bit = (mask_byte >> (idx%8)) & 1;
    if (mask_bit != 0) {
      std::cout << " 0x" << std::setw(2) << data_byte << std::setw(0);
    } else {
      std::cout << " ----";
    }
  }

  std::cout << '\n' << std::setfill(' ') << std::dec;
#endif

  /* Determine the rotate size and the rotate amounts for the first
   * words.  This is complicated slightly by the desire to avoid an
   * expensive modulo operation when the request buffer is smaller
   * than the packet buffer.  In that case, we would need to take the
   * offset of the write within the word modulo the request buffer
   * length to find the rotate amount.  To avoid the modulo operation
   * being expensive, we round the request buffer length up to a power
   * of 2 to determine the rotation buffer length.
   *
   * We handle two cases:
   *
   *   The rotate size is a power of 2, no larger than the packet
   *   word.  The request buffer is no larger than the rotate size.
   *
   *   The rotate size is the request buffer size which is at least as
   *   large as the packet word.
   *
   * At least one of these cases is available and neither requires a
   * modulo operation where the denominator is not a power of 2.
   */
  uint16_t rot_index_mask = ( (1 << request_buffer_index_bits) - 1 );
  uint16_t rot_buf_length;
  uint16_t first_rot_amount;

  if ((1 << request_buffer_index_bits) < packet_buffer_length) {
    /* In this case, rot_buf_length is a power of 2. */
    rot_buf_length = (1 << request_buffer_index_bits);
    first_rot_amount = ( ( rot_buf_length -
                           first_frag_start_word_offset ) &
                         rot_index_mask );
  } else {
    rot_buf_length = std::max(request_buffer_length,
                              packet_buffer_length);

    /* In this case, rot_length is at least as large as
     * packet_buffer_length, so there's no need for a modulo
     * operation.  Note that if first_frag_start_word_offset is zero,
     * the result of the subtraction will be rot_buf_length.  This
     * works in two different ways.  If rot_buf_length is a power of
     * two then the top bit is discarding resulting in a rotation by
     * zero, but that is the correct thing to do in that case.  If
     * rot_buf_length is not a power of two then the rotate amount is
     * does not need any extra bits so it is still within range. */
    first_rot_amount = ( ( rot_buf_length -
                           first_frag_start_word_offset ) &
                         rot_index_mask );
  }

  /* On the second word, we need to skip all the bytes which were
   * handled on the first word. */
  uint16_t second_rot_amount = ( packet_buffer_length -
                                 first_frag_start_word_offset );

  /* Determine the rotation amount for this word, checking whether
   * this is the first word of the region or not. */
  uint16_t rot_amount = ( first_write
                          ? first_rot_amount
                          : state_inout->rotate_amount );

  /* Determine the rotation amount for the next word, assuming there
   * is one and it is within the written region.  This is simply the
   * number of bytes consumed so far. */
  uint16_t new_rot_amount = ( first_write
                              ? second_rot_amount
                              : ( rot_amount +
                                  packet_buffer_length ) );
  state_inout->rotate_amount = new_rot_amount;

#if DEBUG_PACKET_WRITE
  std::cout << "  rot_buf_length=" << int(rot_buf_length)
            << "  rot_amount=" << int(rot_amount)
            << ", new_rot_amount=" << int(new_rot_amount)
            << ".\n";
#endif

  /* Rotate the request data into a buffer. */
  uint8_t rot_data[rot_buf_length];
  nanotube_rotate_down(sizeof(rot_data),
                       rot_buf_length,
                       request_buffer_length,
                       request_buffer_index_bits,
                       rot_data,
                       request_bytes_in,
                       rot_amount);

#if DEBUG_PACKET_WRITE
  std::cout << "  rot_data: " << std::setfill('0') << std::hex;
  for (uint32_t idx=0; idx<sizeof(rot_data); idx++) {
    std::cout << " 0x" << std::setw(2) << uint32_t(rot_data[idx])
              << std::setw(0);
  }
  std::cout << std::dec << std::setfill(' ') << '\n';
#endif

  /* Rotate the request mask into a buffer as follows:
   *   1. Duplicate the mask, padding with zeros.
   *   2. Rotate down by the required number of bytes.
   *   3. Shift down by the remaining bits.
   *
   * Duplicating the mask is required so that the rotation wraps at
   * the correct bit position.
   */
  uint8_t dup_mask[(2*rot_buf_length + 7) / 8];
  nanotube_duplicate_bits(dup_mask, request_mask_in, request_buffer_length,
                          rot_buf_length);

  uint8_t byte_rot_mask[(sizeof(rot_data) + 7) / 8 + 1];
  nanotube_rotate_down(sizeof(byte_rot_mask),
                       sizeof(dup_mask),
                       sizeof(dup_mask),
                       std::max(request_buffer_index_bits,uint8_t(3)) - 3,
                       byte_rot_mask,
                       dup_mask,
                       rot_amount>>3);

  uint8_t rot_mask[(sizeof(rot_data) + 7) / 8];
  nanotube_shift_down_bits(sizeof(rot_mask),
                           rot_mask,
                           byte_rot_mask,
                           rot_amount & 7);

#if DEBUG_PACKET_WRITE
  std::cout << std::setfill('0') << std::hex;

  std::cout << "  dup_mask: ";
  for (uint32_t idx=0; idx<sizeof(dup_mask); idx++) {
    std::cout << " 0x" << std::setw(2) << uint32_t(dup_mask[idx])
              << std::setw(0);
  }
  std::cout << '\n';

  std::cout << "  byte_rot_mask: ";
  for (uint32_t idx=0; idx<sizeof(byte_rot_mask); idx++) {
    std::cout << " 0x" << std::setw(2) << uint32_t(byte_rot_mask[idx])
              << std::setw(0);
  }
  std::cout << '\n';

  std::cout << "  rot_mask: ";
  for (uint32_t idx=0; idx<sizeof(rot_mask); idx++) {
    std::cout << " 0x" << std::setw(2) << uint32_t(rot_mask[idx])
              << std::setw(0);
  }
  std::cout << '\n';

  std::cout << "  rot_mdat: ";
  for (uint32_t idx=0; idx<sizeof(rot_data); idx++) {
    uint32_t val = rot_data[idx];
    uint8_t mask_byte = rot_mask[idx/8];
    uint8_t mask_bit = (mask_byte >> (idx%8)) & 1;
    if (mask_bit != 0)
      std::cout << " 0x" << std::setw(2) << val << std::setw(0);
    else
      std::cout << " ----";
  }
  std::cout << '\n';

  std::cout << std::dec << std::setfill(' ');
#endif

  /* Classify the bytes of the packet buffer. */
  enum byte_class byte_classes[packet_buffer_length];
  nanotube_classify_entries(
    packet_buffer_length,
    packet_buffer_index_bits,
    byte_classes,
    frag_start_word_offset,
    frag_end_word_offset);

  /* Write the packet buffer. */
#ifdef NANOTUBE_SYNTHESIS
#pragma clang loop unroll(full)
#endif
  for (uint16_t index=0; index<packet_buffer_length; index++) {
    uint8_t packet_in_byte = packet_buffer_in[index];
    uint16_t rot_index = index % rot_buf_length;
    uint8_t req_byte = rot_data[rot_index];
    uint8_t mask_byte = rot_mask[rot_index/8];
    uint8_t mask_bit = (mask_byte >> (rot_index&7)) & 1;
    enum byte_class this_byte_class = byte_classes[index];
    uint8_t output_byte =
      ( this_byte_class == BYTE_CLASS_IN && mask_bit != 0
        ? req_byte : packet_in_byte );
    packet_buffer_out[index] = output_byte;
  }

#if DEBUG_PACKET_WRITE
  std::cout << "  Word out: " << std::setfill('0') << std::hex;
  for (uint32_t idx=0; idx<packet_buffer_length; idx++) {
    uint32_t val = packet_buffer_out[idx];
    std::cout << " 0x" << std::setw(2) << val << std::setw(0);
  }
  std::cout << std::dec << std::setfill(' ') << '\n';
#endif
}

///////////////////////////////////////////////////////////////////////////

// Packet resize tap operation
// ===========================
//
// The packet resize tap is used to insert bytes into and remove bytes
// from a packet.  The inserted bytes are zeros and the removed bytes
// are discarded.  To insert non-zero bytes, use a write tap after the
// resize tap.  To examine removed bytes, use a read tap before the
// resize tap.
//
// The tap is split into two stages for performance reasons.  The
// ingress stage produces a control word for each input packet word
// which contains data to be output.  The egress stage uses the
// control word to produce the output packet words.  The egress stage
// may produce more than one output packet word per input packet word
// when it is inserting bytes.  Either or both stages may consume an
// input packet word without producing an output packet word when
// deleting bytes.
//
// The egress stage needs to work out whether it will need to produce
// another output packet word for the current input packet word before
// moving on to the next input packet word.  It needs to do this in a
// single cycle in order to get good performance, so the logic on the
// feedback path needs to be simple.  This is done by sending an
// output limit from the ingress stage to the egress stage as part of
// the control word.  The egress stage will produce packet words while
// all the byte offsets are below the output limit.
//
// Only the egress stage processes packet data.  The ingress stage
// just processes lengths and offsets.  The egress stage needs a
// carried-data buffer to hold input bytes which will need to be
// output, but not in the current output packet word.
//
// Packet and word structures
// --------------------------
//
// The input packet consists of three regions:
//   Unshifted bytes which are before the deleted bytes.
//   Deleted bytes.
//   Shifted bytes which are after the deleted bytes.
//
// The output packet consists of three regions:
//   Unshifted bytes which are before the inserted bytes.
//   Inserted zero bytes.
//   Shifted bytes which are after the inserted bytes.
//
// The first region is called unshifted because these bytes have the
// same offset in the output packet as in the input packet so they can
// be passed through the tap without being shifted into a different
// byte lane (i.e. a different offset in the word).
//
// The third region is called shifted because these bytes typically
// have a different offset in the output packet than in the input
// packet.  This means they need to be shifted into different byte
// lanes as the pass through the tap.
//
// The structure of the input/output packets means that each packet
// word has the following structure:
//   Unshifted bytes.
//   Inserted/deleted bytes.
//   Shifted bytes.
//   Padding bytes after the end of packet.
//
// Any of these regions may be empty.  For the most part, padding
// bytes are treated like shifted bytes.  It's just that the length of
// the final word of the packet will be reduced so that they will be
// ignored by downstream logic.
//
// There are two basic packet structures, one where unshifted and
// shifted bytes are not present in the same packet word, and one
// where they are.  These basic structures may be varied by omitting
// or repeating words which contain only a single byte type and/or by
// truncating the packet at some point.  The positions of the
// divisions between regions within a word may also be varied,
// potentially making regions empty within a word.
//
//   +-------------+   +-------------+   Key:
//   |      U      |   |      U      |     U - Unshifted
//   +---+---------+   +---+-----+---+     D - Deleted
//   | U |   D/I   |   | U | D/I | S |     I - Inserted
//   +---+---------+   +---+-----+---+     S - Shifted
//   |     D/I     |   |      S      |     P - Padding
//   +---------+---+   +-------+-----+
//   |   D/I   | S |   |   S   |  P  |
//   +---------+---+   +-------+-----+
//   |      S      |
//   +-------+-----+
//   |   S   |  P  |
//   +-------+-----+
//
// The structure of the packet implies the following constraints:
//
//   If a word contains unshifted bytes then any previous word was
//   filled with unshifted bytes.
//
//   If a word contains any unshifted bytes then they are all
//   contiguous with the start of the word.
//
//   If a word contains shifted bytes then any later word will be
//   filled with shifted or padding bytes.
//
//   If a word contains any shifted or padding bytes then they are all
//   contiguous with the end of the word.
//
// The structure of the input/output packet words is fixed by the
// contents of the input/output packets.  There's some flexibility in
// the structure of the carried words because the hard requirement is
// that bytes are reliably stored until they are needed.  That doesn't
// imply they need to be stored in a particular location.  For
// simplicity, the bytes in the carried word are stored in the same
// lane as they had in the input word.  This works because the
// unshifted bytes always occupy the low lanes and the shifted bytes
// always occupy the high lanes.  The total number of bytes being
// carried is always less than or equal to the word length because
// otherwise they would have been output while the previous input word
// was being processed.  As a result, the unshifted and shifted bytes
// never overlap.
//
// The ingress stage
// -----------------
//
// The ingress stage is responsible for producing an output limit
// which can be used by the egress stage to quickly work out whether
// it should move on to the next input packet word.  The output limit
// is the lowest packet offset which should not be output by the
// egress stage.  The egress stage will stop earlier than that if it
// cannot produce a complete output word.  The ingress stage must make
// sure the number of carried bytes is always less than the word
// length.
//
// This arrangement allows the ingress stage to calculate the values
// it produces without performing a division by the word length.
// Dividing by the word length is simple when the bus width is a power
// of two, but when the bus width is large, being restricted to a
// power of two is a significant limitation.  Instead, the egress uses
// successive addition to divide the output packet into words.
//
// Care is needed to handle the end of packet.  The ingress must never
// allow the final byte of the output packet to be released until it
// has set the EOP flag.  This means the ingress can only release a
// byte which might be the final byte if it has seen EOP or the
// following byte will be included in the output packet.
//
// The egress stage
// ----------------
//
// There are several sub-stages in the egress stage.  First, the flow
// control sub-stage works out whether the next call will continue
// processing the current packet word or move on to the next one.
// Next, the tracker sub-stage works out how bytes flow through the
// following sub-stages.  Next, the carry sub-stage stores bytes which
// will not be output during this call for the next one and retrieves
// bytes which were previously stored.  The shifter sub-stage then
// shifts bytes from the input word and the carried word into the
// correct byte-lane.  Finally, the combiner sub-stage produces the
// final output word from the unshifted and shifted bytes, filling the
// gap between them with zeros.
//
// In the egress stage, there are four packet words being handled:
//   The incoming carried word from the previous call.
//   The input packet word.
//   The output packet word.
//   The outgoing carried word to the next call.
//
// Words are typically described using the following variables.  This
// regularity makes the resize tap easier to think about and the
// choice of variables allows the feedback logic to be reasonably
// simple.
//
//   unshifted_len: The number of unshifted bytes.
//
//   shifted_start: The output packet offset of the start of first
//     shifted byte.
//
//   word_end: One more than the offset of the last byte in this word,
//     including any padding.
//
// There are various word structures which the egress stage might need
// to deal with during any one call and enumerating the cases would be
// complex and error prone.  We can simplify the problem by thinking
// about the unshifted and shifted bytes separately.
//
// When unshifted bytes arrive at the egress stage, they will be
// included in the next output word.  If a word is not being output
// during this call then they will be carried for the next one.  The
// cases to consider are:
//
//   No word is being output during this call and there are unshifted
//   bytes in the input word.  These bytes will be stored in the
//   carried word for the next call.
//
//       Carried in   Input word       Output word  Carried out
//     +------------+---+--------+    +------------+---+--------+
//     |     -      | U |   -    | -> |      -     | U |   -    |
//     +------------+---+--------+    +------------+---+--------+
//
//   No word is being output during this call and there are no
//   unshifted bytes in the input word.  If there are any unshifted
//   bytes in the carried word then they will be kept for the next
//   call.
//
//       Carried in   Input word       Output word  Carried out
//     +---+--------+------------+    +------------+---+--------+
//     | U |   -    |      -     | -> |      -     | U |   -    |
//     +---+--------+------------+    +------------+---+--------+
//
//   A word is being output during this call and there are unshifted
//   bytes in the input word.  These bytes will be placed in the
//   output word, so no unshifted bytes will be placed in the carried
//   word for the next call.
//
//       Carried in   Input word       Output word  Carried out
//     +------------+---+--------+    +---+--------+------------+
//     |     -      | U |   -    | -> | U |   -    |     -      |
//     +------------+---+--------+    +---+--------+------------+
//
//   A word is being output during this call and there are no
//   unshifted bytes in the input word.  If there are unshifted bytes
//   in the carried word then they will be output during this call, so
//   they are removed from the carried word.
//
//       Carried in   Input word       Output word  Carried out
//     +---+--------+------------+    +---+--------+------------+
//     | U |   -    |      -     | -> | U |   -    |     -      |
//     +---+--------+------------+    +---+--------+------------+
//
// When shifted bytes arrive at the egress stage, they may be output
// directly or stored in the carried word for a later output word.
// Shifted bytes can come from the input word or from the carried
// word.  The bytes are shifted into the correct lane as they are
// transferred to the output word.  If there are any shifted bytes at
// the end of the carried word then they are contiguous with shifted
// bytes at the start of the input word, so these two words are
// concatenated before being shifted.
//
//   +---------------+---------------+
//   | Carried word  |  Input word   |
//   +----------+----+----------+----+
//              | Output word   |
//              +---------------+
//
// There are a few cases to consider:
//
//   No word is being output during this call and there are shifted
//   bytes in the input word.  The shifted bytes will be stored in the
//   carried word.  Note that there can be no shifted bytes in the
//   carried word because otherwise the input word would be full of
//   shifted bytes and there would be a full word to output.
//
//       Carried in   Input word       Output word  Carried out
//     +------------+--------+---+    +------------+------------+
//     |     -      |   -    | S | -> |      -     |   -    | S |
//     +------------+--------+---+    +------------+------------+
//
//   No word is being output during this call and there are no shifted
//   bytes in the input word.  If there are any shifted bytes in the
//   carried word then they will be kept for the next call.
//
//       Carried in   Input word       Output word  Carried out
//     +--------+---+------------+    +------------+------------+
//     |   -    | S |     -      | -> |      -     |   -    | S |
//     +--------+---+------------+    +------------+------------+
//
//   A word is being output during this call and the input word starts
//   with a shifted byte.  If there are any shifted bytes in the
//   carried word then they will be output.  They will not be carried
//   because that would indicate that there were enough bytes to
//   output another word in the previous call.  If there are any
//   shifted bytes in the input word with an offset beyond the end of
//   the output word then they will be carried.
//
//       Carried in   Input word       Output word  Carried out
//     +--------+---+------------+    +---+--------+----+-------+
//     |   -    | S |     S      | -> | - |   S    | -  |   S   |
//     +--------+---+------------+    +---+--------+----+-------+
//
//   A word is being output during this call and the input word does
//   not start with a shifted byte.  In this case, there can be no
//   shifted bytes in the carried word.  If there are any shifted
//   bytes in the input word with an offset before the end of the
//   output word then they will be output.  If there are any other
//   shifted bytes in the input word then they will be carried.
//
//       Carried in   Input word       Output word  Carried out
//     +------------+----+-------+    +--------+---+--------+---+
//     |     -      | -  |   S   | -> |   -    | S |   -    | S |
//     +------------+----+-------+    +--------+---+--------+---+

void nanotube_tap_packet_resize_ingress_state_init(
  /* Outputs. */
  nanotube_tap_packet_resize_ingress_state_t *state)
#ifdef NANOTUBE_SYNTHESIS
  __attribute__((always_inline))
#endif
{
  state->sop = true;
  state->input_offset = 0;
  state->packet_length = 0;
  state->data_eop_seen = 0;
}

void nanotube_tap_packet_resize_ingress_core(
  /* Parameters. */
  nanotube_tap_offset_t packet_word_length,

  /* Outputs. */
  bool *packet_done_out,
  nanotube_tap_packet_resize_cword_t *cword_out,
  nanotube_tap_offset_t *packet_length_out,

  /* State. */
  nanotube_tap_packet_resize_ingress_state_t *state,

  /* Inputs. */
  nanotube_tap_packet_resize_req_t *req_in,
  nanotube_tap_offset_t word_length_in,
  bool word_eop_in)
#ifdef NANOTUBE_SYNTHESIS
  __attribute__((always_inline))
#endif
{
#if DEBUG_RESIZE_IN_TAP
  std::cout << "Resize ingress received word:"
            << " word_length_in=" << word_length_in
            << " word_eop_in=" << word_eop_in
            << "\n"
            << "  Request:"
            << " write_offset=" << req_in->write_offset
            << " delete_length=" << req_in->delete_length
            << " insert_length=" << req_in->insert_length
            << "\n"
            << "  Old state:"
            << " sop=" << int(state->sop)
            << " input_offset=" << int(state->input_offset)
            << "\n";
#endif

  nanotube_tap_packet_resize_req_t req = *req_in;

  //
  // Update the state.
  //

  state->sop = (word_eop_in != false);
  *packet_done_out = (word_eop_in != false);

  // The start of the data in the word.
  nanotube_tap_offset_t input_data_start =
    state->input_offset;

  // The end of the data in the word.  This points before the padding
  // in the EOP word.
  nanotube_tap_offset_t input_data_end =
    state->input_offset + word_length_in;

  // The end of the word.  In non-EOP words, it is the same as
  // input_data_end.  It points after the padding in the EOP word.
  nanotube_tap_offset_t input_word_end =
    state->input_offset + packet_word_length;

  state->input_offset = (word_eop_in ? 0 : input_word_end);

  //
  // Work out the significant offsets into the input/output packets.
  //
  nanotube_tap_offset_t delete_start = req.write_offset;
  nanotube_tap_offset_t delete_end = delete_start + req.delete_length;
  nanotube_tap_offset_t insert_start = req.write_offset;
  nanotube_tap_offset_t insert_end = insert_start + req.insert_length;
  nanotube_tap_offset_t edit_delta =
    req.insert_length - req.delete_length;

#if DEBUG_RESIZE_IN_TAP
  std::cout << "  Offsets:"
            << " delete_start=" << delete_start
            << " delete_end=" << delete_end
            << " insert_start=" << insert_start
            << " insert_end=" << insert_end
            << " edit_delta=" << edit_delta
            << "\n";
#endif

  //
  // Work out which regions have been completed at the start and end
  // of the input data.
  //
  bool unshifted_done_start = (input_data_start >= delete_start);
  bool deleted_done_start = (input_data_start >= delete_end);
  bool unshifted_done_end = (input_data_end >= delete_start);
  bool deleted_done_end = (input_data_end >= delete_end);

#if DEBUG_RESIZE_IN_TAP
  std::cout << "  Regions:"
            << " unshifted_done_start=" << int(unshifted_done_start)
            << " deleted_done_start=" << int(deleted_done_start)
            << " unshifted_done_end=" << int(unshifted_done_end)
            << " deleted_done_end=" << int(deleted_done_end)
            << "\n";
#endif

  //
  // Work out the number of unshifted bytes.
  //
  nanotube_tap_offset_t output_unshifted_len;
  if (!unshifted_done_end) {
    // Before the start of the deleted region.  All bytes are
    // unshifted.
    output_unshifted_len = packet_word_length;

  } else if (!unshifted_done_start) {
    // The deleted region starts in the current word.  Find out the
    // number of bytes in the word which are before it.
    output_unshifted_len = delete_start - input_data_start;

  } else {
    // The deleted region started earlier.  All the unshifted bytes
    // have been processed.
    output_unshifted_len = 0;
  }

  //
  // Work out the output offset of the end of the word.
  //
  nanotube_tap_offset_t output_word_end;
  if (!unshifted_done_end) {
    // Before the end of the unshifted region.  The output offset is
    // the same as the input offset.
    output_word_end = input_word_end;

  } else if (!deleted_done_end) {
    // Within the deleted region.  The output offset is the end of the
    // insert region.
    output_word_end = insert_end;

  } else {
    // After the start of the shifted region.  Adjust the input offset
    // for the inserted/deleted bytes.
    output_word_end = input_word_end + edit_delta;
  }

  //
  // Work out the shifted start offset.
  //
  nanotube_tap_offset_t output_shifted_start;
  if (!deleted_done_end) {
    // Before the end of the deleted region.  There are no shifted
    // bytes, so just copy the output word end.
    output_shifted_start = output_word_end;

  } else if (!deleted_done_start) {
    // The transition from the deleted region to the shifted region is
    // somewhere in the input word.
    output_shifted_start = insert_end;

  } else {
    // After the end of the deleted region.  All bytes are shifted.
    output_shifted_start = input_data_start + edit_delta;
  }

  //
  // Work out the output limit.
  //
  nanotube_tap_offset_t output_limit;
  if (!unshifted_done_end) {
    // The end of the word is before the start of the deleted bytes.
    if (!word_eop_in) {
      // EOP has not been seen.  Since we are before the start of the
      // deleted bytes, the next byte will be included in the output
      // so there is no need to withhold a byte.
      output_limit = input_data_end;

    } else {
      // EOP has been seen.  The output limit includes all the bytes
      // which have been seen plus (word_length-1) of possible padding
      // bytes.
      output_limit = input_data_end + packet_word_length - 1;
    }

  } else if (!deleted_done_end) {
    // The end of the word is before the end of the deleted bytes.
    if (!word_eop_in) {
      // EOP has not been seen.  The next byte is deleted, so the
      // final unshifted byte of this word might be the end of packet.
      // Withhold that byte until it is known whether it needs to be
      // output with the EOP flag set.  This is to handle the case
      // where the unshifted bytes end on a word boundary and then the
      // end of the input packet occurs within the deleted region, so
      // there are no shifted bytes.  The desired behaviour is to
      // output the final word of unshifted bytes with the EOP flag
      // set, but we only know to do that once the input EOP has been
      // seen.
      output_limit = insert_end - 1;

    } else {
      // EOP has been seen.  The output limit is just the output
      // packet length plus up to (word_length-1) bytes of padding.
      output_limit = insert_end + packet_word_length - 1;
    }

  } else { // unshifted_done_end && deleted_done_end
    // After the end of the deleted bytes.
    if (!word_eop_in) {
      // EOP has not been seen.  All the deleted bytes have been seen,
      // so there is no need to withhold a byte to handle EOP.
      output_limit = input_data_end + edit_delta;

    } else {
      // EOP has been seen.  Include (word_length-1) of possible
      // padding bytes.
      output_limit = ( input_data_end + edit_delta +
                       packet_word_length - 1 );
    }
  }

  cword_out->unshifted_len = output_unshifted_len;
  cword_out->shifted_start = output_shifted_start;
  cword_out->word_end = output_word_end;
  cword_out->output_limit = output_limit;
  cword_out->eop = word_eop_in;

#if DEBUG_RESIZE_IN_TAP
  std::cout << "  cword:"
            << " unshifted_len=" << cword_out->unshifted_len
            << " shifted_start=" << cword_out->shifted_start
            << " word_end=" << cword_out->word_end
            << " output_limit=" << cword_out->output_limit
            << " eop=" << int(cword_out->eop)
            << "\n"
            << "  New state:"
            << " sop=" << int(state->sop)
            << " input_offset=" << int(state->input_offset)
            << "\n";
#endif
}

void nanotube_tap_packet_resize_egress_state_init(
  /* Outputs. */
  nanotube_tap_packet_resize_egress_state_t *state)
#ifdef NANOTUBE_SYNTHESIS
  __attribute__((always_inline))
#endif
{
  state->new_word = true;
  state->eop_seen = true;
  state->did_output = true;         // Ignored due to SOP.
  state->prev_word_end = 0;         // Ignored due to SOP.
  state->prev_word_end_adv1 = 0;    // Ignored due to SOP.
  state->prev_word_end_adv2 = 0;    // Ignored due to SOP.
  state->carried_unshifted_len = 0; // Ignored due to SOP.
  state->carried_shifted_start = 0; // Ignored due to SOP.
  state->carried_word_end = 0;      // Ignored due to SOP.
  state->new_pkt = true;
  state->packet_length = 0;
  state->packet_offset = 0;
}

void nanotube_tap_packet_resize_egress_core(
  /* Parameters. */
  nanotube_tap_offset_t packet_word_length,
  unsigned int packet_word_index_bits,

  /* Outputs. */
  bool *input_done_out,
  bool *word_valid_out,
  bool *word_eop_out,
  nanotube_tap_offset_t *word_length_out,
  uint8_t *word_data_out,

  /* State. */
  nanotube_tap_packet_resize_egress_state_t *state,
  uint8_t *carried_data,

  /* Inputs. */
  nanotube_tap_packet_resize_cword_t *cword,
  uint8_t *packet_data_in)
#ifdef NANOTUBE_SYNTHESIS
  __attribute__((always_inline))
#endif
{
#if DEBUG_RESIZE_EG_TAP
  std::cout << "Resize egress invoked:"
            << " unshifted_len=" << cword->unshifted_len
            << " shifted_start=" << cword->shifted_start
            << " word_end=" << cword->word_end
            << " output_limit=" << cword->output_limit
            << " eop=" << int(cword->eop)
            << "\n"
            << "  Old state:"
            << " new_word=" << int(state->new_word)
            << " eop_seen=" << int(state->eop_seen)
            << " did_output=" << int(state->did_output)
            << " prev_word_end=" << state->prev_word_end
            << " prev_word_end_adv1=" << state->prev_word_end_adv1
            << " prev_word_end_adv2=" << state->prev_word_end_adv2
            << " carried_unshifted_len=" << state->carried_unshifted_len
            << " carried_shifted_start=" << state->carried_shifted_start
            << " carried_word_end=" << state->carried_word_end
            << "\n";
#endif

  //
  // The flow control sub-stage.
  //

  bool new_word = state->new_word;
  bool sop = new_word && state->eop_seen;

  // Work out the end offset of this word.
  nanotube_tap_offset_t output_word_end;
  nanotube_tap_offset_t output_word_end_adv1;
  nanotube_tap_offset_t output_word_end_adv2;
  nanotube_tap_offset_t output_word_start;
  if (sop) {
    // The old packet is complete, so start at the end of the first
    // word.
    output_word_end      = 1 * packet_word_length;
    output_word_end_adv1 = 2 * packet_word_length;
    output_word_end_adv2 = 3 * packet_word_length;

  } else if (state->did_output) {
    // Continuing a packet and a word was output, so advance to the
    // next word.
    output_word_end      = state->prev_word_end_adv1;
    output_word_end_adv1 = state->prev_word_end_adv2;
    output_word_end_adv2 = ( state->prev_word_end_adv2 +
                             packet_word_length );

  } else {
    // Continuing a packet but no word was output, so stay with the
    // old counter.
    output_word_end      = state->prev_word_end;
    output_word_end_adv1 = state->prev_word_end_adv1;
    output_word_end_adv2 = state->prev_word_end_adv2;
  }
  output_word_start = output_word_end - packet_word_length;

  // Indicates that we will push a word during this call.
  bool do_output = (output_word_end <= cword->output_limit);

  // Indicates that this call completes the handling of the input
  // word.
  bool word_done = (output_word_end_adv1 > cword->output_limit);

  // Indicates that this output word is the end of packet.
  bool eop = word_done && cword->eop;

  *input_done_out = word_done;
  *word_eop_out = eop;

  // Update the state.
  state->new_word           = word_done;
  state->eop_seen           = cword->eop;
  state->did_output         = do_output;
  state->prev_word_end      = output_word_end;
  state->prev_word_end_adv1 = output_word_end_adv1;
  state->prev_word_end_adv2 = output_word_end_adv2;

  //
  // The tracker sub-stage.
  //

  // Work out how the unshifted bytes are being transferred.

  nanotube_tap_offset_t input_unshifted_len = cword->unshifted_len;
  nanotube_tap_offset_t carried_in_unshifted_len =
    (sop ? 0 : state->carried_unshifted_len);

  nanotube_tap_offset_t output_unshifted_len;

  bool input_any_unshifted = (cword->unshifted_len != 0);

  if (!do_output) {
    if (input_any_unshifted) {
      // No word is being output and there are unshifted bytes in the
      // input word.  Store them in the carried word.
      state->carried_unshifted_len = input_unshifted_len;

    } else {
      // No word is being output and there are no unshifted bytes in
      // the input word.  Keep the carried unshifted bytes if there
      // are any.
    }

  } else { // do_output==true
    // A word is being output, so all the unshifted bytes will be
    // output and none will be carried.
    state->carried_unshifted_len = 0;
  }

  output_unshifted_len = ( (new_word && input_any_unshifted)
                           ? input_unshifted_len
                           : carried_in_unshifted_len );

  // Work out how the shifted bytes are being transferred.

  nanotube_tap_offset_t input_shifted_start = cword->shifted_start;
  nanotube_tap_offset_t input_word_end = cword->word_end;

  assert(input_shifted_start <= input_word_end);
  assert(input_word_end - input_shifted_start <= packet_word_length);

  nanotube_tap_offset_t carried_in_shifted_start =
    (sop ? packet_word_length : state->carried_shifted_start);

  nanotube_tap_offset_t carried_in_word_end =
    (sop ? packet_word_length : state->carried_word_end);

  bool input_any_shifted =
    (input_shifted_start != input_word_end);

  bool input_all_shifted =
    (input_word_end - input_shifted_start == packet_word_length);

  if (!do_output) {
    if (input_any_shifted) {
      // No word is being output and there are shifted bytes in the
      // input word.  Store them in the carried word.
      state->carried_shifted_start = input_shifted_start;
      state->carried_word_end = input_word_end;

    } else if (sop) {
      // No word is being output, there are no shifted bytes in the
      // input and this is the start of a packet.  Clear the carried
      // word left over from the previous packet.
      state->carried_shifted_start = output_word_end;
      state->carried_word_end = output_word_end;

    } else {
      // No word is being output, there are no shifted bytes in the
      // input word and this is not the start of a packet.  Keep the
      // carried shifted bytes if there are any.
    }

  } else { // do_output==true
    if (input_all_shifted) {
      // A word is being output and the input word starts with a
      // shifted byte.  Any shifted bytes beyond the end of the output
      // word will be carried.
      state->carried_shifted_start = output_word_end;
      state->carried_word_end = input_word_end;

    } else {
      // A word is being output and the input word does not start with
      // a shifted byte.  Split the shifted bytes in the input word
      // between the output word and the carried word.
      if (output_word_end <= input_shifted_start) {
        // All the bytes are carried.
        state->carried_shifted_start = input_shifted_start;

      } else if (output_word_end <= input_word_end) {
        // Some bytes are carried.
        state->carried_shifted_start = output_word_end;

      } else {
        // None of the bytes are carried.
        state->carried_shifted_start = input_word_end;
      }
      state->carried_word_end = input_word_end;
    }
  }

  // Work out how many bytes to keep in the carried word.  The
  // unshifted bytes can only come from one place and the shifted
  // bytes also.  We just need to work out where they should come
  // from.
  nanotube_tap_offset_t carried_keep_len;

  if (sop) {
    // This is the first word of the packet.  Do not keep any bytes
    // because they come from a different packet.  This is important
    // to avoid leaking information between packets.
    carried_keep_len = 0;

  } else if (do_output) {
    // A word is being output.  No unshifted bytes need to be kept and
    // any shifted bytes come from the input word.
    carried_keep_len = 0;

  } else if (input_any_unshifted || input_any_shifted) {
    // No word is being output and there are bytes to store from the
    // input word.  Keep any unshifted bytes which are already in the
    // carried word.  There cannot be any shifted bytes in the carried
    // word here because if there were then the input packet word
    // would be full of shifted bytes, causing the carried word to be
    // flushed out.
    assert(carried_in_shifted_start == carried_in_word_end);
    carried_keep_len = carried_in_unshifted_len;

  } else {
    // No word is being output and there are no bytes to store from
    // the input word.  Keep all the carried word.  This is needed
    // when inserting a whole word of zeros.
    carried_keep_len = packet_word_length;
  }

  //
  // The carry sub-stage.
  //

  // Concatenate the incoming carried word and input packet word for
  // the shifter.  The incoming carried word stored here is also used
  // as the source of unshifted bytes when they should come from the
  // carried word.
  uint8_t concat_data[2*packet_word_length];

  memcpy(concat_data, carried_data, packet_word_length);
  memcpy(concat_data+packet_word_length, packet_data_in,
         packet_word_length);

#if DEBUG_RESIZE_EG_TAP
  std::cout << "  Carried in: "
            << std::hex << std::setfill('0');
  for (int i=0; i<packet_word_length; i++) {
    std::cout << " " << std::setw(2) << int(carried_data[i]);
  }
  std::cout << std::dec << std::setfill(' ')
            << "\n";
  std::cout << "  Packet in:  "
            << std::hex << std::setfill('0');
  for (int i=0; i<packet_word_length; i++) {
    std::cout << " " << std::setw(2) << int(packet_data_in[i]);
  }
  std::cout << std::dec << std::setfill(' ')
            << "\n";
#endif

  // Work out which bytes to keep.
  enum byte_class carried_classes[packet_word_length];
  nanotube_classify_entries(
    packet_word_length,
    packet_word_index_bits,
    carried_classes,
    0,
    carried_keep_len);

  for (unsigned i=0; i<packet_word_length; i++) {
    if (carried_classes[i] == BYTE_CLASS_AFTER) {
      // After carried_keep_len, so copy the byte from the input.
      carried_data[i] = packet_data_in[i];
    }
  }

#if DEBUG_RESIZE_EG_TAP
  std::cout << "  Carried out:"
            << std::hex << std::setfill('0');
  for (int i=0; i<packet_word_length; i++) {
    std::cout << " " << std::setw(2) << int(carried_data[i]);
  }
  std::cout << std::dec << std::setfill(' ')
            << "\n";
#endif

  //
  // The shifter sub-stage.
  //

  bool carried_in_any_shifted =
    (!sop && carried_in_shifted_start != carried_in_word_end);

  // Work out the shift amount.
  nanotube_tap_offset_t shift_amount;
  nanotube_tap_offset_t output_shifted_start;
  if (carried_in_any_shifted) {
    // There are shifted bytes in the carried word.  Make sure they
    // are aligned because if any shifted bytes are included then the
    // ones from the carried word will be.  Note that the shifted
    // bytes are always anchored to the end of a word, so we find the
    // difference between the end offsets here.
    shift_amount = output_word_end - carried_in_word_end;
    output_shifted_start = carried_in_shifted_start;

  } else {
    // There are no shifted bytes in the carried word.  Align shifted
    // bytes from the input word if there are any.
    shift_amount = output_word_end_adv1 - input_word_end;
    output_shifted_start = input_shifted_start;
  }

  // Rotate the concatenated word so that it can be muxed into the
  // output word.
  uint8_t shifted_data[packet_word_length];

  nanotube_rotate_down(packet_word_length,       // Output size
                       2*packet_word_length,     // Rotate size
                       2*packet_word_length,     // Input size
                       1+packet_word_index_bits, // Rotate amount bits
                       shifted_data,             // Output
                       concat_data,              // Input
                       shift_amount);            // Rotate amount

#if DEBUG_RESIZE_EG_TAP
  std::cout << "  Shifted:    "
            << std::hex << std::setfill('0');
  for (int i=0; i<packet_word_length; i++) {
    std::cout << " " << std::setw(2) << int(shifted_data[i]);
  }
  std::cout << std::dec << std::setfill(' ')
            << "\n";
#endif

  //
  // The combiner sub-stage.
  //

  // Select the unshifted data.
  uint8_t unshifted_data[packet_word_length];
  if (input_any_unshifted) {
    // The unshifted bytes come from the input packet word.
    memcpy(unshifted_data, packet_data_in, packet_word_length);
  } else {
    // The unshifted bytes come from the incoming carried word which
    // is still stored in concat_data.
    memcpy(unshifted_data, concat_data, packet_word_length);
  }

#if DEBUG_RESIZE_EG_TAP
  std::cout << "  Unshifted:  "
            << std::hex << std::setfill('0');
  for (int i=0; i<packet_word_length; i++) {
    std::cout << " " << std::setw(2) << int(unshifted_data[i]);
  }
  std::cout << std::dec << std::setfill(' ')
            << "\n";
#endif

  // Determine where the inserted bytes should be in the output packet
  // word.
  nanotube_tap_offset_t output_insert_start = output_unshifted_len;
  nanotube_tap_offset_t output_insert_end =
    (output_shifted_start - output_word_start);

  enum byte_class output_classes[packet_word_length];
  nanotube_classify_entries(
    packet_word_length,
    packet_word_index_bits,
    output_classes,
    output_insert_start,
    output_insert_end);

#if DEBUG_RESIZE_EG_TAP
  std::cout << "  Classes:   ";
  for (int i=0; i<packet_word_length; i++) {
    switch (output_classes[i]) {
    case BYTE_CLASS_BEFORE: std::cout << "  B"; break;
    case BYTE_CLASS_IN:     std::cout << "  I"; break;
    case BYTE_CLASS_AFTER:  std::cout << "  A"; break;
    default:                std::cout << "  ?"; break;
    }
  }
  std::cout << "\n";
#endif

  // Multiplex the various sources to form the output word.
  for(unsigned i=0; i<packet_word_length; i++) {
    bool before_insert = (output_classes[i] == BYTE_CLASS_BEFORE);
    bool after_insert = (output_classes[i] == BYTE_CLASS_AFTER);
    word_data_out[i] = ( before_insert
                         ? unshifted_data[i]
                         : ( after_insert
                             ? shifted_data[i]
                             : 0 ) );
#if DEBUG_RESIZE_EG_TAP && 0
    std::cout << "  Combine "
              << std::setw(2) << i << ":"
              << " B=" << int(before_insert)
              << " A=" << int(after_insert)
              << std::setfill('0')
              << " unshifted="
              << std::hex << std::setw(2)
              << int(unshifted_data[i])
              << " shifted="
              << std::hex << std::setw(2)
              << int(shifted_data[i])
              << " output="
              << std::hex << std::setw(2)
              << int(word_data_out[i])
              << std::setfill(' ')
              << "\n";
#endif
  }

#if DEBUG_RESIZE_EG_TAP
  std::cout << "  Packet out: "
            << std::hex << std::setfill('0');
  for (int i=0; i<packet_word_length; i++) {
    std::cout << " " << std::setw(2) << int(word_data_out[i]);
  }
  std::cout << std::dec << std::setfill(' ')
            << "\n";
#endif

  nanotube_tap_offset_t output_len = packet_word_length;
  if (eop) {
    // When eop is set, the output limit is (packet_word_length-1)
    // more than the output packet length.
    output_len = cword->output_limit - (output_word_end - 1);
  }

  *word_length_out = output_len;
  *word_valid_out = do_output;

#if DEBUG_RESIZE_EG_TAP
  std::cout << "  Control:"
            << " carried_keep_len=" << carried_keep_len
            << " shift_amount=" << shift_amount
            << " output_insert_start=" << output_insert_start
            << " output_insert_end=" << output_insert_end
            << "\n"
            << "  Output:"
            << " input_done_out=" << int(*input_done_out)
            << " word_valid_out=" << int(*word_valid_out)
            << " word_eop_out=" << int(*word_eop_out)
            << " word_length_out=" << *word_length_out
            << "\n"
            << "  New state:"
            << " new_word=" << int(state->new_word)
            << " eop_seen=" << int(state->eop_seen)
            << " did_output=" << int(state->did_output)
            << " prev_word_end=" << state->prev_word_end
            << " prev_word_end_adv1=" << state->prev_word_end_adv1
            << " prev_word_end_adv2=" << state->prev_word_end_adv2
            << " carried_unshifted_len=" << state->carried_unshifted_len
            << " carried_shifted_start=" << state->carried_shifted_start
            << " carried_word_end=" << state->carried_word_end
            << "\n";
#endif
}

///////////////////////////////////////////////////////////////////////////
