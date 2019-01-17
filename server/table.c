#include "../configuration.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
    Functiile de aici sunt folosite doar de server.
    Pentru a obtine in client trebuie sa ne conectam la serverul propriu si sa obtinem informatiile.
*/

extern int myKey;
extern int myPort;
extern char* myAddr;

extern char recieveChar(int client);
extern void sendChar(int client, char c);
extern char sendMsg(int sd, char* data, int length);
extern struct node querySucc(int gateway);
extern struct node querySuccReq(char* addr, int port);
extern void notify(struct node origin, struct node dest, char what);

struct node query(int gateway, int key);
struct node queryReq(char* addr, int port, int key);

struct table myTable;

void init()
{
    printf("Initializare Chord...\n");
    if (myTable.active==1)
    {
        printf("Nodul apartine deja unei retele Chord.\n");
        return;
    }
    struct node nod;
    nod.key=myKey;
    nod.port=myPort;
    strcpy(nod.address, myAddr);
    for (int i=0; i<SHA_LIMIT; i++)
        myTable.nodes[i] = nod;
    myTable.succ = nod;
    myTable.pred = nod;
    myTable.active=1;
    printf("Succes\n");
}

void printFingers()
{
    if (myTable.active==0)
    {
        printf("Nodul nu apartine unei retele Chord.\n");
        return;
    }
    printf("Succesorul are cheia %d (%s:%d)\n", myTable.succ.key, myTable.succ.address, myTable.succ.port);
    printf("Predecesorul are cheia %d (%s:%d)\n", myTable.pred.key, myTable.pred.address, myTable.pred.port);
    printf("Tabela Finger: \n");
    for (int i=0; i<SHA_LIMIT; i++)
    {
        printf("Cheia %d apartine nodului %d (%s:%d)\n", (myKey+(1<<i)) % (1<<SHA_LIMIT), myTable.nodes[i].key, myTable.nodes[i].address, myTable.nodes[i].port);
    }
}

void setActive()
{
    myTable.active=1;
}

int isActive()
{
    return myTable.active;
}

int isMine(int key)
{
    if (myTable.succ.key == myKey) // exista un singur nod in retea
        return 1;
    key -= myKey;
    if (key<0)  key+=(1<<SHA_LIMIT);
    int succKey = myTable.succ.key;
    succKey -= myKey;
    if (succKey<0) succKey+=(1<<SHA_LIMIT);
    if (key<succKey)
        return 1;
    return 0;
}

int isMe(struct node nod)
{
    if (myKey==nod.key)
        return 1;
    return 0;
}

void fix_fingers(char* addr, int port) // gateway
{
    struct node myNod;
    myNod.port=myPort;
    myNod.key=myKey;
    strcpy(myNod.address, myAddr);

    for (int i=0; i<SHA_LIMIT; i++)
    {
        int key = (myKey+(1<<i)) % (1<<SHA_LIMIT);
        if (isMine(key))
            myTable.nodes[i] = myNod;
        else
            myTable.nodes[i] = queryReq(addr, port, key);
        printf("%d bifat\n", i);
    }
}

void setFingers(int gateway)
{
    myTable.pred = query(gateway, myKey);
    myTable.succ = querySuccReq(myTable.pred.address, myTable.pred.port);
    struct node myNod;
    myNod.port=myPort;
    myNod.key=myKey;
    strcpy(myNod.address, myAddr);

    for (int i=0; i<SHA_LIMIT; i++)
    {
        int key = (myKey+(1<<i)) % (1<<SHA_LIMIT);
        if (isMine(key))
            myTable.nodes[i] = myNod;
        else
            myTable.nodes[i] = query(gateway, key);
    }
}

void notifyAll()
{
    struct node myNod;
    myNod.port=myPort;
    myNod.key=myKey;
    strcpy(myNod.address, myAddr);
    notify(myNod, myTable.succ, 'P'); //change predecessor
    notify(myNod, myTable.pred, 'S'); // change successor
    notify(myNod, myTable.pred, 'C'); // change all fingers circular
}

void notifyCircular(struct node origin)
{
    notify(origin, myTable.pred, 'C'); // change all fingers circular
}

struct node getMyNode()
{
    struct node nod;
    nod.key=myKey;
    nod.port=myPort;
    strcpy(nod.address, myAddr);
    return nod;
}

struct node getResponsible(int key)
{
    struct node nod;
    //printf("%d %d\n", myKey, key);
    nod.key=myKey;
    nod.port=myPort;
    strcpy(nod.address, myAddr);
    if (key==myKey)
        return nod;
    key-=myKey;
    if(key<0)
        key+=myKey;
    int p = 1, nr=0; // nr este celula din tabel ce reprezinta nodul responsabil
    while (p<=key) p*=2, nr++;
    p/=2; nr--;
    return myTable.nodes[nr];
}

int intLength(int x)
{
    int nr=0;
    while (x) nr++, x/=10;
    return nr;
}

char* pack(struct node nod)
{
    char* data = (char *)malloc(256);
    sprintf(data, "%d:%d:%s", nod.port, nod.key, nod.address);
    data[strlen(nod.address)+intLength(nod.port)+intLength(nod.key)+2] = 0;
    return data;
}

struct node unpack(char* data)
{
    struct node nod;
    nod.key=0;
    nod.port=0;
    int idx=0;
    while (data[idx]!=':')
        nod.port=nod.port*10+data[idx]-'0', idx++;
    idx++;
    while (data[idx]!=':')
        nod.key=nod.key*10+data[idx]-'0', idx++;
    strcpy(nod.address, data+idx+1);
    return nod;
}

struct node getSucc()
{
    return myTable.succ;
}

void setPredecessor(struct node nod)
{
    myTable.pred=nod;
}

void setSuccessor(struct node nod)
{
    myTable.succ=nod;
}

struct node getSuccessor()
{
    return myTable.succ;
}