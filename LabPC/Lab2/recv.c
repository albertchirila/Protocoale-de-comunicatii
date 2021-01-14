#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001


int main(int argc,char** argv){
  msg r, t;
  int sursa, size, new_size, rest;
  init(HOST,PORT);

  memset(r.payload, 0, MAX_LEN);
  memset(t.payload, 0, MAX_LEN);

  if (recv_message(&r)<0){
    perror("Receive message");
    return -1;
  }

  printf("[recv] Got msg with payload: %s\n", r.payload);

  sursa = open(r.payload, O_WRONLY | O_CREAT, 0777);

  sprintf(t.payload, "S-a primit primul mesaj");
  t.len = strlen(t.payload) + 1;
  send_message(&t);

  if (recv_message(&r)<0){
    perror("Receive message");
    return -1;
  }

  printf("[recv] Got msg with payload: %s\n", r.payload);

  sprintf(t.payload, "ACK(%s)", r.payload);
  t.len = strlen(t.payload);
  send_message(&t);

  rest = atoi(r.payload);
  printf("Input file size: %d\n", rest);

  while(rest) {
    if(recv_message(&r) < 0){
      perror("Receive message");
      return -1;
    }

    printf("[recv] Got msg with payload: \n%s\n", r.payload);

    new_size = write(sursa, r.payload, r.len);

    send_message(&r);
    rest = rest - r.len;

    memset(t.payload, 0, MAX_LEN);
  }

  close(sursa);

  
  return 0;
}
