#include <iostream>
#include <cstdlib>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#define BUFSIZE 2048


int existingsession(char * input) {
  return 1;
}

int main (int argc, char *argv[0]) {
  unsigned short port;
  struct sockaddr_in saddr, caddr;
  int sockfd, pid;
  char* buf, *cmd, *arg;
  int ret, i, j;
  socklen_t len;

  buf = (char*)malloc(BUFSIZE);
  memset(buf, 0, BUFSIZE);
  cmd = (char*)malloc(BUFSIZE);
  memset(cmd, 0, BUFSIZE);
  arg = (char*)malloc(BUFSIZE);
  memset(arg, 0, BUFSIZE);


  if (argc != 2) {
    printf("usage: %s <Port>\n", argv[0]);
    exit(1);
  }

  port = atoi(argv[1]);

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("cannot open socket\n");
    exit(1);
  }
  memset((char*)&saddr, 0, sizeof(saddr));
  memset((char*)&caddr, 0, sizeof(caddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  saddr.sin_port = htons(port);

  if (bind(sockfd, (struct sockaddr*)&saddr, sizeof(saddr)) < 0) {
    printf("cannot bind\n");
    exit(1);
  }
  printf("The chat coordinator is now on port %d\n", port);
  while (1) {
    ret = recv(sockfd, buf, BUFSIZE, 0);
    for(i=0;(buf[i]!=0x20)&&(buf[i]!=0x00)&&(buf[i]!='\n');i++);
    cmd = (char*)memcpy(cmd, buf, i);
    for(j=i+1;(buf[j]!=0x20)&&(buf[j]!=0x00)&&(buf[j]!='\n');j++);
    arg = (char*)memcpy(arg, buf+i+1, j-i);

    if (!strcmp(cmd, "Start")) {
      if (existingsession(arg)) {
        printf("this session already exists, exiting\n");
        close(sockfd);
        exit(1);
      }
      if (!(pid = fork())) {
        exit(0);
      }
      else {
        while((pid = waitpid(((pid_t) - 1), NULL, WNOHANG)) > 0);
      }
    }
    else {
      memset(buf, 0, BUFSIZE);
      memset(cmd, 0, BUFSIZE);
      memset(arg, 0, BUFSIZE);
    }
  }
return 0;
}
