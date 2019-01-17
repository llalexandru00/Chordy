#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include "../configuration.h"

/*
    Folosit doar de client pentru a face queryuri la serverul propriu
*/

extern int getDest(char* addr, int port, int fast);
extern int myPort;
extern char* myAddr;

struct node query(int gateway, int key);

int recieveChar(int sd)
{
    char answer[1];
    if (read(sd, answer, 1)<=0)
    {
        printf("Eroare la read(answer) de la propriul server.\n");
        close (sd);     
        return -1;
    }
    return answer[0];
}

char sendChar(int sd, char c)
{
    char type[1];
    type[0] = c;
    if (write (sd, type, 1)<=0)
    {
        printf("Eroare la write(type) catre propriul server.\n");
        close (sd);     
        return -1;
    }
    return 0;
}

char sendMsg(int sd, char* data, int length)
{
    char lng[1];
    lng[0]=length;
    if (write (sd, lng, 1) <= 0)
    {
        printf("Eroare la write(length) catre propriul server");
        close (sd);    
        return -1;
    }
    if (write (sd, data, length)<=0)
    {
        printf("Eroare la write(data) catre propriul server.\n");
        close (sd);
        return -1;
    }
    return 0;
}

int initChord()
{
    int sd;
    if ((sd = getDest(myAddr, myPort, 1))==-1)
    {
        printf("Eroare: Nu am reusit conexiunea\n");
        return -1;
    }
    sendChar(sd, 'I'); // Initialize
    recieveChar(sd);
    close(sd);
    return 0;
}

int printFingersChord()
{
    int sd;
    if ((sd = getDest(myAddr, myPort, 0))==-1)
    {
        printf("Eroare: Nu am reusit conexiunea\n");
        return -1;
    }
    sendChar(sd, 'P'); // Print
    recieveChar(sd);
    close(sd);
    return 0;
}

int isActiveChord()
{
    int sd;
    if ((sd = getDest(myAddr, myPort, 0))==-1)
    {
        printf("Eroare: Nu am reusit conexiunea\n");
        return -1;
    }
    sendChar(sd, 'A'); // Print
    char ans = recieveChar(sd);
    close(sd);
    return ans;
}

int joinNetworkChord(char* addr, int port)
{
    int sd;
    if ((sd = getDest(myAddr, myPort, 1))==-1)
    {
        printf("Eroare: Nu am reusit conexiunea la %s:%d\n", addr, port);
        return -1;
    }
    sendChar(sd, 'J'); // Join
    char data[256] = "";
    sprintf(data, "%d:%s", port, addr);
    sendMsg(sd, data, strlen(data));
    char ans = recieveChar(sd);
    close(sd);
    return ans;
}

struct node queryChord(int key)
{
    struct node nod;
    nod.key=0;
    nod.port=0;
    strcpy(nod.address, "");

    int sd;
    if ((sd = getDest(myAddr, myPort, 0))==-1)
    {
        printf("Eroare: Nu am reusit conexiunea\n");
        return nod;
    }
    nod = query(sd, key);
    close(sd);
    return nod;
}