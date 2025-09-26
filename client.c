#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>

/* simple client, takes two parameters, the server domain name,
and the server port number */

typedef struct msg_details {
  unsigned short sz;
  unsigned long time1;
  unsigned long time2;
} msg_details;

int main(int argc, char** argv) {
  struct timeval tvalAfter;
  gettimeofday (&tvalAfter, NULL);
  
  /* our client socket */
  int sock;
  
  /* variables for identifying the server */
  unsigned int server_addr;
  struct sockaddr_in sin;
  struct addrinfo *getaddrinfo_result, hints;
  
  /* convert server domain name to IP address */
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET; /* indicates we want IPv4 */
  
  if (getaddrinfo(argv[1], NULL, &hints, &getaddrinfo_result) == 0) {
    server_addr = (unsigned int) ((struct sockaddr_in *) (getaddrinfo_result->ai_addr))->sin_addr.s_addr;
    freeaddrinfo(getaddrinfo_result);
  }
  
  /* server port number */
  unsigned short server_port = atoi (argv[2]);

  /* size in bytes of each message to send*/
  unsigned int size_param = atoi (argv[3]); 

  /* number of message exchanges to perform */
  unsigned int count_param = atoi (argv[4]); 

  // just for making sure the message is being sent correctly, we can remove later
  char *debug_text = "";
  if (argc >= 6) {
    if (strcmp(argv[5], "-m") == 0) {
      if (argc >= 7) {
        debug_text = argv[6];
      }
    }
  }

  unsigned char *ping, *pong;
  ping = malloc(size_param);
  pong = malloc(size_param);

  uint16_t total_size = htons((uint16_t)size_param);
  memcpy(ping, &total_size, 2); 
  memset(ping + 18, 0, size_param - 18); // is this necessary? Isn't already 0 by default?

  if (argc >= 7) {
    memcpy(ping + 18, debug_text, strlen(debug_text));
  }
  
  /* create a socket */
  if ((sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
  {
    perror ("opening TCP socket");
    abort ();
  }
  
  /* fill in the server's address */
  memset (&sin, 0, sizeof (sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = server_addr;
  sin.sin_port = htons(server_port);
  
  /* connect to the server */
  if (connect(sock, (struct sockaddr *) &sin, sizeof (sin)) < 0)
  {
    perror("connect to server failed");
    abort();
  }

  // Now that we've successfully connected, let's send a ping to the server.
  for (unsigned int i = 0; i < count_param; i++) {
    // int ping_send = send(sock, input_buffer, size_param, 0);
    // int ping_send = send(sock, "PING", 5, 0);

    struct timeval tval_send;
    gettimeofday(&tval_send, NULL);

    uint64_t net_time1 = htobe64(tval_send.tv_sec);
    uint64_t net_time2 = htobe64(tval_send.tv_usec);
    memcpy(ping + 2, &net_time1, 8);
    memcpy(ping + 10, &net_time2, 8);

    unsigned int left_ping, left_pong;
    left_ping = size_param;
    left_pong = size_param;

    unsigned char *ping2, *pong2;
    ping2 = ping;
    pong2 = pong;

    while (left_ping > 0) {
      ssize_t ping_send = send(sock, ping2, left_ping, 0);

      if (ping_send > 0) {
        left_ping -= ping_send;
        ping2 += ping_send;
        continue;
      }

      if (ping_send == 0) {
        exit(1);
      }

      if (errno == EAGAIN || errno == EINTR) {
        continue;
      }

      exit(1);
    }

    while (left_pong > 0) {
      ssize_t pong_send = recv(sock, pong2, left_pong, 0);

      if (pong_send > 0) {
        left_pong -= pong_send;
        pong2 += pong_send;
        continue;
      }

      if (pong_send == 0) {
        exit(1);
      }

      if (errno == EAGAIN || errno == EINTR) {
        continue;
      }

      exit(1);
    }

    // let's record the receiving timestamp
    struct timeval tval_curr;
    gettimeofday(&tval_curr, NULL);

    // if (pong_receive <= 0) {
    //   printf("Error with receiving.\n");
    //   break;
    // }

    uint64_t orig_time1;
    memcpy(&orig_time1, pong + 2, 8);
    orig_time1 = be64toh(orig_time1);

    uint64_t orig_time2;
    memcpy(&orig_time2, pong + 10, 8);
    orig_time2 = be64toh(orig_time2);

    uint64_t final_time1, final_time2;
    final_time1 = (uint64_t) 1000000 * (uint64_t) orig_time1 + (uint64_t) orig_time2;
    final_time2 = (uint64_t) 1000000 * (uint64_t) tval_curr.tv_sec + (uint64_t) tval_curr.tv_usec;

    double curr_lat = (uint64_t) (final_time2 - final_time1) / (double) 1000;
    printf("%.3f", curr_lat);
    printf("\n");

    if (argc >= 7) {
      fwrite(pong + 18, 1, size_param - 18, stdout);
      printf("\n");
    }
    
  }

  return 0;
}
