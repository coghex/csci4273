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
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define BUFSIZE 2048
#define TCPSIZE 512
#define MAXUSERS 512

struct user {
  int id;
  int lastmsg;
};
struct message {
  int authorid;
  int length;
  time_t time;
  char text[80];
};

int main(int argc, char *argv[]) {
  fd_set set;
  int fd, pid, i, j, k, parentport, myport, ret;
  struct timeval timeout;
  int childfd;
  int clientlen;
  struct sockaddr_in clientaddr;
  struct hostent *hostp;
  char *buf, *cmd, *arg1, *arg2, *sendbuf;
  struct tm *nowtime;
  int hisid, usrid, msgid, unid, timerid;
  clock_t diff;
  //Variables that need to be global used shard mem
  hisid = shmget(IPC_PRIVATE, BUFSIZE*sizeof(struct message), SHM_R | SHM_W);
  struct message *history = (struct message *)shmat(hisid, NULL, 0);
  usrid = shmget(IPC_PRIVATE, MAXUSERS*sizeof(struct user), SHM_R | SHM_W);
  struct user *users = (struct user *)shmat(usrid, NULL, 0);
  msgid = shmget(IPC_PRIVATE, sizeof(int), SHM_R | SHM_W);
  int *msg = (int *)shmat(msgid, NULL, 0);
  unid = shmget(IPC_PRIVATE, sizeof(int), SHM_R | SHM_W);
  int *usernum = (int *)shmat(unid, NULL, 0);
  timerid = shmget(IPC_PRIVATE, sizeof(int), SHM_R | SHM_W);
  clock_t *start = (clock_t *)shmat(timerid, NULL, 0);
  shmctl (hisid , IPC_RMID , 0);
  shmctl (usrid , IPC_RMID , 0);
  shmctl (msgid , IPC_RMID , 0);
  shmctl (unid , IPC_RMID , 0);
  shmctl (timerid , IPC_RMID , 0);
  *msg=0;
  *usernum=0;

  buf = (char*)malloc(BUFSIZE);
  memset(buf, 0, BUFSIZE);
  cmd = (char*)malloc(BUFSIZE);
  memset(cmd, 0, BUFSIZE);
  arg1 = (char*)malloc(BUFSIZE);
  memset(arg1, 0, BUFSIZE);
  arg2 = (char*)malloc(BUFSIZE);
  memset(arg2, 0, BUFSIZE);
  sendbuf = (char*)malloc(BUFSIZE);
  memset(sendbuf, 0, BUFSIZE);
  memset((char*)&clientaddr, 0, sizeof(clientaddr));

  if (argc > 3) {
    printf("silly wall arguments\n");
  }
  fd = atoi(argv[0]);
  parentport = atoi(argv[2]);
  myport = atoi(argv[3]);
  clientlen = sizeof(clientaddr);
  if (listen(fd, 10) < 0) {
    printf("cannot listen\n");
  }
  while(1) {
    if ((childfd = accept(fd, (struct sockaddr *) &clientaddr, (socklen_t*)&clientlen)) < 0) {
      printf("cannot accept, %s\n", strerror(errno));
    }
    if (!(pid = fork())) {
      close(fd);
      hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
          sizeof(clientaddr.sin_addr.s_addr), AF_INET);
      if (hostp == NULL) {
        printf("cannot find host\n");
      }
      struct user curuser;
      curuser.id = *usernum;
      curuser.lastmsg = 0;
      users[*usernum] = curuser;
      (*usernum)++;
      //snprintf(sendbuf, BUFSIZE, "welcome to chat, you are user %d\n", curuser.id);
      //sendto(childfd, sendbuf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));

      while(1) {
        memset(buf, 0, BUFSIZE);
        memset(sendbuf, 0, BUFSIZE);
        memset(cmd, 0, BUFSIZE);
        memset(arg1, 0, BUFSIZE);
        memset(arg2, 0, BUFSIZE);

        //timeout.tv_sec=0;
        //timeout.tv_usec=10000;
        //ret = select(childfd, &set, NULL, NULL, &timeout);
        //if (ret == -1) {
        //  printf("select error\n");
        //}
        //else if (ret != 0) {
          read(childfd, buf, TCPSIZE);
          for(i=0;(buf[i]!=0x20)&&(buf[i]!=0x00)&&(buf[i]!='\n')&&(buf[i]!=0x0d);i++);
          cmd = (char*)memcpy(cmd, buf, i);
          for(j=i+1;(buf[j]!=0x20)&&(buf[j]!=0x00)&&(buf[j]!='\n'&&(buf[j]!=0x0d));j++);
          arg1 = (char*)memcpy(arg1, buf+i+1, j-i);
          for(k=j+1;(buf[k]!=0x00)&&(buf[k]!=0x0d&&(buf[k]!='\n')&&(buf[k]!=3)&&(buf[k]!=4));k++);
          arg2 = (char*)memcpy(arg2, buf+j+1, k-j);
        //}

        if (!strcmp(cmd, "Submit")) {
          *start = clock();
          struct message newmsg;
          newmsg.authorid = curuser.id;
          newmsg.length = atoi(arg1);
          newmsg.time = time(NULL);
          strncpy(newmsg.text, arg2, atoi(arg1));
          newmsg.text[newmsg.length] = 0;
          history[*msg] = newmsg;
          (*msg)++;
        }
        if (!strcmp(cmd, "Leave")) {
          *start = clock();
          snprintf(sendbuf, BUFSIZE, "bye\n");
          sendto(childfd, sendbuf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
          break;
        }
        if (!strcmp(cmd, "GetNext")) {
          *start = clock();
          if (users[curuser.id].lastmsg >= *msg) {
            snprintf(sendbuf, BUFSIZE, "-1\n");
            sendto(childfd, sendbuf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
          }
          else {
            nowtime = localtime(&(history[(users[curuser.id].lastmsg)].time));
            //snprintf(sendbuf, BUFSIZE, "Most recent message:\n%d/%d/%d %d:%d:%d person %d> %s\n",
            //    nowtime->tm_mon,
            //    nowtime->tm_mday, nowtime->tm_year-100, nowtime->tm_hour, nowtime->tm_min,
            //    nowtime->tm_sec, history[(users[curuser.id].lastmsg)].authorid, history[(users[curuser.id].lastmsg)].text);
            snprintf(sendbuf, BUFSIZE, "%d %s\n", (int)strlen(history[(users[curuser.id].lastmsg)].text), history[(users[curuser.id].lastmsg)].text);
            sendto(childfd, sendbuf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
            users[curuser.id].lastmsg++;
          }
        }
        if (!strcmp(cmd, "GetAll")) {
          *start = clock();
          snprintf(sendbuf, BUFSIZE, "%d\n", *msg-users[curuser.id].lastmsg );
          sendto(childfd, sendbuf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
          for(i=users[curuser.id].lastmsg;i<*msg;i++) {
            snprintf(sendbuf, BUFSIZE, "%d %s\n", (int)strlen(history[i].text), history[i].text);
            sendto(childfd, sendbuf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
            memset(sendbuf, 0, BUFSIZE);
          }
          users[curuser.id].lastmsg = *msg;
        }
      }
      shmdt(history);
      shmdt(users);
      shmdt(msg);
      shmdt(usernum);
      shmdt(start);


      exit(0);
    }
    else {
      close(childfd);
      while ((pid = waitpid(((pid_t) - 1), NULL, WNOHANG)) > 0);
    }
  }
  return 0;
}
