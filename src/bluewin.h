#include <w32api/winsock2.h>  // Includes windows.h
#include <w32api/ws2bth.h>
#define ESOCKTNOSUPPORT     WSAESOCKTNOSUPPORT
#define ETIMEDOUT    WSAETIMEDOUT
#define ECONNREFUSED WSAECONNREFUSED
#define EHOSTDOWN    WSAEHOSTDOWN
#define EINPROGRESS  WSAEINPROGRESS
#define bdaddr_t     BTH_ADDR
#define sockaddr_rc  _SOCKADDR_BTH
#define rc_family    addressFamily
#define rc_bdaddr    btAddr
#define rc_channel   port
#define PF_BLUETOOTH PF_BTH
#define AF_BLUETOOTH PF_BLUETOOTH
#define BTPROTO_RFCOMM      BTHPROTO_RFCOMM
#define BDADDR_ANY   (&(BTH_ADDR){BTH_ADDR_NULL})
#define bacpy(dst,src)      memcpy((dst),(src),sizeof(BTH_ADDR))
#define bacmp(a,b)   memcmp((a),(b),sizeof(BTH_ADDR))

int write(SOCKET s, char const *cmd, int size) { send(s, cmd, size, 0); }
int read(SOCKET s, char *cmd, int size) { recv(s, cmd, size, 0); }

ULONG AddrStringToBtAddr(IN const char * pszRemoteAddr, OUT BTH_ADDR * pRemoteBtAddr)
{
   ULONG ulAddrData[6], ulRetCode = 0;
   BTH_ADDR BtAddrTemp = 0;

   if ( ( NULL == pszRemoteAddr ) || ( NULL == pRemoteBtAddr ) )
   {
       ulRetCode = 1;
       goto CleanupAndExit;
   }
   *pRemoteBtAddr = 0;
   // 1) Parse string into a 6 membered array of unsigned long integers 
   sscanf(pszRemoteAddr, 
          "%02x:%02x:%02x:%02x:%02x:%02x",
          &ulAddrData[0],&ulAddrData[1],&ulAddrData[2],&ulAddrData[3],&ulAddrData[4],&ulAddrData[5]);

   // Pack 6 integers into a BTH_ADDR
   for ( int i=0; i<6; i++ )
   {
       BtAddrTemp = (BTH_ADDR)( ulAddrData[i] & 0xFF );
       *pRemoteBtAddr = ( (*pRemoteBtAddr) << 8 ) + BtAddrTemp;
   }
 CleanupAndExit:
   return ulRetCode;
}

// MIMIC Raspberry PI GPIO with random input and stderr message output
#include <time.h>
#include <sys/time.h>

int digitalRead(int p)          { return(time(0)&1);                      }
void digitalWrite(int p, int v) { fprintf(stderr,"set pin %d to %d\n", p, v); }
void pwmWrite(int p, int v)    { fprintf(stderr,"PWM(%d) on pin %d\n", v, p); }

void startWinSock(void) {
  WORD wVersionRequested;
  WSADATA wsaData;
  int err;

  wVersionRequested = MAKEWORD( 2, 2 );
 
  err = WSAStartup( wVersionRequested, &wsaData );
  if ( err != 0 ) {
    fprintf(stderr, "Unable to find a usable WinSock DLL\n");
    exit(0);
  }
  /* Confirm WinSock DLL supports 2.2.*/
  if ( LOBYTE( wsaData.wVersion ) != 2 ||
       HIBYTE( wsaData.wVersion ) != 2 ) {
    fprintf(stderr, "Unable to find a usable WinSock DLL\n");
    WSACleanup( );
    exit(0);
  }
}
