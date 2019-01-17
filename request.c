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
#include "configuration.h"

/*
    Folosit de client/server pentru a face queryuri la alte servere
*/

extern int errno;
extern int myKey;
extern void setActive();
extern int isActive();
extern void setFingers(int gateway);
extern char* pack(struct node);
extern struct node unpack(char*);
extern void notifyAll();
extern struct node getMyNode();

extern char recieveChar(int client);
extern void sendChar(int client, char c);
extern char sendMsg(int sd, char* data, int length);

int getDest(char* addr, int port, int fast)
{
    int size=strlen(addr);
    for (int i=size-1; i>=0; i--)
        if (addr[i]!='.' && !('0'<=addr[i] && addr[i]<='9'))
            addr[i]=0;
    printf("Ma conectez la: %s : %d.\n", addr, port);
    int sd;
    struct sockaddr_in server;
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror ("Eroare la socket().");
        return -1;
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(addr);
    server.sin_port = htons(port);

    if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
        perror ("Eroare la connect().");
        return -1;
    }

    char key[1];
    if (fast==1)
        key[0] = 0;
    else
        key[0] = (char) myKey;
    if (write (sd, key, 1) <= 0)
    {
        printf("Eroare la write(type) spre %s : %d.\n", addr, port);
        return -1;
    }

    printf("Conexiune reusita.\n");
    return sd;
}

int sendMessage(char* addr, int port, int length, char* text)
{
    int sd;
    if (length>255)
    {
        printf("Eroare: Mesaj prea mare.\n");
        return -1;
    }

    if ((sd = getDest(addr, port, 0))==-1)
    {
        printf("Eroare: Nu am reusit conexiunea\n");
        return -1;
    }
    
    char* type = "M"; //Message
    if (write (sd, type, 1) <= 0)
    {
        printf("Eroare la write(type) spre %s : %d.\n", addr, port);
        close (sd);    
        return -1;
    }

    char lng[1];
    lng[0]=length;
    if (write (sd, lng, 1) <= 0)
    {
        printf("Eroare la write(length) spre %s : %d.\n", addr, port);
        close (sd);    
        return -1;
    }

    if (write (sd, text, length) <= 0)
    {
        printf("Eroare la write(text) spre %s : %d.\n", addr, port);
        close (sd);    
        return -1;
    }

    printf("Mesaj trimis cu succes.\n");
    close (sd);    
    return 0;
}

int joinNetwork(char* addr, int port)
{
    if (isActive())
    {
        printf("Nodul actual apartine deja unei retele Chord.\n");
        return -1;
    }
    printf("Verificam daca nodul la care ne conectam apartine unei retele Chord...\n");
    int sd;
    if ((sd = getDest(addr, port, 0))==-1)
    {
        printf("Eroare: Nu am reusit conexiunea la %s : %d\n", addr, port);
        return -1;
    }

    sendChar(sd, '+'); // Multiple

    sendChar(sd, 'C');
    char answer = recieveChar(sd);

    if (answer=='N')
    {
        printf("Nodul la care ne conectam nu apartine unei retele Chord.\n");
        sendChar(sd, '-'); //Stop
        close(sd);
        return -1;
    }
    printf("Nodul la care ne conectam apartine unei retele Chord.\n");
    printf("Se seteaza tabela Finger.\n");
    setFingers(sd);
    setActive();
    notifyAll();
    sendChar(sd, '-'); //Stop
    close(sd);
    return 0;
}


// Daca avem deja gatewayul, daca nu folosim queryReq pentru a stabili o conexiune
struct node query(int gateway, int key)
{
    struct node nod;
    nod.port=0;
    nod.key=0;
    strcpy(nod.address, "");

    printf("Obtin informatii despre cheia: %d\n", key);
    sendChar(gateway, 'Q');
    sendChar(gateway, (char)key);

    char lng[1];
    if (read (gateway, lng, 1) <= 0)
    {
        printf("Eroare la read(length) de la %d\n", gateway);
        return nod;
    }
    char data[256];
    bzero(data, 256);
    if (read (gateway, data, lng[0])<=0)
    {
        printf("Eroare la read(data) de la %d\n", gateway);
        return nod;
    }
    nod = unpack(data);
    return nod;
}

struct node queryReq(char* addr, int port, int key)
{
    int sd;
    struct node nod;
    nod.port=0;
    nod.key=0;
    strcpy(nod.address, "");
    if ((sd = getDest(addr, port, 0))==-1)
    {
        printf("Eroare: Nu am reusit conexiunea la %s : %d\n", addr, port);
        return nod;
    }
    nod = query(sd, key);
    close(sd);
    return nod;
}

struct node querySucc(int gateway)
{
    struct node nod;
    nod.port=0;
    nod.key=0;
    printf("Obtin informatii despre succesor\n");
    strcpy(nod.address, "");
    sendChar(gateway, 'S');

    char lng[1];
    if (read (gateway, lng, 1) <= 0)
    {
        printf("Eroare la read(length) de la %d\n", gateway);
        return nod;
    }
    char data[256];
    bzero(data, 256);
    if (read (gateway, data, lng[0])<=0)
    {
        printf("Eroare la read(data) de la %d\n", gateway);
        return nod;
    }
    nod = unpack(data);
    return nod;
}

struct node querySuccReq(char* addr, int port)
{
    int sd;
    struct node nod;
    nod.port=0;
    nod.key=0;
    strcpy(nod.address, "");
    printf("Obtin informatii despre succesor\n");

    if ((sd = getDest(addr, port, 0))==-1)
    {
        printf("Eroare: Nu am reusit conexiunea la %s : %d\n", addr, port);
        return nod;
    }
    nod = querySucc(sd);
    close (sd); 
    return nod;
}

void notify(struct node origin, struct node dest, char what) // what in P, S, C predecessor, successor, all circular
{
    int sd;
    printf("Notific pe %d (%s:%d)\n", dest.key, dest.address, dest.port);
    if ((sd = getDest(dest.address, dest.port, 1))==-1)
    {
        printf("Eroare: Nu am reusit conexiunea la %s : %d\n", dest.address, dest.port);
        return;
    }

    sendChar(sd, 'N');
    sendChar(sd, what);
    char* data = pack(origin);
    sendMsg(sd, data, strlen(data));
    free(data);
    if (what=='C')
    {
        struct node myNod =getMyNode();
        char* data = pack(myNod);
        sendMsg(sd, data, strlen(data));
        free(data);
    }
    close(sd);
}