#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>

int main(int argc, char **argv){

    openlog("writer",0,LOG_USER);
    if(argc != 3){
        syslog(LOG_ERR,"Invalid args %d!\n",argc);
        return 1;
    }

    // get arguments
    char* filepath = argv[1];
    char* writestr = argv[2];

    FILE* fd = fopen(filepath, "w");
    if (fd == NULL){
        syslog(LOG_ERR,"%s\n", strerror(errno));
        return 1;
    }

    int ret = fwrite(writestr, sizeof(char), strlen(writestr), fd);
    if ( ret != strlen(writestr)){
        syslog(LOG_ERR,"Write failed!!");
        fclose(fd);
        return 1;
    }
    syslog(LOG_DEBUG,"Writing %s to %s", writestr, filepath);

    fclose(fd);
    return 0;
}