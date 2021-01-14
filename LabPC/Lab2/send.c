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


int main(int argc,char** argv){
  init(HOST,PORT);
  msg t;
  int sursa, size, new_size;
  
  memset(t.payload, 0, MAX_LEN);

  sprintf(t.payload, "Am trimis primul mesaj");
  t.len = strlen(t.payload) + 1;
  send_message(&t);
  
  // Check response:
  if (recv_message(&t)<0){
    perror("Receive error ...");
    return -1;
  }
  else {
    printf("[send] Got reply with payload: %s\n", t.payload);
    
  }

  sursa = open("file", O_RDONLY);
  size = lseek(sursa, 0, SEEK_END);

  sprintf(t.payload, "%d", size);
  t.len = strlen(t.payload) + 1;
  send_message(&t); //s-a trimis al doilea mesaj


  lseek(sursa, 0, SEEK_SET);

   if (recv_message(&t)<0){
    perror("Receive error ...");
    return -1;
  }
  else {
    printf("[send] Got reply with payload: %s\n", t.payload);
    
  }

  while ((new_size = read(sursa, t.payload, MAX_LEN - 1))){
    if(new_size < 0){
      perror("Unable to read from input file\n");
    } else {
      t.len = new_size;
      send_message(&t);

      if (recv_message(&t) < 0){
        perror("receive error");
      }
      else {
        printf("[send] Got reply with payload: \nACK(%s)\n", t.payload);
      }

      memset(t.payload, 0, MAX_LEN);
    }

  }

  close(sursa);

  return 0;
}
