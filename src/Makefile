LIBS = -lbluetooth -lwiringPi  # Linux 

# LIBS = -lwsock32  # Windows

PREV_BT=98:D3:31:30:2A:D1

#DEF_BT=98:D3:31:30:2A:D1
DEF_BT=98:D3:31:40:1D:57

rendezvous	: rendezvous.cpp
	gcc -Wno-write-strings -o rendezvous rendezvous.cpp $(LIBS)

unpair	:	rendezvous
	bluez-simple-agent hci0 $(PREV_BT) remove

pair	:	rendezvous
	bluetooth-agent 1234 &
	bluez-simple-agent hci0 $(DEF_BT)

test	:	rendezvous
	./rendezvous $(DEF_BT) -v 2

install	: srendezvous
	cp srendezvous /etc/init.d/
	chmod +x /etc/init.d/srendezvous
	/etc/init.d/srendezvous start

start : rendezvous
	/etc/init.d/srendezvous start

stop : rendezvous
	/etc/init.d/srendezvous stop


