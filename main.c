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

#define MAXSIZE 2000
#define _GNU_SOURCE
bool pipe_flag=false;
int pipefd[2];
int status;
int command_size=0;
int temp;
//special inputs "<" ">" "2>"
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
    if (argc!=1)
      printf("execute yash with no arguments");
    else
      {
	do{
	  //reading line
	  line = readlinee();
	  //printf("%s",line);
	  //interpret the code
	  commands = interpret(line);
	  //for (int x=0; commands[x]!=NULL;x++)
	  //printf("%s\n",commands[x]);
	  //execute the code
	  status = execute(commands);
	  //free the memory
	  status=1;
	  free(commands);
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
  
  do{
    c = getchar();
    
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

  if (tokens==NULL)
    printf("no memory allocated");

  token=strtok(line,"  ");
  
  while(token!=NULL)
  {
    tokens[index]= token;
    //printf("%s\n",token);
    if(*token=='|') {
      pipe_flag =true;
      //printf("pipe in place");
    }
      token=strtok(NULL,"  ");
    index++;
    command_size++;
  }

    return tokens;
}

int execute(char **commands)
{
  //pipefd[0] read pipefd[1] write
  int pid;
  int index=0,result;
  char **argv= malloc(sizeof(char*)*MAXSIZE),buf;
  int argc=0;
  
  pid = fork();
  if (pid<0)
    printf("Error forking");
  else if(pid==0)
    {//child
      for (int i=0; i<command_size; i++)
	{
	  result = casefunc(commands[i]);
	  //printf("%i",result);
	  switch (result)
	  {
	  case 0://>
	    //if (oversize(i,command_size))
	     fd1=openfd(commands[i+1],O_WRONLY|O_CREAT,O_DIRECTORY|O_EXCL);
	    fd2=openfd(commands[i-1],O_RDONLY|O_ASYNC|O_CLOEXEC|O_CREAT,O_DIRECTORY|O_EXCL);
	    closefd(STDIN_FILENO);
	    closefd(STDOUT_FILENO);
	    dup3fd(fd1,0,O_CLOEXEC);//0 write //1 read
	    dup3fd(fd2,1,O_CLOEXEC);
	    closefd(fd1);
	    closefd(fd2);
	    while(read(1,&buf,1)>0)
	      write(0,&buf,1);
	    i++;
	    break;
	  case 1://<
	    
	    fd2=openfd(commands[i+1],O_WRONLY|O_ASYNC|O_CLOEXEC|O_CREAT,O_EXCL|O_DIRECTORY);//Need to check if not opened
	    closefd(STDIN_FILENO);
	    
	    dup3fd(fd2,0,O_CLOEXEC);
	    closefd(fd2);
	    //argv[argc]=commands[i+1];
	    //execvp(argv[0],argv);
	    //i++;
	    break;
	  case 2://2>
	    // Code
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
	    //printf("%s",commands[i]);
	    break;
	  }
	}
      argv[argc]=NULL;
      execvp(argv[0],argv);
      perror("COuld not execute");
      exit(1);
    }
  else {
    waitpid(-1, &status, 0);
    printf("return of function");
    free(argv);
    }
  return 0;
}
//casefunc '>'=0 '<'=1 '2>'=2 '|'=3 '&'=4  
int casefunc(char *commands)
{
  
   if (strcmp(commands,"<")==0)
    {
      if (strcmp(commands,">")==0)
       
	  return 0;
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
