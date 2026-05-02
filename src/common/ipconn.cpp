#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ipconn.h"
#include <process.h>
#include <algorithm>

// *****************************************************************************
// IPCONN implementation
// *****************************************************************************

C8 *IPCONN::address_string(sockaddr_in *address, 
                          C8          *text, 
                          S32          text_array_size,
                          bool         include_port)
{
   assert(text != NULL);

   memset(text, 0, text_array_size);
   S32 n = text_array_size - 1;

   S32 p = ntohs(address->sin_port);

   if (include_port && (p != 0))
      {
      char ip_buffer[INET_ADDRSTRLEN];
      if (inet_ntop(AF_INET, &(address->sin_addr), ip_buffer, sizeof(ip_buffer)) != NULL) {
          _snprintf(text, n, "%s:%d", ip_buffer, p);
      } else {
          _snprintf(text, n, "UnknownIP:%d", p); // Fallback
      }
      }
   else
      {
      char ip_buffer[INET_ADDRSTRLEN];
      if (inet_ntop(AF_INET, &(address->sin_addr), ip_buffer, sizeof(ip_buffer)) != NULL) {
          _snprintf(text, n, "%s", ip_buffer);
      } else {
          _snprintf(text, n, "UnknownIP"); // Fallback
      }
      }

   return text;
}

bool IPCONN::parse_address(C8 *IP_address, S32 default_port, sockaddr_in *dest)
{
   C8  dest_name[1024] = { 0 };
   char port_str[16];
   S32 server_port_num = default_port;
  
   strncpy(dest_name, IP_address, sizeof(dest_name)-1);
  
   C8 *colon = strrchr(dest_name,':');
  
   if (colon != NULL) 
      {
      *colon = 0;
      server_port_num = atoi(&colon[1]);
      }

   colon = strchr(dest_name,':');

   if (colon != NULL)      // handle cases with multiple port numbers appended by batch files, etc.
      {
      *colon = 0;
      }
  
   memset(dest, 0, sizeof(*dest));
  
   _snprintf(port_str, sizeof(port_str), "%d", server_port_num);

   struct addrinfo hints, *result, *ptr;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;        // IPv4 only
   hints.ai_socktype = SOCK_STREAM;  // TCP socket
   hints.ai_protocol = IPPROTO_TCP;

   int status = getaddrinfo(dest_name, port_str, &hints, &result);
   if (status != 0) {
       // Try to parse as numeric IP if getaddrinfo failed for hostname
       if (inet_pton(AF_INET, dest_name, &(dest->sin_addr)) == 1) {
           dest->sin_family = AF_INET;
           dest->sin_port = htons((U16)server_port_num);
           return TRUE;
       }
       // If inet_pton also failed, then it's a real error
       WSASetLastError(status); // Set last error for consistency
       return FALSE;
   }

   // Loop through all returned addresses and use the first IPv4 one
   for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
       if (ptr->ai_family == AF_INET) {
           memcpy(dest, ptr->ai_addr, ptr->ai_addrlen);
           break;
       }
   }

   freeaddrinfo(result);

   if (ptr == NULL) {
       // No IPv4 address found
       WSASetLastError(WSAHOST_NOT_FOUND);
       return FALSE;
   }
   
   return TRUE;
}

bool IPCONN::hostname_string(sockaddr_in *address, 
                            C8          *text, 
                            S32          text_array_size,
                            S32          timeout_ms)
{
   assert(text != NULL);

   if (timeout_ms == 0)
      {
      address_string(address, text, text_array_size, FALSE); // Pass FALSE for include_port as hostname_string typically doesn't include port
      return TRUE;
      }

   char host_buffer[NI_MAXHOST];
   char service_buffer[NI_MAXSERV];

   int status = getnameinfo((struct sockaddr *)address, sizeof(*address),
                            host_buffer, sizeof(host_buffer),
                            service_buffer, sizeof(service_buffer),
                            NI_NAMEREQD); // Request hostname

   memset(text, 0, text_array_size);

   if (status == 0) {
       strncpy(text, host_buffer, text_array_size - 1);
       text[text_array_size - 1] = '\0';
       return TRUE;
   } else {
       address_string(address, text, text_array_size, FALSE);
       return FALSE; // Indicate failure to resolve hostname
   }
}

IPCONN::IPCONN()
{
   TCP_socket = INVALID_SOCKET;
   active = FALSE;
   local = FALSE;
   blocking_mode = FALSE;

   memset(&server_addr,    0, sizeof(server_addr));
   memset(local_hostname,  0, sizeof(local_hostname));
   memset(remote_hostname, 0, sizeof(remote_hostname));

   xmit_byte_count = 0;
   recv_byte_count = 0;

   rcv_buf     = NULL;
   rcv_head    = 0;
   rcv_tail    = 0;
   rcv_bufsize = 0;

   connection_time = 0;
   last_recv_time  = 0;
   ws_started      = FALSE;
   last_error_code = 0;
   user_int        = 0;
   user_void       = NULL;
}

IPCONN::~IPCONN()
{
   disconnect();
}

void IPCONN::disconnect(void)
{
   active = FALSE;

   if (TCP_socket != INVALID_SOCKET)
      {
      closesocket(TCP_socket);
      TCP_socket = INVALID_SOCKET;
      }

   if (rcv_buf != NULL)
      {
      free(rcv_buf);
      rcv_buf = NULL;
      }

   if (ws_started)
      {
      WSACleanup();
      ws_started = FALSE;
      }
}

void IPCONN::connect(C8  *IP_address,                              
                     S32  default_port,         
                     S32  receive_buffer_size)
{
   C8 msg[256] = { 0 };

   if (ws_started || active)
      {
      return;
      }

   WSADATA wsadata;
  
   WORD wRequestVer = MAKEWORD(2,2);
  
   if (WSAStartup(wRequestVer, &wsadata)) 
      {
      message_printf(IP_ERROR, "Couldn't open WinSock");
      return;
      }

   ws_started = TRUE;

   struct addrinfo hints, *res, *p;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;        // IPv4 only
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;      

   struct in_addr inaddrIP;
   inaddrIP.s_addr = INADDR_NONE;

   if (gethostname(local_hostname, sizeof(local_hostname)-1) == SOCKET_ERROR) 
      {
      message_printf(IP_ERROR, "Local hostname unobtainable");
      return;
      }
  
   if (getaddrinfo(local_hostname, NULL, &hints, &res) == 0) {
       for (p = res; p != NULL; p = p->ai_next) {
           if (p->ai_family == AF_INET) {
               inaddrIP = ((struct sockaddr_in*)p->ai_addr)->sin_addr; 
               break;
           }
       }
       freeaddrinfo(res);
   }

   char ip_buffer[INET_ADDRSTRLEN];
   if (inet_ntop(AF_INET, &inaddrIP, ip_buffer, sizeof(ip_buffer)) != NULL) { 
       strcpy(ipnum, ip_buffer);
   } else {
       strcpy(ipnum, "Unknown"); 
   }
  
   message_printf(IP_VERBOSE, "Using Windows Sockets V%d.%d (%s)",
      HIBYTE(wsadata.wVersion), 
      LOBYTE(wsadata.wVersion),
      wsadata.szDescription);

   message_printf(IP_NOTICE, "Initializing host %s, address %s",
      local_hostname,
      ipnum);
  
   TCP_socket = ::socket(PF_INET, SOCK_STREAM, 0);
  
   if (TCP_socket == INVALID_SOCKET) 
      {
      message_printf(IP_ERROR, "Error: invalid socket (%s)", error_text(msg,sizeof(msg)));
      return;
      }

   if (!parse_address(IP_address, default_port, &server_addr))
      {
      message_printf(IP_ERROR, "Host '%s' not found (code %d)", IP_address, WSAGetLastError());
      return;
      }

   message_printf(IP_NOTICE, "Attempting to connect to server %s:%d . . .",
           [&]() {
               char ip_buffer_local[INET_ADDRSTRLEN];
               return inet_ntop(AF_INET, &server_addr.sin_addr, ip_buffer_local, sizeof(ip_buffer_local)) ? ip_buffer_local : "UnknownIP";
           }(),
           ntohs(server_addr.sin_port));
  
   S32 result = ::connect(TCP_socket,
            (sockaddr *) &server_addr,
                          sizeof(server_addr));
  
   if (result) 
      {
      message_printf(IP_ERROR, "Connection failed: %s", error_text(msg,sizeof(msg))); 
      return;
      }
  
   DWORD dwVal = !IPCONN_NAGLE;
  
   setsockopt(TCP_socket,    
              IPPROTO_TCP,
              TCP_NODELAY,
      (C8 *) &dwVal,
              sizeof(dwVal));
  
   set_blocking_mode(FALSE);
  
   set_send_window(IPCONN_SO_XMITBUF_BYTES);
   set_recv_window(IPCONN_SO_RECVBUF_BYTES);

   local = FALSE;

   if (!_strnicmp(IP_address,"localhost",9))
      {
      local = TRUE;
      }
   else
      {
      U32 remote_IP = ntohl(server_addr.sin_addr.S_un.S_addr);
      U32 local_IP  = ntohl(inaddrIP.S_un.S_addr);

      local = (remote_IP & 0xffffff00) == (local_IP & 0xffffff00);

      if (remote_IP == ((127 << 24) | (0 << 16) | (0 << 8) | 1))
         {
         local = TRUE;   
         }
      }

   address_string(&server_addr, remote_hostname, sizeof(remote_hostname));

   rcv_bufsize = receive_buffer_size;

   rcv_buf = (C8 *) malloc(rcv_bufsize);

   if (rcv_buf == NULL)
      {
      message_printf(IP_ERROR, "Out of memory");
      return;
      }

   last_recv_time  = timer.us();
   connection_time = last_recv_time;

   message_printf(IP_NOTICE, "Connected to server %s:%d",
           [&]() {
               char ip_buffer_local[INET_ADDRSTRLEN];
               return inet_ntop(AF_INET, &server_addr.sin_addr, ip_buffer_local, sizeof(ip_buffer_local)) ? ip_buffer_local : "UnknownIP";
           }(),
           ntohs(server_addr.sin_port));

   active = TRUE;
   on_connect();
}

void IPCONN::server_accept(sockaddr_in *who, 
                           SOCKET       requestor,
                           S32          receive_buffer_size)
{
   if (active)
      {
      return;
      }

   struct addrinfo hints, *res, *p;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;        
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;      

   struct in_addr inaddrIP;
   inaddrIP.s_addr = INADDR_NONE;

   if (gethostname(local_hostname, sizeof(local_hostname)-1) == SOCKET_ERROR) 
      {
      message_printf(IP_ERROR, "Local hostname unobtainable");
      return;
      }
  
   if (getaddrinfo(local_hostname, NULL, &hints, &res) == 0) {
       for (p = res; p != NULL; p = p->ai_next) {
           if (p->ai_family == AF_INET) {
               inaddrIP = ((struct sockaddr_in*)p->ai_addr)->sin_addr; 
               break;
           }
       }
       freeaddrinfo(res);
   }
  
   message_printf(IP_VERBOSE, "Server \"%s\", address %s",
      local_hostname,
      ipnum);

   TCP_socket = requestor;

   DWORD dwVal = !IPCONN_NAGLE;
  
   setsockopt(TCP_socket,    
              IPPROTO_TCP,
              TCP_NODELAY,
      (C8 *) &dwVal,
              sizeof(dwVal));
  
   dwVal = 1;

   ioctlsocket(TCP_socket,   
               FIONBIO,
               &dwVal);
  
   set_send_window(IPCONN_SO_XMITBUF_BYTES);
   set_recv_window(IPCONN_SO_RECVBUF_BYTES);

   local = FALSE;

   U32 remote_IP = ntohl(who->sin_addr.S_un.S_addr);
   U32 local_IP  = ntohl(inaddrIP.S_un.S_addr);

   local = (remote_IP & 0xffffff00) == (local_IP & 0xffffff00);

   if (remote_IP == ((127 << 24) | (0 << 16) | (0 << 8) | 1))
      {
      local = TRUE;   
      }

   server_addr = *who;
   address_string(&server_addr, remote_hostname, sizeof(remote_hostname));

   rcv_bufsize = receive_buffer_size;

   rcv_buf = (C8 *) malloc(rcv_bufsize);

   if (rcv_buf == NULL)
      {
      message_printf(IP_ERROR, "Out of memory");
      return;
      }

   last_recv_time  = timer.us();
   connection_time = last_recv_time;

   message_printf(IP_VERBOSE, "Incoming connection from %s:%d",
           [&]() {
               char ip_buffer_local[INET_ADDRSTRLEN];
               return inet_ntop(AF_INET, &who->sin_addr, ip_buffer_local, sizeof(ip_buffer_local)) ? ip_buffer_local : "UnknownIP";
           }(),
           ntohs(who->sin_port));

   active = TRUE;
   on_connect();
}

C8 *IPCONN::error_text(C8 *text, S32 text_array_size, DWORD last_error)
{
   LPVOID lpMsgBuf = "Hosed";       

   last_error_code = (last_error == 0) ? GetLastError() : last_error;

   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
                 NULL,
                 last_error_code,
                 0,
       (LPTSTR) &lpMsgBuf, 
                 0, 
                 NULL);

   memset(text, 0, text_array_size);
   strncpy(text, (C8 *) lpMsgBuf, text_array_size-1);

   if (strcmp((C8 *) lpMsgBuf,"Hosed"))
      {
      LocalFree(lpMsgBuf);
      }

   return text;
}

bool IPCONN::monitor(S32 bytes_so_far, S32 bytes_total)
{
   return TRUE;
}

void IPCONN::message_sink(IPMSGLVL level,   
                          C8      *text)
{
   ::printf("%s\n",text);
}

void IPCONN::message_printf(IPMSGLVL level,
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
      if (!isspace((U8) *end)) 
         {
         break;
         }

      *end = 0;
      end--;
      }

   message_sink(level,
                buffer);
}

S32 IPCONN::user_word(void)
{
   return user_int;
}

S32 IPCONN::set_user_word(S32 val)
{
   S32 prev = user_int;
   user_int = val;
   return prev;
}

void *IPCONN::user_ptr(void)
{
   return user_void;
}

void *IPCONN::set_user_ptr(void *val)
{
   void *prev = user_void;
   user_void = val;
   return prev;
}

void IPCONN::set_blocking_mode(bool mode)
{
   if (TCP_socket == INVALID_SOCKET)
      {
      return;
      }

   blocking_mode = mode;

   DWORD dwVal = blocking_mode ? 0:1;

   ioctlsocket(TCP_socket,   
               FIONBIO,
              &dwVal);
}

S32 IPCONN::get_send_window(void)
{
   S32 result = 0;
   socklen_t slen = sizeof(result);

   getsockopt(TCP_socket, 
              SOL_SOCKET,
              SO_SNDBUF,
      (C8 *) &result,
             &slen);

   return result;
}

S32 IPCONN::get_recv_window(void)
{
   S32 result = 0;
   socklen_t slen = sizeof(result);

   getsockopt(TCP_socket, 
              SOL_SOCKET,
              SO_RCVBUF,
      (C8 *) &result,
             &slen);

   return result;
}

void IPCONN::set_send_window(S32 bytes)
{
   setsockopt(TCP_socket,
              SOL_SOCKET,
              SO_SNDBUF,
      (C8 *) &bytes,
              sizeof(bytes));
}

void IPCONN::set_recv_window(S32 bytes)
{
   setsockopt(TCP_socket,
              SOL_SOCKET,
              SO_RCVBUF,
      (C8 *) &bytes,
              sizeof(bytes));
}

bool IPCONN::status(void)
{
   return active;
}

S32 IPCONN::last_error(void)
{
   return last_error_code;
}

void IPCONN::host_info(C8          **local_name, 
                       C8          **remote_name, 
                       sockaddr_in **remote_addr)
{
   if (local_name  != NULL) *local_name  =  local_hostname;
   if (remote_name != NULL) *remote_name =  remote_hostname;
   if (remote_addr != NULL) *remote_addr = &server_addr;
}

void IPCONN::update_remote_hostname(C8 *new_remote_hostname)
{
   memset(remote_hostname, 0, sizeof(remote_hostname));
   strncpy(remote_hostname, new_remote_hostname, sizeof(remote_hostname)-1);
}

S64 IPCONN::statistics(S32 *bytes_received,
                       S32 *bytes_sent,
                       S32 *xmit_bit_rate,
                       S32 *recv_bit_rate)
{
   if (bytes_received != NULL) *bytes_received = recv_byte_count;
   if (bytes_sent     != NULL) *bytes_sent     = xmit_byte_count;

   S64 age = timer.us() - connection_time;
   if (age < 1) age = 1;

   if (xmit_bit_rate != NULL)
      {
      *xmit_bit_rate = (S32) (((S64) 8000000 * (S64) xmit_byte_count) / age);
      }

   if (recv_bit_rate != NULL)
      {
      *recv_bit_rate = (S32) (((S64) 8000000 * (S64) recv_byte_count) / age);
      }

   return age;
}

C8 *IPCONN::age_string(C8  *text,  
                       S32  text_array_size)     
{
   assert(text != NULL);
   memset(text, 0, text_array_size);

   S64 age = timer.us() - connection_time;
   if (age < 1) age = 1;

   return timer.duration_string(age, text, text_array_size);
}

bool IPCONN::is_protected(void)
{
   return FALSE;
}

bool IPCONN::is_local(void)
{
   return local;
} 

S64 IPCONN::us_since_recv(void)
{
   return timer.us() - last_recv_time;
}

SOCKET IPCONN::socket(void)
{
   return TCP_socket;
}

void IPCONN::on_connect(void)
{
}

void IPCONN::on_lengthy_operation(void)
{
}

void IPCONN::on_server_discard(IPDREASON reason)
{
}

bool IPCONN::send_block(void *block, 
                        S32   bytes_to_send,
                        S32  *sent_bytes, 
                        S32   max_send_retries,
                        S32   send_retry_delay_ms,
                        bool  disconnect_on_failure)
{
   C8  msg[256] = { 0 };
   S32 sent     = 0;
   S32 n_bytes  = bytes_to_send;

   if (sent_bytes != NULL) 
      {
      *sent_bytes = 0;
      }

   if ((!active) || (block == NULL) || (n_bytes == 0))
      {
      return FALSE;    
      }

   S32 retry_count = 0;

   for (;;)
      {
      S32 result = send(TCP_socket,
                 (C8 *) block,
                        n_bytes,
                        0);

      if (result != SOCKET_ERROR)
         {
         xmit_byte_count += result;

         block   = ((C8 *) block) + result;
         n_bytes -= result;
         sent    += result;

         if (sent_bytes != NULL) 
            {
            *sent_bytes = sent;
            }

         if (n_bytes <= 0)
            {
            break;                     // All bytes have been sent
            }
         }
      else
         {
         if (sent_bytes != NULL) 
            {
            *sent_bytes = sent;
            }

         S32 WSA_error = WSAGetLastError();

         if (WSA_error != WSAEWOULDBLOCK)
            {
            if ((WSA_error == 0) || (WSA_error == WSAECONNRESET) || (WSA_error == WSAECONNABORTED))
               message_printf(IP_DISCON, "send() disconnected: %s", error_text(msg,sizeof(msg), WSA_error)); 
            else
               message_printf(IP_ERROR, "send() failed: %s", error_text(msg,sizeof(msg), WSA_error)); 

            active = FALSE;
            return FALSE;
            }
         }

      if (++retry_count > max_send_retries)
         {
         break;                        
         }

      on_lengthy_operation();
      Sleep(send_retry_delay_ms);
      }

   if (retry_count > max_send_retries)
      {
      message_printf(IP_ERROR, "send() failed with WSAEWOULDBLOCK, %d retries exhausted", max_send_retries);

      if (disconnect_on_failure) 
         {
         active = FALSE;
         }
      }
   else if (retry_count > 0)
      {
      message_printf(IP_DEBUG, "send() buffers unavailable until %d of %d retries", retry_count, max_send_retries);
      }

   return (sent == bytes_to_send);
}

bool IPCONN::send_printf(C8 *fmt,
                         ...)
{
   if (!active)
      {
      return FALSE;
      }

   C8 buffer[4096] = { 0 };

   va_list ap;

   va_start(ap, 
            fmt);

   _vsnprintf(buffer,
              sizeof(buffer)-1,
              fmt,
              ap);

   va_end(ap);

   return send_block(buffer, (S32)strlen(buffer));
}

S32 IPCONN::receive_poll(S32 *fetched_this_call)
{
   C8 msg[256] = { 0 };

   if (fetched_this_call != NULL)
      {
      *fetched_this_call  = 0;
      }

   if (!active) 
      {
      return 0;
      }

   C8 *seg[2] = { NULL, NULL };
   S32 len[2] = { 0,    0    };

   if (rcv_head < rcv_tail)
      {
      seg[0] = &rcv_buf[rcv_head];
      len[0] = (rcv_tail - rcv_head)-1;     
      }
   else
      {
      if (rcv_tail == 0)                    
         {
         seg[0] = &rcv_buf[rcv_head];
         len[0] = (rcv_bufsize - rcv_head)-1; 
         }
      else
         {
         seg[0] = &rcv_buf[rcv_head];
         len[0] =  rcv_bufsize - rcv_head;  

         seg[1] = rcv_buf;
         len[1] = rcv_tail-1;               
         }
      }

   S32 bytes_fetched = 0;

   for (S32 s=0; s < 2; s++)
      {
      if (len[s] == 0)
         {
         continue;
         }

      S32 result = recv(TCP_socket,
                 (C8 *) seg[s],
                        len[s],
                        0);

      DWORD WSA_error = WSAGetLastError();

      if (result == 0)                 
         {
         if ((WSA_error == 0) || (WSA_error == WSAECONNRESET) || (WSA_error == WSAECONNABORTED))
            message_printf(IP_DISCON, "recv() disconnected: %s", error_text(msg,sizeof(msg), WSA_error)); 
         else
            message_printf(IP_ERROR, "recv() error: %s", error_text(msg,sizeof(msg), WSA_error)); 
            
         active = FALSE;
         return 0;
         }

      if (result != SOCKET_ERROR)      
         {
         recv_byte_count += result;
         last_recv_time = timer.us();
         }
      else
         {
         result = 0;                   

         if (WSA_error != WSAEWOULDBLOCK)
            {
            if ((WSA_error == 0) || (WSA_error == WSAECONNRESET) || (WSA_error == WSAECONNABORTED))
               message_printf(IP_DISCON, "recv() disconnected: %s", error_text(msg,sizeof(msg), WSA_error)); 
            else
               message_printf(IP_ERROR, "recv() failed: %s", error_text(msg,sizeof(msg), WSA_error)); 

            active = FALSE;
            return 0;
            }
         }

      bytes_fetched += result;
      rcv_head += result;

      if (rcv_head >= rcv_bufsize)
         {
         rcv_head -= rcv_bufsize;
         }

      if (result < len[s])   
         {
         break;
         }
      }

   if (fetched_this_call  != NULL)
      {
      *fetched_this_call = bytes_fetched;
      }

   if (rcv_head >= rcv_tail)
      return rcv_head - rcv_tail;
   else
      return rcv_head + rcv_bufsize - rcv_tail;
}

S32 IPCONN::read_block(void *dest, S32 n_bytes)
{
   if ((!active) || (n_bytes == 0))
      {
      return 0;                                       
      }

   if (rcv_head == rcv_tail)
      {
      receive_poll();                                 
      }                                               
                                                      
   C8 *d = (C8 *) dest;

   if (rcv_head >= rcv_tail)                  
      {
      n_bytes = std::min(rcv_head - rcv_tail, n_bytes);

      if (d != NULL)
         {
         memcpy(d, &rcv_buf[rcv_tail], n_bytes);      
         }
      }
   else
      {
      n_bytes = std::min(rcv_head + rcv_bufsize - rcv_tail, n_bytes);
      S32 n = std::min(n_bytes, rcv_bufsize - rcv_tail);

      if (d != NULL)
         {
         memcpy(d,  &rcv_buf[rcv_tail], n);           
         memcpy(d+n, rcv_buf,           n_bytes-n);   
         }
      }

   rcv_tail += n_bytes;

   if (rcv_tail >= rcv_bufsize)
      {
      rcv_tail -= rcv_bufsize;
      }

   return n_bytes;
}

bool IPCONN::read_block(void *dest, S32 n_bytes, S32 timeout_ms)
{
   S32 start = timeGetTime();

   for (;;)
      {
      S32 avail = read_block(dest, n_bytes);

      n_bytes -= avail;

      if (n_bytes == 0)
         {
         return TRUE;
         }

      S32 end = timeGetTime();

      if ((end - start) >= timeout_ms)
         {
         return FALSE;
         }

      dest = (U8 *) dest + avail;
      Sleep(0);
      }
}

bool IPCONN::read_text_file(FILE   *outfile,
                            C8     *command_string,    
                            C8     *command_term,
                            S32     timeout_msec,    
                            S32     dropout_msec,
                            S32     repeat_cnt,       
                            S32     EOS,
                            S32     max_line_chars)
{                   
   if (!active)
      {
      return FALSE;
      }

   S64 timeout_usec = (S64) timeout_msec * 1000LL;

   if (command_string != NULL)
      {
      message_printf(IP_NOTICE, "Transmitting \"%s\" . . .", command_string);
      send_printf("%s%s", command_string, command_term);
      }

   C8 *incoming_line = (C8 *) alloca(max_line_chars);

   if (incoming_line == NULL)
      {
      message_printf(IP_ERROR, "No room for %d-character reply buffer", max_line_chars);
      return FALSE;
      }

   S32 len = 0;
   S32 lines_received = 0;
   S32 bytes_received = 0;

   S64 start_time = timer.us();
   S64 last_line_time = start_time;

   bool done = FALSE;

   while (!done)
      {
      if ((timeout_usec > 0) && ((timer.us() - last_line_time) >= timeout_usec))
         {
         break;
         }

      if (!active)
         {
         return FALSE;
         }

      S32 avail = receive_poll();

      if (avail == 0)
         {
         Sleep(10);
         continue;
         }

      if (dropout_msec > 0)
         {
         timeout_usec = (S64) dropout_msec * 1000LL;
         }
      
      bytes_received += avail;

      for (S32 i=0; i < avail; i++)
         {
         if ((timeout_usec > 0) && ((timer.us() - last_line_time) >= timeout_usec))
            {
            done = 1;
            break;
            }

         C8 ch = 0;
         assert(read_block(&ch, 1));

         incoming_line[len++] = ch;

         if ((ch == EOS) || (len == max_line_chars-1))
            {
            incoming_line[len] = 0;
            fprintf(outfile,"%s",incoming_line);
            message_printf(IP_VERBOSE,"%s", incoming_line);
            len = 0;

            last_line_time = timer.us();

            if (!monitor(bytes_received, -1))
               {
               disconnect();
               return FALSE;
               }

            if ((++lines_received >= repeat_cnt) && (repeat_cnt != 0))
               {
               done = 1;
               break;
               }
            }
         }
      }

   if (lines_received > 0)
      {
      DOUBLE Hz = ((DOUBLE) lines_received * 1E6) / ((DOUBLE) (timer.us() - start_time));
      message_printf(IP_NOTICE,"%d line(s) received (average rate = %.1lf lines/sec)",lines_received, Hz);
      }

   return TRUE;
}

bool IPCONN::read_text_line(C8       *incoming_line,
                            S32       incoming_max_bytes,
                            const C8 *command_string,    
                            const C8 *command_term,
                            S32       timeout_msec,    
                            S32       dropout_msec,
                            S32       EOS)
{                   
   if (!active)
      {
      return FALSE;
      }

   S64 timeout_usec = (S64) timeout_msec * 1000LL;

   if (command_string != NULL)
      {
      send_printf("%s%s", command_string, command_term);
      }

   memset(incoming_line, 0, incoming_max_bytes);

   S32 len = 0;
   S32 bytes_received = 0;

   S64 start_time   = timer.us();
   S64 last_rx_time = start_time;

   bool done   = FALSE;
   bool result = FALSE;

   while (!done)
      {
      if ((timeout_usec > 0) && ((timer.us() - last_rx_time) >= timeout_usec))
         {
         result = FALSE;
         break;
         }

      if (!active)
         {
         result = FALSE;
         break;
         }

      S32 avail = receive_poll();

      if (avail == 0)
         {
         Sleep(10);
         continue;
         }

      last_rx_time = timer.us();

      if (dropout_msec > 0)
         {
         timeout_usec = (S64) dropout_msec * 1000LL;
         }
      
      bytes_received += avail;

      for (S32 i=0; i < avail; i++)
         {
         if ((timeout_usec > 0) && ((timer.us() - last_rx_time) >= timeout_usec))
            {
            done   = TRUE;
            result = FALSE;
            break;
            }

         C8 ch = 0;
         assert(read_block(&ch, 1));

         incoming_line[len++] = ch;

         if ((ch == EOS) || (len == incoming_max_bytes-1))
            {
            incoming_line[len] = 0;
            done   = TRUE;
            result = TRUE;
            break;
            }
         }
      }

   return result;
}

// *****************************************************************************
// UDPXCVR implementation
// *****************************************************************************

UDPXCVR::UDPXCVR(S32 _listen_port)
{
   UDP_socket    = INVALID_SOCKET;
   active        = FALSE;
   blocking_mode = FALSE;
   listen_port   = _listen_port;

   memset(&remote_addr,    0, sizeof(remote_addr));
   memset(local_hostname,  0, sizeof(local_hostname));
   memset(remote_hostname, 0, sizeof(remote_hostname));
   memset(ipnum,           0, sizeof(ipnum));

   ws_started      = FALSE;
   last_error_code = 0;
   user_int        = 0;
   user_void       = NULL;
}

UDPXCVR::~UDPXCVR()
{
   shutdown();
}

void UDPXCVR::shutdown(void)
{
   active = FALSE;

   if (UDP_socket != INVALID_SOCKET)
      {
      closesocket(UDP_socket);
      UDP_socket = INVALID_SOCKET;
      }

   if (ws_started)
      {
      WSACleanup();
      ws_started = FALSE;
      }
}

bool UDPXCVR::startup(void)
{
   C8 msg[256] = { 0 };

   if (ws_started || active)
      {
      return FALSE;
      }

   WORD wRequestVer = MAKEWORD(2,2);

   WSADATA wsadata;
   memset(&wsadata, 0, sizeof(wsadata));

   if (WSAStartup(wRequestVer, &wsadata))
      {
      return FALSE;
      }

   ws_started = TRUE;

   struct addrinfo hints, *res, *p;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;        
   hints.ai_socktype = SOCK_DGRAM;   
   hints.ai_flags = AI_PASSIVE;      

   struct in_addr inaddrIP;
   inaddrIP.s_addr = INADDR_NONE;

   if (gethostname(local_hostname, sizeof(local_hostname)-1) == SOCKET_ERROR) 
      {
      message_printf(IP_ERROR, "Local hostname unobtainable");
      return FALSE;
      }
  
   if (getaddrinfo(local_hostname, NULL, &hints, &res) == 0) {
       for (p = res; p != NULL; p = p->ai_next) {
           if (p->ai_family == AF_INET) {
               inaddrIP = ((struct sockaddr_in*)p->ai_addr)->sin_addr;
               break;
           }
       }
       freeaddrinfo(res);
   }
  
   message_printf(IP_VERBOSE, "Using Windows Sockets V%d.%d (%s)",
      HIBYTE(wsadata.wVersion),
      LOBYTE(wsadata.wVersion),
      wsadata.szDescription);

   message_printf(IP_NOTICE, "Initializing host %s, address %s",
      local_hostname,
      ipnum);

   UDP_socket = ::socket(PF_INET, 
                         SOCK_DGRAM, 
                         0);

   if (UDP_socket == INVALID_SOCKET)
      {
      message_printf(IP_ERROR, "Error: invalid socket (%s)", error_text(msg,sizeof(msg)));
      return FALSE;
      }

   sockaddr_in recv_addr;
   memset(&recv_addr, 0, sizeof(recv_addr));

   recv_addr.sin_family      = AF_INET;
   recv_addr.sin_port        = htons((S16) listen_port);
   recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

   S32 result = bind(UDP_socket,
       (sockaddr *) &recv_addr,
                     sizeof(recv_addr));

   if (result)
      {
      message_printf(IP_ERROR, "bind() failed: %s", error_text(msg,sizeof(msg))); 
      return FALSE;
      }

   active = TRUE;

   set_blocking_mode(TRUE);
   return TRUE;
}

C8 *UDPXCVR::error_text(C8 *text, S32 text_array_size, DWORD last_error)
{
   LPVOID lpMsgBuf = "Hosed";       

   last_error_code = (last_error == 0) ? GetLastError() : last_error;

   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL,
                 last_error_code,
                 0,
       (LPTSTR) &lpMsgBuf,
                 0,
                 NULL);

   memset(text, 0, text_array_size);
   strncpy(text, (C8 *) lpMsgBuf, text_array_size-1);

   if (strcmp((C8 *) lpMsgBuf,"Hosed"))
      {
      LocalFree(lpMsgBuf);
      }

   return text;
}

void UDPXCVR::message_sink(IPMSGLVL level,   
                           C8      *text)
{
   ::printf("%s\n",text);
}

void UDPXCVR::message_printf(IPMSGLVL level,
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
      if (!isspace((U8) *end)) 
         {
         break;
         }

      *end = 0;
      end--;
      }

   message_sink(level,
                buffer);
}

S32 UDPXCVR::user_word(void)
{
   return user_int;
}

S32 UDPXCVR::set_user_word(S32 val)
{
   S32 prev = user_int;
   user_int = val;
   return prev;
}

void *UDPXCVR::user_ptr(void)
{
   return user_void;
}

void *UDPXCVR::set_user_ptr(void *val)
{
   void *prev = user_void;
   user_void = val;
   return prev;
}

void UDPXCVR::set_blocking_mode(bool mode)
{
   if (!active)
      {
      if (!startup())
         {
         return;
         }
      }

   blocking_mode = mode;

   DWORD dwVal = blocking_mode ? 0:1;

   ioctlsocket(UDP_socket,   
               FIONBIO,
              &dwVal);
}

void UDPXCVR::set_timeout_ms(S32 ms)
{
   if (!active)
      {
      if (!startup())
         {
         return;
         }
      }

   setsockopt(UDP_socket,    
              SOL_SOCKET,
              SO_RCVTIMEO,
      (C8 *) &ms,
              sizeof(ms));
}

bool UDPXCVR::status(void)
{
   return active;
}

S32 UDPXCVR::last_error(void)
{
   return last_error_code;
}

SOCKET UDPXCVR::socket(void)
{
   if (!active)
      {
      if (!startup())
         {
         return INVALID_SOCKET;
         }
      }

   return UDP_socket;
}

void UDPXCVR::on_lengthy_operation(void)
{
}

bool UDPXCVR::set_remote_address(const C8  *IP_address,
                                S32        default_port)         
{
   C8 msg[256] = { 0 };

   if (!active)
      {
      if (!startup())
         {
         return FALSE;
         }
      }

   C8 remote_address_name[1024] = { 0 };
   char port_str[16];
   S32 remote_port_num = default_port;
  
   strncpy(remote_address_name, IP_address, sizeof(remote_address_name)-1);
  
   C8 *colon = strrchr(remote_address_name,':');
  
   if (colon != NULL) {
       *colon = '\0';
       remote_port_num = atoi(colon + 1);
   }

   colon = strchr(remote_address_name, ':');
   if (colon != NULL) { 
       *colon = '\0';
   }

   _snprintf(port_str, sizeof(port_str), "%d", remote_port_num);

   memset(&remote_addr,0,sizeof(remote_addr));
  
   struct addrinfo hints, *result, *ptr;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;        
   hints.ai_socktype = SOCK_DGRAM;   
   hints.ai_protocol = IPPROTO_UDP;

   int status = getaddrinfo(remote_address_name, port_str, &hints, &result);
   if (status != 0) {
       message_printf(IP_ERROR, "Host '%s' not found (error %d)", remote_address_name, status);
       return FALSE;
   }

   memcpy(&remote_addr, result->ai_addr, result->ai_addrlen);
   freeaddrinfo(result);
  
   message_printf(IP_NOTICE, "UDP remote address: %s:%d", 
           [&]() {
               char ip_buffer_local[INET_ADDRSTRLEN];
               return inet_ntop(AF_INET, &remote_addr.sin_addr, ip_buffer_local, sizeof(ip_buffer_local)) ? ip_buffer_local : "UnknownIP";
           }(),
           ntohs(remote_addr.sin_port));
  
   IPCONN::address_string(&remote_addr, remote_hostname, sizeof(remote_hostname));
   
   S32 conn_result = ::connect(UDP_socket,
            (sockaddr *) &remote_addr,
                          sizeof(remote_addr));
  
   if (conn_result) 
      {
      message_printf(IP_ERROR, "UDP address association failed: %s", error_text(msg,sizeof(msg))); 
      return FALSE;
      }

   return TRUE;
}

bool UDPXCVR::send_block(void       *block, 
                        S32         bytes_to_send,
                        sockaddr   *dest,
                        S32         dest_size,
                        S32        *sent_bytes, 
                        S32         max_send_retries,
                        S32         send_retry_delay_ms,
                        bool        disconnect_on_failure)
{
   C8 msg[256] = { 0 };
   S32 sent    = 0;
   S32 n_bytes = bytes_to_send;

   if (sent_bytes != NULL) 
      {
      *sent_bytes = 0;
      }

   if ((!active) || (block == NULL) || (n_bytes == 0))
      {
      return FALSE;    
      }

   S32 retry_count = 0;

   for (;;)
      {
      S32 result = sendto(UDP_socket,
                   (C8 *) block,
                          n_bytes,
                          0,
                          dest,
        (dest_size > 0) ? dest_size : sizeof(*dest));

      if (result != SOCKET_ERROR)
         {
         block   = ((C8 *) block) + result;
         n_bytes -= result;
         sent    += result;

         if (sent_bytes != NULL) 
            {
            *sent_bytes = sent;
            }

         if (n_bytes <= 0)
            {
            break;            
            }
         }
      else
         {
         if (sent_bytes != NULL) 
            {
            *sent_bytes = sent;
            }

         S32 WSA_error = WSAGetLastError();

         if (WSA_error != WSAEWOULDBLOCK)
            {
            message_printf(IP_ERROR, "sendto() failed: %s", error_text(msg,sizeof(msg), WSA_error)); 

            active = FALSE;
            return FALSE;
            }
         }

      if (++retry_count > max_send_retries)
         {
         break;
         }

      on_lengthy_operation();
      Sleep(send_retry_delay_ms);
      }

   if (retry_count > max_send_retries)
      {
      message_printf(IP_ERROR, "sendto() failed with WSAEWOULDBLOCK, %d retries exhausted", max_send_retries);
      
      if (disconnect_on_failure) 
         {
         active = FALSE;
         }
      }
   else if (retry_count > 0)
      {
      message_printf(IP_DEBUG, "sendto() returned WSAEWOULDBLOCK (succeeded after %d of %d retries)", retry_count, max_send_retries); 
      }

   return (sent == bytes_to_send);
}

bool UDPXCVR::send_printf(C8 *fmt,
                         ...)
{
   if (!active)
      {
      return FALSE;
      }

   C8 buffer[4096] = { 0 };

   va_list ap;

   va_start(ap, 
            fmt);

   _vsnprintf(buffer,
              sizeof(buffer)-1,
              fmt,
              ap);

   va_end(ap);

   return send_block(buffer, (S32)strlen(buffer));
}

bool UDPXCVR::send_printf(sockaddr *dest,
                         S32       dest_size, 
                         C8       *fmt,
                         ...)
{
   if (!active)
      {
      return FALSE;
      }

   C8 buffer[4096] = { 0 };

   va_list ap;

   va_start(ap, 
            fmt);

   _vsnprintf(buffer,
              sizeof(buffer)-1,
              fmt,
              ap);

   va_end(ap);

   return send_block(buffer, (S32)strlen(buffer), dest, dest_size);
}

S32 UDPXCVR::read_block(void     *dest,
                       S32       n_bytes,
                       sockaddr *sender)
{
   if ((!active) || (n_bytes == 0))
      {
      return 0;
      }

   S32 result = 0;

   if (sender == NULL)              
      {                             
      result = recv(UDP_socket, 
             (C8 *) dest,
                    n_bytes,
                    0);
      }
   else
      {
      S32 sz = sizeof(*sender);

      result = recvfrom(UDP_socket,
                 (C8 *) dest,
                        n_bytes,
                        0,
                        sender,
                       &sz);
      }

   return result;
}
