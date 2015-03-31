//
// Rendezvous in Space		Museum of Life and Science
// Peter Reintjes		November 2014

// PIN ASSIGNMENTS
#define EncoderB 2
#define EncoderA 3
#define LOW_LIMIT_SWITCH  4  // Limit switch
#define E1   5
#define M1   6

#define EOT "end_of_data"

// Vertical Direction/Speed Parameters
#define DEFAULT_MAX_HEIGHT 9000.0       // Don't go higher than this

double height;
int vertical_speed;
double max_height = DEFAULT_MAX_HEIGHT;

#define UP 1
#define DOWN 0
#define FAST_SHUTTLE  250
#define SLOW_SHUTTLE  100
#define SPEED_RANGE ((double)(FAST_SHUTTLE-SLOW_SHUTTLE))
#define ALTITUDE_SPEED ((int)(SPEED_RANGE * ((max_height-(double)alt)/max_height)))

#define speed(p,v)      analogWrite(p,v)

// Interrupt handler to keep track of altitude encoder
int hgt, alt;
void moved() {
  if(digitalRead(EncoderA )) {
    if (hgt==1400) {
	alt++;
	hgt=0;
    }
    hgt++;
  } else {
    if (hgt < 1) {
	if (alt>0) alt--;
	hgt=1400;
    }
    hgt--; 
  }
}

// One second's worth of upward/downward motion
void vertical(int dir) {
	vertical_speed = SLOW_SHUTTLE + ALTITUDE_SPEED;
	digitalWrite(M1,dir);
	speed(E1, vertical_speed);
	delay(1000);
	digitalWrite(E1,0);
}

#define VOLTS_TO_PWM(v) ( int ((v*255.0)/4.86) )

//#define HEIGHT_TO_PWM(h) ((int) (68.0 + 100.0*(1.0-(((double)height)/6500.0))))

#define DEAD_SLOW       30

int velocity_PWM(void) {
	int velocity = ((int)(35.0 + 168.0*(1.0-(alt/8200.0))));
	if (velocity < 10) {
		if (velocity < 0) Serial.println(" Sure enough, neg PWM!");
		velocity = 10;
	}
	return velocity;
}

int cntr;
int inactivity;
boolean resetting;

boolean debounce(int p) {
	int infloop = 1;
	while(infloop++) {
		if (infloop%100 == 0) {
			Serial.println(" stuck in debounce!");
			inactivity = -1;
		}
		boolean bit = digitalRead(p);
		int cnt = 0;
		for(int i=0;i<3;i++) {
			if (digitalRead(p) == bit) cnt++;
		}
		if (cnt == 3) return(bit);
	}
}

// Emergency re-entry needs to be fast!
void reentry() {
	digitalWrite(M1,DOWN);
	speed(E1,FAST_SHUTTLE);
	while (debounce(LOW_LIMIT_SWITCH))
	{
		delay(100);
	}
	digitalWrite(E1,0);
	alt = 0;
	hgt = 0;
}

void setup() {
  resetting = true;
  Serial.begin(9600);
  pinMode(E1,OUTPUT);
  pinMode(M1,OUTPUT);
  pinMode(LED_BUILTIN,     OUTPUT);
  pinMode(LOW_LIMIT_SWITCH,INPUT_PULLUP);

  pinMode(EncoderA,INPUT_PULLUP); /* A */
  pinMode(EncoderB,INPUT_PULLUP);  /* B INT0 on falling edge */ 

  hgt = 1000;  // Hitting the limit switch will establish altitude
  alt = 1000;
  attachInterrupt(0,moved,FALLING);

  // Reverse the motor until it triggers the limit switch
  height = max_height;
  if (debounce(LOW_LIMIT_SWITCH))
	 reentry();
  cntr = 0;
  inactivity = 0;
  resetting = false;
}

void respondToRequest(void)
{
	String is = "";
	while (Serial.available() > 0)  // Read a line of input
	{
		int c  = Serial.read();
		if ( c < 32 ) break;
		is += (char)c;
		if (Serial.available() == 0) // It is possible we're too fast
			delay(100);
	}
	if ( is.length() > 0 )  {   // process the command
		int value = 0;
		if (is.length() > 2)
			value = atoi(&is[2]);
		if (!process_command(is[0], is[1], value))
			Serial.println(" bad flow command [" + is + "]");
	}
}

boolean process_command(char c1, char c2, int value)
{
boolean rval = true;
byte d;
char buffer[20];
	switch(c2)
	{
		case '1': d = 1; break;
		case '0': d = 0; break;
		default : break;
	}
	switch(c1)
	{
		case 'a': // REVOLUTIONS
			sprintf(buffer,"%06d",alt);
			Serial.println(buffer);
			break;
		case 'h': // FRACTIONS
			sprintf(buffer,"%06d",hgt);
			Serial.println(buffer);
			break;
		case 'm':
			if (c2 == 's') max_height = value;
			else {
				sprintf(buffer,"%06d",(int)max_height);
				Serial.println(buffer);
			}
			break;
		case 'd':
			vertical(DOWN);
			break;
		case 'l':
			if (digitalRead(c2))
				digitalWrite(c2,0);
			else
				digitalWrite(c2,1);
			break;
		case 'r':
			reentry();
			break;
		case 's':
			sprintf(buffer,"%06d",vertical_speed);
			Serial.println(buffer);
			break;
		case 'u':
		case 'v':
			vertical(UP);
			break;

		default:
			rval = false;
	}
	Serial.println(EOT);
	return rval;
}

void loop() 
{
  delay(10);
  respondToRequest();
  if ((cntr++ % 5000) == 0) {
	if (inactivity == -1) {
		setup();  // Reset if stuck (in debounce loop?)
		return;
	}
	inactivity++;
	if (inactivity > 36) {  // 3 min timeout before emergency re-entry
		if (debounce(LOW_LIMIT_SWITCH))
			reentry();
		inactivity = 0;
	}
  }
}


