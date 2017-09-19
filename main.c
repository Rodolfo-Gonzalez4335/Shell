/**
   Rodolfo Gonzalez rg36763 OS project 1
*/
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <features.h>
#include <signal.h>

#define MAXSIZE 2000
#define _GNU_SOURCE
bool pipe_flag=false;
int pipefd[2];
int status;
int command_size=0;
int temp;
int saved_stdout,saved_stdin;
//special child inputs "|"
//signals to keep in mind SIGINT,SIGTSTP,SIGCHLD
//SPECIAL PIPING &
typedef struct {
  char* std_out;
  char* std_in;
  char* std_error;
}Commands;

static int fd1,fd2;
static void closefd(int);
static int openfd(const char *pathname,int falgs,mode_t mode);
static int dup3fd(int oldfd,int newfd,int flags);

int findpipe(char** commands, int start);
char **algorithm(int argc,char **argv,int index, char **commands);
char *readlinee(void);
char **interpret(char* line);
int execute(char** commands);
int casefunc(char *commands);

int main(int argc, char* argv[])
{

    int pd[2];
    pid_t pid;
    char *pager, *argv0;
    char *line;
    char **commands;
    int status;//seeing if it does excecute
    saved_stdout=dup(1);
    saved_stdin=dup(0);
    if (argc!=1)
      printf("execute yash with no arguments");
    else
      {
	do{
	  //reading line
	  line = readlinee();
	  commands = interpret(line);
	  status = execute(commands);
	  //free the memory
	  status=1;
	  //free(commands);
	}while(status==1);
	printf("End of program");
	free(line);
      }
   
  return 0;
}

static void closefd(int fd){
  if (close(fd)==-1)
    {
      perror("Bad file descriptor:'(");
      exit(1);
     }
}

static int openfd(const char *pathname,int flags,mode_t mode){
  
  temp=open(pathname,flags,mode);
  if (temp==-1)
    {
      perror("BadOpenFileee :/\n");
      exit(1);
    }
  return temp;
}

static int dup3fd(int oldfd,int newfd, int flags)
{
  temp=dup2(oldfd,newfd);
    if (temp ==-1)
  {
    perror("BadFileDescriptor");
      exit(0);
  }
    return temp;
}

char  *readlinee(void){
  char c='\0';
  char *buf= (char*)malloc(sizeof(char)*MAXSIZE);
  int index=0;
  char temp;
  dup2(saved_stdout,0);
  dup2(saved_stdin,1);


  do{
    c = getchar();
    if (c==EOF) perror("getchar error");
    printf("%c",c);
    //check if EOF must have different out put
    if (c == EOF || c=='\n')
      {
	buf[index]='\0';
	return buf;
      }
    else {
      buf[index]=c;
      index++;
    }
  }while(c);
    
    return buf;//should not happen just keeping compiler happy
}

char **interpret(char* line)
{ 

  char **tokens= malloc(sizeof(char*)*MAXSIZE);
  char *token;
  int index=0;
  command_size=0;

  if (tokens==NULL)
    printf("no memory allocated");

  token=strtok(line,"  ");
  
  while(token!=NULL)
  {
    tokens[index]= token;
    if(*token=='|') {
      pipe_flag =true;
      //printf("pipe in place");
    }
      token=strtok(NULL,"  ");
    index++;
    command_size++;
  }
  //free(line);
  return tokens;
}

int findpipe(char** commands, int start)
{
  for(int i=start; i<command_size; i++)
    {
      if(strcmp(commands[i],"|")==0)
	return i;
    }
  perror("No pipe char found");
  return 0;
}
char **algorithm(int argc,char **argv,int index, char **commands)
{
  int result;
  char buf;
  int fd1,fd2,fd3;
  int tempfd;
  int fdflag=false;
  int pid;
        for (int i=index; i<command_size; i++)
	{
	  result = casefunc(commands[i]);

	  switch (result)
	    {
	    case 0://>
	      fd1=openfd(commands[i+1],O_WRONLY|O_CREAT|O_CLOEXEC,O_DIRECTORY|O_EXCL);
	     
	      dup3fd(fd1,0,O_CLOEXEC);//0 write //1 read
	      closefd(fd1);
	      
	      fd2=openfd(commands[i-1],O_RDONLY|O_CLOEXEC,O_DIRECTORY|O_EXCL);
	      dup3fd(fd2,1,O_CLOEXEC);
	      closefd(fd2);
	      
	      while(read(1,&buf,1)>0)
		write(0,&buf,1);
	      argc--;
	      argv[argc]=NULL;
	      i++;
	      break;
	    case 1://<	    
	      pid=fork();
	      if (pid==0)
		{
		  fd2=openfd(commands[i+1],O_RDONLY|O_CLOEXEC,O_EXCL|O_DIRECTORY);
		  //Need to check if not opened
		  //closefd(0);
		  dup3fd(fd2,0,O_CLOEXEC);
		  closefd(fd2);
		  argv[argc]= commands[i+1];
		  argv[argc-1]=commands[i-1];
		  execlp(argv[argc-1],argv[argc],NULL);
		  perror("COuld not execute");
		  exit(1);
		}
	      else if(pid>0) {
		waitpid(-1, &status, 0);
		argc--;
		argv[argc]=NULL;
		i++;
	      }
	      break;
	    case 2://2>
	      fd3=openfd(commands[i+1],O_RDWR|O_CREAT|O_ASYNC|O_CLOEXEC,O_DIRECTORY|S_IRWXU|S_IRWXG|S_IRWXO);
	      dup3fd(fd3,2,O_CLOEXEC);//0 write //1 read
	      closefd(fd3);
	      i++;
	      break;
	    case 3://|
	      // Code
	      break;
	    case 4://&
	      // Code
	      break;
	    case 5://
	      // code
	      break;
	    default:
	      argv[argc]=commands[i];
	      argc++;
	      break;
	    }
	}
	pid=fork();
	if (pid==0)
	  {
	    argv[argc]=NULL;
	    execvp(argv[index],argv);
	    perror("COuld not execute");
	    kill(pid,SIGTERM);
	    exit(1);
	  }
	else if(pid>0)
	  {
	    waitpid(-1, &status, 0);
	  }
	
}

int execute(char **commands)
{
  //pipefd[0] read pipefd[1] write
  int pid;
  int index=0,result;
  char **argv= malloc(sizeof(char*)*MAXSIZE),buf;
  int argc=0;   
  algorithm(argc,argv,0,commands); 
  return status;
}
//casefunc '>'=0 '<'=1 '2>'=2 '|'=3 '&'=4  
int casefunc(char *commands)
{
  if (strcmp(commands,">")==0)
	  return 0;
  else if (strcmp(commands,"<")==0)
    {
      
      return 1;
    }
  else if (strcmp(commands,"2>")==0)
    {
      return 2;
    }
  else if (strcmp(commands,"|")==0)
    {
      return 3;
    }
  else if (strcmp(commands,"&")==0)
    {
      return 4;
    }
  else//need to check for special commands
    return -1;
}
