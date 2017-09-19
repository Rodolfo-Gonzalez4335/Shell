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
bool std_error_flag=false;
bool child_flag=false;
bool pri_n=false;
int pipefd[2];
int pid1,pid2,pid;
int status;
int command_size=0;
int temp;
int saved_stdout,saved_stdin,saved_stderr;

//SPECIAL PIPING &
typedef struct process
{//gotten from GNU
  struct process *next;       /* next process in pipeline */
  char **argv;                /* for exec */
  pid_t pid;                  /* process ID */
  char completed;             /* true if process has completed */
  char stopped;               /* true if process has stopped */
  int status;                 /* reported status value */
} process;

typedef struct job
{
  struct job *next;           /* next active job */
  char *command;              /* command line, used for messages */
  process *first_process;     /* list of processes in this job */
  pid_t pgid;                 /* process group ID */
  char notified;              /* true if user told about stopped job */
  int stdin, stdout, stderr;  /* standard i/o channels */
} job;

job *first_job = NULL;

static int fd1,fd2,fd3;
static void closefd(int);
static int openfd(const char *pathname,int falgs,mode_t mode);
static int dup3fd(int oldfd,int newfd,int flags);

int finderror(char** commands, int start,int size);
int findpipe(char** commands, int start);
char **algorithm(int argc,char **argv,int index, char **commands, int size);
char *readlinee(void);
char **interpret(char* line);
int execute(char** commands);
int casefunc(char *commands);
void SIGhandler(int signo);
//-----------------
job *
find_job (pid_t pgid)
{
  job *j;

  for (j = first_job; j; j = j->next)
    if (j->pgid == pgid)
      return j;
  return NULL;
}
int
job_is_stopped (job *j)
{
  process *p;

  for (p = j->first_process; p; p = p->next)
    if (!p->completed && !p->stopped)
      return 0;
  return 1;
}
//-----------------

int main(int argc, char* argv[])
{

    char *pager, *argv0;
    char *line;
    char **commands;
    int status;//seeing if it does excecute
    saved_stdout=dup(1);
    saved_stdin=dup(0);
    saved_stderr=dup(2);
    if (argc!=1)
      printf("execute yash with no arguments");
    else
      {
	do{
	  signal(SIGINT,SIGhandler);
	  signal(SIGTSTP,SIGhandler);
	  signal(SIGCHLD,SIGhandler);

	  pri_n=false;
	  line = readlinee();
	  commands = interpret(line);
	  status = execute(commands);
	  //free the memory
	  //status=1;
	  //free(commands);
	}while(1);
	printf("End of program");
	free(line);
      }
   
  return 0;
}


void SIGhandler(int signo) {
  switch (signo)
    {
    case SIGINT:
      if (!child_flag)
	{pri_n=true;}
      else
	{
	  kill(getpid(),SIGINT);
	}
      signal(signo,SIG_IGN);
      
      
      signal(SIGINT,SIGhandler);
      break;
    case SIGTSTP:
      if (!child_flag)
	{
	  signal(signo,SIG_IGN);
	  pri_n=true;
	}
      else{
	signal(signo,SIG_DFL);
      }
      signal(SIGTSTP,SIGhandler);
	  break;
    case SIGCHLD:
      
      break;
      }
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
  dup2(saved_stderr,2);
  printf("# ");
  do{
    c = getchar();
    if (c==EOF) exit(0);
    //printf("%c",c);
    //check if EOF must have different out put
    if (c=='\n')
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
  pipe_flag=false;
  std_error_flag=false;

  if (tokens==NULL)
    printf("no memory allocated");

  token=strtok(line,"  ");
  
  while(token!=NULL)
  {
    tokens[index]= token;
    if(strcmp(token,"|")==0) {
      pipe_flag =true;
      //printf("pipe in place");
    }
    if(strcmp(token,"2>")==0)
      std_error_flag=true;
      token=strtok(NULL,"  ");
    index++;
    command_size++;
  }
  //free(line);
  return tokens;
}


int finderror(char** commands, int start,int size){

  for(int i=start; i<size; i++)
    {
      if(strcmp(commands[i],"2>")==0)
	return i;
    }
  perror("No error char found");
  return 0;
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
char **algorithm(int argc,char **argv,int index, char **commands,int command_size)
{
  int result;
  char buf;
  int tempfd;
  int fdflag=false;
  int pid;
  if (std_error_flag)//need to adjut so that it just prints error but not terminate
    {
      int error_index = finderror(commands,index,command_size);
      fd3=openfd(commands[error_index+1],O_RDWR|O_CREAT|O_ASYNC|O_CLOEXEC,O_DIRECTORY|S_IRWXU|S_IRWXG|S_IRWXO);
      close(2);
      dup3fd(fd3,2,O_CLOEXEC);//0 write //1 read
      closefd(fd3);
    }

  
  for (int i=index; i<command_size; i++)
    {
      result = casefunc(commands[i]);

      switch (result)
	{
	case 0://>
	  fd1=openfd(commands[i+1],O_WRONLY|O_ASYNC|O_CREAT|O_CLOEXEC,O_DIRECTORY|O_EXCL);
	     
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
	      fd2=openfd(commands[i+1],O_RDONLY|O_ASYNC|O_CLOEXEC,O_EXCL|O_DIRECTORY);
	      dup3fd(fd2,0,O_CLOEXEC);
	      closefd(fd2);
	      argv[argc]= commands[i+1];
	      while (casefunc(commands[i-1])==-1)
		{
		  argv[argc-1]=commands[i-1];
		  argc--;
		  i--;
		}
	      execlp(argv[argc],argv[argc],NULL);
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
	  //taken care of on top
	  i++;
	  break;
	case 3://|
	  //while (write(pipefd[1],&buf,1)!=0)
	  argv[argc]=NULL;
	  execvp(argv[0],argv);
	  perror("COuld not execute");
	  exit(1);
	  break;
	case 4://&
	  // Code
	  break;
	case 5://fg
	  // code
	  break;
	case 6://bg
	  //
	  break;
	case 7://jobs
	  //
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
    execvp(argv[0],argv);
    perror("Yash could not execute");
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
  int index=0,result;
  char **argv= malloc(sizeof(char*)*MAXSIZE),buf;
  int argc=0;
  int pipeloc;
  child_flag=false;
  
  if (!pipe_flag)
    algorithm(argc,argv,0,commands,command_size);
  else {
    pipeloc=findpipe(commands,0);
    if (pipe(pipefd)<0) perror("pipe creation error");
    pid1=fork();
    if (pid1<0) {perror("error generating child"); exit(1);}
    else if(pid1>0)
      {
	
	pid2=fork();
	 if (pid2<0) {perror("error generating child"); exit(1);}
	 else if (pid2>0){//parent
	   closefd(pipefd[0]);
	   closefd(pipefd[1]);
	   int count=0;
	   while (count<2){
	     waitpid(-1, &status, WUNTRACED | WCONTINUED);
	     
	     if (pid2 == -1) 
	       {
		   perror("waitpid");
		   exit(EXIT_FAILURE);
	       }
	     /*
	       if (WIFEXITED(status)) {
		 printf("child %d exited, status=%d\n", pid, WEXITSTATUS(status));count++;
	       } else if (WIFSIGNALED(status)) {
		 printf("child %d killed by signal %d\n", pid, WTERMSIG(status));count++;
	       }  if (WIFSTOPPED(status)) {
		 printf("%d stopped by signal %d\n", pid,WSTOPSIG(status));
		 printf("Sending CONT to %d\n", pid);
		 sleep(4); //sleep for 4 seconds before sending CONT
		 kill(pid,SIGCONT);
	       } else if (WIFCONTINUED(status)) {
		 printf("Continuing %d\n",pid);
		 }*/
	   }
	 }
	 else 
	   {//child 2
	     child_flag=true;
	     
	     sleep(1);
	     setpgid(0,pid1);
	     closefd(pipefd[1]);//right side of pipe
	     dup3fd(pipefd[0], STDIN_FILENO,0);
	     //closefd(pipefd[0]);
	     algorithm(argc,argv,pipeloc+1,commands,command_size);
	     
	   }
	 
      }
    else 
      {//child 1
	child_flag=true;
	setsid();
	closefd(pipefd[0]);
	dup3fd(pipefd[1], STDOUT_FILENO,0);
	algorithm(argc,argv,0,commands,pipeloc+1);//leftside of pipe
      }
          
  }
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
  else if (strcmp(commands,"fg")==0)
    return 5;
  else if (strcmp(commands,"bg")==0)
    return 6;
  else if (strcmp(commands,"jobs")==0)
    return 7;
  else//need to check for special commands
    return -1;
}
