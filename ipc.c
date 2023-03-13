#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <sys/socket.h>




int makepipe(int fd[2]);  //0 success -1 failed



int main()
{
    printf("start!\n");

    return 0;
}

int makepipe(int fd[2])
{
    return socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
}