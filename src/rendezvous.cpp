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
#include <errno.h>
#endif
/*
 * Global variables for error conditions, how many times to retry connections,
 * maximum wait for read, etc.
 */

struct timeval g_timeout;
bool           g_reset = true;

void readTimeout(int sockfd,int secs) {
  int optval = 1;
  g_timeout.tv_sec = secs;
  g_timeout.tv_usec = 0;

//  Socket read timeout
  if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&g_timeout,
		  sizeof(g_timeout)) < 0) {
    fprintf(stderr,"failed to set recv timeout on socket(%d)\n",sockfd);
    fprintf(stderr,"Note: Shuttle must hit lower limit switch before communicating\n");
    fprintf(stderr,"Note: You can test this by clicking the limit switch by hand\n");
    fprintf(stderr,"Note: But then the ALTITUDE (and corresponding speed) will be incorrect\n");
  }

  if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0) {
    fprintf(stderr,"failed to set REUSEADDR on socket(%d)\n",sockfd);
  }

//  Socket connection timeout (not at this time)
//  if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&g_timeout,
//                sizeof(g_timeout)) < 0)
//        error("setsockopt failed\n");
}

int get_socket() {
  int s = -1;
  int g_tries = 10;
  while (s == -1 && 0 < g_tries--) {
#ifdef WINDOWS
    s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
#else
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
#endif
    if (s == -1) {
      fprintf(stderr, "Trying to create a Bluetooth socket...%d\n", get_error);
      usleep(1000000);
    }
  }
  readTimeout(s,10);
  return s;
}

#ifndef WINDOWS
#include <wiringPi.h>
#endif

const char *eot = "end_of_data\r\n";


int bluetoothSocket(char *dest) {
  initialize;   // this does something in Windows
  str2ba(dest, &addr.rc_bdaddr);
  btport(1);
  int s = get_socket();
  int g_tries = 10;
  while ( s != -1 && connect(s, (struct sockaddr *)&addr, sizeof(addr)) && 0 < g_tries) {
    g_tries--;
    fprintf(stderr, "Trying to connect to %s error %d\n", dest, get_error);
    //    close(s);
    usleep(1000000);
    // s = get_socket();
  }
  if (0 == g_tries) {
    fprintf(stderr, "Completely failed to get socket/connection going (s=%d)\n",s);
    if (s != -1) close(s);
    sleep(10);
    g_reset = true; /* Re-try initialization */
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
  usleep(100000);  // Give the guy a chance to respond fully
  bytes_read = read(s, buf, sizeof(buf));
  if (errno == EAGAIN || errno == EWOULDBLOCK && bytes_read == 0) {
    close(s);
    g_reset = true;
    return (char *)"NAK1";
  }

/*
 * fprintf(stderr,"eot[%s:%d]tb[%d]br[%d]buf[%s]\n",
 *               eot,strlen(eot),total_bytes,bytes_read,buf);
 */

  while (bytes_read > 0 && total_bytes < sizeof(buf) ) {
    total_bytes += bytes_read;
    if (!strcmp(&buf[total_bytes-strlen(eot)], eot))
      bytes_read = 0;
    else {
      bytes_read = read(s, &buf[total_bytes], sizeof(buf)-total_bytes);
      if (errno == EAGAIN || errno == EWOULDBLOCK && bytes_read == 0) {
	close(s);
	g_reset = true;
	return (char *)"NAK2";
      }
    }
  }

  if (v>1) fprintf(stderr,"[[%s]]\n", buf);
  if( total_bytes > 0 ) return buf;

  close(s);
  g_reset = true;
  return (char *)"NAK3"; /* communication failure/timeout/buffer overflow */
}


#define Volts2Pwm(v)  ((int)((1023.0*v)/2.8))
const int pwmPin  = 18; // PWM LED - Broadcom pin 18, P1 pin 12
const int upPin   = 23;   // Active-low button - Broadcom pin 23, P1 pin 16
const int downPin = 24; // Active-low button - Broadcom pin 22, P1 pin 15

const float min_speed_volts = 1.1;
const float max_speed_volts = 2.8;
const float speed_volt_range = (max_speed_volts-min_speed_volts);
const int min_speed_pwm   = Volts2Pwm(min_speed_volts);
const int max_speed_pwm   = Volts2Pwm(max_speed_volts);

const int max_altitude = 135;
const int min_altitude = 1;
const float altitude_range =  (((float)max_altitude)-((float)min_altitude));
const int velocity_factor = 10;

int velocity, actual_altitude, rough_altitude;

// Clipped PWM velocity from altitude
int altitude_velocity(int alt) {
  int pwm;
  float alt_fraction = (  ((float)max_altitude - (float) alt) / ((float)altitude_range));
  float vlts = min_speed_volts +  speed_volt_range * alt_fraction;

  if (alt <= min_altitude)     pwm = max_speed_pwm;
  else if (v >= max_altitude)  pwm = min_speed_pwm;
  else
    pwm = Volts2Pwm(vlts);
  if (v>0) fprintf(stderr, "alt=%d, pwm=%d, volts=%f\n", alt, pwm, vlts);
  if (pwm < 0) pwm = 0;
  if (pwm >1023) pwm = 1023;
  return pwm;
}

bool debounce_up() {
      if (     digitalRead(upPin)
	   && !digitalRead(downPin)
	   &&  digitalRead(upPin)
	   && !digitalRead(downPin))
	return true;
      return false;
}

bool debounce_down() {
      if (     digitalRead(downPin)
	   && !digitalRead(upPin)
	   &&  digitalRead(downPin)
	       && !digitalRead(upPin) )
	return true;
      return false;
}

#define DEAD_SLOW  80  // PWM for really slow, landed shuttle

void park() {
   pwmWrite(pwmPin, min_speed_pwm-40);
   usleep(400000);
   pwmWrite(pwmPin, min_speed_pwm-80);
   usleep(400000);
   pwmWrite(pwmPin, min_speed_pwm-120);
   usleep(400000);
   pwmWrite(pwmPin, min_speed_pwm-180);
   usleep(2500000);
   pwmWrite(pwmPin, min_speed_pwm-250);
   usleep(150000);
   pwmWrite(pwmPin, DEAD_SLOW);
}

int rendezvous(char *addr)
{
    int cntr = 0;
    int s;
    while(1) {
      cntr++;
      while (g_reset) {
	if (v>0) fprintf(stderr, "(re)Initializing rendezvous\n");
	g_reset = false;
	s = bluetoothSocket(addr);
	if (v>1) fprintf(stderr, "connected at %d\n",s);
	wiringPiSetupGpio();          // Broadcom pin numbers
	if (v>2) fprintf(stderr, "wpi setup");
	
	pinMode(pwmPin, PWM_OUTPUT);  // Set Shuttle speed as PWM output
	pinMode(upPin, INPUT);        // Increase Altitude
	pinMode(downPin, INPUT);      // Decrease Altitude
	
	pullUpDnControl(upPin, PUD_UP);   // Enable pull-up resistors
	pullUpDnControl(downPin, PUD_UP);
      }
      bool firsttime = true;
      while (debounce_up()) {
	if (actual_altitude <= 1 && firsttime) { //LAUNCH!
	  pwmWrite(pwmPin, altitude_velocity(0));
	  firsttime = false;
	}
	converse(s, "v\n");
	//	usleep(100000);
	if ( rough_altitude < max_altitude ) rough_altitude += 4;
	pwmWrite(pwmPin, altitude_velocity(rough_altitude));
	cntr = 0;
      }
      while (debounce_down()) {
	converse(s, "d\n");
	//	usleep(800000);
	if ( rough_altitude > min_altitude ) rough_altitude -= 5;
	if (rough_altitude < min_altitude) rough_altitude = 0;
	if (rough_altitude == 0) {
	  readTimeout(s,60); // Special (long) timeout for "re-entry" command
	  converse(s, "r\n");  // Invoke re-entry, just in case we aren't at the limit switch
	  readTimeout(s,10);
	  park();
	}
	else 
	  pwmWrite(pwmPin, altitude_velocity(rough_altitude));
	cntr = 0;
      }

      usleep(100000); /* 0.1 seconds */

      if (cntr > 400) {
	if (actual_altitude > 0) {
	  readTimeout(s,60); // Special (long) timeout for "re-entry" command
	  converse(s, "r\n");  // Start re-entry
	  readTimeout(s,10); // Reset normal timeout
	}
	cntr = 0;
      }

      /*
       * Re-adjust shuttle velocity according to actual altitude
       * But only so often (every 4 seconds currently, 20 X 0.2s)
       */
	 if (cntr % 20 == 0) {
	   const char *reply = converse(s, "a\n");
	   if ( sscanf(reply, "%d", &actual_altitude) == 1)  {
	     if (v>0) fprintf(stderr, "Got altitude [%d]\n", actual_altitude);
	     if (v>0) fprintf(stderr, "Rough altitude was [%d]\n", rough_altitude);
	     rough_altitude = actual_altitude;
	     if (actual_altitude !=  0)
	       pwmWrite(pwmPin, altitude_velocity(actual_altitude));
	   } else {
	     fprintf(stderr, "Failed to get altitude from [%s]\n", reply);
	     /* This is where we need to redo the bluetooth socket*/
	     g_reset = true;
	   }
	 } /* Once in a while*/
 }
    return 0;
}

void usage() {
    fprintf(stderr, "usage:  bluetest <BLUETOOTH-MACADDRESS> [-v N]\nwhere N = debug verbosity. First argument must be Bluetooth address\n");
    exit(0);
}

int main(int argc, char **argv)
{
  g_reset = true;

  if (argc > 3) {
    if (!strcmp(argv[2],"-v")) v = atoi(argv[3]);
    else usage();
  }
  /* Simple check for valid bluetooth address format */
  if (argc < 2 || strlen(argv[1]) != 17) usage();

  setbuf(stderr,NULL);

  if (v>0) fprintf(stderr, "max_speed_pwm = %d\n", max_speed_pwm);
  if (v>0) fprintf(stderr, "min_speed_pwm = %d\n", min_speed_pwm);
  rendezvous(argv[1]);
  fprintf(stderr, "returned from rendezvous. This should not happen.\n");
  return 0;
}

