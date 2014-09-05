#include <iostream>
#include <cstdlib>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#define BUFSIZE 2048
#define TCPSIZE 512

int main(int argc, char *argv[]) {
  int fd, pid, i, j;
  int childfd;
  int clientlen;
  struct sockaddr_in clientaddr;
  struct hostent *hostp;
  char *buf, *cmd, *arg;

  if (argc > 1) {
    printf("silly wall arguments\n");
  }
  buf = (char*)malloc(BUFSIZE);
  fd = atoi(argv[0]);
  clientlen = sizeof(clientaddr);
  if (listen(fd, 10) < 0) {
    printf("cannot listen\n");
  }
  while(1) {
    if ((childfd = accept(fd, (struct sockaddr *) &clientaddr, (socklen_t*)&clientlen)) < 0) {
      printf("cannot accept\n");
    }
    if (!(pid = fork())) {
      close(fd);
      hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
          sizeof(clientaddr.sin_addr.s_addr), AF_INET);
      if (hostp == NULL) {
        printf("cannot find host\n");
      }
      memset(buf, 0, BUFSIZE);
      read(childfd, buf, TCPSIZE);

      for(i=0;(buf[i]!=0x20)&&(buf[i]!=0x00)&&(buf[i]!='\n');i++);
      cmd = (char*)memcpy(cmd, buf, i);
      for(j=i+1;(buf[j]!=0x20)&&(buf[j]!=0x00)&&(buf[j]!='\n');j++);
      arg = (char*)memcpy(arg, buf+i+1, j-i);

      if (!strcmp(cmd, "Submit")) {
      }

      exit(0);
    }
    else {
      close(childfd);
      while ((pid = waitpid(((pid_t) - 1), NULL, WNOHANG)) > 0);
    }
  }
  close(fd);
  return 0;
}
