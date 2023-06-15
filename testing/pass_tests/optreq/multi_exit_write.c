/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum nanotube_bus_id_t {
  NANOTUBE_BUS_ID_SB = 0,
  NANOTUBE_BUS_ID_SHB = 1,
  NANOTUBE_BUS_ID_ETH = 2,
  NANOTUBE_BUS_ID_X3RX = 3,
  NANOTUBE_BUS_ID_X3TX = 4,
  NANOTUBE_BUS_ID_TOTAL
};
typedef enum {
  NANOTUBE_PACKET_PASS = 0x0,
  NANOTUBE_PACKET_DROP = 0x1,
} nanotube_kernel_rc_t;

typedef struct nanotube_context nanotube_context_t;
typedef struct nanotube_packet  nanotube_packet_t;
typedef nanotube_kernel_rc_t nanotube_kernel_t(nanotube_context_t *context,
                                               nanotube_packet_t *packet);

size_t nanotube_packet_read(nanotube_packet_t* packet, uint8_t* data_out,
                            size_t offset, size_t length) __attribute__((weak));

size_t nanotube_packet_write_masked(nanotube_packet_t* packet,
                                    const uint8_t* data_in,
                                    const uint8_t* mask,
                                    size_t offset, size_t length) __attribute__((weak));

nanotube_kernel_rc_t process_packet(nanotube_context_t *nt_ctx,
                                    nanotube_packet_t *packet) __attribute__((weak));

void nanotube_add_plain_packet_kernel(const char* name,
                                      nanotube_kernel_t* kernel,
                                      int bus_type,
                                      int capsules)
{
  kernel(NULL, NULL);
}

static int write_count = 0;

size_t nanotube_packet_read(nanotube_packet_t* packet, uint8_t* data_out,
                            size_t offset, size_t length)
{
  size_t i;
  printf("read(%d, %d)\n", (int)offset, (int)length);
  for(i=0; i<length; i++) {
    data_out[i] = ((offset>>8)^offset)+write_count;
    offset++;
  }
  return 0;
}

size_t nanotube_packet_write_masked(nanotube_packet_t* packet,
                                    const uint8_t* data_in,
                                    const uint8_t* mask,
                                    size_t offset, size_t length)
{
  size_t i;
  write_count++;
  printf("write(%d, %d)", (int)offset, (int)length);
  for(i=0; i<length; i++) {
    printf("%s %02x", (i%8 == 0)?"\n ":"", data_in[i]);
  }
  printf("\nMask:");
  for(i=0; i<(length+7)/8; i++) {
    printf("%s %02x", (i%8 == 0)?"\n ":"", mask[i]);
  }
  printf("\n");
  return length;
}

nanotube_kernel_rc_t process_packet(nanotube_context_t *nt_ctx,
                                    nanotube_packet_t *packet)
{
  uint8_t buf1[8];
  uint8_t mask[1] = {0xff};
  int i;
  nanotube_packet_read(packet, buf1, 0, 8);
  if (buf1[0] == 0) {
    nanotube_packet_write_masked(packet, buf1, mask, 4, 8);
    return NANOTUBE_PACKET_PASS;
  } else {
    nanotube_packet_write_masked(packet, buf1, mask, 0, 8);
    return NANOTUBE_PACKET_DROP;
  }
}

void nanotube_setup()
{
  nanotube_add_plain_packet_kernel("kernel", process_packet,
                                   NANOTUBE_BUS_ID_SB, 1);
}

int main()
{
  nanotube_setup();
  return 0;
}
