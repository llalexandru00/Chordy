#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include "../configuration.h"

extern int errno;
extern int hash(unsigned char* buffer, int bufsize, int limit);
int runServer(int port, char* address);

extern void initChord();
extern void printFingersChord();
extern int isActiveChord();
extern int joinNetworkChord(char* addr, int port);
extern struct node queryChord(int key);

extern int sendMessage(char* addr, int port, int length, char* text);

extern int myKey;
int myPort;
char* myAddr;

char *args[10], *base;

int split_command(char* str)
{
    if (strchr(str, ' ')==NULL)
    {
        base = str;
        base[strlen(base)-1]=0;
        return 0;
    }
    char *p = strtok(str, " ");
    base = strdup(p);
    int anr=0;
    while ((p = strtok(NULL, " ")))
    {
        args[anr] = strdup(p);
        anr++;
    }
    args[anr-1][strlen(args[anr-1])-1]=0;
    return anr;
}

int myKey;

void setMyKey(int port, char* address)
{
    char data[256] = "";
    sprintf(data, "%d", port);
    strcat(data, address);
    myKey = hash(data, strlen(data), 1<<SHA_LIMIT);
}

int main (int argc, char *argv[])
{
    if (argc != 3)
    {
        printf ("Sintaxa: %s <adresa> <port>\n", argv[0]);
        return -1;
    }

    setMyKey(atoi(argv[2]), argv[1]);
    myPort = atoi(argv[2]);
    myAddr = argv[1];

    if (WITH_SERVER)
        switch(fork())
        {
            case -1:
                perror("Eroare la fork().");
                return errno;
            case 0:
                runServer(atoi(argv[2]), argv[1]);
                exit(0);
            default:;
        }
    
    while (1)
    {
        printf("Comanda: ");
        char str[1000];
        fgets(str, 1000, stdin);
        int anr = split_command(str);
        if (strcmp(base, "exit")==0)
        {
            break;
        }
        else if (strcmp(base, "write")==0)
        {
            if (anr<3)
            {
                printf("Sintaxa: write <addr> <port> <text>\n");
                continue;
            }
            char text[1000];
            strcpy(text, args[2]);
            for (int i=3; i<anr; i++)
            {
                strcat(text, " ");
                strcat(text, args[i]);
            }
            sendMessage(args[0], atoi(args[1]), strlen(text), text);
        }
        else if (strcmp(base, "init")==0)
        {
            initChord();
        }
        else if (strcmp(base, "finger")==0)
        {
            printFingersChord();
        }
        else if (strcmp(base, "active")==0)
        {
            if (isActiveChord())
                printf("Nodul actual apartine unei retele Chord\n");
            else
                printf("Nodul actual nu apartine unei retele Chord\n"); 
        }
        else if (strcmp(base, "join")==0)
        {
            if (anr<2)
            {
                printf("Sintaxa: join <addr> <port>\n");
                continue;
            }
            joinNetworkChord(args[0], atoi(args[1]));
        }
        else if (strcmp(base, "query")==0)
        {
            if (anr<1)
            {
                printf("Sintaxa: query <key>\n");
                continue;
            }
            struct node nod = queryChord(atoi(args[0]));
            printf("Cheia %d apartine nodului %d\n", atoi(args[0]), nod.key);
            printf("Adresa: %s\n", nod.address);
            printf("Port: %d\n", nod.port);
        }
        else if (strcmp(base, "mykey")==0)
        {
            printf("Cheia este: %d\n", myKey);
        }
        else
            printf("Comanda neidentificata.\n");
    }

    return 0;
}
