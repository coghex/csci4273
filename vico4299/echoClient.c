#include <sys/types.h>
#include <sys/socket.h>

#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef INADDR_NONE
#define INADDR_NONE     0xffffffff
#endif  /* INADDR_NONE */

extern int	errno;

int	TCPecho(const char *host, const char *portnum, SSL_CTX *ctx);
int	errexit(const char *format, ...);
int	connectsock(const char *host, const char *portnum);

#define	LINELEN		128

/*------------------------------------------------------------------------
 * main - TCP client for ECHO service
 *------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
	char	*host = "localhost";	/* host to use if none supplied	*/
	char	*portnum = "5004";	/* default server port number	*/
  SSL_METHOD *meth;
  SSL_CTX *ctx;

	switch (argc) {
	case 1:
		host = "localhost";
		break;
	case 3:
		host = argv[2];
		/* FALL THROUGH */
	case 2:
		portnum = argv[1];
		break;
	default:
		fprintf(stderr, "usage: TCPecho [host [port]]\n");
		exit(1);
	}

  SSL_library_init();
  SSL_load_error_strings();
  meth = SSLv3_method();
  ctx = SSL_CTX_new(meth);
  if (SSL_CTX_use_certificate_chain_file(ctx, "server.cert") <= 0){
    ERR_print_errors_fp(stderr);
    exit(1);
  }
  if (!(SSL_CTX_load_verify_locations(ctx, "cacert.pem", NULL))){
    ERR_print_errors_fp(stderr);
    exit(1);
  }

  TCPecho(host, portnum, ctx);
	exit(0);
}

/*------------------------------------------------------------------------
 * TCPecho - send input to ECHO service on specified host and print reply
 *------------------------------------------------------------------------
 */
int
TCPecho(const char *host, const char *portnum, SSL_CTX *ctx)
{
	char	buf[LINELEN+1];		/* buffer for one line of text	*/
	int	s, n;			/* socket descriptor, read count*/
	int	outchars, inchars;	/* characters sent and received	*/
  SSL *ssl;

	s = connectsock(host, portnum);
  ssl = SSL_new(ctx);
  SSL_set_fd(ssl, s);
  if ( SSL_connect(ssl) == -1 )
    ERR_print_errors_fp(stderr);

	while (fgets(buf, sizeof(buf), stdin)) {
		buf[LINELEN] = '\0';	/* insure line null-terminated	*/
		outchars = strlen(buf);
		(void) SSL_write(ssl, buf, outchars);

		/* read it back */
		for (inchars = 0; inchars < outchars; inchars+=n ) {
			n = SSL_read(ssl, &buf[inchars], outchars - inchars);
			if (n < 0)
				errexit("socket read failed: %s\n",
					strerror(errno));
		}
		fputs(buf, stdout);
	}
  SSL_free(ssl);
  SSL_CTX_free(ctx);
}

/*------------------------------------------------------------------------
 * errexit - print an error message and exit
 *------------------------------------------------------------------------
 */
int
errexit(const char *format, ...)
{
        va_list args;

        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        exit(1);
}

/*------------------------------------------------------------------------
 * connectsock - allocate & connect a socket using TCP 
 *------------------------------------------------------------------------
 */
int
connectsock(const char *host, const char *portnum)
/*
 * Arguments:
 *      host      - name of host to which connection is desired
 *      portnum   - server port number
 */
{
        struct hostent  *phe;   /* pointer to host information entry    */
        struct sockaddr_in sin; /* an Internet endpoint address         */
        int     s;              /* socket descriptor                    */


        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;

    /* Map port number (char string) to port number (int)*/
        if ((sin.sin_port=htons((unsigned short)atoi(portnum))) == 0)
                errexit("can't get \"%s\" port number\n", portnum);

    /* Map host name to IP address, allowing for dotted decimal */
        if ( phe = gethostbyname(host) )
                memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
        else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
                errexit("can't get \"%s\" host entry\n", host);

    /* Allocate a socket */
        s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s < 0)
                errexit("can't create socket: %s\n", strerror(errno));

    /* Connect the socket */
        if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
                errexit("can't connect to %s.%s: %s\n", host, portnum,
                        strerror(errno));
        return s;
}

