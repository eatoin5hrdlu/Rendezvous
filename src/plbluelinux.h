#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdint.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

static struct sockaddr_rc addr = {
  AF_BLUETOOTH,                                // .rc_family
  (bdaddr_t){{0xff,0xff,0xff,0xff,0xff,0xff}}, // .rc_bdaddr
  (uint8_t) 1                                  // .rc_channel
  };

#define initialize (void)0
#define btport(n)  (void)0
#define get_error  0


