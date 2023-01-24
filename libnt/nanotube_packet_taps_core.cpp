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
#define DEBUG_RESIZE_TAP  0

#if ( DEBUG_ROTATE_DOWN || \
      DEBUG_CLASSIFY || \
      DEBUG_DUPLICATE || \
      DEBUG_PACKET_READ || \
      DEBUG_PACKET_WRITE || \
      DEBUG_RESIZE_TAP )
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
#if __clang__
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

#if __clang__
#pragma clang loop unroll(full)
#endif
  for (index = 0; index < rot_buffer_length; index++) {
    temp[0][index] = ( index < buffer_in_length ? buffer_in[index] : 0 );
  }

  /* Perform each stage of the rotation, each corresponding with a
   * single bit of the rotation amount and starting with the MSB. */
#if __clang__
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

#if __clang__
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
#if __clang__
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
#if __clang__
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
#if __clang__
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
#if __clang__
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
#if __clang__
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
#if __clang__
  __attribute__((always_inline))
#endif
{
#if __clang__
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
#if __clang__
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

#if __clang__
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
#if __clang__
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
#if __clang__
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
#if __clang__
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
#if __clang__
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
#if __clang__
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
#if __clang__
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
#if __clang__
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

// The packet resize tap is used to insert bytes into and remove bytes
// from a packet.  The inserted bytes are zeros and the removed bytes
// are discarded.  To insert non-zero bytes, use a write tap after the
// resize tap.  To examine removed bytes, use a read tap before the
// resize tap.
//
// The tap is split into two stages for performance reasons.  The
// ingress stage produces a control word for each input packet word.
// The egress stage uses the control word to produce the output packet
// words.  The egress stage may produce more than one output packet
// word per input packet word when it is inserting bytes.  It may
// consume an input packet word without producing an output packet
// word when deleting bytes.
//
// The egress stage needs to work out whether it will need to produce
// another output packet word for the current input packet word before
// moving on to the next input packet word.  It needs to do this in a
// single cycle in order to get good performance.  The calculation is
// somewhat involved, so the ingress stage performs most of the
// calculation so that the egress stage can complete it in a single
// cycle.
//
// Only the egress stage processes packet data.  The ingress stage
// just processes lengths and offsets.  The egress stage needs a
// carried-data buffer to hold input bytes which will need to be
// output, but not in the current output packet word.  The ingress
// stage works out which bytes get output and which bytes get written
// to the carried-data buffer.
//
// The output packet consists of three regions:
//   Unshifed bytes from before the deleted bytes.
//   Inserted zero bytes.
//   Shifted bytes from after the deleted bytes.
//
// The first region is called unshifted because these bytes have the
// same offset in the output packet as in the input packet so they can
// be passed through the tap without being shifted into a different
// byte lane.
//
// The third region is called shifted because these bytes typically
// have a different offset in the output packet than in the input
// packet.  This means they need to be shifted into different byte
// lanes as the pass through the tap.
//
// The output packet word and carried-data are each made up of (in
// this order):
//   Either:
//     Unshifted input bytes from before the deleted bytes
//     Inserted bytes from the request
//   or:
//     Carried bytes from the previous invocation.
//   Shifted input bytes from after the deleted bytes
//
// The bytes carried over to the next invocation are made up of:
//   Unshifted input bytes from before the deleted bytes
//   Inserted bytes from the request
//   Shifted input bytes from after the deleted bytes
//
// The following cases are not exhaustive but they show how it
// works.  Each diagram shows the structure of the outputs words and
// carried words.  The codes in the diagrams are:
//
//   None: No output.
//   U<n>: Bytes are unshifted from input word <n>.
//   Zero: Inserted zero bytes.
//   S<n>: Bytes are shifted from input word <n>.
//   DC:   Don't care.  Bytes will be discarded.
//
// Case 1: Insert into the middle of a word.
//
//    0           Output           31     0         Carried            31
//   +-------------------------------+   +-------------------------------+
// 0 |              U0               |   |             DC                |
//   +---------+------------+--------+   +------------+------------------+
// 1 |    U1   |    Zero    |   S1   |   |     S1     |        DC        |
//   +---------+--+---------+--------+   +------------+------------------+
// 2 |     S1     |        S2        |   |     S2     |        DC        |
//   +------------+------------------+   +------------+------------------+
//
// Case 2: Insert spans a word boundary.
//
//    0           Output           31     0         Carried            31
//   +----------------------+--------+   +------+--------+---------------+
// 0 |         U0           |  Zero  |   | Zero |   S0   |      DC       |
//   +------+--------+------+--------+   +------+--------+---------------+
// 1 | Zero |   S0   |      S1       |   |      S1       |      DC       |
//   +------+--------+---------------+   +---------------+---------------+
// 2 |      S1       |      S2       |   |      S2       |      DC       |
//   +---------------+---------------+   +---------------+---------------+
//
// Case 3: Delete bytes from the middle of a word.
//
//    0           Output           31     0         Carried            31
//   +-------------------------------+   +------+---------------+--------+
// 0 |             None              |   |  U0  |       S0      |   DC   |
//   +------+---------------+--------+   +------+---------------+--------+
// 1 |  U0  |       S0      |   S1   |   |          S1          |   DC   |
//   +------+---------------+--------+   +----------------------+--------+
// 2 |          S1          |   S2   |   |          S2          |   DC   |
//   +----------------------+--------+   +----------------------+--------+
//
// Case 4: Delete bytes across a word boundary.
//
//    0           Output           31     0         Carried            31
//   +-------------------------------+   +--------------------+----------+
// 0 |             None              |   |         U0         |    DC    |
//   +--------------------+----------+   +--------+-----------+----------+
//   |         U0         |    S1    |   |   S1   |          DC          |
//   +--------+-----------+----------+   +--------+----------------------+
//   |   S1   |          S2          |   |   S2   |          DC          |
//   +--------+----------------------+   +--------+----------------------+

void nanotube_tap_packet_resize_ingress_state_init(
  /* Outputs. */
  nanotube_tap_packet_resize_ingress_state_t *state)
#if __clang__
  __attribute__((always_inline))
#endif
{
  state->new_req = true;
  state->packet_length = 0;
  state->packet_offset = 0;
  state->packet_rot_amount = 0;
  state->unshifted_length = 0;
  state->edit_started = false;
  state->edit_delete_length = 0;
  state->edit_insert_length = 0;
  state->edit_carried_len = 0;
  state->shifted_started = true;
  state->shifted_carried_len = 0;
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
  nanotube_tap_packet_resize_req_t *resize_req_in,
  nanotube_tap_offset_t word_length_in,
  bool word_eop_in)
#if __clang__
  __attribute__((always_inline))
#endif
{
  // The sequential algorithm being modelled has the following stages:
  //   1. Copy unshifted bytes before the edit.
  //   2. Insert bytes.
  //   3. Delete bytes.
  //   4. Copy shifted bytes after the edit.
  //
  // Steps 2 and 3 are merged since they can be performed in parallel.
  // The result is a three stage pipeline:
  //   1. Copy unshifted bytes (unshifted).
  //   2. Insert and delete bytes (edit).
  //   3. Copy shifted bytes (shifted).
  //
  // Each stage of the ingress pipeline needs to keep track of a
  // subset of the following variables.
  //   first_word - This is the first word being processed
  //   last_word - This is the last word being processed
  //   started - This stage has started processing
  //   length - The number of bytes remaining (2 lengths for edit)
  //   space - The number of bytes available in the current word
  //   carried_len - The number of bytes carried to the next stage
  //   input_start_offset - The first input byte processed
  //   input_end_offset - After the last input byte processed
  //
  // The following general state variables are kept:
  //   new_req - The previous request has been completed.
  //   packet_rot_amount - The amount to rotate the packet data.
  //   resize_req - The unmodified resize request.

  nanotube_tap_packet_resize_req_t resize_req = *resize_req_in;
  nanotube_tap_offset_t next_offset;

  bool sop = state->new_req;
  if (state->new_req) {
#if DEBUG_RESIZE_TAP
    std::cout << "New request:"
              << " write_offset=" << unsigned(resize_req.write_offset)
              << " insert_length=" << unsigned(resize_req.insert_length)
              << " delete_length=" << unsigned(resize_req.delete_length)
              << "\n";
#endif
  }

  //
  // Process the incoming packet word.
  //

  state->new_req = (word_eop_in != 0);
  *packet_done_out = (word_eop_in != 0);

#if DEBUG_RESIZE_TAP
  std::cout << "Received word:"
            << " SOP=" << unsigned(sop)
            << " EOP=" << unsigned(word_eop_in)
            << " word_length_in=" << unsigned(word_length_in)
            << "\n";
#endif

  // Calculate the input packet offset at the end of this word.
  next_offset = state->packet_offset + word_length_in;
  state->packet_offset = (word_eop_in ? 0 : next_offset);

  //
  // Keep track of the output word.
  //

  nanotube_tap_offset_t output_insert_start = 0;
  nanotube_tap_offset_t output_insert_end = 0;
  bool output_done = false;

  nanotube_tap_offset_t accum_length = 0;
  nanotube_tap_offset_t accum_insert_start = 0;
  nanotube_tap_offset_t accum_insert_end = 0;


  //
  // Process the "unshifted" stage.
  //

  if (sop)
    state->unshifted_length = resize_req.write_offset;

  nanotube_tap_offset_t unshifted_space = word_length_in;

  bool unshifted_last_word = ( state->unshifted_length <= unshifted_space );

  nanotube_tap_offset_t unshifted_input_end_offset;
  if (!unshifted_last_word)
    unshifted_input_end_offset = word_length_in;
  else
    unshifted_input_end_offset = state->unshifted_length;
  assert(unshifted_input_end_offset <= word_length_in);

  // Add unshifted bytes to the output or the accumulator.
  if (unshifted_last_word) {
    accum_length = state->unshifted_length;
    accum_insert_start = state->unshifted_length;
    accum_insert_end = state->unshifted_length;
  } else if (word_eop_in) {
    accum_length = word_length_in;
    accum_insert_start = word_length_in;
    accum_insert_end = word_length_in;
  } else {
    output_insert_start = packet_word_length;
    output_insert_end = packet_word_length;
    output_done = true;
  }

  state->unshifted_length = state->unshifted_length - packet_word_length;

#if DEBUG_RESIZE_TAP
  std::cout << "unshifted: "
            << "out[" << output_insert_start
            << ".." << output_insert_end
            << "] "
            << (output_done ? "enabled" : "disabled")
            << " acc[" << accum_insert_start
            << ".." << accum_insert_end
            << "] len=" << accum_length
            << "\n";
#endif


  //
  // Process the "edit" stage.
  //

  if (sop) {
    state->edit_delete_length = resize_req.delete_length;
    state->edit_insert_length = resize_req.insert_length;
  }

  bool edit_prev_started = sop ? false : state->edit_started;

  if (unshifted_last_word)
    state->edit_started = true;
  else
    state->edit_started = edit_prev_started;

  bool edit_first_word = (state->edit_started && !edit_prev_started);

  nanotube_tap_offset_t edit_input_start_offset;
  if (edit_prev_started)
    edit_input_start_offset = 0;
  else
    edit_input_start_offset = unshifted_input_end_offset;

#ifndef __clang__
  assert(edit_input_start_offset <= word_length_in);
#endif

  nanotube_tap_offset_t edit_space = ( word_length_in - edit_input_start_offset );

  bool edit_last_word = ( state->edit_started &&
                          state->edit_delete_length <= edit_space );

  nanotube_tap_offset_t edit_input_end_offset;
  if (!edit_last_word)
    edit_input_end_offset = word_length_in;
  else
    edit_input_end_offset = ( edit_input_start_offset +
                              state->edit_delete_length );

#ifndef __clang__
  assert(edit_input_end_offset <= word_length_in);
#endif

  // Add bytes to the output buffer or accumulator.
  if (edit_first_word) {
    nanotube_tap_offset_t insert_space = (packet_word_length - accum_length);
    if (state->edit_insert_length > insert_space) {
      // If the output word is full then push it out.
      output_insert_start = edit_input_start_offset;
      output_insert_end = packet_word_length;
      output_done = true;
      accum_length = ( state->edit_insert_length - insert_space );
      accum_insert_start = 0;
      accum_insert_end = accum_length;
    } else {
      // Just append the inserted bytes to the accumulator.
      accum_length += state->edit_insert_length;
      accum_insert_end = accum_length;
    }
    state->edit_carried_len = accum_length;

  } else if (edit_prev_started) {
    output_done = false;
    accum_length = state->edit_carried_len;
    accum_insert_start = state->edit_carried_len;
    accum_insert_end = state->edit_carried_len;
  } else {
    state->edit_carried_len = 0;
  }

  if (state->edit_started)
    state->edit_delete_length -= (edit_input_end_offset - edit_input_start_offset);

#if DEBUG_RESIZE_TAP
  std::cout << "edit: "
            << "out[" << output_insert_start
            << ".." << output_insert_end
            << "] "
            << (output_done ? "enabled" : "disabled")
            << " acc[" << accum_insert_start
            << ".." << accum_insert_end
            << "] len=" << accum_length
            << ( edit_prev_started ? " continues" :
                 ( state->edit_started ? " starting" :
                   "stopped" ) )
            << "\n";
#endif


  //
  // Process the "shifted" stage.
  //

  bool shifted_prev_started = sop ? false : state->shifted_started;

  if (edit_last_word)
    state->shifted_started = true;
  else
    state->shifted_started = shifted_prev_started;

  bool shifted_first_word = (state->shifted_started && !shifted_prev_started);

  nanotube_tap_offset_t shifted_input_start_offset;
  if (shifted_prev_started)
    shifted_input_start_offset = 0;
  else
    shifted_input_start_offset = edit_input_end_offset;

#ifndef __clang__
  assert(shifted_input_start_offset <= word_length_in);
#endif

  if (shifted_first_word) {
    nanotube_tap_offset_t shifted_length = ( word_length_in -
                                             shifted_input_start_offset );
    nanotube_tap_offset_t shifted_space = (packet_word_length - accum_length);

    state->packet_rot_amount =
      ( shifted_input_start_offset > accum_length
        ? nanotube_tap_offset_t(shifted_input_start_offset - accum_length)
        : nanotube_tap_offset_t(shifted_input_start_offset + shifted_space) );

    if (shifted_length > shifted_space) {
      // If the output word is full then push it out.
      output_insert_start = accum_insert_start;
      output_insert_end = accum_insert_end;
      output_done = true;
      accum_length = shifted_length - shifted_space;
      accum_insert_start = 0;
      accum_insert_end = 0;
    } else {
      // Add the shifted bytes to the accumulator.
      accum_length += shifted_length;
    }
    state->shifted_carried_len = accum_length;

  } else if (shifted_prev_started) {
    // If this is not the first cycle then output the carried bytes
    // from the previous word followed by shifted bytes from this
    // word.  Carry over the remaining bytes to the next word.  The
    // same number of bytes are carried over each cycle because the
    // input word size is equal to the output word size.  Compute
    // the actual split using word_length_in in order to handle the
    // end of packet condition.
    nanotube_tap_offset_t space = ( packet_word_length - state->shifted_carried_len );
    if ( word_length_in <= space ) {
      output_done = false;
      accum_length = word_length_in + state->shifted_carried_len;
      accum_insert_start = state->shifted_carried_len;
      accum_insert_end = state->shifted_carried_len;
    } else {
      output_insert_start = state->shifted_carried_len;
      output_insert_end = state->shifted_carried_len;
      output_done = true;
      accum_length = word_length_in - space;
      accum_insert_start = 0;
      accum_insert_end = 0;
    }
  } else {
    state->packet_rot_amount = 0;
    state->shifted_carried_len = 0;
  }

#if DEBUG_RESIZE_TAP
  std::cout << "shifted: "
            << "out[" << output_insert_start
            << ".." << output_insert_end
            << "] "
            << (output_done ? "enabled" : "disabled")
            << " acc[" << accum_insert_start
            << ".." << accum_insert_end
            << "] len=" << accum_length
            << ( shifted_prev_started ? " continues" :
                 ( state->shifted_started ? " starting" :
                   "stopped" ) )
            << "\n";
#endif


  //
  // Work out the structure of the output and carried words.
  //

  bool accum_valid = (accum_length != 0);
  bool accum_push = (accum_valid && word_eop_in!=0);


  //
  // Send the control word.
  //

  nanotube_tap_packet_resize_cword_t cword;

  cword.packet_rot = state->packet_rot_amount;
  cword.output_insert_start = ( output_done
                                ? output_insert_start
                                : accum_insert_start );
  cword.output_insert_end = ( output_done
                              ? output_insert_end
                              : accum_insert_end );
  cword.carried_insert_start = accum_insert_start;
  cword.carried_insert_end = accum_insert_end;
  cword.select_carried = edit_prev_started ? 1 : 0;
  cword.push_1 = ( output_done || accum_push );
  cword.push_2 = ( output_done && accum_push );
  cword.eop = word_eop_in;
  cword.word_length = ( word_eop_in != 0
                        ? ( accum_valid
                            ? nanotube_tap_offset_t(accum_length)
                            : word_length_in )
                        : packet_word_length );

#if DEBUG_RESIZE_TAP
  std::cout << "Classification:"
            << " edit: " << unsigned(edit_prev_started)
            << "->" << unsigned(state->edit_started)
            << " shifted: " << unsigned(shifted_prev_started)
            << "->" << unsigned(state->shifted_started)
            << "\n";

  std::cout << "Control word:\n"
            << " packet_rot=" << cword.packet_rot
            << " select_carried=" << unsigned(cword.select_carried)
            << "\n";

  std::cout << "  output_insert_start="
            << unsigned(cword.output_insert_start)
            << " output_insert_end="
            << unsigned(cword.output_insert_end)
            << "\n";

  std::cout << "  carried_insert_start="
            << unsigned(cword.carried_insert_start)
            << " carried_insert_end="
            << unsigned(cword.carried_insert_end)
            << "\n";

  std::cout << "  push_1=" << cword.push_1
            << " push_2=" << cword.push_2
            << " eop=" << unsigned(cword.eop)
            << " word_length=" << cword.word_length
            << "\n";
#endif

  *cword_out = cword;
}

void nanotube_tap_packet_resize_egress_state_init(
  /* Outputs. */
  nanotube_tap_packet_resize_egress_state_t *state)
#if __clang__
  __attribute__((always_inline))
#endif
{
  state->new_req = true;
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
#if __clang__
  __attribute__((always_inline))
#endif
{
  bool new_req = state->new_req;
  nanotube_tap_offset_t output_insert_start =
    (new_req ? cword->output_insert_start : packet_word_length);
  nanotube_tap_offset_t output_insert_end =
    (new_req ? cword->output_insert_end : packet_word_length);
  bool select_carried = (new_req ? cword->select_carried : 1);
  bool push_2 = (new_req ? cword->push_2 : 0);

  state->new_req = (push_2 == 0);
  *input_done_out = (push_2 == 0);

  // Rotate the input word so that it can be muxed into the output
  // word or the word carried over to the next invocation.
  uint8_t rot_input_data[packet_word_length];

  nanotube_rotate_down(packet_word_length, packet_word_length,
                       packet_word_length, packet_word_index_bits,
                       rot_input_data, packet_data_in,
                       cword->packet_rot);

#if DEBUG_RESIZE_TAP
  std::cout << "cword: eop=" << unsigned(cword->eop)
            << " word_length=" << unsigned(cword->word_length)
            << " push_1=" << unsigned(cword->push_1)
            << " push_2=" << unsigned(push_2)
            << "\n";
#endif

  *word_eop_out = push_2 ? bool(0) : cword->eop;
  *word_length_out = push_2 ? packet_word_length : cword->word_length;

  uint8_t selected_data[packet_word_length];
  memcpy(selected_data, packet_data_in, packet_word_length);
  if (select_carried != 0)
    memcpy(selected_data, carried_data, packet_word_length);

  enum byte_class output_classes[packet_word_length];
  nanotube_classify_entries(
    packet_word_length,
    packet_word_index_bits,
    output_classes,
    output_insert_start,
    output_insert_end);

  // Multiplex the various sources to form the output word.
#if __clang__
#pragma clang loop unroll(full)
#endif
  for(unsigned i=0; i<packet_word_length; i++) {
    bool before_insert = output_classes[i] == BYTE_CLASS_BEFORE;
    bool after_insert = output_classes[i] == BYTE_CLASS_AFTER;
    word_data_out[i] = ( before_insert
                         ? selected_data[i]
                         : ( after_insert
                             ? rot_input_data[i]
                             : 0 ) );
  }

  enum byte_class carried_classes[packet_word_length];
  nanotube_classify_entries(
    packet_word_length,
    packet_word_index_bits,
    carried_classes,
    cword->carried_insert_start,
    cword->carried_insert_end);

  // Multiplex the various sources to form the carried word.
#if __clang__
#pragma clang loop unroll(full)
#endif
  for(unsigned i=0; i<packet_word_length; i++) {
    bool before_insert = carried_classes[i] == BYTE_CLASS_BEFORE;
    bool after_insert = carried_classes[i] == BYTE_CLASS_AFTER;
    carried_data[i] = ( before_insert
                        ? selected_data[i]
                        : ( after_insert
                            ? rot_input_data[i]
                            : 0 ) );
  }

  if (cword->push_1) {
#if DEBUG_RESIZE_TAP
    std::cout << "Output word: eop=" << unsigned(*word_eop_out)
              << " length=" << unsigned(*word_length_out)
              << "\n";
#endif
  }
  *word_valid_out = cword->push_1;
}

///////////////////////////////////////////////////////////////////////////
