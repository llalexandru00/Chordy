#define SHA_LIMIT 8 // so 256 possible keys
#define WITH_SERVER 1   // for debugging purposes
#define TO_LOG 0    // server output redirect in log.txt

struct node
{
    char address[32];
    int port; 
    int key;
};

struct table
{
    int active;
    struct node succ, pred;
    struct node nodes[SHA_LIMIT+2];
};