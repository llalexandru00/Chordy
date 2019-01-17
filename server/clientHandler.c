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
    Folosit doar de server pentru a gestiona clientii
*/
extern int myKey;

extern int isActive();
extern void printFingers();
extern void init();
extern struct node getResponsible(int key);
extern struct node getSucc();
extern char* pack(struct node x);
extern struct node unpack(char* x);
extern struct node getSuccessor();
extern void setPredecessor(struct node nod);
extern void setSuccessor(struct node nod);
extern int isMine(int key);
extern int isMe(struct node nod);
extern void notifyAll();
extern void notifyCircular(struct node origin);
extern void fix_fingers(char* addr, int port);

extern void sendChar(int client, char c);
extern char recieveChar(int client);
extern char sendMsg(int sd, char* data, int length);

extern int joinNetwork(char* addr, int port);
extern struct node queryReq(char* addr, int port, int key);
extern struct node query(int gateway, int key);

int messageUnpack(int client)
{
    char lng[1];
    if (read (client, lng, 1) <= 0)
    {
        printf("Eroare la read(lng) de la %d.\n", client);
        return -1;
    }
    char text[256];
    bzero(text, 256);
    if (read(client, text, lng[0])<=0)
    {
        printf("Eroare la read(text) de la %d.\n", client);
        return -1;
    }
    printf("Primit de la %d: %s\n", client, text);
    return 0;
}

int joinUnpack(int client)
{
    char lng[1];
    if (read (client, lng, 1) <= 0)
    {
        printf("Eroare la read(lng) de la %d.\n", client);
        return -1;
    }
    char text[256];
    bzero(text, 256);
    if (read(client, text, lng[0])<=0)
    {
        printf("Eroare la read(text) de la %d.\n", client);
        return -1;
    }
    int port=0;
    int idx=0;
    while (text[idx]!=':')
        port=port*10+text[idx]-'0', idx++;
    char addr[32];
    strcpy(addr, text+idx+1);
    joinNetwork(addr, port);
    return 0;
}

int getKey(int client)
{
    char key[1];
    if (read (client, key, 1) <= 0)
    {
        printf("Eroare la read(type) de la %d.\n", client);
        return -1;
    }
    return (int) key[0];
}

int sendCharTo(int client, char c)
{
    char answer[1];
    answer[0] = c;
    if (write (client, answer, 1) <= 0)
    {
        printf("Eroare la write(answer) spre %d.\n", client);
        close(client);   
        return -1;
    }
    return 0;
}

void notifyUnpack(int client, char what)
{
    char lng[1];
    if (read (client, lng, 1) <= 0)
    {
        printf("Eroare la read(lng) de la %d.\n", client);
        return;
    }
    char text[256];
    bzero(text, 256);
    if (read(client, text, lng[0])<=0)
    {
        printf("Eroare la read(text) de la %d.\n", client);
        return;
    }
    struct node nod = unpack(text);
    if (what=='P')
        setPredecessor(nod);
    else if (what=='S')
        setSuccessor(nod);
    else if (what=='C')
    {
        char lng[1];
        if (read (client, lng, 1) <= 0)
        {
            printf("Eroare la read(lng) de la %d.\n", client);
            return;
        }
        char text[256];
        bzero(text, 256);
        if (read(client, text, lng[0])<=0)
        {
            printf("Eroare la read(text) de la %d.\n", client);
            return;
        }
        struct node prev = unpack(text);
        if (isMe(nod))
            return;
        fix_fingers(prev.address, prev.port);
        notifyCircular(nod);
    }
}

char handle(int client)
{
    char type[1];
    if (read(client, type, 1) <= 0)
    {
        printf("Eroare la read(type) de la %d.\n", client);
        close(client);   
        return -1;
    }
    if (type[0] == '+')
    {
        while (handle(client)==0);
    }
    if (type[0]=='-')
    {
        return 1;
    }
    if (type[0]=='Q')
    {
        int key = recieveChar(client);
        if (key<0) key+=256;
        struct node resp = getResponsible(key);
        //printf("%d:%d:%s\n", resp.port, resp.key, resp.address);
        if (resp.key == myKey) 
        {
            if (isMine(key))
            {
                char* data = pack(resp);
                sendMsg(client, data, strlen(data));
                free(data);
            }
            else
            {
                resp = getSuccessor();
                struct node nod = queryReq(resp.address, resp.port, key);
                char* data = pack(nod);
                sendMsg(client, data, strlen(data));
                free(data);
            }
        }
        else
        {
            struct node nod = queryReq(resp.address, resp.port, key);
            char* data = pack(nod);
            sendMsg(client, data, strlen(data));
            free(data);
        }
    }
    if (type[0]=='S')
    {
        struct node succ = getSucc();
        char* data = pack(succ);
        sendMsg(client, data, strlen(data));
        free(data);
    }
    if (type[0]=='N')
    {
        int what = recieveChar(client);
        notifyUnpack(client, what);
    }
    if (type[0] == 'M')
        messageUnpack(client);
    if (type[0] == 'A')
    {
        sendCharTo(client, (char)isActive());
    }
    if (type[0] == 'I')
    {
        init();
        sendCharTo(client, '0');
    }
    if (type[0] == 'P')
    {
        printFingers();
        sendCharTo(client, '0');
    }
    if (type[0] == 'J')
    {
        joinUnpack(client);
        sendCharTo(client, '0');
    }
    if (type[0] == 'C')
    {
        if (isActive())
            sendCharTo(client, 'Y');
        else
            sendCharTo(client, 'N');
    }
    return 0;
}