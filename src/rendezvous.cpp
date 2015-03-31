/*
 * C program to run Rendezvous in Space
 *
 * 0) Connect to shuttle via Bluetooth
 * 1) Check for UP/DOWN button presses
 * 2) Send appropriate bluetooth command
 *            calculate rough speed adjustment
 *            Output PWM value for orbital speed of shuttle
 *
 * 3) If no button presses,
 *            request altitude from shuttle and make exact speed calculation
 *            Output PWM value for orbital speed of shuttle
 *
 * Compile with:
 * Linux (RPi)      gcc -Wno-write-strings -o rendezvous rendezvous.cpp -lbluetooth
 * Windows           "          "               "           "           -lwsock32
 *
 */
int v = 0; // Set verbosity of debugging print statements 0=off

#define LINUX 1     // In theory Linux other than RPi would work, but would need some GPIO (via arduino?)
// #define WINDOWS 1

#ifdef WINDOWS
#include "plbluewindows.h"
#else
#include "plbluelinux.h"
#endif
int g_tries = 10;

int get_socket() {
  int s = -1;
  while (s == -1 && 0 < g_tries--) {
#ifdef WINDOWS
    s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
#else
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
#endif
    if (s == -1) {
      fprintf(stderr, "Trying to create a Bluetooth socket...%d\n", get_error);
      usleep(200000);
    }
  }
  return s;
}

#ifndef WINDOWS
#include <wiringPi.h>
#endif

const char *eot = "end_of_data\r\n";


int bluetoothSocket(char *dest) {

  initialize;
  str2ba(dest, &addr.rc_bdaddr);
  btport(1);
  int s = get_socket();

  while ( s != -1 && connect(s, (struct sockaddr *)&addr, sizeof(addr)) && 0<g_tries--) {
    fprintf(stderr, "Trying to connect to %s error %d\n", dest, get_error);
    close(s);
    usleep(200000);
    s = get_socket();
  }
  return s;
}

/* Write cmd to socket, wait, and then return response */

char *converse(int s, char const *cmd) {
  static char buf[1024];
  int total_bytes = 0;
  int bytes_read;
  if (v>1) fprintf(stderr,"cmd:[%s]\n",cmd);

  memset(buf,0,1024);
  write(s, cmd, strlen(cmd));
  sleep(0.1);  // Give the guy a chance to respond fully
  bytes_read = read(s, buf, sizeof(buf));
  /*
   * fprintf(stderr,"eot[%s:%d]tb[%d]br[%d]buf[%s]\n",eot,strlen(eot),total_bytes,bytes_read,buf);
   */
  while (bytes_read > 0 && total_bytes < sizeof(buf) ) {
    total_bytes += bytes_read;
    if (!strcmp(&buf[total_bytes-strlen(eot)], eot))
      bytes_read = 0;
    else
      bytes_read = read(s, &buf[total_bytes], sizeof(buf)-total_bytes);
  }
  if (v>1) fprintf(stderr,"[[%s]]\n", buf);
  if( total_bytes > 0 ) return buf;
  return (char *)"NAK";  /* indicate communication failure or buffer overflow? */
}

const int pwmPin  = 18; // PWM LED - Broadcom pin 18, P1 pin 12
const int upPin   = 23;   // Active-low button - Broadcom pin 23, P1 pin 16
const int downPin = 24; // Active-low button - Broadcom pin 22, P1 pin 15

const int maxSpeed = 800;
const int minSpeed = 100;

const int max_altitude = 600;
const int min_altitude = 0;
const int velocity_factor = 10;

int velocity, actual_altitude;

void clip_velocity(int v) {
  if (v < minSpeed)       velocity = minSpeed;
  else if (v > maxSpeed)  velocity = maxSpeed;
  else                    velocity = v;
}

int rendezvous(char *addr)
{
  fprintf(stderr, "Entering rendezvous\n");
  int s = bluetoothSocket(addr);
  if (v>1) fprintf(stderr, "connected at %d\n",s);
  wiringPiSetupGpio();          // Broadcom pin numbers
  if (v>2) fprintf(stderr, "wpi setup");

    pinMode(pwmPin, PWM_OUTPUT);  // Set Shuttle speed as PWM output
    pinMode(upPin, INPUT);        // Increase Altitude
    pinMode(downPin, INPUT);      // Decrease Altitude

    pullUpDnControl(upPin, PUD_UP);   // Enable pull-up resistors
    pullUpDnControl(downPin, PUD_UP);
    int cntr = 0;
    while(1) {
      cntr++;
      while (digitalRead(upPin) && !digitalRead(downPin)) {
	converse(s, "v\n");
	usleep(800000);
	clip_velocity(velocity - 3);
	if (v>0) fprintf(stderr, "new velocity %d\n", velocity);
	pwmWrite(pwmPin, velocity);
      }
      while (digitalRead(downPin) && !digitalRead(upPin)) {
	converse(s, "d\n");
	usleep(800000);
	clip_velocity(velocity + 3);
	if (v>0) fprintf(stderr, "new velocity %d\n", velocity);
	pwmWrite(pwmPin, velocity);
      }

      usleep(200000);

      /*
       * Re-adjust shuttle velocity according to actual altitude
       * But only so often
       */
	 if (cntr % 50 == 0) {
	   const char *reply = converse(s, "a\n");
	   if ( sscanf(reply, "%d", &actual_altitude) == 1)  {
	     if (v>1) fprintf(stderr, "Got altitude [%d]\n", actual_altitude);
	   } else
	     fprintf(stderr, "Failed to get altitude from [%s]\n", reply);

	   if (v>2) fprintf(stderr, "Shuttle says altitude is %d\n", actual_altitude);

	   if (v>0) fprintf(stderr, "Current (rough) velocity is %d\n", velocity);
	   velocity = minSpeed + ((max_altitude - actual_altitude)/velocity_factor);
	   if (v>0) fprintf(stderr, "Computed velocity is %d\n", velocity);

	   if (velocity < minSpeed) velocity = minSpeed;
	   if (velocity > maxSpeed) velocity = maxSpeed;
	   if (v>0) fprintf(stderr, "Adjusted for range as %d\n", velocity);
	   pwmWrite(pwmPin, velocity);

	 } /* Once in a while */
 }
    return 0;
}

void usage() {
    fprintf(stderr, "usage:  bluetest <BLUETOOTH-MACADDRESS> [-v N]\nwhere N = debug verbosity\n");
    exit(0);
}

int main(int argc, char **argv)
{
  if (argc > 3) {
    if (!strcmp(argv[2],"-v")) v = atoi(argv[3]);
    else usage();
  }
  if (argc < 2 || strlen(argv[1]) != 17) usage();

  setbuf(stderr,NULL);
  rendezvous(argv[1]);
  fprintf(stderr, "returned from rendezvous. WTF?\n");
  return 0;
}

