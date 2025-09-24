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

  
  char *buffer, *sendbuffer;
  int size = 500;
  int count;
  int num;
  
  /* allocate a memory buffer in the heap */
  /* putting a buffer on the stack like:
  
  char buffer[500];
  
  leaves the potential for
  buffer overflow vulnerability */

  buffer = (char *) malloc(size);
  if (!buffer)
  {
    perror("failed to allocated buffer");
    abort();
  }

  char input_buffer[65536];
  
  // first 2 bytes for the data size
  uint16_t total_size = htons((uint16_t)size_param);
  memcpy(buffer, &total_size, 2); 

  // next 8 bytes for the datas time (seconds)
  uint64_t second = htons((uint16_t)tvalAfter.tv_sec); 
  memcpy(buffer, &second, 8); 

  // next 8 bytes for the datas time (milli-seconds)
  uint64_t milliseconds = htons((uint16_t)tvalAfter.tv_usec); 
  memcpy(buffer, &milliseconds, 8); 


  sendbuffer = (char *) malloc(size);
  if (!sendbuffer)
  {
    perror("failed to allocated sendbuffer");
    abort();
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

  // Quick sanity check
  // printf("Input txt is: \n");
  // printf(curr_msg.txt);
  // printf("\n");

  unsigned char msg_buf[size_param]; // the buffer we'll pass to send()

  uint16_t net_size_param = htons(size_param);
  
  memcpy(msg_buf, &net_size_param, 2);

  // memset(msg_buf + 18, 0, size_param - 18); resetting

  char inp_text[size_param - 17];

  fgets(inp_text, sizeof(inp_text), stdin);
  memcpy(msg_buf + 18, inp_text, size_param - 18);
  
  // fread(msg_buf + 18, 1, size_param - 18, stdin);

  // Now that we've successfully connected, let's send a ping to the server.
  for (unsigned int i = 0; i < count_param; i++) {
    // int ping_send = send(sock, input_buffer, size_param, 0);
    // int ping_send = send(sock, "PING", 5, 0);

    struct timeval tval_send;
    gettimeofday(&tval_send, NULL);

    uint64_t net_time1 = htobe64(tval_send.tv_sec);
    uint64_t net_time2 = htobe64(tval_send.tv_usec);
    memcpy(msg_buf + 2, &net_time1, 8);
    memcpy(msg_buf + 10, &net_time2, 8);

    // NOTE: need to do network-byte order stuff here

    // int ping_send = send(sock, &curr_details, sizeof(curr_details), 0);
    int ping_send = send(sock, msg_buf, size_param, 0);

    if (ping_send < 0) {
      printf("Error with sending.\n");
      break;
    }

    // Now let's receive the pong back.
    int pong_receive = recv(sock, buffer, size_param, MSG_WAITALL);
    // int pong_receive = recv(sock, buffer, 5, 0);

    // let's record the receiving timestamp
    struct timeval tval_curr;
    gettimeofday(&tval_curr, NULL);

    if (pong_receive <= 0) {
      printf("Error with receiving.\n");
      break;
    }

    uint64_t orig_time1;
    memcpy(&orig_time1, buffer + 2, 8);
    orig_time1 = be64toh(orig_time1);

    uint64_t orig_time2;
    memcpy(&orig_time2, buffer + 10, 8);
    orig_time2 = be64toh(orig_time2);

    uint64_t final_time1, final_time2;
    final_time1 = (uint64_t) 1000000 * (uint64_t) orig_time1 + (uint64_t) orig_time2;
    final_time2 = (uint64_t) 1000000 * (uint64_t) tval_curr.tv_sec + (uint64_t) tval_curr.tv_usec;

    double curr_lat = (uint64_t) (final_time2 - final_time1) / (double) 1000;
    printf("%f", curr_lat);
    printf("\n");

    fwrite(buffer + 18, 1, size_param - 18, stdout);
  }

  printf("got here client\n");

  return 0;

//   /* everything looks good, since we are expecting a
//   message from the server in this example, let's try receiving a
//   message from the socket. this call will block until some data
//   has been received */
//   count = recv(sock, buffer, size, 0);
//   if (count < 0)
//   {
//     perror("receive failure");
//     abort();
//   }
  
//   /* in this simple example, the message is a string, 
//   we expect the last byte of the string to be 0, i.e. end of string */
//   if (buffer[count-1] != 0)
//   {
//     /* In general, TCP recv can return any number of bytes, not
//     necessarily forming a complete message, so you need to
//     parse the input to see if a complete message has been received.
//     if not, more calls to recv is needed to get a complete message.
//     */
//     printf("Message incomplete, something is still being transmitted\n");
//   } 
//   else
//   {
//     // printf("Here is what we got: %s", buffer);
//     printf(buffer);
//   }
  
//   while (1){ 
//     printf("\nEnter the type of the number to send (options are char, short, int, or bye to quit): ");
//     fgets(buffer, size, stdin);
//     if (strncmp(buffer, "bye", 3) == 0) {
//       /* free the resources, generally important! */
//       close(sock);
//       free(buffer);
//       free(sendbuffer);
//       return 0;
//     }
    
//     /* first byte of the sendbuffer is used to describe the number of
//     bytes used to encode a number, the number value follows the first 
//     byte */
//     if (strncmp(buffer, "char", 4) == 0) {
//       sendbuffer[0] = 1;
//     } else if (strncmp(buffer, "short", 5) == 0) {
//       sendbuffer[0] = 2;
//     } else if (strncmp(buffer, "int", 3) == 0) {
//       sendbuffer[0] = 4;
//     } else {
//       printf("Invalid number type entered, %s\n", buffer);
//       continue;
//     }
    
//     printf("Enter the value of the number to send: ");
//     fgets(buffer, size, stdin);
//     num = atol(buffer);
    
//     switch(sendbuffer[0]) {
//       case 1:
//       *(char *) (sendbuffer+1) = (char) num;
//       break;
//       case 2:
//       /* for 16 bit integer type, byte ordering matters */
//       *(short *) (sendbuffer+1) = (short) htons(num);
//       break;
//       case 4:
//       /* for 32 bit integer type, byte ordering matters */
//       *(int *) (sendbuffer+1) = (int) htonl(num);
//       break;
//       default:
//       break;
//     }
//     // send(sock, sendbuffer, sendbuffer[0]+1, 0);
//     send(sock, "PING", 5, 0);
//   }
  
  return 0;
}
