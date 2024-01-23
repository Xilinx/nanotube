#include "nanotube_api.h"
#include "test.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

static const std::size_t PAD_SIZE = 8;

struct test_info {
  std::size_t offset;
  std::size_t length;
  std::vector<uint8_t> mask;
};

static const std::vector<test_info> test_infos = {
  { 0, 8, { 0xff, 0x00 } },
  { 8, 8, { 0x00, 0xff } },
  { 4, 8, { 0xf0, 0x0f } },
  { 2, 4, { 0x3c, 0x00 } },
  { 4, 8, { 0xf0, } },
  { 8, 2, { 0x00, } },
};

static void test_masks()
{
  std::vector<uint8_t> buffer;

  /* Iterate over the test infos. */
  for( std::size_t i = 0; i < test_infos.size(); i++ ) {
    auto *info = &(test_infos[i]);

    if( test_verbose >= 1 ) {
      std::cout << "Test case " << i
                << ": offset=" << info->offset
                << ": length=" << info->length
                << "\n";
    }

    /* Initialize the buffer. */
    uint8_t sentinel = (i+2)*0x11;
    std::size_t mask_size = info->mask.size();
    std::size_t buffer_size = 2*PAD_SIZE + mask_size;
    buffer.resize(buffer_size);
    uint8_t *buf_ptr = &(buffer.front());
    memset(buf_ptr, sentinel, buffer_size);

    /* Generate the mask. */
    nanotube_init_mask(buf_ptr+PAD_SIZE, mask_size, info->offset,
                       info->length);

    /* Check that the pad regions have not been modified. */
    for( std::size_t i = 0; i < PAD_SIZE; i++ ) {
      assert_eq(buf_ptr[i], sentinel);
      assert_eq(buf_ptr[i+PAD_SIZE+mask_size], sentinel);
    }

    /* Check the mask. */
    const uint8_t *output_mask = buf_ptr+PAD_SIZE;
    const uint8_t *expect_mask = &(info->mask.front());
    if( test_verbose >= 2 )
      test_dump_buffer("output_mask", output_mask, mask_size);
    assert_array_eq(output_mask, expect_mask, mask_size);
  }
}

int main(int argc, char *argv[]) {
  test_init(argc, argv);
  test_masks();
  return test_fini();
}
/* vim: set ts=8 et sw=2 sts=2 tw=75: */
