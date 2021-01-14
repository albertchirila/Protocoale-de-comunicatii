#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10000
void verificare(char msg[14]){
  char x = 0;
  for (int i=0; i < 12; i++){
     x = x ^ msg[i]; 
  }
  msg[12] = x;

}

int main(int argc,char** argv){
  init(HOST,PORT);
  msg t;
  int i=0, count = 500, space;

  printf("%d %d\n", atoi(argv[1]), atoi(argv[2]));

  int BDP = atoi(argv[2]);
  int window_size = (BDP * 1000) / (200 * 8);
  space = window_size;

  printf("%d\n", window_size);

  for(i = 0; i < count; i++){
    if (space){
      sprintf(t.payload, "%s", "Hello world!");
      verificare(t.payload);
      t.len = strlen(t.payload) + 1;
      send_message(&t);
      space--;
    }
    else
      if(recv_message(&t)<0){
        perror("ERROR");
        return -1;
      }
      else
      {
        printf("[send] Got ACK: %s\n", t.payload);
        space++;
        i--;
      }
  }

  while(recv_message(&t)>0){
    printf("[send] Got ACK: %s\n", t.payload);
        space++;
    if(space == window_size) break;
  }
  return 0;
}
