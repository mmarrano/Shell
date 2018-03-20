#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXLINE 80 /* The maximum length command */

/* history variables */
char* history[10];
int historyNum = 0;

/* global variable used with the stack */
char* pastDir;

/* structure for stack */
struct stack{
  char* stk[80];
  int top;
};

/* stack initializing */
typedef struct stack STACK;
STACK s;

/* push method for stack */
void push(char* path){
  if(s.top == (80 - 1)){
    return;
  } else{
    s.top++;
    s.stk[s.top] = strdup(path);
  }
}

/* pop method for stack */
char* pop(){
  char* path = strdup(s.stk[s.top]);
  s.top--;
  return path;
}

/* execute command method */
void execute(char* input){
  /* variables for fork and the child's status */
  pid_t pid;
  int status;

  /* variables for command we are running */
  char* args[MAXLINE/2 + 1];
  int argc = 0;

  /* variable to keep track of if there is an & or not */
  int ampersand = 0;

  /* variables used for cd commands */
  char buf[500];
  char* last;
  char* home = "/home/";
  char* user;
  char* userDir;

  /* parse through the command and make it in correct format */
  args[argc] = strtok(input, " ");
  argc++;
  while(args[argc - 1] != NULL){
    args[argc] = strtok(NULL, " ");
    argc++;
  }
  argc--;

  /* check to see if the last argument of this command in an & */
  if(!strcmp("&", args[argc - 1])){
    args[argc - 1] = NULL;
    argc--;
    ampersand = 1;
  }

  /* all my cd stuff */
  if(!strcmp("cd", args[0])){ /* if just 'cd' is entered, treat it as if it was 'pwd' */
    if(args[1] == NULL){
      system("pwd");
    } else if(!strcmp("~", args[1])){ /* if 'cd ~' is entered, go to home directory */
      /* push current directory onto stack */
      pastDir = strdup(getcwd(buf, sizeof(buf)));
      push(pastDir);

      /* obtain user directory */
      user = getenv("USER");
      userDir = malloc(strlen(home) + strlen(user) + 1);
      strcpy(userDir, home);
      strcat(userDir, user);

      /* go to home directory */
      chdir(userDir);
    } else if(!strcmp("-", args[1])){ /* if 'cd -' is entered, pop stack and go to that directory */
      /* if there is anything in the stack, pop() it and go to that directory */
      if(s.top >= 0){
        last = strdup(pop());
        chdir(last);
      }
    } else{ /* if anything else if entered after cd, enter here */
      /* push current directory onto stack */
      pastDir = strdup(getcwd(buf, sizeof(buf)));
      push(pastDir);

      /* if the cd command does not take us anywhere, pop() it back off the stack because it's useless */
      if(chdir(args[1]) == -1){
        pop();
      }
    }

    return;
  }

  /* Execute command */
  pid = fork();
  if(pid == 0){ /* Child */
    /* execute the command */
    execvp(args[0], args);
    exit(0);
  } else{ /* Parent */
    if(!ampersand){ /* If there was no &, wait for the child to finish, otherwise run concurrently */
      waitpid(pid, &status, 0);
    }
  }
}

/* display history method */
void displayHistory(){
  int i = 0;
  int j = 0;

  /* if there are less than 10 commands in history, we have to print only 'historyNum' commands */
  if(historyNum <= 10){
    for(i = historyNum - 1; i >= 0; i--, j++){
      printf("%i %s\n", i + 1, history[9 - j]);
    }
  } else{ /* if there are more than 10 commands in history, just keep it simple and print the history array */
    for(i = 9; i >= 0; i--, j++){
      printf("%i %s\n", historyNum - j, history[i]);
    }
  }
}

/* run command from history method */
void runHistory(char* args){
  int index;
  int temp;

  /* if history is empty, say so */
  if(historyNum == 0){
    printf("No commands in history.\n");
    return;
  }

  /* if user entered !!, we will execute() history[9], so index will equal 9 */
  if('!' == args[1]){
    index = 9;
  } else{ /* if user entered a number instead of ! after the first exclamation point */
    /* get the number they entered if it is a number */
    index = atoi(&args[1]);

    /* if historyNum is less than 10, we will go from historyNum -> 0 so we do not go out of bounds */
    temp = historyNum - 10;
    if(temp < 0){
      temp = 0;
    }

    /* if the number requested is not valid, say so */
    if(index > historyNum || index < temp){
      printf("No such command in history.\n");
      return;
    }

    /* set index to correct position in history array */
    index = 9 - (historyNum - index);
  }

  /* execute the command */
  execute(history[index]);
}

/* add command to history method */
void addHistory(char* args){
  int i = 0;

  /* if the array is full, free the least recent command put into the history array */
  if(history[0] != NULL){
    free(history[0]);
  }

  /* cycle the history array, so everything is pushed down one array spot to free history[9] for the new command */
  for(i = 0; i < 9; i++){
    history[i] = history[i + 1];
  }

  /* place the newly added command into history[9] */
  history[9] = strdup(args);
  historyNum++;
}

/* main method */
int main(void){
  /* initialize the top of the stack as -1 for error handling */
  s.top = -1;

  /* variables to manage things inside the while loop */
  int should_run = 1;
  int i;

  /* variables used to measure how many executions we will have */
  int numCmds = 0;
  char* cmds[10];

  /* variables used for the arguments */
  char* args = (char*) malloc(MAXLINE);
  size_t args_size = 80;
  int length;

  /* lets get a nice and clean terminal before doing stuff */
  system("clear");

  /* while loop we stay in until 'exit' is entered */
  while(should_run){
    printf("osh>");
    fflush(stdout);

    /* get user input */
    getline(&args, &args_size, stdin);

    /* set end of arguments to null terminator */
    length = strlen(args);
    args[length - 1] = '\0';

    if(!strcmp("exit", args)){ /* if user wants to exit, set should_run to 0 */
      should_run = 0;
    } else if(!strcmp("history", args)){ /* if user wants to view history, then display it */
      displayHistory();
    } else if('!' == args[0]){ /* if user started with an !, run the command they want from history */
      runHistory(args);
    } else{
      /* see if the user wants to run multiple commands, if so strtok() the string and get each of those commands separated */
      cmds[numCmds] = strtok(args, ";");
      numCmds++;
      while(cmds[numCmds - 1] != NULL){
        cmds[numCmds] = strtok(NULL, ";");
        numCmds++;
      }
      numCmds--;

      /* execute each of the commands */
      for(i = 0; i < numCmds; i++){
        addHistory(cmds[i]);
        execute(cmds[i]);
        if(i != numCmds - 1){
          printf("\n");
        }
      }

      numCmds = 0;
    }
  }

  free(args);
  return 0;
}
