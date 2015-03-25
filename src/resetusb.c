#include <stdio.h>
#include <usb.h>
/* Compile with :  gcc -o resetusb resetusb.c -lusb */
int main(void)
{
      struct usb_bus *busses;
      usb_init();
      usb_find_busses();
      usb_find_devices();
      busses = usb_get_busses();
      struct usb_bus *bus;
      int c, i, a;
      /* ... */
      for (bus = busses; bus; bus = bus->next) {
        struct usb_device *dev;
        int val;
        usb_dev_handle *junk;
        for (dev = bus->devices; dev; dev = dev->next) {
          char buf[1024];
          junk = usb_open ( dev );
          if ( junk == NULL ){
            printf("Can't open %p (%s)\n", dev, buf );
          } else {
	    usb_get_string_simple(junk,dev->descriptor.iManufacturer,buf,1023);
	    if (strcmp(buf,"HP Deluxe Webcam KQ246AA")==0)
	      val = usb_reset(junk);
          }
          usb_close(junk);
        }
      }
}
/*
....
if ( junk == NULL ){
printf("Can't open %p (%s)\n", dev, buf );
} else if (strcmp(buf,"WHAT EVER THE DEVICE NAME IS")==0){
val = usb_reset(junk);
printf( "reset %p %d (%s)\n", dev, val, buf );
}
....

printf("Device name is: %s\n", buf);
*/
