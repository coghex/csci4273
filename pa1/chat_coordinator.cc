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
//the size of all my buffers cause im lazy
#define BUFSIZE 2048

//BRAAAAAINS!
void handle_sigchild(int sig) {
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
  //silly -Wall warnings, i need this variable
  sig++;
}

//the nodes in the list that will hold all of the servers
struct server {
  int port;
  int pid;
  struct server* next;
  char* name;
};

//this server is not real, but the list head
struct server* head;

//finds if the server already exists and returns the port if it does
int existingsession(char * input) {
  struct server *node = head;
  while(node->next!=NULL) {
    if (!strcmp(node->name, input)) {
      return node->port;
    }
    node = node->next;
  }
  if (!strcmp(node->name, input)) {
    return node->port;
  }

  return 0;
}

//mallocs and adds the server to the list
void addserver(char * name, int port) {
  struct server *node = head;
  while(node->next!=NULL) {
    node=node->next;
  }
  node->next = (struct server*)malloc(sizeof(struct server));
  node=node->next;
  node->port = port;
  node->name = (char *)malloc(BUFSIZE);
  strcpy(node->name, name);
}

void setpid(int port, int pid) {
  struct server *node = head;
  while(node->port != port) {
    node=node->next;
    if (node->next == NULL) {
      break;
    }
  }
  if (node->port == port) {
    node->pid = pid;
  }
}

//removes a server from the list and rearranges the list
void termsession(int p) {
  struct server *node = head;
  while(node->next->port != p) {
    node=node->next;
  }
  kill(node->next->pid, 9);
  if (node->next->next != NULL) {
    node->next = node->next->next;
  }
  else {
    node->next = NULL;
  }
}

int main (int argc, char *argv[0]) {
  unsigned short port;
  struct sockaddr_in saddr, caddr, tcpsaddr, tcpcaddr;
  int sockfd, pid, tcpsockfd, tcpconnfd;
  char* buf, *cmd, *arg, *sendbuf;
  int ret, i, j, p;
  int tcpport=31337;
  socklen_t clen, tcpclen;

  //registering the child process handler
  struct sigaction sa;
  sa.sa_handler = &handle_sigchild;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  if (sigaction(SIGCHLD, &sa, 0) == -1) {
    perror(0);
    exit(1);
  }

  //first i allocate all of the buffers
  //all of this is super hacky c code because yolo
  buf = (char*)malloc(BUFSIZE);
  memset(buf, 0, BUFSIZE);
  cmd = (char*)malloc(BUFSIZE);
  memset(cmd, 0, BUFSIZE);
  arg = (char*)malloc(BUFSIZE);
  memset(arg, 0, BUFSIZE);
  sendbuf = (char*)malloc(BUFSIZE);
  memset(sendbuf, 0, BUFSIZE);

  //we want to make sure the user specifies a port
  if (argc != 2) {
    printf("usage: %s <Port>\n", argv[0]);
    exit(1);
  }

  port = atoi(argv[1]);

  //if the port they specified is the default tcp starting port, we
  //change that port by one, nbd
  if (port == 31337) {
    tcpport = 31338;
  }

  //initialize the head of the list with dummy values
  //so we dont get null pointer exceptions
  head = (struct server*)malloc(sizeof(struct server));
  head->port = -1;
  head->name = (char *)malloc(BUFSIZE);
  strcpy(head->name, "head1337\n");

  //opens the udp socket
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("cannot open udp socket\n");
    exit(1);
  }
  //initializes the address structures
  memset((char*)&saddr, 0, sizeof(saddr));
  memset((char*)&caddr, 0, sizeof(caddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  saddr.sin_port = htons(port);

  //binds to the udp socket
  if (bind(sockfd, (struct sockaddr*)&saddr, sizeof(saddr)) < 0) {
    printf("cannot bind\n");
    exit(1);
  }
  printf("The chat coordinator is now on port %d\n", port);
  //this is our infinite loop
  while (1) {
    clen = sizeof(caddr);
    //block until we recv something, then find out where it came from
    if ((ret = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&caddr, &clen)) < 0) {
      sendto(sockfd, "-1\n", 3, 0, (struct sockaddr *)&caddr, sizeof(caddr));
      goto cleanup;
    }
    //seperate the arguments from the supplied buffer in a super hacky way
    for(i=0;(buf[i]!=0x20)&&(buf[i]!=0x00)&&(buf[i]!='\n');i++);
    cmd = (char*)memcpy(cmd, buf, i);
    for(j=i+1;(buf[j]!=0x20)&&(buf[j]!=0x00)&&(buf[j]!='\n');j++);
    arg = (char*)memcpy(arg, buf+i+1, j-i);

    //if we receive a start command
    if (!strcmp(cmd, "Start") || !strcmp(cmd, "s")) {
      //does the session already exist?
      if (existingsession(arg)) {
        printf("a server by the name %s is already running\n", arg);
        sendto(sockfd, "-1\n", 3, 0, (struct sockaddr *)&caddr, sizeof(caddr));
        goto cleanup;
      }
      //open the tcp socket
      if ((tcpsockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("cannot open tcp socket\n");
        sendto(sockfd, "-1\n", 3, 0, (struct sockaddr *)&caddr, sizeof(caddr));
        goto cleanup;
      }
      //add this server to the list, since at this point, it will probably be made
      addserver(arg, tcpport);
      memset((char*)&tcpsaddr, 0, sizeof(tcpsaddr));
      memset((char*)&tcpcaddr, 0, sizeof(tcpcaddr));
      tcpsaddr.sin_family = AF_INET;
      tcpsaddr.sin_addr.s_addr = htonl(INADDR_ANY);
      tcpsaddr.sin_port = htons(tcpport);
      //bind to the tcp socket, if there is an error, remove the session from the
      //list, not sure if that works, but im gonna guess it does
      if (bind(tcpsockfd, (struct sockaddr*)&tcpsaddr, sizeof(tcpsaddr)) < 0) {
        printf("cannot bind to port %d\n", tcpport);
        sendto(sockfd, "-1\n", 3, 0, (struct sockaddr *)&caddr, sizeof(caddr));
        termsession(p);
        close(tcpport);
        goto cleanup;
      }
      //accept on that port
      tcpclen = sizeof(tcpcaddr);
      tcpconnfd = accept(tcpsockfd, (struct sockaddr*)&tcpcaddr, &tcpclen);
      printf("starting server %s on port %d\n", arg, tcpport);
      //fork a new session to do whatevs
      snprintf(sendbuf, BUFSIZE, "The Chat server is now running on port %d\n", tcpport);
      sendto(sockfd, sendbuf, BUFSIZE, 0, (struct sockaddr *)&caddr, sizeof(caddr));
      memset(sendbuf, 0, BUFSIZE);

      if (!(pid = fork())) {
        close(sockfd);
        //passes in the open tcp socket so that our server doesnt have
        //to cat /proc/self/fd or lsof or something stupid like that
        sprintf(arg, "%d", tcpsockfd);
        char *serverargs[2] = {arg, NULL};
        execv("chat_server", serverargs);
        exit(0);
      }
      else {
        //we want to remember the pid
        setpid(tcpport, pid);
        close(tcpconnfd);
        //increment the tcp port by one, it would be tough to run out
        tcpport++;
      }
    }
    //on find we return the port, simple enough
    if (!strcmp(cmd, "Find") || !strcmp(cmd, "f")) {
      if ((p = existingsession(arg))) {
        snprintf(sendbuf, BUFSIZE, "%d\n", p);
        sendto(sockfd, sendbuf, BUFSIZE, 0, (struct sockaddr *)&caddr, sizeof(caddr));
        goto cleanup;
      }
      else {
        printf("a server by the given name is not running\n");
        sendto(sockfd, "-1\n", 3, 0, (struct sockaddr *)&caddr, sizeof(caddr));
        goto cleanup;
      }
    }
    //terminate calls the termsession code, its pretty straitforward
    if (!strcmp(cmd, "Terminate") || !strcmp(cmd, "t")) {
      if ((p = existingsession(arg))) {
        close(tcpport);
        termsession(p);
      }
      else {
        printf("a server by the given name is not running\n");
        sendto(sockfd, "-1\n", 3, 0, (struct sockaddr *)&caddr, sizeof(caddr));
        goto cleanup;
      }

    }
    //what would hacky c code be without gotos, labels, and memsets
    cleanup:
    memset(buf, 0, BUFSIZE);
    memset(cmd, 0, BUFSIZE);
    memset(arg, 0, BUFSIZE);
  }
return 0;
}
