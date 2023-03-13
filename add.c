#include <stdio.h>
#include <string.h>
#include <time.h>

#include <unistd.h>

const int ilinemax = 1024;
int main()
{
    int i1,i2,n;
    char line[ilinemax];
    char *errorstr = "invalid input\n";
    while((n=read(STDIN_FILENO,line,ilinemax) ) > 0)
    {
        line[n] = 0;



        time_t t = time(0);
        struct tm *tt = localtime(&t);
        fprintf(stdout,"[%d-%02d-%02d %02d:%02d:%02d]",
            tt->tm_year+1970, tt->tm_mon+1, tt->tm_mday, tt->tm_hour, tt->tm_min,tt->tm_sec);
        printf("add read line:%s", line);
        fflush(stdout);     //连接管道后 为全缓冲类型 需要冲洗 否则printf中的不显示

        if(sscanf(line, "%d%d", &i1, &i2) != 2)
        {
            n = strlen(errorstr);
            if (write(STDOUT_FILENO, errorstr, n) != n)
            {
                printf("write error %d\n", getpid());
            }

        }
        else
        {

            fflush(stdout);
            sprintf(line, "add %d %d is %d\n",i1, i2, i1+i2);
            n = strlen(line);
            if (write(STDOUT_FILENO, line, n) != n)
            {
                printf("write add error %d\n", getpid());
            }

        }
    }

    return 0;
}