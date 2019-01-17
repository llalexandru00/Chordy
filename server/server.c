#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "../configuration.h"
// SHA_LIMIT maxim 256 deoarece in protocol cheia este de un byte

int port;
char* address;
extern int errno;
extern int myKey;
extern char handle();
extern int getKey(int client);
struct sockaddr_in server;
struct sockaddr_in from;

int setPort(int argc, char* argv[])
{
    if (argc<2)
        return -1;
    port = atoi(argv[1]);
    return 0;
}

void setServer()
{
    bzero (&server, sizeof (server));
    bzero (&from, sizeof (from));
    server.sin_family = AF_INET;	
    server.sin_addr.s_addr = inet_addr(address);
    server.sin_port = htons(port);
}

void sighandler (int sig)
{
    if (wait(NULL) < 0)
	    perror ("Eroare la wait().\n");
}

int runServer (int local_port, char* local_address)
{	
    int sd;
    if (TO_LOG)
        freopen("log.txt", "w", stdout);

    if (signal (SIGCHLD, sighandler) == SIG_ERR)
    {
        perror ("signal()");
        return 1;
    }

    port = local_port;
    address = local_address;

    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror ("Eroare la socket().\n");
        return errno;
    }

    setServer();

    if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
        perror ("Eroare la bind().\n");
        return errno;
    }

    if (listen (sd, 20) == -1)
    {
        perror ("Eroare la listen().\n");
        return errno;
    }

    while (1)
    {
        int client;
        unsigned int length = sizeof (from);
        fflush (stdout);
        client = accept(sd, (struct sockaddr *) &from, &length);
        int key = getKey(client);
        fflush (stdout);

        if (key==0)
        {
            handle(client);
            continue;
        }

        if (client < 0)
        {
            perror ("Eroare la accept().\n");
            continue;
        }

        switch(fork())
        {
            case -1:
                perror("Eroare la fork().\n");
                continue;
            case 0:
                handle(client);
                close(client);
                exit(0);
            default:;
        }

        /*bzero (msg, 100);
        printf ("[server]Asteptam mesajul...\n");
        fflush (stdout);

        if (read (client, msg, 100) <= 0)
	    {
	        perror ("[server]Eroare la read() de la client.\n");
	        close (client);
	        continue;
	    }
	
        printf ("[server]Mesajul a fost receptionat...%s\n", msg);
        bzero(msgrasp,100);
        strcat(msgrasp,"Hello ");
        strcat(msgrasp,msg);
      
        printf("[server]Trimitem mesajul inapoi...%s\n",msgrasp);
        if (write (client, msgrasp, 100) <= 0)
	    {
	        perror ("[server]Eroare la write() catre client.\n");
	        continue;
	    }
        else
	        printf ("[server]Mesajul a fost trasmis cu succes.\n");
        close (client);*/
    }
}
