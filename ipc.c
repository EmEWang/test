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

// 共享内存   ipcs -m
#include <sys/shm.h>
void testshmsrver();
void testshmcilent();

//posix 消息队列
#include <mqueue.h>
void testposixmsg();

// posix 信号量
#include <semaphore.h>
void testposixsem();
void testposixsem2();

// posix 共享内存
#include <sys/mman.h>
void testposixshm();


// UNIX域套接字 本机双工管道
int unixpipe(int fd[2]);  //0 success -1 failed
void testunixpipe();
void testunixsocket();
void testunixsocketserver();
void testunixsocketclient();


// 发送fd
void send_fd(int socket, int *fds, int n);
void recv_fd(int socket, int** fds, int n);
void send_fd_send();
void send_fd_recv();



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
    // testshmsrver();
    // testshmcilent();
    // testposixmsg();
    // testposixsem();
    testposixsem2();
    // testunixpipe();
    // testunixsocket();
    // testunixsocketserver();
    // testunixsocketclient();
    // send_fd_send();
    // send_fd_recv();

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
// 此结构 1为long 2以后是数据 也可以有34等字段 用的时候去掉1字段的长度
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


    // 生成key 1参数 路径名 2参数 项目id
    key_t ky = ftok("./ipc", 1);
    printf("generate key:%d\n", ky);
	// 建立消息队列
    // 1参数为 IPC_PRIVATE 则创建新id 同时2参数 flag要指明IPC_CTREAT
	msgid = msgget((key_t)1234, 0666 | IPC_CREAT);
    printf("msdid:%d\n", msgid);
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
        //3参数nbytes去掉type字段长度
        //4参数flag为0 阻塞方式  IPC_NOWAIT 非阻塞方式 失败返回-1 error为EAGAIN
        //linux限制 最大长度81292 一个特定队列最大字节16284
        //当队列满时 默认是阻塞的
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
        //4参数type 0返回第一条消息  >0队列中消息类型为type的第一条消息
        //<0队列消息类型绝对值<=type的类型最小消息
        //5参数falg 0默认阻塞 IPC_NOWAIT 不阻塞 若没消息返回-1 error为ENOMSG
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
    // 2参数 IPC_RMID 删除 IPC_STAT 获取msqid_ds(3参数) IPC_SET 设置 msqid_ds
    // 3参数 msqid_ds 指针
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
    //信号创建 semget 独立于信号初始化 semctl
    int nsems = 6; //设置信号量集的信号量数量 >1表示某个id有一组信号量
    //2参数 信号量的数量
    //3参数为权限
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
    unsigned short arrs[16] = {0};
	stat.buf = &semds;
    stat.array = arrs;
    //每个信号量的结构如
    // struct {
    //     unsigned short semval;  //信号量值 >=0
    //     pid_t          sempid;  //上次操作的PID
    //     unsigned short semncnt; //等候的进程数量 senmval>curval
    //     unsigned short semvcnt; //等候的进程数量 senmval==0
    //     ...
    // };


    //2参数 semnum
    //3参数 IPC_STAT 取 semid_ds 结构 IPC_SET 设置 semid_ds IPC_RMID 立即删除信号量 再读出错 EIDRM
    //      GETVAL  取 semnum 的 semval
    //      SETVAL  设 semnum 的 semval 由4参数  arg.val 指定
    //      GETPID  取 semnum 的 sempid
    //      GETNCNT 取 semnum 的 semncnt
    //      GEZNCNT 取 semnum 的 semzvnt
    //      GETALL  取集合中所有的信号量值 存储在arg.array
    //      SETALL  设置集合中所有的信号量值为arg.array
    // 除了 GETALL 所有的GET命令返回相应的值 其他命令成功0 错误-1 设置erro
    semctl(sem_id, 0, IPC_STAT,&stat);
    printf("uid = %d,mode = %o,sem_otime = %ld,sem_nsems = %ld\n",
        semds.sem_perm.uid,semds.sem_perm.mode,semds.sem_otime,semds.sem_nsems);
    printf("semval = %d,semncnt = %d\n",semctl(sem_id, 0, GETVAL), semctl(sem_id, 0, GETNCNT));


    //定义信号量集操作
    struct sembuf buf[2];
    buf[0].sem_num = 0; //除非使用一组信号量 否则它为0 0表示第一个信号量 1表示第二个信号量 以此类推
    buf[0].sem_op = 3; //正值释放资源数 负值申请资源
    buf[0].sem_flg = IPC_NOWAIT; //0 阻塞模式 IPC_NOWAIT 非阻塞

    buf[1].sem_num = 1;
    buf[1].sem_op = 14;
    buf[1].sem_flg = IPC_NOWAIT;

    //semctl(sem_id, 0, GETALL,arrs);

    //对信号量集操作
    // 3参数 npos 操作集合中的数量 其值不用超过getsem中设置的nsems
    if (semop(sem_id, buf, 2) < 0)
    {
        printf("semop\n");
        exit(1);
    }

    //GETALL 第二个参数 不考虑
    semctl(sem_id, 12, GETALL,arrs);

    semctl(sem_id, 0, IPC_STAT,&stat);
    printf("uid = %d,mode = %o,sem_otime = %ld,sem_nsems = %ld\n",
        semds.sem_perm.uid,semds.sem_perm.mode,semds.sem_otime,semds.sem_nsems);
    //semval 当前的信号量的值
    printf("semval = %d,semncnt = %d\n",semctl(sem_id, 0, GETVAL), semctl(sem_id, 0, GETNCNT));
    printf("semval = %d,semncnt = %d\n",semctl(sem_id, 1, GETVAL), semctl(sem_id, 1, GETNCNT));

    buf[0].sem_op = -1;
    buf[1].sem_op = -2;
    if (semop(sem_id, buf, 2) < 0)
    {
        printf("semop\n");
        exit(1);
    }
    printf("semval = %d,semncnt = %d\n",semctl(sem_id, 0, GETVAL), semctl(sem_id, 0, GETNCNT));
    printf("semval = %d,semncnt = %d\n",semctl(sem_id, 1, GETVAL), semctl(sem_id, 1, GETNCNT));
    if (semop(sem_id, buf, 2) < 0)
    {
        printf("semop\n");
        exit(1);
    }
    printf("semval = %d,semncnt = %d\n",semctl(sem_id, 0, GETVAL), semctl(sem_id, 0, GETNCNT));
    printf("semval = %d,semncnt = %d\n",semctl(sem_id, 1, GETVAL), semctl(sem_id, 1, GETNCNT));
    if (semop(sem_id, buf, 2) < 0)
    {
        printf("semop\n");
        exit(1);
    }
    semctl(sem_id, 0, IPC_STAT,&stat);
    printf("uid = %d,mode = %o,sem_otime = %ld,sem_nsems = %ld\n",
        semds.sem_perm.uid,semds.sem_perm.mode,semds.sem_otime,semds.sem_nsems);
    printf("semval = %d,semncnt = %d\n",semctl(sem_id, 0, GETVAL), semctl(sem_id, 0, GETNCNT));
    printf("semval = %d,semncnt = %d\n",semctl(sem_id, 1, GETVAL), semctl(sem_id, 1, GETNCNT));

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

void testshmsrver()
{
    int shmid = shmget((key_t)1212, 1024, 0666|IPC_CREAT);
    if (shmid == -1)
    {
        printerr("shmget");
        return;
    }

    printf("shmid:%d\n", shmid);
    struct shmid_ds shmi;
   int ret = shmctl(shmid, IPC_STAT, &shmi);
   if (ret == -1)
    {
        printerr("shmctl");
        return;
    }

    void* ptr = shmat(shmid, 0, 0);
    if (ptr == (void*)-1)
    {
        printerr("shmat");
        return;
    }
    strcpy(ptr, "12345");
    ret = shmdt(ptr);
    if (ret == -1)
    {
        printerr("shmctl");
        return;
    }
}

void testshmcilent()
{
    int shmid = shmget((key_t)1212, 0, 0666);
    if (shmid == -1)
    {
        printerr("shmget");
        return;
    }

    void* ptr = shmat(shmid, 0, 0);
    if (ptr == (void*)-1)
    {
        printerr("shmat");
        return;
    }


    printf("shmid:%d context:%s\n", shmid, (char*)ptr);

    int ret = 0;
    ret = shmdt(ptr);
    if (ret == -1)
    {
        printerr("shmdt");
        return;
    }

    ret = shmctl(shmid, IPC_RMID, 0);
    if (ret == -1)
    {
        printerr("shmctl");
        return;
    }
}

// gcc编译的时候，使用 -l 参数加上 rt 库
// posix_mqsend.c : gcc posix_mqsend.c -o posix_mqsend -lrt
// posix_mqrecv.c : gcc posix_mqrecv.c -o posix_mqsend -lrt
//
// vscode tasks.json
// "args": [
//                 "-fdiagnostics-color=always",
//                 "-g",
//                 "-pthread",
//                 "${file}",
//                 "-o",
//                 "${fileDirname}/${fileBasenameNoExtension}",
//                 "-lrt"         <---------------   放到这里 若放到 "-pthread"之后则报错
//             ],

// 消息队列和管道和FIFO有很大的区别，主要有以下两点：
// 一个进程向消息队列写入消息之前，并不需要某个进程在该队列上等待该消息的到达，
//   而管道和FIFO是相反的，进程向其中写消息时，管道和FIFO必需已经打开来读，
//   否则写进程就会阻塞（默认情况下），那么内核会产生SIGPIPE信号。
// IPC的持续性不同。管道和FIFO是随进程的持续性，当管道和FIFO最后一次关闭发生时，
//   仍在管道和FIFO中的数据会被丢弃。消息队列是随内核的持续性，即一个进程向消息队列写入消息后，
//   然后终止，另外一个进程可以在以后某个时刻打开该队列读取消息。只要内核没有重新自举，消息队列没有被删除。

// POSIX消息队列的创建和关闭
// POSIX消息队列的创建，关闭和删除用到以下三个函数接口：
// #include <mqueue.h>
// mqd_t mq_open(const char *name, int oflag, /* mode_t mode, struct mq_attr *attr */);
// //成功返回消息队列描述符，失败返回-1
// mqd_t mq_close(mqd_t mqdes);
// mqd_t mq_unlink(const char *name);
//  //成功返回0，失败返回-1
//
// mq_open 用于打开或创建一个消息队列。
// name:表示消息队列的名字，它符合POSIX IPC的名字规则。
// oflag:表示打开的方式，和open函数的类似。有必须的选项：O_RDONLY，O_WRONLY，O_RDWR，
//   还有可选的选项：O_NONBLOCK，O_CREAT，O_EXCL。
// mode:是一个可选参数，在oflag中含有O_CREAT标志且消息队列不存在时，才需要提供该参数。
//  表示默认访问权限。可以参考open。
// attr:也是一个可选参数，在oflag中含有O_CREAT标志且消息队列不存在时才需要。
//   该参数用于给新队列设定某些属性，如果是空指针，那么就采用默认属性。
// mq_open返回值是mqd_t类型的值，被称为消息队列描述符。在Linux 2.6.18中该类型的定义为整型：
// #include <bits/mqueue.h>
// typedef int mqd_t;
//
// mq_close 用于关闭一个消息队列，和文件的close类型，关闭后，消息队列并不从系统中删除。
//  一个进程结束，会自动调用关闭打开着的消息队列。
// mq_unlink 用于删除一个消息队列。消息队列创建后只有通过调用该函数或者是内核自举才能进行删除。
//   每个消息队列都有一个保存当前打开着描述符数的引用计数器，和文件一样，因此本函数能够实现类似于unlink函数删除一个文件的机制。
// POSIX消息队列的名字所创建的真正路径名和具体的系统实现有关，
//   关于具体POSIX IPC的名字规则可以参考《UNIX 网络编程 卷2：进程间通信》的P14。
// 经过测试，在Linux 2.6.18中，所创建的POSIX消息队列不会在文件系统中创建真正的路径名。
//   且POSIX的名字只能以一个’/’开头，名字中不能包含其他的’/’。

// POSIX消息队列的属性
// POSIX标准规定消息队列属性mq_attr必须要含有以下四个内容：
// long mq_flags //消息队列的标志：0或O_NONBLOCK,用来表示是否阻塞
// long mq_maxmsg //消息队列的最大消息数
// long mq_msgsize //消息队列中每个消息的最大字节数
// long mq_curmsgs //消息队列中当前的消息数目
//
// 在Linux 2.6.18中mq_attr结构的定义如下：
// #include <bits/mqueue.h>
// struct mq_attr
// {
//   long int mq_flags;      /* Message queue flags.  */
//   long int mq_maxmsg;   /* Maximum number of messages.  */
//   long int mq_msgsize;   /* Maximum message size.  */
//   long int mq_curmsgs;   /* Number of messages currently queued.  */
//   long int __pad[4];
// };

// POSIX消息队列的属性设置和获取可以通过下面两个函数实现：
// #include <mqueue.h>
// mqd_t mq_getattr(mqd_t mqdes, struct mq_attr *attr);
// mqd_t mq_setattr(mqd_t mqdes, struct mq_attr *newattr, struct mq_attr *oldattr);
//                                //成功返回0，失败返回-1
// mq_getattr用于获取当前消息队列的属性，mq_setattr用于设置当前消息队列的属性。
//   其中mq_setattr中的oldattr用于保存修改前的消息队列的属性，可以为空。
// mq_setattr可以设置的属性只有mq_flags，用来设置或清除消息队列的非阻塞标志。
//   newattr结构的其他属性被忽略。mq_maxmsg和mq_msgsize属性只能在创建消息队列时通过mq_open来设置。
//   mq_open只会设置该两个属性，忽略另外两个属性。mq_curmsgs属性只能被获取而不能被设置

// POSIX消息队列的使用
// POSIX消息队列可以通过以下两个函数来进行发送和接收消息：
// #include <mqueue.h>
// mqd_t mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned msg_prio);
// //成功返回0，出错返回-1
// mqd_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned *msg_prio);
// //成功返回接收到消息的字节数，出错返回-1
// #ifdef __USE_XOPEN2K
// mqd_t mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned msg_prio, const struct timespec *abs_timeout);
// mqd_t mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned *msg_prio, const struct timespec *abs_timeout);
// #endif
//
// mq_send 向消息队列中写入一条消息，mq_receive从消息队列中读取一条消息。
// mqdes:消息队列描述符；
// msg_ptr:指向消息体缓冲区的指针；
// msg_len:消息体的长度，其中mq_receive的该参数不能小于能写入队列中消息的最大大小，
//   即一定要大于等于该队列的mq_attr结构中mq_msgsize的大小。如果mq_receive中的msg_len小于该值，
//   就会返回EMSGSIZE错误。POXIS消息队列发送的消息长度可以为0。
// msg_prio:消息的优先级；它是一个小于MQ_PRIO_MAX的数，数值越大，优先级越高。
//   POSIX消息队列在调用mq_receive时总是返回队列中最高优先级的最早消息。
//   如果消息不需要设定优先级，那么可以在mq_send是置msg_prio为0，mq_receive的msg_prio置为NULL。
// 还有两个XSI定义的扩展接口限时发送和接收消息的函数：mq_timedsend和mq_timedreceive函数。
//  默认情况下mq_send和mq_receive是阻塞进行调用，可以通过mq_setattr来设置为O_NONBLOCK。

// POSIX消息队列的限制
// POSIX消息队列本身的限制就是mq_attr中的mq_maxmsg和mq_msgsize，
//   分别用于限定消息队列中的最大消息数和每个消息的最大字节数。在前面已经说过了，
//   这两个参数可以在调用mq_open创建一个消息队列的时候设定。当这个设定是受到系统内核限制的。
// 下面是在Linux 2.6.18下shell对启动进程的POSIX消息队列大小的限制：
// # ulimit -a |grep message
// POSIX message queues     (bytes, -q) 819200
//
// 限制大小为800KB，该大小是整个消息队列的大小，不仅仅是最大消息数*消息的最大大小；
//   还包括消息队列的额外开销。前面我们知道Linux 2.6.18下POSIX消息队列默认的最大消息数和消息的最大大小分别为：
// mq_maxmsg = 10
// mq_msgsize = 8192
void testposixmsg()
{
    mqd_t mqID;	//创建消息队列描述符
    const char *name = "/anonymQueue";
    mqID = mq_open(name, O_RDWR | O_CREAT, 0666, NULL);

    if (mqID < 0)
    {
        if (errno == EEXIST)
        {
            mq_unlink(name);
            mqID = mq_open(name, O_RDWR | O_CREAT, 0666, NULL);
        }
        else
        {
            printerr("mq_open");
            return;
        }
    }

    struct mq_attr mqAttr;
    if (mq_getattr(mqID, &mqAttr) < 0)
    {
        printerr("mq_getattr");
        return;
    }

    printf("mq_flags:%ld\n",mqAttr.mq_flags);
    printf("mq_maxmsg:%ld\n",mqAttr.mq_maxmsg);
    printf("mq_msgsize:%ld\n",mqAttr.mq_msgsize);
    printf("mq_curmsgs:%ld\n",mqAttr.mq_curmsgs);


    // send
    char msg[] = "yuki";
    for (int i = 1; i <= 5; ++i)
    {
        if (mq_send(mqID, msg, sizeof(msg), i) < 0)	//发送消息
        {
            printerr("mq_send");
        }
        printf("send message %d success prio %d.\n", i, i);
        //sleep(1);
    }

    // recv
    char *buf = malloc(mqAttr.mq_msgsize);	//获得消息队列的大小
    int prio;
    for (int i = 1; i <= 5; ++i)
    {
        if (mq_receive(mqID, buf, mqAttr.mq_msgsize, &prio) < 0)	//接受消息
        {
            printerr("mq_receive");
            continue;
        }
        printf("receive message %d success prio %d. \n", i, prio);
    }

    // 删除消息队列
    mq_unlink(name);

    {
        mqd_t mqID2;	//消息队列描述符
        struct mq_attr attr2;
        attr2.mq_maxmsg = 9;
        attr2.mq_msgsize = 1023;
        const char* name2 = "/anonymQueueX";

        mq_unlink(name2);
        mqID2 = mq_open(name2, O_RDWR | O_CREAT, 0666, &attr2);

        if (mqID2 < 0)
        {
            if (errno == EEXIST)
            {
                mq_unlink(name2);
                mqID2 = mq_open(name2, O_RDWR | O_CREAT, 0666, &attr2);

                if(mqID2 < 0)
                {
                    printerr("mq_open");
                    return;
                }
            }
            else
            {
                printerr("mq_open");
                return;
            }
        }

        struct mq_attr mqAttr;
        if (mq_getattr(mqID2, &mqAttr) < 0)
        {
            printerr("mq_getattr");
            return ;
        }

        printf("mq_flags:%ld\n",mqAttr.mq_flags);
        printf("mq_maxmsg:%ld\n",mqAttr.mq_maxmsg);
        printf("mq_msgsize:%ld\n",mqAttr.mq_msgsize);
        printf("mq_curmsgs:%ld\n",mqAttr.mq_curmsgs);

        mq_unlink(name2);
    }


}

// gcc 编译时，要加 -pthread
void testposixsem()
{
    // 命名信号量
    sem_t *sem;
    const char* name = "/sem_name";
    //参数4 表示初始的信号量值
    sem = sem_open(name, O_CREAT, 0555, 0);
    if(sem == SEM_FAILED)
    {
        printerr("sem_open");
        return;
    }

    int pid = 0;
    if((pid=fork())==0)
    {
        if (sem_wait(sem) == -1)
        {
            printerr("sem_wait");
            return;
        }

        printf("child\n");

        sem_unlink(name);

        exit(0);
    }
    else if (pid > 0)
    {
        sleep(1);
        printf("parent\n");
        sleep(1);
        if (sem_post(sem) == -1)
        {
            printerr("sem_post");
            return;
        }
    }
    else
    {
        printerr("fork");
        exit(0);
    }
}

void testposixsem2()
{
    // 匿名信号量
    sem_t *sem;

    // 信号量放入共享内存
    const char* name = "posixshm_sem";
    int size = 1024;
    int fd = shm_open(name, O_CREAT|O_RDWR, 0666);
    ftruncate(fd, size);
    char * p = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    sem = (sem_t*)p;

    //参数2 pshared 控制信号量的类型，=0 用于同一多线程的同步
    //  >0 用于多个进程间的同步,此时sem必须放在共享内存中
    //参数3 表示初始的信号量值
    int ret = sem_init(sem, 3, 0);
    if(ret == -1)
    {
        printerr("sem_init");
        return;
    }

    int val;
    sem_getvalue(sem, &val);
    printf("sem size %lu val:%d\n", sizeof(sem_t), val);

    int pid = 0;
    if((pid=fork())==0)
    {
        sem_getvalue(sem, &val);
        printf("child sem val:%d\n", val);
        if (sem_wait(sem) == -1)
        {
            printerr("sem_wait");
            return;
        }
        sem_getvalue(sem, &val);
        printf("child wait sem val:%d\n", val);
        sem_destroy(sem);

        exit(0);
    }
    else if (pid > 0)
    {
        sleep(1);
        printf("parent start\n");
        sleep(1);
        if (sem_post(sem) == -1)
        {
            printerr("sem_post");
            return;
        }
        sem_getvalue(sem, &val);
        printf("parent post sem val:%d\n", val);

        sleep(1);
    }
    else
    {
        printerr("fork");
        exit(0);
    }

    munmap(p, size);
    shm_unlink(name);
}

void testposixshm()
{
    const char* name = "posixshm";
    int size = 1024;
    int fd = shm_open(name, O_CREAT|O_RDWR, 0666);
    ftruncate(fd, size);
    char * p = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    memset(p, 'A', size);
    munmap(p, size);


    // 删除
    //shm_unlink(name);
}

int unixpipe(int fd[2])
{
    // 第1个参数domain，表示协议族，只能为AF_LOCAL或者AF_UNIX。
    // 第2个参数type，表示协议，可以是SOCK_STREAM或者SOCK_DGRAM。
    //   用SOCK_STREAM建立的套接字对是管道流，与一般的管道相区别的是，
    //   套接字对建立的通道是双向的，即每一端都可以进行读写。
    // 第3个参数protocol，表示类型，只能为0。
    // 第4个参数sv[2]是接收代表两个套接口的整数数组。每一个文件描述符代表一个套接口，并且与另一个并没有区别。
    // 1.该函数只能用于UNIX域（LINUX）下。
    // 2.只能用于有亲缘关系的进程（或线程）间通信。
    // 3.所创建的套节字对作用是一样的，均能够可读可写（而管道PIPE只能进行单向读或写）。
    // 4.这对套接字可以用于全双工通信，每一个套接字既可以读也可以写。
    //   例如，可以往sv[0]中写，从sv[1]中读；或者从sv[1]中写，从sv[0]中读；
    // 5.该函数是阻塞的,且如果往一个套接字(如sv[0])中写入后，再从该套接字读时会阻塞，
    //   只能在另一个套接字中(sv[1])上读成功；
    // 6. 读、写操作可以位于同一个进程，也可以分别位于不同的进程，如父子进程。
    //   如果是父子进程时，一般会功能分离，一个进程用来读，一个用来写。
    //   因为文件描述副sv[0]和sv[1]是进程共享的，所以读的进程要关闭写描述符, 反之，写的进程关闭读描述符。
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

        for (size_t i = 0; i < 3; i++)
        {
            sprintf(buf, "p send %lu\n", i + 1);
            write(fd[0], buf, strlen(buf));
        }

        n = read(fd[0], buf, 100);
        if(n < 0)
        {
            printerr("parent read");
            return;
        }
        else if(n == 0)
        {
            return;
        }
        else
        {
            write(STDOUT_FILENO, buf, n);
        }
}
    else
    {
        close(fd[0]);
        char buf[100] = {0};
        int n = 0;

        for (size_t i = 0; i < 2; i++)
        {
            sprintf(buf, "c send %lu\n", i + 1);
            write(fd[1], buf, strlen(buf));
        }

        n = read(fd[1], buf, 100);
        if(n < 0)
        {
            printerr("parent read");
            return;
        }
        else if(n == 0)
        {
            return;
        }
        else
        {
            write(STDOUT_FILENO, buf, n);
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
    const char* name = "unix.socket";
    strcpy(un.sun_path, name);

    if ((fd = socket(AF_UNIX,SOCK_STREAM,0)) < 0)
    {
        printerr("socket");
        return;
    }

    unlink(name);

    size = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
    if(bind(fd, (struct sockaddr*)&un, size) < 0)
    {
        printerr("bind");
        return;
    }

    printf("bind success\n");
}

void testunixsocketserver()
{
    struct sockaddr_un un;
    int fd, size;
    un.sun_family = AF_UNIX;
    const char* name = "unix.socket";
    strcpy(un.sun_path, name);

    //建立socket
    if ((fd = socket(AF_UNIX,SOCK_STREAM,0)) < 0)
    {
        printerr("socket");
        return;
    }

    // 删除文件 若存在则会bind失败
    unlink(name);

    // 绑定socket
    size = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
    if(bind(fd, (struct sockaddr*)&un, size) < 0)
    {
        printerr("bind");
        return;
    }

    printf("bind success\n");

    // 监听
    int ret = listen(fd, 10);
    if (ret == -1)
    {
        printerr("listen");
        close(fd);
        return;
    }

    struct sockaddr_un un_c;
    socklen_t len;
    bzero(&un_c, sizeof(un_c));

    for (size_t i = 0; i < 3; i++)
    {
        // 接受远程连接
        int fd_c = accept(fd, (struct sockaddr*)&un_c, &len);
        if (fd_c == -1)
        {
            printerr("listen");
            close(fd);
            return;
        }
        printf("fd_c:%d len:%d name:%s\n", fd_c, len, un_c.sun_path);

        char buf[100] = {0};
        int num = 0;
        while (1)
        {
            snprintf(buf, sizeof(buf), "server %lu %d pid %d\n", i, ++num, getpid());
            int len = 0;
            len = write(fd_c, buf, strlen(buf));

            if((len=read(fd_c, buf, 100)) < 0)
            {
                printerr("read");
                close(fd_c);
                break;
            }
            else if (len == 0)
            {
                close(fd_c);
                printf("client close\n");
                break;
            }
            else
            {
                //printf("recv:%s\n", (char*)buf);
                write(STDOUT_FILENO, buf, len);
            }
        }
    }
}


void testunixsocketclient()
{
    // 客户端可以绑定一个un 也可以不用绑定
    struct sockaddr_un un;
    int fd, size, size_s;
    un.sun_family = AF_UNIX;
    const char* name = "unix.socket_client";
    strcpy(un.sun_path, name);

    if ((fd = socket(AF_UNIX,SOCK_STREAM,0)) < 0)
    {
        printerr("socket");
        return;
    }
    size = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
    unlink(name);
    if (bind(fd, (struct sockaddr*)&un, size) < 0)
    {
        printerr("bind");
        return;
    }


    //连接server
    struct sockaddr_un un_s;
    un_s.sun_family = AF_UNIX;
    const char* name_s = "unix.socket";
    strcpy(un_s.sun_path, name_s);
    size_s = offsetof(struct sockaddr_un, sun_path) + strlen(un_s.sun_path);
    int ret = 0;
    if ((ret=connect(fd, (struct sockaddr*)&un_s, size_s))<0)
    {
        printerr("connect");
        return;
    }

    char buf[100] = {0};
    int len = 0;

    while (1)
    {
        len = read(fd, buf, sizeof(buf));
        write(STDOUT_FILENO, buf, len);
        len = read(STDIN_FILENO, buf, sizeof(buf));
        if(len < 0)
        {
            printerr("read");
            return;
        }
        else if (len == 0)
        {
            write(STDOUT_FILENO, "end\n", 4);
            return;
        }
        else
        {
            if (write(fd, buf, len) != len)
            {
                printerr("write");
                return;
            }
        }
    }
}

// struct cmsghdr *CMSG_FIRSTHDR(struct msghdr *mhdrptr);
// 返回：指向第一个cmsghdr结构的指针，无辅助数据时为NULL

// struct cmsghdr *CMSG_NXTHDR(struct msghdr *mhdrptr, struct cmsghdr *cmsgptr);
// 返回：指向下一个cmsghdr结构的指针，不再有辅助数据对象时为NULL

// unsigned char *CMSG_DATA(struct cmsghdr *cmsgptr);
// 返回：指向与cmsghdr结构关联的数据的第一个字节的指针

// unsigned int CMSG_LEN(unsigned int length);
// 返回：给定数据量下存放到cmsg_len中的值

// unsigned int CMSG_SPACE(unsigned int length);
// 返回：给定数据量下一个辅助数据对象总的大小

// CMSG_LEN和CMSG_SPACE的区别在于，前者不计辅助数据对象中数据部分之后可能的填充字节，
//   因而返回的是用于存放在cmsg_len成员中的值，后者计上结尾处可能的填充字节，
//   因而返回的是用于为辅助对象动态分配空间的大小值。
// CMSG_FIRSTHDR返回指向第一个辅助数据对象的指针，
//   然而如果在msghdr结构中没有辅助数据（或者msg_control为一个空指针，或者cmsg_len小于一个cmsghdr结构的大小）
//   ，那就返回一个空指针。当控制缓冲区中不再有下一个辅助数据对象时，CMSG_NXTHDR也返回一个空指针。
void send_fd(int socket, int *fds, int n)
{
    struct msghdr msg = {0};
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(n * sizeof(int))], data;
    memset(buf, 0, sizeof(buf));
    struct iovec io = {.iov_base = &data, .iov_len = 1};

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(n * sizeof(int));

    memcpy((int*)CMSG_DATA(cmsg), fds, n * sizeof(int));
    if(sendmsg(socket, &msg, 0) < 0)
    {
        printerr("sendmsg");
        return;
    }
}

void recv_fd(int socket, int** fds, int n)
{
    *fds = malloc(n * sizeof(int));
    struct msghdr msg = {0};
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(n * sizeof(int))], data;
    memset(buf, 0, sizeof(buf));
    struct iovec io = {.iov_base = &data, .iov_len = 1};

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    if(recvmsg(socket, &msg, 0) < 0)
    {
        printerr("recvmsg");
        return;
    }

    cmsg = CMSG_FIRSTHDR(&msg);
    memcpy(*fds, (int*)CMSG_DATA(cmsg), n * sizeof(int));

    return;
}

void send_fd_send()
{
    struct sockaddr_un un;
    const char* name = "unix.socket_send_fd";
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    bzero(&un, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);
    int size_s = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);

    int fds = open("socket_send_fd", O_RDWR|O_CREAT, 0666);
    if (connect(fd, (struct sockaddr*)&un, size_s) < 0)
    {
        printerr("connect");
        return;
    }

    send_fd(fd, &fds, 1);

    return;
}

void send_fd_recv()
{
    struct sockaddr_un un;
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    const char* name = "unix.socket_send_fd";
    bzero(&un, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);
    int size_s = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);

    unlink(name);

    int ret = 0;
    ret = bind(fd, (struct sockaddr*)&un, size_s);
    ret = listen(fd, 10);
    int fdc = accept(fd, 0, 0);

    int* fdr= NULL;
    recv_fd(fdc, &fdr, 1);
    if (fdr == NULL)
    {
       printf("NULL fdr\n");
       return;
    }

    char buf[100] = {0};
    sprintf(buf, "revc fd pid:%d\n", getpid());
    write(*fdr, buf, strlen(buf));
    close(*fdr);
}
