#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

char verificare(char msg[14]){
  char x = 0;
  int i;
  for(i = 0; i < 12; i++){
    x = x ^ msg[i];
  }

  return x;
}

int main(int argc,char** argv){
  msg r,t;
  init(HOST,PORT);

  int i, count=500, j=0;
  for(i = 0; i < count; i++){
    if(recv_message(&r) < 0){
      perror("ERROR");
      return -1;
    }
    else{
      printf("[recv] Message received: %s\n", r.payload);
      char res = verificare(r.payload);
      if (res == r.payload[12]) printf("E BUN!\n");
      else printf("NU E BUN!\n");
      sprintf(t.payload, "%s %d", "ACK", j);
      j++;
      t.len = strlen(t.payload) + 1;
      send_message(&t);
    }
  }
  return 0;
}
