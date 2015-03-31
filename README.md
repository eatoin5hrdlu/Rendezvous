# Rendezvous
Bluetooth Enabled Shuttle for Rendezvous in Space (Aerospace Exhibit)

This code base includes Arduino code for the Shuttle (Rendezvous.ino) and a C program (bluetest.cpp) for the Raspberry Pi Host.
The host code can be compiled under Windows for testing: the Raspberry Pi's GPIO inputs are faked with random bits and the outputs turn into stderr messages.


PAIRING:

Still no pairing, so use command line tools to pair with the Bluetooth device:

$ hcitool scan  (look for the device)

NN:NN:NN:NN:NN:NN     <- you should see something like this

$ bluetooth-agent 1234 &    (or whatever the pairing code is)

$ bluez-simple-agent hci0 NN:NN:NN:NN:NN:NN

( where        0 in hci0 corresponds to your dongle
  and NN:NN:NN... is the address shown by "hcitool scan"


SETTING UP AUTO-RUN (as root):

1) Copy the file srendezvous into /etc/init.d
  (change values/defaults in this file as needed)

2) Run the command:
	update-rc.d /etc/init.d/srendezvous default

