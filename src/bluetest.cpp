/*
 * Compile with:
 * Linux      gcc -Wno-write-strings -o bluetest bluetest.cpp -lbluetooth
 * Windows     "          "               "           "       -lwsock32
 */
#define __USE_W32_SOCKETS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WINDOWS 1

#ifdef WINDOWS
#include "bluewin.h"
#else
#include <sys/socket.h>
#include <wiringPi.h>
#include <stdint.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#endif

int bluetoothSocket(char *dest) {

#ifdef WINDOWS
  struct sockaddr_rc addr = {
    32,
    (bdaddr_t) 0xffffffff,
    0x1101,
    1
  };
  startWinSock();
  AddrStringToBtAddr(dest, &addr.rc_bdaddr);
  addr.port = 1;
#else
  struct sockaddr_rc addr = {
    AF_BLUETOOTH,
    (bdaddr_t){{0xff,0xff,0xff,0xff,0xff,0xff}},
    (uint8_t) 1
  };
  //    .rc_family  = AF_BLUETOOTH,
  //    .rc_bdaddr  = (bdaddr_t){{0xff,0xff,0xff,0xff,0xff,0xff}},
  //    .rc_channel = (uint8_t) 1
#endif

  int s = -1;
  while (s == -1) {
    s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    if (s == -1) {
      fprintf(stderr, "Trying to create a Bluetooth socket...%d\n", WSAGetLastError());
      Sleep(3000);
    }
  }

  while ( connect(s, (struct sockaddr *)&addr, sizeof(addr)) ) {
    fprintf(stderr, "Trying to connect to %s WSA error %d\n", dest, WSAGetLastError());
    Sleep(4000);
  }
  return s;
}

/* Write cmd to socket, wait, and then return response */

char *converse(int s, char const *cmd) {
  static char buf[1024];
  int bytes_read;
  memset(buf,0,1024);

  write(s, cmd, strlen(cmd));

  Sleep(4000);  // Give the guy a chance to respond fully

  bytes_read = read(s, buf, sizeof(buf));
  fprintf(stderr,"%s\n", buf);
  if( bytes_read > 0 ) return buf;

  return (char *)"NAK";  /* indicate communication failure */
}

const int pwmPin = 18; // PWM LED - Broadcom pin 18, P1 pin 12
const int upPin = 23;   // Active-low button - Broadcom pin 23, P1 pin 16
const int downPin = 22; // Active-low button - Broadcom pin 22, P1 pin 15

const int maxSpeed = 500;
const int minSpeed = 100;

const int max_altitude = 600;
const int min_altitude = 0;
const int velocity_factor = 10;

int velocity, actual_altitude;

int main(int argc, char **argv)
{
    int s = bluetoothSocket("98:D3:31:30:2A:D1");

#ifdef RPi
    wiringPiSetupGpio();          // Broadcom pin numbers

    pinMode(pwmPin, PWM_OUTPUT);  // Set Shuttle speed as PWM output

    pinMode(upPin, INPUT);        // Increase Altitude
    pinMode(downPin, INPUT);      // Decrease Altitude

    pullUpDnControl(upPin, PUD_UP);   // Enable pull-up resistors
    pullUpDnControl(downPin, PUD_UP);
#else
#endif

  while(1) {
      while (digitalRead(upPin) && !digitalRead(downPin)) {
	converse(s, "v\n");
        Sleep(1000);
      }
      while (digitalRead(downPin) && !digitalRead(upPin)) {
	converse(s, "d\n");
        Sleep(1000);
      }

      /* Re-adjust shuttle velocity according to actual altitude */

      actual_altitude = atoi(converse(s, "a\n"));
      fprintf(stderr, "Shuttle says altitude is %d\n", actual_altitude);

      velocity = minSpeed + ((max_altitude - actual_altitude)/velocity_factor);
      fprintf(stderr, "Computed velocity is %d\n", velocity);

      if (velocity < minSpeed) velocity = minSpeed;
      if (velocity > maxSpeed) velocity = maxSpeed;
      fprintf(stderr, "Adjusted for range as %d\n", velocity);

      pwmWrite(pwmPin, velocity);
 }
    return 0;
}

