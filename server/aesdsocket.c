#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define __USE_XOPEN2K 1

int main(int argc, char **argv[]){

    int ret;
    struct addrinfo hints;
    struct addrinfo* servinfo;

    memset(&hints, 0, sizeof(hints));
    
    ret = getaddrinfo();
    // int s;
    // s = socket();

}
