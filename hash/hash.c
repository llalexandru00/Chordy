#include "sha1.h"

int hash(unsigned char* buffer, int bufsize, int limit)
{
    SHA1_CTX ctx;
    unsigned char hash[20];
    int i;

    SHA1Init(&ctx);
    for(i=0;i<1000;i++)
        SHA1Update(&ctx, buffer, bufsize);
    SHA1Final(hash, &ctx);

    long long output=0;
    for(i=0;i<20;i++)
    {
        output *= 256;
        output += hash[i];
        output %= limit;
    }
    return (int) output;
}