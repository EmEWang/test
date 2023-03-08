#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <bits/stat.h>
#include <sys/sysmacros.h>

#include <errno.h>
#include <signal.h>
#include <sys/types.h>

void testFile(const char* name);
void teststdFile(const char* name);
void testsysname();
void testsigal1();

int main()
{
    printf("this is test program!\n");

    int ia = 1;
    char ca = 's';
    char sza[100] = "abcd";

    printf("size: long %lu\n", sizeof(long));

    struct timespec times;
    //times.tv_nsec = UTIME_NOW;

    //testFile("/home/lixiang/wjg/1");
    //teststdFile("/home/lixiang/wjg/1");
    //testsysname();
    testsigal1();

    printf("test end!\n");
}

void testFile(const char* name)
{
    struct stat buf;
    if(stat(name, &buf ) == -1)
    {
        printf("stat error %s\n", name);
        return;
    }

    printf("file %s\n", name);
    printf("st_blocks %ld st_blksize %ld\n", buf.st_blocks, buf.st_blksize);
    printf("st_nlink %lu st_inod %lu\n", buf.st_nlink, buf.st_ino);
    printf("st_dev %d/%d\n", major(buf.st_dev), minor(buf.st_dev));
    if(S_ISCHR(buf.st_mode)|| S_ISBLK(buf.st_mode))
    {
        printf("st_rdev %s %d/%d", (S_ISCHR(buf.st_dev)?"charcater":"block"), 
            major(buf.st_rdev), minor(buf.st_rdev));        
    }

}


void teststdFile(const char* name)
{
    FILE* pf = fopen(name,"r+");

    if (pf == NULL)
    {
        printf("fopen error %s\n", name);
        return;
    }

    fclose(pf);


    const static int bufsize = 48;
    char buf[48] = {0};
    
    FILE *memfile = fmemopen(buf, bufsize, "w+");
    if(memfile == NULL)
    {
        printf("fmemopen error\n");
        return;
    }

    memset(buf, 'a', bufsize-2);
    buf[bufsize-2] = '\0';
    buf[bufsize-1] = 'X';
    fprintf(memfile, "12");
    printf("before flush:%s\n", buf);
    fflush(memfile);
    printf("after flush:%s -> %d %d\n", buf, strlen(buf), ftell(memfile));

    memset(buf, 'b', bufsize-2);
    buf[bufsize-2] = '\0';
    buf[bufsize-1] = 'X';
    fprintf(memfile, "12");
    printf("before fseek:%s\n", buf);
    fseek(memfile,0,SEEK_SET);
    printf("after fseek:%s -> %d %d\n", buf, strlen(buf), ftell(memfile));

    memset(buf, 'c', bufsize-2);
    buf[bufsize-2] = '\0';
    buf[bufsize-1] = 'X';
    fprintf(memfile, "12");
    printf("before fclose:%s\n", buf);
    fclose(memfile);
    printf("after fclose:%s -> %d %d\n", buf, strlen(buf), ftell(memfile));
}


#include <sys/utsname.h>
void testsysname()
{
    char buf[1024] = {0};
    if(gethostname(buf,1024) == -1)
    {
        printf("gethostname error\n");
        return;
    }
    printf("hostname:%s\n", buf);

    struct utsname name;
    if(uname(&name) == -1)
    {
        printf("uname error\n");
        return;
    }
    printf("uname.sysname:%s\n", name.sysname);
    printf("uname.nodename:%s\n", name.nodename);
    printf("uname.release:%s\n", name.release);
    printf("uname.version:%s\n", name.version);
    printf("uname.machine:%s\n", name.machine);
    printf("uname.__domainname:%s\n", name.__domainname);
}


void sig_hup(int signal)
{
    printf("SIGHUP reveive pid=%d\n", getpid());
}
void sig_cont(int signal)
{
    printf("SIGCONT reveive pid=%d\n", getpid());
}
void print_pids(char* str)
{
    printf("%s pid=%d ppid=%d pgid=%d tpgid=%d sid=%d\n",
        str, getpid(), getppid(),getpgrp(),tcgetpgrp(STDIN_FILENO),getsid(0));
}
void testsigal1()
{
    char c;
    pid_t pid;

    print_pids("parents");
    if((pid=fork())<0)
    {
        printf("fork error %d  %s\n",errno, strerror(errno));
        return;
    }

    if(pid > 0)
    {
        sleep(3);
        printf("parent exit\n");
    }
    else
    {
        print_pids("child");

        pid_t cpid;
        if((cpid = fork()) < 0)
        {
            printf("fork error %d  %s\n",errno, strerror(errno));
            return;
        }
        else if(cpid == 0)
        {
            print_pids("child of child");
            return;
        }

        signal(SIGHUP, sig_hup);
        signal(SIGCONT, sig_cont);
        kill(getpid(), SIGTSTP);
        //kill(getpid(), SIGHUP);
        print_pids("child");

        if(read(STDIN_FILENO, &c, 1) != 1)
            printf("read error %d %s\n", errno, strerror(errno));
        
        print_pids("child");
        printf("child exit\n");
    }

    exit(0);
}


