#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

char* readInput(void);
int parse(char * string, char ** arguments, char **pipeedArguments);
int findPipe(char *string, char *pipeArguments[2]);
void findSpace(char *string, char** parsed);
void exec(char **arguments);
void pExec(char **arguments, char ** parg);
void rExec(char **arg, char **parg, int mode);
int findOut(char *s, char *directedArguments[2]);
int findIn(char *s, char *directedArguments[2]);
#define MAX_LINE		80 /* 80 chars per line, per command */

int main(void)
{
  char * userInput, *oldInput;
  char *args[MAX_LINE/2 + 1];	/* command line (of 80) has max of 40 arguments */
  char *pargs[MAX_LINE/2 + 1];
  int should_run = 1;
  int mode = 0;
  oldInput = (char*) malloc(sizeof(char));
  

  while (should_run){
      printf("mysh:~$ ");
      fflush(stdout);
      

        userInput = readInput();
        userInput[strlen(userInput) -1] = '\0';
        
        mode = parse(userInput, args, pargs);
         
        if(!strcmp(args[0], "!!"))
        {
          if(oldInput[0] == '\0')
            printf("No commands in history.");
          else
          {
            mode = parse(oldInput, args, pargs);
            exec(args);
          }
        }
        else if(!strcmp(args[0], "exit"))
        {
          should_run =0;
        }
        else
        {
          if(mode == 0)
          {
            exec(args);
          }
          else if (mode == 1)
          {
            pExec(args, pargs);
          }
          else 
          {
           rExec(args, pargs, mode); 
          }
          strcpy(oldInput, userInput);
        }
      /**
        * After reading user input, the steps are:
        * (1) fork a child process
        * (2) the child process will invoke execvp()
        * (3) if command includes &, parent and child will run concurrently
        */
  }

  return 0;
}
char* readInput()
{
  char* input;
  input = (char*) malloc(80 * sizeof(char));
  fgets(input, 80, stdin);
  if(strlen(input) > 0)
  {
    return input;
  }
  else 
    return NULL;
}

int parse(char * s, char **arg, char **parg )
{
  int mode = 0;
  char *parsedarg[2];
  char *inArg[2];
  char *outArg[2];
  mode = findPipe(s, parsedarg);
  if(mode == 0)
    mode = findIn(s, parsedarg);
  if(mode == 0)
    mode = findOut(s, parsedarg);

  if(mode == 1)
  {
    findSpace(parsedarg[0], arg);  //find the first argument
    findSpace(parsedarg[1], parg); //find the second argumetn which can be piped or directed
  }
  else if(mode == 2)
  {
    findSpace(parsedarg[0], arg);
    findSpace(parsedarg[1], parg);
  }
  else if(mode == 3)
  {
    findSpace(parsedarg[0], arg);
    findSpace(parsedarg[1], parg);
  }
  else
    findSpace(s, arg);
  
  return mode;
}
int findPipe(char *s, char *parg[2])
{
  int index;
  for(index = 0; index < 2; index ++)
  {
    parg[index] = strsep(&s, "|");
    if(parg[index] == NULL)
    {
      break;
    }
  }
  if (parg[1] == NULL) 
    return 0;
  else
  {
    return 1;
  }
}
int findIn(char *s, char *parg[2])
{
  int i;
  for(i = 0; i <2; i++)
  {
    parg[i] = strsep(&s, "<");
    if(parg[i] == NULL)
    {
      break; //< not found, break out of loop
    }
  }
 
  if(parg[1] == NULL)// < not found, test for >
    return 0;
  else
  {
    return 2; // < found
  }
}
int findOut(char *s, char *parg[2])
{
  int i;
  for(i = 0; i <2; i++)
  {
    parg[i] = strsep(&s, ">");
    if(parg[i] == NULL)
    {
      break; //< not found, break out of loop
    }
  }
  
  if(parg[1] == NULL)// < not found, test for >
    return 0;
  else
  {
    return 3; // < found
  }
}

void findSpace(char *s, char** arg)
{
  int index;

  for(index = 0; index < 80; index++)
  {
      arg[index] = strsep(&s, " ");
    if(arg[index] == NULL)
      break;
    if(strlen(arg[index]) == 0)
      index--;
  }
}

void exec(char **arg)
{

    int i = 0, bg = 0;
    while(arg[i] != NULL)
    {
      i++;
    }
    if(!strcmp(arg[i-1], "&"))
    {
    bg = 1;
    arg[i -1] = NULL;
    }

  pid_t pid = fork();
  if(pid < 0)
  {
    printf("Failed process creation\n");
    return;
  }
  else if(pid == 0)
  {    
    if(bg)
    {
      
      execvp(arg[0], arg);
      exit(0);
    }
    else
    {
    execvp(arg[0], arg);
    exit(0);
    }
  }
  else if(pid > 0)
  {
    if(bg)
    {
      return;
    }
    else
    {
      wait(NULL);
      printf("Child process complete\n");
      return;
    }
  }
  
}

void pExec(char **arg, char **parg)
{
  int fd[2];
  pid_t pid1;

  if(pipe(fd) == -1)
  {
    printf("Failed pipe creation");
    return ;
  }
  pid1 = fork();
  if(pid1 < 0)
  {
    printf("Failed process creation");
    return ;
  }
  else if (pid1 > 0) //parent
  {
      close(fd[1]);
      dup2(fd[0], STDIN_FILENO);
      close(fd[0]);
      execvp(parg[0], parg);
      return;
  }
  else // pid1 child
  {
    close(fd[0]); // close write end
    dup2(fd[1], STDOUT_FILENO); // duplicate to write end
    close(fd[1]); // close read end
    execvp(arg[0], arg) ;
    return;
  }
}
void rExec(char **arg, char **parg, int mode)
{
  
  pid_t pid;
  int fd;
  if(mode == 2)// <
  {
    pid = fork();
    if(pid < 0)
    {
      printf("Failed process creation");
      return;
    }
    else if (pid == 0)
    {
     
      fd = open(parg[0], O_RDONLY);
      dup2(fd, 0);
      close(fd);
      execvp(arg[0], arg);
    }
    else{
      wait(NULL);
      return;
    }
  }
  else if (mode == 3) //>
  {
     pid = fork();
    if(pid < 0)
    {
      printf("Failed process creation");
      return;
    }
    else if (pid == 0)
    {
      fd = open(parg[0], O_WRONLY | O_CREAT | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
      dup2(fd, STDOUT_FILENO);
      dup2(fd, 2); 
      close(fd);
      execvp(arg[0], arg);
    }
    else{
      wait(NULL);
      return;
    }

  }
}
