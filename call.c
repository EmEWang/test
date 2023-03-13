#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <errno.h>
#include <signal.h>
#include <sys/types.h>

const int ilinemax = 1024;

void sig_pipe(int);

int main()
{
    int fd1[2],fd2[2],n;
    pid_t pid;
    char line[ilinemax];

    if(signal(SIGPIPE, sig_pipe) == SIG_ERR)
    {
        printf("signal error %d %s\n", errno, strerror(errno));
        return 0;
    }

    if (pipe(fd1) < 0 || pipe(fd2))
    {
        printf("pipe error %d %s\n", errno, strerror(errno));
        return 0;
    }

    if((pid = fork()) < 0)
    {
        printf("fork error %d %s\n", errno, strerror(errno));
        return 0;
    }
    else if(pid > 0)    //parent
    {
        close(fd1[0]);
        close(fd2[1]);

        while (fgets(line, ilinemax, stdin) != NULL)
        {
            n = strlen(line);

            printf("line fget:%s", line);
            if (write(fd1[1], line, n) != n)
            {
                printf("write error %d %s\n", errno, strerror(errno));
                return 0;
            }

            if ((n = read(fd2[0], line, ilinemax)) < 0)
            {
                printf("read error %d %s\n", errno, strerror(errno));
                return 0;
            }
            
            if (n == 0)
            {
                printf("chiled close\n");
                return 0;
            }

            line[n] = 0;
            int nf;
            if ((nf = fputs(line, stdout)) == EOF)
            {
                printf("fputs error %d %s\n", errno, strerror(errno));
                return 0;
            }            
        }

        if (ferror(stdin))
            {
                printf("ferror \n");
                return 0;
            }

        return 0;
    }
    else                //child
    {
        close(fd1[1]);
        close(fd2[0]);

        if (fd1[0] != STDIN_FILENO)
        {
            dup2(fd1[0], STDIN_FILENO);
            close(fd1[0]);
        }

        if (fd2[1] != STDOUT_FILENO)
        {
            dup2(fd2[1], STDOUT_FILENO);
            close(fd2[1]);
        }

        if(execl("./add", "add", "(char*)0") < 0)
        {
            printf("execl error %d %s\n", errno, strerror(errno));
                return 0;
        }

        return 0;        
    }

    return 0;
}

void sig_pipe(int sig)
{
    printf("sig_pipe %d %s\n", sig, strsignal(sig));
}


