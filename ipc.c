#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>



void printerr(const char* str);
void testpipe();
void testpopen();
void testpopen2();
void testfifo();

// 消息队列   ipcs -q
#include <sys/msg.h>
void testmsg_send();
void testmsg_recv();

// 信号量     ipcs -s    ipcrm -a sem 删除所有信号
#include <sys/sem.h>
void testsem();

// UNIX域套接字 本机双工管道
int unixpipe(int fd[2]);  //0 success -1 failed
void testunixpipe();
void testunixsocket();


int main()
{
    printf("start ipc!\n");

    // testpipe();
    // testpopen();
    // testpopen2();
    // testfifo();
    // testmsg_send();
    // testmsg_recv();
    // testsem();
    testunixpipe();

    printf("end ipc!\n");
    return 0;
}

void printerr(const char* str)
{
    printf("%s %d %s\n", str, errno, strerror(errno));
}

void testpipe()
{
    int fd[2];

    if(pipe(fd) == -1)
    {
        printerr("pipe");
        return;
    }

    int fk = fork();
    if (fk == -1)
    {
        printerr("fork");
        return;
    }
    else if (fk > 0)
    {
        close(fd[1]);

        char buf[1024];
        int len = read(fd[0], buf, sizeof(buf));

        write(STDOUT_FILENO, "child recv:", 11);
        write(STDOUT_FILENO, buf, len);
        return;
        exit(0);
    }

    char buf[1024];
    int len = read(STDIN_FILENO, buf, sizeof(buf));
    write(fd[1], buf, len);

    return;
}

void testpopen()
{
    FILE * fp = popen("ls -l", "r");

    int c;
    while((c=fgetc(fp)) != EOF)
        fputc(c, stdout);
    int ret = pclose(fp);

    printf("ret:%d\n", ret);
}

void testpopen2()
{
    FILE * fp = popen("./add", "r");

    char line[1024] = {0};
    for (;;)
    {
        fputs("prompt:", stdout);
        fflush(stdout);
        if (fgets(line,1024,fp) == NULL)
        {
            break;
        }
        if(fputs(line, stdout) == EOF)
        {
            printerr("fputs");
            break;
        }
    }

    int ret = pclose(fp);
    if (ret == -1)
    {
        printerr("pclose");
    }
    printf("ret:%d\n", ret);
}

// 如果“只写”方式打开文件，写进程会阻塞直到有一个读进程来读这个FIFO管道。
// 就是说：没有进程来读文件，则写进程会阻塞在open语句。
// 所以要read和write两个程序一起运行才能顺利运行
void testfifo()
{
    const char * pname = "./fifo.file";
    if(remove(pname) == -1)
        printf("remove %s error %d %s\n", pname, errno, strerror(errno));;
    if(mkfifo(pname, 0666|O_CREAT) < 0)
    {
        printerr("mkfifo");
        //return ;
    };

    int fk = fork();
    if (fk < 0)
    {
        printerr("fork");
        return ;
    }
    else if (fk > 0)
    {
        int fd = open(pname, O_WRONLY);
        char * buf = "12345\n";
        write(fd, buf, strlen(buf));
        close(fd);
        return;
    }
    else
    {
        int fd = open(pname, O_RDONLY);
        char buf[1024];
        int len = read(fd, buf,1024);
        close(fd);
        write(STDOUT_FILENO, buf, len);
        return;
    }
}


#define MAX_TEXT 512
struct msg_st
{
	long int msg_type;
	char text[BUFSIZ];
};
void testmsg_send()
{
    int running = 1;
	struct msg_st data;
	char buffer[BUFSIZ];
	int msgid = -1;

	//建立消息队列
	msgid = msgget((key_t)1234, 0666 | IPC_CREAT);
	if(msgid == -1)
	{
		fprintf(stderr, "msgget failed with error: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	//向消息队列中写消息，直到写入end
	while(running)
	{
		//输入数据
		printf("Enter some text: ");
		fgets(buffer, BUFSIZ, stdin);
		data.msg_type = 1;    //注意2
		strcpy(data.text, buffer);
		//向队列发送数据
		if(msgsnd(msgid, (void*)&data, BUFSIZ, 0) == -1)
		{
			fprintf(stderr, "msgsnd failed\n");
			exit(EXIT_FAILURE);
		}
		//输入end结束输入
		if(strncmp(buffer, "end", 3) == 0)
			running = 0;
		sleep(1);
	}
	exit(EXIT_SUCCESS);
}


void testmsg_recv()
{
    int running = 1;
	int msgid = -1;
	struct msg_st data;
	long int msgtype = 0; //注意1

	//建立消息队列
	msgid = msgget((key_t)1234, 0666 | IPC_CREAT);
	if(msgid == -1)
	{
		fprintf(stderr, "msgget failed with error: %d\n", errno);
		exit(EXIT_FAILURE);
	}
	//从队列中获取消息，直到遇到end消息为止
	while(running)
	{
		if(msgrcv(msgid, (void*)&data, BUFSIZ, msgtype, 0) == -1)
		{
			fprintf(stderr, "msgrcv failed with errno: %d\n", errno);
			exit(EXIT_FAILURE);
		}
		printf("You wrote: %s\n",data.text);
		//遇到end结束
		if(strncmp(data.text, "end", 3) == 0)
			running = 0;
	}
	//删除消息队列
	if(msgctl(msgid, IPC_RMID, 0) == -1)
	{
		fprintf(stderr, "msgctl(IPC_RMID) failed\n");
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}


#include <sys/types.h>
#include <sys/ipc.h>
void testsem()
{
    //奇怪的一点创建的信号不能直接申请资源 必须先释放才能在申请 开始资源默认为0
    //创建信号量集
    int nsems = 10; //设置信号量集的信号量数量为10
    int sem_id = semget(IPC_PRIVATE, nsems, 0666);
    if (sem_id < 0)
    {
        printf("创建信号量集失败\n");
        exit(1);
    }
    printf("成功创建信号量集，标识符为 %d\n", sem_id);

    // struct semid_ds sds;
    // union semun {
    //     int val;
    //     struct semid_ds *buf;
    //     unsigned int *array;
    // };
    union semun {
        int              val;    /* Value for SETVAL */
        struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
        unsigned short  *array;  /* Array for GETALL, SETALL */
        struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                    (Linux-specific) */
    };

    union semun stat = {0};
	struct semid_ds semds = {0};
	stat.buf = &semds;
    semctl(sem_id, 0, IPC_STAT,&stat);
    long ii = semds.sem_nsems;
    printf("uid = %d,mode = %o,sem_otime = %ld,sem_nsems = %ld\n",semds.sem_perm.uid,semds.sem_perm.mode,semds.sem_otime,semds.sem_nsems);
    printf("semval = %d,semncnt = %d\n",semctl(sem_id, 0, GETVAL), semctl(sem_id, 0, GETNCNT));


    //定义信号量集操作
    struct sembuf buf;
    buf.sem_num = 0; //除非使用一组信号量，否则它为0
    buf.sem_op = 3; //正值释放资源数 负值申请资源
    buf.sem_flg = IPC_NOWAIT;

    //对信号量集进行释放资源操作
    if (semop(sem_id, &buf, 1) < 0)
    {
        printf("semop\n");
        exit(1);
    }

    semctl(sem_id, 0, IPC_STAT,&stat);
    ii = semds.sem_nsems;
    printf("uid = %d,mode = %o,sem_otime = %ld,sem_nsems = %ld\n",semds.sem_perm.uid,semds.sem_perm.mode,semds.sem_otime,semds.sem_nsems);
    //semval 当前的信号量的值
    printf("semval = %d,semncnt = %d\n",semctl(sem_id, 0, GETVAL), semctl(sem_id, 0, GETNCNT));

    buf.sem_op = -1;
    if (semop(sem_id, &buf, 1) < 0)
    {
        printf("semop\n");
        exit(1);
    }
    printf("semval = %d,semncnt = %d\n",semctl(sem_id, 0, GETVAL), semctl(sem_id, 0, GETNCNT));
    if (semop(sem_id, &buf, 1) < 0)
    {
        printf("semop\n");
        exit(1);
    }
    printf("semval = %d,semncnt = %d\n",semctl(sem_id, 0, GETVAL), semctl(sem_id, 0, GETNCNT));
    if (semop(sem_id, &buf, 1) < 0)
    {
        printf("semop\n");
        exit(1);
    }
    semctl(sem_id, 0, IPC_STAT,&stat);
    ii = semds.sem_nsems;
    printf("uid = %d,mode = %o,sem_otime = %ld,sem_nsems = %ld\n",semds.sem_perm.uid,semds.sem_perm.mode,semds.sem_otime,semds.sem_nsems);
    printf("semval = %d,semncnt = %d\n",semctl(sem_id, 0, GETVAL), semctl(sem_id, 0, GETNCNT));

    //查看系统IPC状态
    system("ipcs -s");

    //删除信号量集
    if (semctl(sem_id, 0, IPC_RMID) < 0)
    {
        printf("删除信号量集失败\n");
        exit(1);
    }

    exit(0);
}



int unixpipe(int fd[2])
{
    // 第1个参数domain，表示协议族，只能为AF_LOCAL或者AF_UNIX。
    // 第2个参数type，表示协议，可以是SOCK_STREAM或者SOCK_DGRAM。用SOCK_STREAM建立的套接字对是管道流，与一般的管道相区别的是，套接字对建立的通道是双向的，即每一端都可以进行读写。
    // 第3个参数protocol，表示类型，只能为0。
    // 第4个参数sv[2]是接收代表两个套接口的整数数组。每一个文件描述符代表一个套接口，并且与另一个并没有区别。
    // 1.该函数只能用于UNIX域（LINUX）下。
    // 2.只能用于有亲缘关系的进程（或线程）间通信。
    // 3.所创建的套节字对作用是一样的，均能够可读可写（而管道PIPE只能进行单向读或写）。
    // 4.这对套接字可以用于全双工通信，每一个套接字既可以读也可以写。例如，可以往sv[0]中写，从sv[1]中读；或者从sv[1]中写，从sv[0]中读；
    // 5.该函数是阻塞的,且如果往一个套接字(如sv[0])中写入后，再从该套接字读时会阻塞，只能在另一个套接字中(sv[1])上读成功；
    // 6. 读、写操作可以位于同一个进程，也可以分别位于不同的进程，如父子进程。如果是父子进程时，一般会功能分离，一个进程用来读，一个用来写。因为文件描述副sv[0]和sv[1]是进程共享的，所以读的进程要关闭写描述符, 反之，写的进程关闭读描述符。
    // errno含义:
    // EAFNOSUPPORT:本机上不支持指定的address。
    // EFAULT： 地址sv无法指向有效的进程地址空间内。
    // EMFILE： 已经达到了系统限制文件描述符，或者该进程使用过量的描述符。
    // EOPNOTSUPP：指定的协议不支持创建套接字对。
    // EPROTONOSUPPORT：本机不支持指定的协议。
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
    if(ret == -1)
        printerr("socketpair");

    return ret;
}

void testunixpipe()
{
    int fd[2];

    if(unixpipe(fd) != 0)
        return;

    int fk = fork();
    if(fk == -1)
    {
        printerr("fork");
        return;
    }
    else if(fk > 0)
    {
        close(fd[1]);
        char buf[100] = {0};
        int n = 0;

        for (size_t i = 0; i < 2; i++)
        {
            sprintf(buf, "p send %d\n", i + 1);
            write(fd[0], buf, strlen(buf));
            int n = read(fd[0], buf, 100);
            if(n < 0)
            {
                printerr("parent read");
                continue;
            }
            else if(n == 0)
            {
                break;
            }
            else
            {
                write(STDOUT_FILENO, buf, n);
            }
        }
    }
    else
    {
        close(fd[0]);
        char buf[100] = {0};
        int n = 0;

        for (size_t i = 0; i < 2; i++)
        {
            sprintf(buf, "c send %d\n", i + 1);
            write(fd[1], buf, strlen(buf));
            int n = read(fd[1], buf, 100);
            if(n < 0)
            {
                printerr("parent read");
                continue;
            }
            else if(n == 0)
            {
                break;
            }
            else
            {
                write(STDOUT_FILENO, buf, n);
            }
        }
    }
}


#include <sys/un.h>
#include <stddef.h>
//#define offsetof(TYPE, MEMBER) ((int)&((TYPE*)0)->MEMBER)
void testunixsocket()
{
    struct sockaddr_un un;
    int fd, size;
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, "unix.socket");

    if ((fd = socket(AF_UNIX,SOCK_STREAM,0)) < 0)
    {
        printerr("socket");
        return;
    }

    size = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
    if(bind(fd, (struct sockaddr*)&un, size) < 0)
    {
        print("bind");
        return;
    }

    printf("bind success\n");
}
