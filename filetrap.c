/*
simple utility to demonstrate the file handle pitfall by Raymond
Compile:
gcc -g3 -o filetrap filetrap.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define OPT_RUNAS_DAEMON 1
#define OPT_CLOSE_STDERR (1<<1)

#define MAX_MESSAGE_SIZE 1024

int g_loglvl=0;
FILE * fptr_log=NULL;
void log_init()
{
    if(fptr_log!=NULL)
        return;
    fptr_log=fopen("filetrap.log", "w");
}
void log_close()
{
    if(fptr_log!=NULL)
        fclose(fptr_log);
}
void d4d(int nlevel, const char * szFormat, ...)
{
    char msg[MAX_MESSAGE_SIZE]={0};
    va_list va;

    if(nlevel<g_loglvl)
        return;

    va_start( va, szFormat );

    vsnprintf( msg, 
		MAX_MESSAGE_SIZE, szFormat, va );

    va_end(va);

    // write to file ...
    if(fptr_log)
       fwrite(msg, strlen(msg), 1, fptr_log);
 
    puts(msg);
}
int daemon_init(unsigned int options)
{
    int i;
    pid_t pid;
    
    pid=fork();
    if(pid<0) // fork failed
    {
       return -1;
    }
    else if (pid !=0 )
    {
        // parent exits 
        exit(0); 
    }
    
    if(options&OPT_CLOSE_STDERR)
    {
        for(i=0;i<=STDERR_FILENO;i++)
        {   
           d4d(0, "standard handle %d is being closed\n", i);
           close(i);
        }
    }

    return 0;
}
void usage(const char * msg)
{
    if(msg)
       puts(msg);

    puts("filetrap: a utility to trigger a serious bug\n"
           "- Raymond Zhang (rev 1.0)\n\n"
           "filetrap [[-|/]d]  run as daemon (default)\n" 
           "         [[-|/]c]  run as console\n"    
           "         [[-|/]w]  take workaround\n" 
           "         [[-|/]h]  show this help\n"
        );
}
int parse_cmdline(int argc, char **argv,
           unsigned int * options)
{
    int i=1,j=0;

    while(i<argc)
    {
       if(argv[i][0]=='-'
           ||argv[i][0]=='/')
          j++; 
       switch(argv[i][j]) 
       {
       case 'h':
       case '?':
          usage(NULL);
          break; 
       case 'd': //run as daemon
          *options |= OPT_RUNAS_DAEMON;
          break;
       case 'c':
          *options &= ~OPT_RUNAS_DAEMON;
          break;
       case 'w': //workaround
          *options &= ~OPT_CLOSE_STDERR;
          break;
       case 'l':
          if( ++i >= argc )
          {
             usage( "-l missing log level value\n" );
             return -1;
          }
          g_loglvl=atoi(argv[i]);
          break;   
       default:
          d4d(0, "bad argument[%d][%d]: %s\n", i, j, argv[i]);
          break;     
       }
       i++;   j=0;
    }
    d4d(0, "got cmd lines options 0x%x\n", *options);

    return 0;
}

int calc_gauge(int p)
{
    p = p == 0 ? 88 : p;

    return (p&100);
}

void do_business()
{
    int    fd_sys,fd_device,fd_output, gauge;
    char buffer[10];

    d4d(0, "begin business logic\n");

    fd_sys=open("filesys.txt",
		 O_RDWR|O_CREAT|O_TRUNC);
    d4d(0, "open sysfile returned %d with errno %d\n", 
           fd_sys, errno);
    if(fd_sys<0 || (errno!=0) )
       perror("open sysfile failed\n");       

    fd_device=open("/dev/huadeng0", O_RDWR|O_CREAT);
    d4d(0, "open device file returned %d with errno %d\n",
          fd_device, errno);   
    if(fd_device<0 || (errno!=0) )
       perror("open device file failed\n");       
    
    fd_output=open("fileout.txt", O_RDWR|O_CREAT);
    d4d(0, "open outfile with return %d with errno %d\n", 
          fd_output, errno);   
    if(fd_output<0 || (errno!=0) )
       perror("open outfile failed\n");       

    //
    // business logic run here
    //
    do
    {
        read(fd_device, buffer, 2);
        buffer[2]=0;

        gauge = calc_gauge(atoi(buffer)); //

        sprintf(buffer, "%d", gauge);
     	
        write(fd_device, buffer, 2);
    
        d4d(0, "sent %s to driver\n", buffer);
        sleep(1);
    }while(1);


    // done, cleanup   
    if(fd_sys>-1) close(fd_sys);
    if(fd_device>-1) close(fd_device);
    if(fd_output>-1) close(fd_output);
}
int main( int argc, char **argv )
{ 
    int ret=0;
    int options=OPT_RUNAS_DAEMON|OPT_CLOSE_STDERR;
    
    log_init();

    ret=parse_cmdline(argc, argv, &options); 
    if(ret<0)
        goto TAG_RET;

    if( (options&OPT_RUNAS_DAEMON)
          && ((ret=daemon_init(options))<0)) 
    {
        d4d(0, "run as daemon failed\n");
        ret=-1; 
    }        

    // conduct business now
    do_business();

TAG_RET:
    d4d(0, "exit with %d, bye\n", ret);

    log_close();
    return ret;	
}

