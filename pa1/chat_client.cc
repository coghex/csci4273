#include <iostream>
#include <cstdlib>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int c, i, j, k;
  int udpfd, tcpfd, n;
  struct sockaddr_in udpservaddr,udpcliaddr;
  struct sockaddr_in tcpservaddr,tcpcliaddr;
  char *udpsendbuf, *udprecbuf, *tcpsendbuf, *tcprecbuf;
  char *input, *cmd, *arg, *silly;
  char current[128];
  cmd = (char*)malloc(128);
  arg = (char*)malloc(128);
  input = (char*)malloc(128);
  udpsendbuf = (char*)malloc(128);
  udprecbuf = (char*)malloc(128);
  tcpsendbuf = (char*)malloc(128);
  tcprecbuf = (char*)malloc(2048);
  silly = (char*)malloc(128);

  if (argc<3) {
    printf("Usage: chat_client <hostname> <port>\n");
    exit(0);
  }

  //socket crap
  udpfd=socket(AF_INET, SOCK_DGRAM, 0);
  bzero(&udpservaddr, sizeof(udpservaddr));
  udpservaddr.sin_family = AF_INET;
  udpservaddr.sin_addr.s_addr = inet_addr(argv[1]);
  udpservaddr.sin_port=htons(atoi(argv[2]));
  tcpfd=socket(AF_INET,SOCK_STREAM,0);

  while(1) {
    i=0;
    memset(input, 0, 128);
    memset(cmd, 0, 128);
    memset(arg, 0, 128);
    memset(udpsendbuf, 0, 128);
    memset(udprecbuf, 0, 128);
    memset(tcpsendbuf, 0, 128);
    memset(tcprecbuf, 0, 2048);

    while(1) {
      c = getchar();
      if (c=='\n') {
        break;
      }
      else {
        input[i]=c;
        i++;
      }
    }
    for(j=0;(input[j]!='\n')&&(input[j]!=0x20);j++);
    cmd = (char *)memcpy(cmd, input, j);
    for(k=j+1;(input[k]!='\n')&&(input[k]!=0x00)&&(input[k]!=0x0d);k++);
    arg = (char*)memcpy(arg, input+j+1, k-j);

    if (!strcmp(cmd, "Start")) {
      strcpy(udpsendbuf, "Start ");
      strcat(udpsendbuf, arg);
      sendto(udpfd, udpsendbuf, strlen(udpsendbuf), 0,
          (struct sockaddr *)&udpservaddr,sizeof(udpservaddr));
      n=recvfrom(udpfd, udprecbuf, 128, 0, NULL, NULL);
      udprecbuf[n]=0;

      if (!strcmp(udprecbuf, "-1")) {
        printf("a server by that name is already running\n");
        continue;
      }
      bzero(&tcpservaddr,sizeof(tcpservaddr));
      tcpservaddr.sin_family = AF_INET;
      tcpservaddr.sin_addr.s_addr=inet_addr(argv[1]);
      tcpservaddr.sin_port=htons(atoi(udprecbuf));

      connect(tcpfd, (struct sockaddr *)&tcpservaddr, sizeof(tcpservaddr));
      memset(current, 0, 128);
      strcpy(current, arg);
      printf("A new chat session %s has been created and you have joined this session\n", arg);
    }
    if (!strcmp(cmd, "Join")) {
      strcpy(udpsendbuf, "Find ");
      strcat(udpsendbuf, arg);
      sendto(udpfd, udpsendbuf, strlen(udpsendbuf), 0,
          (struct sockaddr *)&udpservaddr,sizeof(udpservaddr));
      n=recvfrom(udpfd, udprecbuf, 128, 0, NULL, NULL);
      udprecbuf[n]=0;

      bzero(&tcpservaddr,sizeof(tcpservaddr));
      tcpservaddr.sin_family = AF_INET;
      tcpservaddr.sin_addr.s_addr=inet_addr(argv[1]);
      tcpservaddr.sin_port=htons(atoi(udprecbuf));

      connect(tcpfd, (struct sockaddr *)&tcpservaddr, sizeof(tcpservaddr));
      memset(current, 0, 128);
      strcpy(current, arg);
      printf("You have joined the chat session %s\n", arg);
    }
    if (!strcmp(cmd, "Submit")) {
      strcpy(tcpsendbuf, "Submit ");
      sprintf(silly, "%d ", (int)strlen(arg));
      strcat(tcpsendbuf, silly);
      strcat(tcpsendbuf, arg);
      sendto(tcpfd, tcpsendbuf, strlen(tcpsendbuf), 0,
          (struct sockaddr *)&tcpservaddr,sizeof(tcpservaddr));
      n=recvfrom(tcpfd, tcprecbuf, 2048, 0, NULL, NULL);
      tcprecbuf[n]=0;
    }
    if (!strcmp(cmd, "GetNext")) {
      strcpy(tcpsendbuf, "GetNext");
      sendto(tcpfd, tcpsendbuf, strlen(tcpsendbuf), 0,
          (struct sockaddr *)&tcpservaddr,sizeof(tcpservaddr));
      n=recvfrom(tcpfd, tcprecbuf, 2048, 0, NULL, NULL);
      tcprecbuf[n]=0;
      printf("%s", tcprecbuf);
    }
    if (!strcmp(cmd, "GetAll")) {
      strcpy(tcpsendbuf, "GetAll");
      sendto(tcpfd, tcpsendbuf, strlen(tcpsendbuf), 0,
          (struct sockaddr *)&tcpservaddr,sizeof(tcpservaddr));
      n=recvfrom(tcpfd, tcprecbuf, 2048, 0, NULL, NULL);
      tcprecbuf[n]=0;
      printf("%s", tcprecbuf);
    }

    if (!strcmp(cmd, "Leave")) {
      strcpy(tcpsendbuf, "Leave");
      sendto(tcpfd, tcpsendbuf, strlen(tcpsendbuf), 0,
          (struct sockaddr *)&tcpservaddr,sizeof(tcpservaddr));
      n=recvfrom(tcpfd, tcprecbuf, 2048, 0, NULL, NULL);
      tcprecbuf[n]=0;
      printf("You have left the chat session %s\n", current);
    }

    if (!strcmp(cmd, "Exit")) {
      printf("bye\n");
      break;
    }
  }
  close(udpfd);
  close(tcpfd);
  free(cmd);
  free(arg);
  free(input);
  free(udpsendbuf);
  free(udprecbuf);
  free(tcpsendbuf);
  free(tcprecbuf);
  free(current);

  return 0;
}
