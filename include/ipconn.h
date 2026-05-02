#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mmsystem.h>
#include "timeutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include "typedefs.h"
// *****************************************************************************
//
// IPCONN: Lightweight wrapper for TCP/IP sockets (non-blocking by default)
//
// IPSERVER: Template that implements a simple TCP/IP server, managing 
// an array of IPCONN-derived connection instances on behalf of the app
//
// UDPXCVR: Lightweight transceiver for UDP datagrams (blocking by default)
//
// john@miles.io 12-Mar-10
//
// *****************************************************************************

#pragma once                           

#include <malloc.h> // For alloca

#ifndef _WINSOCK2API_
  #include <winsock2.h> // Ensure winsock2.h is included before ws2tcpip.h for some compilers
  #include <ws2tcpip.h>
  #include <mswsock.h>
#endif

//
// Configuration
//

#ifndef IPCONN_DEFAULT_RECV_BUF_BYTES          // Circular buffer used to reduce overhead of calling recv() for small block sizes
  #define IPCONN_DEFAULT_RECV_BUF_BYTES 8192   // (Reducing this value too far can stall low-bandwidth connections) 
#endif                                         
                                        
#ifndef IPCONN_SO_RECVBUF_BYTES
  #define IPCONN_SO_RECVBUF_BYTES 32768        // TCP send/receive windows (Win32 defaults are 8K each)
#endif

#ifndef IPCONN_SO_XMITBUF_BYTES
  #define IPCONN_SO_XMITBUF_BYTES 32768
#endif

#ifndef IPCONN_NAGLE
  #define IPCONN_NAGLE 0                       // Nagle disabled by default
#endif

enum IPMSGLVL
{
   IP_DEBUG = 0,                        // Debugging traffic
   IP_VERBOSE,                          // Low-level notice
   IP_DISCON,                           // Reason for disconnection (connection was lost, possibly intentionally dropped)
   IP_NOTICE,                           // Standard notice
   IP_WARNING,                          // Warning (does not imply loss of connection or failure of operation)
   IP_ERROR                             // Error (implies failure of operation, and/or abnormal connection loss)
};

enum IPDREASON
{
   IPDR_REPLACED,                       // Server is booting this connection to make room for a new one
   IPDR_DISCONNECTED,                   // Server is deleting this connection because it's no longer valid
   IPDR_TIMED_OUT,                      // Server is deleting this connection due to keepalive expiration
   IPDR_TERMINATING                     // Server is cleaning up all remaining connections at termination time
};

enum IPSRVRSTATUS
{
   IPSS_FAILURE,                        // Server's constructor didn't run successfully for some reason
   IPSS_INITIALIZED,                    // Server's constructor finished; public vars are valid (but possibly not meaningful)
   IPSS_LISTENING                       // Server is actively maintaining connections
};



class IPCONN
{
   SOCKET TCP_socket;                   // TCP socket used to communicate with server
   bool   active;                       // TRUE if socket has a valid connection
   bool   local;                        // TRUE if host is on our machine or class-C subnet
   bool   blocking_mode;                // TRUE if socket has been set to block

   sockaddr_in server_addr;             // Various other buffers and vars 
   C8          local_hostname[1024];          
   C8          remote_hostname[1024];
   S32         xmit_byte_count;
   C8          ipnum[512]; // Local IP address in dotted-quad format
   S32         recv_byte_count;
   C8         *rcv_buf;
   S32         rcv_head;
   S32         rcv_tail;
   S32         rcv_bufsize;
   S64         connection_time;
   S64         last_recv_time;
   bool        ws_started;
   DWORD       last_error_code;
   S32         user_int;
   void       *user_void;

public:

   USTIMER timer;                        // Public, for ease of access to routines such as log.printf()

   //
   // Generate dotted-quad address string based on IPv4 AF_INET socket designator
   //

   static C8 *address_string(sockaddr_in *address, 
                             C8          *text, 
                             S32          text_array_size,
                             bool         include_port = TRUE);

   //
   // Populate sockaddr_in with numeric address cooresponding to ASCII string
   // (dotted-quad or qualified hostname with optional port designator)
   //

   static bool parse_address(C8 *IP_address, S32 default_port, sockaddr_in *dest);

   //                                                                          
   // Reverse-DNS utility routine to identify remote hosts
   //
   // This routine can take a long time, so it's better to use
   // a worker thread with getnameinfo(), as IPSERVER does
   //

   static bool hostname_string(sockaddr_in *address, 
                               C8          *text, 
                               S32          text_array_size,
                               S32          timeout_ms);

   //
   // Constructor
   //

   IPCONN();

   //
   // Destructor releases connection object and Winsock reference
   // 

   virtual ~IPCONN();

   virtual void disconnect(void);

   //
   // Establish connection
   //
   // Should be called right after construction to activate the connection object.  
   // Subsequent calls will have no effect.  (The connection may have been dropped
   // locally or at the server end, or it may have failed for other reasons -- 
   // call status() frequently to check for loss of connectivity.)
   //
   // Separate from constructor because the caller might want to subclass some virtual methods,
   // e.g. on_connect()
   //

   virtual void connect(C8  *IP_address,                              
                        S32  default_port,         // (Used if the IP_address doesn't specify a :port substring)
                        S32  receive_buffer_size = IPCONN_DEFAULT_RECV_BUF_BYTES);

   //
   // Called by server after a successful accept() operation that returns
   // a socket.  Not normally used by apps
   //

   virtual void server_accept(sockaddr_in *who, 
                              SOCKET       requestor,
                              S32          receive_buffer_size = IPCONN_DEFAULT_RECV_BUF_BYTES);

   //
   // Copy OS-specific error text to buffer and return it
   //

   virtual C8 *error_text(C8 *text, S32 text_array_size, DWORD last_error = 0);

   //
   // Progress monitor function for lengthy operations
   // Connection is dropped if this function returns FALSE
   //

   virtual bool monitor(S32 bytes_so_far, S32 bytes_total);

   //
   // Error/status message sink
   //

   virtual void message_sink(IPMSGLVL level,   
                             C8      *text);

   virtual void message_printf(IPMSGLVL level,
                               C8 *fmt,
                               ...);

   // Message sink implementation
   // Get/set user words
   //

   virtual S32 user_word(void);

   virtual S32 set_user_word(S32 val);

   virtual void *user_ptr(void);

   virtual void *set_user_ptr(void *val);

   //
   // Set/clear blocking mode
   //
   // High-throughput apps are better off with blocking sockets, as OS blocking is somewhat
   // more efficient than sleeping on incomplete read calls at the application level.
   // Timeout management is somewhat more complicated, though...
   //

   virtual void set_blocking_mode(bool mode);

   //
   // Get/set send/recv window sizes
   //

   virtual S32 get_send_window(void);

   virtual S32 get_recv_window(void);

   virtual void set_send_window(S32 bytes);

   virtual void set_recv_window(S32 bytes);

   //
   // Return TRUE if connection is valid, or FALSE if any routines have detected closure
   // or other fatal conditions
   //
   // The app (or server) should delete the IPCONN object once its status has gone FALSE, 
   // but it's OK to check only once per frame, or otherwise when the app is in an appropriate
   // context.  There is no need to check status() before every IPCONN call; the other 
   // methods are designed to behave harmlessly if the connection is invalid.  E.g., 
   // read_block() will return 0 bytes, send_block() will return FALSE, etc.  
   //

   virtual bool status(void);

   //
   // Return last error code reported by OS
   //

   virtual S32 last_error(void);

   //
   // Transport-specific info functions
   //

   virtual void host_info(C8          **local_name  = NULL, 
                          C8          **remote_name = NULL, 
                          sockaddr_in **remote_addr = NULL);

   virtual void update_remote_hostname(C8 *new_remote_hostname);

   //
   // Connection statistics
   //

   virtual S64 statistics(S32 *bytes_received = NULL,
                          S32 *bytes_sent     = NULL,
                          S32 *xmit_bit_rate  = NULL,
                          S32 *recv_bit_rate  = NULL);

   //
   // Connection age (in string form)
   //

   virtual C8 *age_string(C8  *text,  
                          S32  text_array_size);

   //
   // Return TRUE if this is a server-client connection that should never be 
   // disconnected to make room for another one
   //

   virtual bool is_protected(void);

   //
   // Return TRUE if the target of this connection is local to the host or its class-C block
   // 

   virtual bool is_local(void);

   //
   // Time in milliseconds since traffic was received
   //

   virtual S64 us_since_recv(void);

   //
   // Accessor for underlying OS socket 
   //

   virtual SOCKET socket(void);

   //
   // Called just before successful return from connect() or server_accept()
   // 

   virtual void on_connect(void);

   //
   // Called periodically during lengthy operations to give some time back to
   // the application
   //
   // Currently this only happens when send_block() is backlogged
   //

   virtual void on_lengthy_operation(void);

   //
   // Called when an IPCONN object is about to be deleted by the IPSERVER that
   // owns it
   //

   virtual void on_server_discard(IPDREASON reason);

   //
   // Transmit a block of data
   //
   // Retries logjammed send() calls for the specified # of times, waiting for the specified
   // interval between each attempt, then optionally reports an error and tears down the 
   // connection
   //
   // Returns FALSE if the operation failed to send all supplied data for any reason
   // Caller should check status() to see if the connection is still intact
   //

   bool send_block(void *block, 
                           S32   bytes_to_send,
                           S32  *sent_bytes            = NULL, 
                           S32   max_send_retries      = 300,
                           S32   send_retry_delay_ms   = 10,
                           bool  disconnect_on_failure = TRUE);

   //
   // printf() wrapper for send_block()
   //

   bool send_printf(C8 *fmt,
                            ...);

   //
   // Fetch as much data from socket as will fit in our receive buffer, which can store 
   // up to rcv_bufsize-1 bytes (1 byte is lost for housekeeping) 
   //
   // Return total amount of data available in the receive buffer
   //

   virtual S32 receive_poll(S32 *fetched_this_call = NULL);

   //
   // Copy incoming data out of the receive buffer, returning the
   // amount actually copied
   //
   // Together with receive_poll(), this is basically a wrapper around
   // the recv() call, intended to minimize ring transitions for apps 
   // that read a few bytes at a time.  It could be further optimized 
   // for small transfers by not using memcpy()
   //
   // Up to rcv_bufsize-1 bytes may be returned
   //

   virtual S32 read_block(void *dest, S32 n_bytes);

   //
   // Blocking version of above returns TRUE if request fulfilled, FALSE if timed out
   //
   // No fixed limit on destination buffer size
   //

   virtual bool read_block(void *dest, S32 n_bytes, S32 timeout_ms);

   //
   // Basic text send/receive functions, useful for issuing
   // queries and retrieving the resulting data in text form
   //

   virtual bool read_text_file(FILE   *outfile,
                               C8     *command_string = NULL,    // NULL = do not transmit a command first
                               C8     *command_term   = "",
                               S32     timeout_msec   = 3000,    // 0 = no timeout
                               S32     dropout_msec   = 3000,
                               S32     repeat_cnt     = 0,       // 0 = indefinite
                               S32     EOS            = 10,
                               S32     max_line_chars = 512);

   virtual bool read_text_line(C8       *incoming_line,
                               S32       incoming_max_bytes = 512,
                               const C8 *command_string     = NULL,    // NULL = do not transmit a command first
                               const C8 *command_term       = "",
                               S32       timeout_msec       = 3000,    // 0 = no timeout 
                               S32       dropout_msec       = 3000,
                               S32       EOS                = 10);

};



//
// In response to incoming connection activity, IPSERVER::update() creates and deletes 
// objects of class T, where T should be either IPCONN or a class that inherits
// from it.  The application has free access to the IPSERVER's array of 
// connection objects (clients[]), but it must not alter the contents
// of this array or try to retain any indexes, pointers, or other information
// related to the client array between calls to IPSERVER::update()!
//

enum IPRDNSSTATE
{
   IPRDNS_READY,              // RDNS thread has not yet begun running; thread handle is not valid
   IPRDNS_IN_USE,             // RDNS thread is in progress, waiting on getnameinfo()
   IPRDNS_EXIT_OK,            // RDNS thread has exited, and FQDN field contains a valid domain name
   IPRDNS_EXIT_ERR            // RDNS thread has exited, but FQDN field contains error information
};

struct IPRDNSSTRUC
{
   sockaddr_in address;          // Copy of remote address structure for the connection
   HANDLE      hThread;          // Worker thread assigned to execute getnameinfo()
   C8          FQDN[NI_MAXHOST]; // Fully-qualified domain name written by getnameinfo() thread
   IPRDNSSTATE thread_state;     // Status of request thread
   S32         latency_mS;       // Time used by getnameinfo()
   void       *orig_client;      // Original client that initiated this request
};                               

template <class T, S32 max_clients> class IPSERVER
{
   //
   // Private data and methods 
   //

   SOCKET      monitor_socket;    
   S32         active;
   DWORD       last_error_code;
   S32         user_int; 
   void       *user_void;
   bool        ws_started;
   bool        initialized;
   S64         keepalive_time_us;
   IPRDNSSTRUC RDNS [max_clients];       // RDNS request structure for each client slot

   virtual bool init(void)
      {
      monitor_socket    = INVALID_SOCKET;
      active            = FALSE;
      last_error_code   = 0;
      ws_started        = FALSE;
      initialized       = FALSE;
      keepalive_time_us = 0;
      memset(hostname, 0, sizeof(hostname));
      memset(ipnum,    0, sizeof(ipnum));

      port = 0;
      memset(clients,  0, sizeof(clients));

      for (S32 i=0; i < max_clients; i++)
         {
         RDNS[i].thread_state = IPRDNS_READY;
         RDNS[i].hThread      = 0;
         }

      //
      // Init Winsock
      //

      WORD wRequestVer = MAKEWORD(2,2);

      memset(&wsadata, 0, sizeof(wsadata));
      if (WSAStartup(wRequestVer, &wsadata))
         {
         return FALSE;
         }

      ws_started = TRUE;

      //
      // Get host's IP address with gethostname(), followed by gethostbyname()
      // if necessary
      //

      if (gethostname(hostname, sizeof(hostname)-1) == SOCKET_ERROR)
         {
         strcpy(hostname,"(IP unknown)");
         strcpy(ipnum, hostname);
         }
      else
         {
         in_addr server_addr;
         memset(&server_addr,0,sizeof(server_addr));

         server_addr.S_un.S_addr = inet_addr(hostname);

         if (server_addr.S_un.S_addr == INADDR_NONE)
            {
            //
            // Currently we just use the first entry from the address list, 
            // which isn't ideal...
            //

            LPHOSTENT lphp = gethostbyname(hostname);

            if (lphp == NULL)
               {
               return FALSE;
               }

            server_addr = *(struct in_addr *) (lphp->h_addr_list[0]);
            }

         strcpy(ipnum,inet_ntoa(server_addr));
         }

      initialized = TRUE;
      return TRUE;
      }

   //
   // Find an available client slot, booting an existing client if necessary to
   // make room
   //
   // Returns -1 if the array is full of protected clients
   //

   virtual S32 obtain_slot(sockaddr_in *request_addr)
      {
      // 
      // First try to find an empty slot
      //
   
      S32 slot;
   
      for (slot=0; slot < max_clients; slot++)
         {
         if (clients[slot] == NULL)
            {
            break;
            }
         }
   
      if (slot < max_clients)
         {
         return slot;
         }
   
      //
      // If all slots are occupied, find the oldest connection and delete it
      // Try to boot remote clients before local ones
      //
   
      S64 oldest_local_client_time  =  0;
      S32 oldest_local_client       = -1;
      S64 oldest_remote_client_time =  0;
      S32 oldest_remote_client      = -1;
   
      for (slot=0; slot < max_clients; slot++)
         {
         T *C = clients[slot];
   
         if (C->is_protected())
            {
            continue;
            }

         S64 age = C->statistics();

         if (C->is_local())
            {
            if (age > oldest_local_client_time)
               {
               oldest_local_client_time = age;
               oldest_local_client      = slot;
               }
            }
         else
            {
            if (age > oldest_remote_client_time)
               {
               oldest_remote_client_time = age;
               oldest_remote_client      = slot;
               }
            }
         }
   
      slot = oldest_remote_client;
   
      if (slot == -1)
         {
         slot = oldest_local_client;
         }

      if (slot == -1)
         {
         return -1;        // all existing connections are protected 
         }

      clients[slot]->on_server_discard(IPDR_REPLACED);

      message_printf(IP_NOTICE, "Slot %d: Booted to make room for new connection", slot);

      delete clients[slot];
      clients[slot] = NULL;

      return slot;
      }

   //
   // Thread procedure to execute reverse-DNS request on each incoming
   // connection
   //

   static UINT WINAPI RDNS_thread_proc(LPVOID pParam)
      {
      SetThreadName("RDNS thread");
      IPRDNSSTRUC *R = (IPRDNSSTRUC *) pParam;

      S32 start_time = timeGetTime();
                                            
      S32 result = getnameinfo((sockaddr *) &R->address,
                                sizeof(R->address),
                                R->FQDN,
                                sizeof(R->FQDN),
                                NULL,
                                0,
                                0);

      R->latency_mS = timeGetTime() - start_time;

      if (result == 0)
         {
         R->thread_state = IPRDNS_EXIT_OK;
         }
      else
         {
         _snprintf(R->FQDN,sizeof(R->FQDN)-1,"unresolvable, error code %d", WSAGetLastError());
         R->thread_state = IPRDNS_EXIT_ERR;
         }

      return TRUE;
      }

   virtual void RDNS_close_request(IPRDNSSTRUC *R)
      {
      WaitForSingleObject(R->hThread, INFINITE);     
      CloseHandle(R->hThread);
      R->hThread = 0;
      R->thread_state = IPRDNS_READY;
      }

public:
   //
   // Public data members
   //

   T *clients  [max_clients];                // Client connections, accessible to app between calls to IPSERVER::update()

   C8  hostname[512];                        // Server hostname
   C8  ipnum   [512];                        // Dotted-quad copy of our IP address
   S32 port;                                 // Server listens on this port

   WSADATA wsadata;                          // Winsock attributes

   //
   // Construction
   //

   IPSERVER()
      {
      user_int  = 0;
      user_void = NULL;

      init();
      }                 

   virtual ~IPSERVER()
      {
      shutdown();
      }

   //
   // Enable server operation
   //

   virtual void startup(S32 listen_port)
      {
      C8 msg[256] = { 0 };

      if (!initialized)
         {
         if (!init())
            {
            return;
            }
         }

      if (active)
         {
         return;
         }

      port = listen_port;

      message_printf(IP_VERBOSE, "Using Windows Sockets V%d.%d (%s)",
         HIBYTE(wsadata.wVersion),
         LOBYTE(wsadata.wVersion),
         wsadata.szDescription);

      message_printf(IP_NOTICE, "Initializing host %s, address %s:%d",
         hostname,
         ipnum,
         port);

      //
      // Create monitor socket and mark it non-blocking
      //

      monitor_socket = socket(PF_INET, SOCK_STREAM, 0);

      if (monitor_socket == INVALID_SOCKET)
         {
         message_printf(IP_ERROR,"Bad monitor socket");
         return;
         }

      DWORD dwVal = 1;

      ioctlsocket(monitor_socket,
                  FIONBIO,
                 &dwVal);

      dwVal = !IPCONN_NAGLE;

      setsockopt(monitor_socket,
                 IPPROTO_TCP,
                 TCP_NODELAY,
         (C8 *) &dwVal,
                 sizeof(dwVal));

      //
      // Assign socket's IP address and port
      //
      // INADDR_ANY allows us to work with any interface that receives
      // traffic destined for our port.  The default binding address for 
      // transmission is the 0th interface as discovered above
      //

      sockaddr_in monitor_addr;

      memset(&monitor_addr,0,sizeof(monitor_addr));

      monitor_addr.sin_family      = AF_INET;
      monitor_addr.sin_addr.s_addr = htonl(INADDR_ANY);
      monitor_addr.sin_port        = htons((S16) port);

      S32 result = bind(monitor_socket,
          (sockaddr *) &monitor_addr,
                        sizeof(monitor_addr));

      if (result)
         {
         message_printf(IP_ERROR, "bind() failed: %s", error_text(msg,sizeof(msg))); 
         return;
         }

      //
      // Set up to listen for connection requests from clients
      //

      memset(clients, 0, sizeof(clients));

      result = listen(monitor_socket,
                      5);              // connection backlog = 5 (max allowed for non-server versions of Windows)

      if (result)
         {
         message_printf(IP_ERROR, "listen() failed: %s", error_text(msg,sizeof(msg))); 
         return;
         }

      active = TRUE;
      }

   //
   // Shut down
   //
   // (Multiple startup/shutdown cycles are permitted)
   //

   virtual void shutdown(void)
      {
      for (S32 slot=0; slot < max_clients; slot++)
         {
         //
         // Ensure all RDNS threads have exited cleanly
         //

         IPRDNSSTRUC *R = &RDNS[slot];

         if (R->hThread != 0)
            {
            message_printf(IP_NOTICE, "Slot %d: Waiting for RDNS thread to finish . . .", slot);
            RDNS_close_request(R);
            }

         //
         // Boot each client
         //

         T *C = clients[slot];

         if (C != NULL)
            {
            C->on_server_discard(IPDR_DISCONNECTED);

            message_printf(IP_NOTICE, "Slot %d: Released at server shutdown", slot);

            delete C;
            clients[slot] = NULL;
            }
         }

      if (monitor_socket != INVALID_SOCKET)
         {
         closesocket(monitor_socket);
         monitor_socket = INVALID_SOCKET;
         }

      if (ws_started)
         {
         WSACleanup();
         ws_started = FALSE;
         }

      active = FALSE;
      initialized = FALSE;
      }

   //
   // Give the app a chance to reject connection attempts, do
   // handshaking, etc.
   //

   virtual bool on_request(SOCKET requestor, sockaddr_in *from_addr)
      {
      return TRUE;
      }

   virtual void message_sink(IPMSGLVL level,   
                             C8      *text)
      {
      ::printf("%s\n",text);
      }

   virtual void message_printf(IPMSGLVL level,
                               C8 *fmt,
                               ...)
      {
      C8 buffer[4096] = { 0 };

      va_list ap;

      va_start(ap, 
               fmt);

      _vsnprintf(buffer,
                 sizeof(buffer)-1,
                 fmt,
                 ap);

      va_end(ap);

      //
      // Remove trailing whitespace
      //

      C8 *end = &buffer[strlen(buffer)-1];

      while (end > buffer)
         {
         if (!isspace((C8) *end)) 
            {
            break;
            }

         *end = 0;
         end--;
         }

      message_sink(level,
                   buffer);
      }

   //
   // Get/set user values
   //
   // User values survive shutdown/startup calls
   // New connections inherit the server's user values
   //

   virtual S32 user_word(void)
      {
      return user_int;
      }

   virtual S32 set_user_word(S32 val)
      {
      S32 prev = user_int;
      user_int = val;
      return prev;
      }

   virtual void *user_ptr(void)
      {
      return user_void;
      }

   virtual void *set_user_ptr(void *val)
      {
      void *prev = user_void;
      user_void = val;
      return prev;
      }

   //
   // Return server status based on whether or not the constructor
   // ran successfully, and whether or not it's actively listening for 
   // connections
   // 

   virtual IPSRVRSTATUS status(void)
      {
      if (active)      return IPSS_LISTENING;
      if (initialized) return IPSS_INITIALIZED;

      return IPSS_FAILURE;    
      }

   //
   // Return last error code reported by OS
   //

   virtual S32 last_error(void)
      {
      return last_error_code;
      }

   //
   // Clean up inactive connections and poll for new ones
   //
   // Optionally swallow any incoming data on managed connections, e.g. if 
   // clients send nothing but keepalive traffic
   // 
   // Returns # of newly-accepted connections, or -1 if none
   // 
   // WARNING: This function alters the contents of the clients[] array -- don't retain any
   // state that will be corrupted if entries are created or deleted!
   //

   virtual S32 update(bool consume_received_data = FALSE)
      {
      C8 msg[256] = { 0 };

      if (!active)
         {
         return -1;
         }

      S32 slot;

      for (slot=0; slot < max_clients; slot++)
         {
         //
         // Consume keepalive traffic
         //

         T *C = clients[slot];

         if ((C != NULL) && consume_received_data)
            {
            C->read_block(NULL, C->receive_poll());
            }

         //
         // Check for completed RDNS request
         //

         IPRDNSSTRUC *R = &RDNS[slot];

         if ((R->thread_state != IPRDNS_EXIT_OK) && (R->thread_state != IPRDNS_EXIT_ERR))
            {
            continue;
            }

         if (C == R->orig_client)
            {  
            message_printf(IP_NOTICE, "Slot %d: Client is %s (%d ms)",
               slot, R->FQDN, R->latency_mS);

            C->update_remote_hostname(R->FQDN);
            }
         else
            {
            message_printf(IP_NOTICE, "Slot %d: Previous client was %s (%d ms)",
               slot, R->FQDN, R->latency_mS);
            }

         RDNS_close_request(R);
         }

      //
      // Clean up any unused or timed-out connections
      //

      for (slot=0; slot < max_clients; slot++)
         {
         C8 msgbuf[64] = { 0 };

         T *C = clients[slot];
         if (C == NULL) continue;

         S32 xmit_rate = 0, recv_rate = 0;
         C->statistics(NULL, NULL, &xmit_rate, &recv_rate);

         if (!C->status())
            {
            C->on_server_discard(IPDR_DISCONNECTED);

            message_printf(IP_NOTICE, "Slot %d: Disconnected (%s, R=%d bps, X=%d bps)", 
               slot, C->age_string(msgbuf, sizeof(msgbuf)), recv_rate, xmit_rate);

            delete C;
            clients[slot] = NULL;
            }
         else
            {
            if (keepalive_time_us > 0)
               {
               S64 t = C->us_since_recv();

               if (t > keepalive_time_us)
                  {
                  C->on_server_discard(IPDR_TIMED_OUT);

                  message_printf(IP_NOTICE, "Slot %d: Timed out (%d ms, limit=%d ms) (%s, R=%d,X=%d bps)", 
                     slot, (S32) (t / 1000), (S32) (keepalive_time_us / 1000), 
                     C->age_string(msgbuf, sizeof(msgbuf)), recv_rate, xmit_rate);

                  delete C;
                  clients[slot] = NULL;
                  }
               }
            }
         }
   
      //
      // Poll for new connection requests
      //

      S32 new_connections;

      for (new_connections=0; new_connections < max_clients; new_connections++)
         {
         SOCKET      requestor;
         sockaddr_in request_addr;
         S32         request_addr_size;

         memset(&request_addr, 0, sizeof(request_addr));
         request_addr_size = sizeof(request_addr);

         requestor = accept(monitor_socket,
              (sockaddr *) &request_addr,
                           &request_addr_size);

         if (requestor == INVALID_SOCKET)
            {
            S32 WSA_error = WSAGetLastError();

            if (WSA_error != WSAEWOULDBLOCK)
               {
               message_printf(IP_ERROR, "accept() failed: %s", error_text(msg,sizeof(msg), WSA_error)); 
               active = FALSE;
               }

            break;
            }

         //
         // Connection request received -- find an available client slot, booting
         // an existing client if necessary to make room
         //

         if (!on_request(requestor, &request_addr))
            {
            closesocket(requestor);
            continue;
            }

         slot = obtain_slot(&request_addr);

         if (slot == -1)
            {
            break;
            }

         //
         // Accept the new connection
         // 

         T *C = new T;

         C->set_user_word(user_word());
         C->set_user_ptr(user_ptr());

         C->server_accept(&request_addr, requestor, IPCONN_DEFAULT_RECV_BUF_BYTES);

         if (!C->status())
            {
            delete C;
            continue;
            }

         //
         // Log it
         //

         clients[slot] = C;

         C8 *hname = NULL;
         C->host_info(NULL, &hname, NULL);

         if (C->is_local())
            {
            message_printf(IP_NOTICE, "Slot %d: Local client %s accepted",
                           slot,
                           hname);
            }
         else
            {
            message_printf(IP_NOTICE, "Slot %d: Remote client %s accepted",
                           slot,
                           hname);
            }

         //
         // Start thread to perform asynchronous reverse-DNS lookup, so we can
         // log the client's fully-qualified domain name
         //
         // (This functionality is cosmetic; if the slot's RDNS worker thread is 
         // already running, leave it alone)
         // 

         IPRDNSSTRUC *R = &RDNS[slot];

         if (R->thread_state == IPRDNS_READY)
            {
            R->address      = request_addr;
            R->FQDN[0]      = 0;
            R->thread_state = IPRDNS_IN_USE;
            R->orig_client  = C;

            unsigned int stupId;
            R->hThread = (HANDLE) _beginthreadex(NULL,                   
                                                 0,                              
                                                 RDNS_thread_proc,
                                                 R,
                                                 0,                              
                                                &stupId);
            }
         }

      return new_connections;
      }
};



class UDPXCVR                           // General-purpose UDP transceiver
{
   SOCKET      UDP_socket;
   bool        active;
   bool        blocking_mode;
   S32         listen_port;

   sockaddr_in remote_addr;
   C8          local_hostname[1024];          
   C8          remote_hostname[1024];

   bool        ws_started;
   DWORD       last_error_code;
   S32         user_int;
   void       *user_void;
   C8          ipnum[512]; // Local IP address in dotted-quad format

   USTIMER     timer;

public:

   UDPXCVR(S32 _listen_port = 0);

   //
   // Destructor releases socket object and Winsock reference
   // 

   virtual ~UDPXCVR();

   virtual void shutdown(void);

   //
   // Startup function will be called automatically from most functions that access the socket
   // (no need to call it explicitly)
   //

   virtual bool startup(void);

   //
   // Copy OS-specific error text to buffer and return it
   //

   virtual C8 *error_text(C8 *text, S32 text_array_size, DWORD last_error = 0);

   //
   // Error/status message sink
   //

   virtual void message_sink(IPMSGLVL level,   
                             C8      *text);

   virtual void message_printf(IPMSGLVL level,
                               C8 *fmt,
                               ...);

   //
   // Get/set user words
   //

   virtual S32 user_word(void);

   virtual S32 set_user_word(S32 val);

   virtual void *user_ptr(void);

   virtual void *set_user_ptr(void *val);

   //
   // Set/clear blocking mode (normally enabled for UDP sockets)
   //

   virtual void set_blocking_mode(bool mode);

   //
   // Set blocking timeout in milliseconds
   //

   virtual void set_timeout_ms(S32 ms);

   //
   // Return TRUE if UDP transceiver instance is valid, FALSE otherwise
   //

   virtual bool status(void);

   //
   // Return last error code reported by OS
   //

   virtual S32 last_error(void);

   //
   // Accessor for underlying OS socket 
   //

   virtual SOCKET socket(void);

   // On lengthy operation (non-virtual)
   // Called periodically during lengthy operations to give some time back to
   // the application
   //
   // Currently this only happens when send_block() is backlogged
   //

   virtual void on_lengthy_operation(void);
   
   //
   // Optionally hardwire the address of the intended remote peer for subsequent send/receive calls
   //
   // If this function isn't used, the peer's address must be obtained from read_block()
   // and used in subsequent calls to send_block()
   //
   
   virtual bool set_remote_address(const C8  *IP_address,
                                   S32        default_port);

   //
   // Transmit a block of data
   //
   // Retries logjammed send() calls for the specified # of times, waiting for the specified
   // interval between each attempt, then optionally reports an error and tears down the 
   // connection. (Normally UDP transmissions shouldn't block, but this is supported for 
   // compatibility with the TCP/IP version.) 
   //
   // Returns FALSE if the operation failed for any reason
   //
   // *dest is typically a sockaddr structure filled in by a call to read_block() that 
   // received some data (e.g., a connection request) from the remote peer.  If NULL, the
   // set_remote_address() function must have been called prior to send_block()
   //

   virtual bool send_block(void       *block, 
                           S32         bytes_to_send,
                           sockaddr   *dest                  = NULL,
                           S32         dest_size             = 0,
                           S32        *sent_bytes            = NULL, 
                           S32         max_send_retries      = 300,
                           S32         send_retry_delay_ms   = 10,
                           bool        disconnect_on_failure = TRUE);

   //
   // printf() wrapper for send_block()
   //

   virtual bool send_printf(C8 *fmt,
                            ...);

   bool send_printf(sockaddr *dest,
                            S32       dest_size, 
                            C8       *fmt,
                            ...);

   //
   // Try to read a datagram of the specified size, returning the maximum # of bytes to
   // retrieve
   //
   // If sender is NULL, the caller will be unable to reply unless set_remote_address() has been
   // called to provide the peer's address explicitly
   //
   // For UDP, any available unread data beyond n_bytes is lost
   //

   S32 read_block(void     *dest,
                          S32       n_bytes,
                          sockaddr *sender = NULL);
};
