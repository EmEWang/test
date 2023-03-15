#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <float.h>

#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <bits/stat.h>
#include <sys/sysmacros.h>

#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>

#include <dirent.h>


//C语言不支持默认参数
void printerr(const char* str);
void testtype();
void testdir(const char* dir);
void testconf();
void testFile(const char* name);
void teststdFile(const char* name);
void testsysname();
void testsigal1();
void testthread();
void testthread2();
void testthread_barrier();
void testsyslog();
void testlock();
void testmmap();


// test
int main()
{
    printf("this is test program!\n");

    // testtype();
    // testdir("./");
    // testconf();
    // testFile("/home/lixiang/wjg/1");
    // teststdFile("/home/lixiang/wjg/1");
    // testsysname();
    // testsigal1();
    // testthread();
    // testthread2();
    // testthread_barrier();
    //testsyslog();
    // testlock();
    testmmap();

    printf("test end!\n");
}

void printerr(const char* str)
{
    printf("%s %d %s\n", str, errno, strerror(errno));
}
void testtype()
{
    int ia = 1;
    char ca = 's';
    char sza[100] = "abcd";

    void *pv = (void*)12;

    int bb[10];
    printf("int[10] %d\n",sizeof(bb));   //40

    printf("addr sza %x\n", sza);
    printf("addr ca %x\n", &ca);
    printf("addr pv %x\n", pv);

    printf("size: long %lu\n", sizeof(long));

    struct timespec times;
    //times.tv_nsec = UTIME_NOW;
}

void testdir(const char* strdir)
{
    printf("cur dir:%s\n", strdir);
    DIR *dir;
    struct dirent *di;

    if ((dir=opendir(strdir)) == NULL)
    {
        printf("open dir error %d %s\n", errno, strerror(errno));
        return;
    }

    while ((di=readdir(dir)) != NULL)
    {
        printf("%s\n", di->d_name);
    }

    closedir(dir);
}

void testconf()
{
    printf("_SC_CLK_TCK:%ld\n", sysconf(_SC_CLK_TCK));
    printf("_SC_NZERO:%ld\n", sysconf(_SC_NZERO));
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
    printf("st_mode %x\n", buf.st_mode);
    printf("st_size %ld\n", buf.st_size);
    printf("st_blocks %ld st_blksize %ld\n", buf.st_blocks, buf.st_blksize);
    printf("st_nlink %lu st_inod %lu\n", buf.st_nlink, buf.st_ino);
    printf("st_dev %d/%d\n", major(buf.st_dev), minor(buf.st_dev));
    if(S_ISCHR(buf.st_mode)|| S_ISBLK(buf.st_mode))
    {
        printf("st_rdev %s %d/%d", (S_ISCHR(buf.st_dev)?"charcater":"block"),
            major(buf.st_rdev), minor(buf.st_rdev));
    }


    int fd = open(name, O_RDONLY);
    if(fd == -1)
    {
        printf("open file error %d %s\n", errno, strerror(errno));
        return;
    }
    int val;
    val = fcntl(fd, F_GETFD,0);
    fcntl(fd, F_SETFD, val);
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

        //进程组变成孤儿进程组时 每个进程会接收到 SIGHUP 挂断信号 紧接着又会收到 SIGCONT 信号
        SIG_DFL,SIG_ERR,SIG_IGN;  //信号默认处理方法 返回出错 信号忽略
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

void tfunc1(void *arg)
{
    int *pi = (int*)arg;
    printf("thread id !!! %lu pid %d arg %d\n", pthread_self(), getpid(), *pi);
    pthread_exit((void*)234);
}
void testthread()
{
    pthread_t tid;

    int i = 123;
    void *pv = NULL;
    pthread_create(&tid, NULL, tfunc1, &i);

    pthread_join(tid, &pv);
    printf("thread id ~~~ %lu pid %d return %d\n", pthread_self(), getpid(), (int)pv); //两者一样
    printf("thread id ~~~ %lu pid %d return %d\n", pthread_self(), getpid(), (int*)pv);
}


pthread_once_t ponce = PTHREAD_ONCE_INIT;
void tfunc_once()
{
    printf("tfunc_once\n");
}
void tfunc2()
{
    // 执行一次方法
    pthread_once(&ponce,tfunc_once);
    printf("thread id %lu pid %d\n", pthread_self(), getpid());
    sleep(5);
}
void testthread2()
{
    int count = 5;
    pthread_t *tid = malloc(sizeof(pthread_t) * count);

    for (int i = 0; i < count; i++)
    {
       pthread_create(tid+i, NULL, tfunc2, NULL);
    }

    for (int i = 0; i < count; i++)
    {
        pthread_join(*(tid+i), NULL);
    }

    printf("main thread\n");
    free(tid);
}

pthread_barrier_t b;
void tfunc_barrier()
{
    printf("int tid %lu\n", pthread_self());
    sleep(1);
    pthread_barrier_wait(&b);
}
void testthread_barrier()
{
    int count = 3;

    pthread_barrier_init(&b, NULL, count+1);

    for (int i = 0; i < count; i++)
    {
        pthread_t tid;
        pthread_create(&tid, NULL, tfunc_barrier, NULL);
    }

    pthread_barrier_wait(&b);


    printf("come to barrier\n");
}

#include <syslog.h>
void testsyslog()
{
    // /var/log/syslog
    if(0)
    {
    //setlogmask (LOG_UPTO (LOG_NOTICE));  //设置日志级别
    //setlogmask (0);
    openlog ("exampleprog", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    syslog (LOG_NOTICE, "Program started by User %d", getuid ());
    syslog (LOG_INFO, "A tree falls in a forest %m");
    closelog ();
    }

    if(1)
    {
        syslog(LOG_INFO, "test log ww");
    }


}


int lockfile(int fd, int cmd, int type, int where, off_t off, off_t len)
{
    struct flock lock;

    lock.l_type = type;
    lock.l_whence = where;
    lock.l_start = off;
    lock.l_len = len;

    int ret = fcntl(fd, cmd, &lock);
    if (ret < 0)
    {
        printf("error fcntl %d %s\n", errno, strerror(errno));
        return ret;
    }

    if (cmd == F_GETLK)
    {
        if (lock.l_type == F_UNLCK)
        {
            return 0;
        }

        return lock.l_pid;
    }

    return ret;
}
int lockfile_rd(int fd, int where, off_t off, off_t len)
{
    return lockfile(fd, F_SETLK, F_RDLCK, where, off, len);
}
int lockfilew_rd(int fd, int where, off_t off, off_t len)
{
    return lockfile(fd, F_SETLKW, F_RDLCK, where, off, len);
}
int lockfile_wr(int fd, int where, off_t off, off_t len)
{
    return lockfile(fd, F_SETLK, F_WRLCK, where, off, len);
}
int lockfilew_wr(int fd, int where, off_t off, off_t len)
{
    return lockfile(fd, F_SETLKW, F_WRLCK, where, off, len);
}
int lockfile_un(int fd, int where, off_t off, off_t len)
{
    return lockfile(fd, F_SETLK, F_UNLCK, where, off, len);
}
int testfile_rd(int fd, int where, off_t off, off_t len)
{
    return lockfile(fd, F_GETLK, F_RDLCK, where, off, len);
}
int testfile_wr(int fd, int where, off_t off, off_t len)
{
    return lockfile(fd, F_GETLK, F_WRLCK, where, off, len);
}

void testlock()
{
    int fd = open("lockfile", O_RDWR|O_CREAT, 0666);
    if (fd < 0)
    {
        printerr("open lockfile error");
        return;
    }

    int ret;
    if((ret = testfile_wr(fd, SEEK_SET, 0, 0)) != 0)
    {
        printf("locked pid %d\n", ret);
    }

    if (lockfile_wr(fd, SEEK_SET, 0, 0) < 0)
    {
        printerr("lockfile_wr error");
        return;
    }

    ftruncate(fd, 0);

    char buf[100] = {0};
    snprintf(buf, sizeof(buf), "pid:%d\n", getpid());
    write(fd, buf, strlen(buf));
    close(fd);
}


#include <sys/mman.h>
void testmmap()
{
    int fd = open("./mman.file", O_RDWR);
    if (fd == -1)
    {
        printf("open file error %d\n", errno);
        return;
    }

    void *mm = mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (mm == MAP_FAILED)
    {
        printf("mmap error %d\n", errno);
        return;
    }

    close(fd);

    printf("mm:%s\n", mm);
    sprintf(mm, "abcd12");
    printf("MM:%s\n",mm);

    munmap(mm, 0x1000);
}
