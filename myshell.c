#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define MAX_HISTORY 10



#define BUFFER_SIZE 50

static char buffer[BUFFER_SIZE];

/* Global history variables because handlers cannot take arguments*/

char *history[MAX_HISTORY];
int history_size = 0;



/**
 * setup() reads in the next command line, separating it into distinct tokens
 * using whitespace as delimiters. setup() sets the args parameter as a
 * null-terminated string.
 */

void handle_SIGTSTP() {
	
	/* printing enumerated history list*/
	
	printf("\nHistory[%d]:\n",history_size);
	for(int i = history_size; i>0;i--){
		
		printf("%d: %s",i,history[MAX_HISTORY-i]);
		fflush(stdout);
	}
} 
/* historify() function to add commands to history
   starts by adding up to 10 commands, when the limit of 10 reached
   deletes last element, moves everything down and adds new one on top*/
void historify(char *history[], char command[], int *history_size){
	int i = 0;
	
	if(*history_size < MAX_HISTORY){
		history[MAX_HISTORY-*history_size-1]=strdup(command);
		*history_size=*history_size+1;		
	} else {
		int j = MAX_HISTORY-1;
		for (j;j>0;j--){
			history[j]=strdup(history[j-1]);
		}
		history[0]=strdup(command);
	}
}
/* Modified setup function, now works with history stuff and also has a choice variable
   The choice variable is a signal wether it is a new command or something from history
   Implemented this way for reusability of setup command
   This way it can take strings from history and split them again*/
void setup(char inputBuffer[], char *args[],int *background, char **history, int *history_size, int choice)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
   
    ct = 0;
    
    if(choice==0){
    /*if choice 0, there is either now history
      or it is just a new command
      reads what the user enters on the command line */
    int j = 0;
    /* clean up buffer */
    for(j;j<MAX_LINE;j++){
    	inputBuffer[j]='\0';
    }
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE); 
    } else {
    /* if choice is anything but 0 - it must be a valid number of a history element
       due to previous checks 
       Prepares a specific command from history to be split again*/
    printf("Command from history:%s\n",history[MAX_HISTORY-choice]);
    inputBuffer=strdup(history[MAX_HISTORY-choice]);
    length = strlen(history[MAX_HISTORY-choice]);
    }
	
    /* put it in history in origianl format*/
    historify(history,inputBuffer,history_size);
    
    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */
    if (length < 0){
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

    /* examine every character in the inputBuffer */
    for (i=0;i<length;i++) {
        switch (inputBuffer[i]){
          case ' ':
          case '\t' :               /* argument separators */
            if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                ct++;
            }
            inputBuffer[i] = '\0'; /* add a null char; make a C string */
            start = -1;
            break;
          case '\n':                 /* should be the final char examined */
            if (start != -1){
                    args[ct] = &inputBuffer[start];    
                ct++;
            }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
            break;
          default :             /* some other character */
            if (start == -1)
                start = i;
            if (inputBuffer[i] == '&'){
                *background  = 1;
                start = -1;
                inputBuffer[i] = '\0';
            }
          }
     }   
     
     args[ct] = NULL; /* just in case the input line was > 80 */
}



int main(void)
{
char inputBuffer[MAX_LINE];      /* buffer to hold the command entered */
    int background;              /* equals 1 if a command is followed by '&' */
    char *args[(MAX_LINE/2)+1];  /* command line (of 80) has max of 40 arguments */
    
    /*To keep track of commands*/
    int command_num=1;
    /* A prompt according to assignment specifications */
    printf("Welcome to aashell. My pid is %d\n\n", getpid());
    
    
    /* handler initialization*/
    struct sigaction handler;
    handler.sa_handler = handle_SIGTSTP;
    handler.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &handler, NULL);
 
    
    
    while (1){            /* Program terminates normally inside setup */
       background = 0;
       
       /* cmd prompt*/
       printf("\naashell[%d]:",command_num);
       fflush(stdout);
       
       command_num++;
       setup(inputBuffer,args,&background,history,&history_size,0);     /* get next command */
       
       /* Detecting r command
          if just r - print last command
          if any other number - check if it is valid and then set it up for execution*/
       if (strcmp(args[0],"r")==0){

          if (args[1]==NULL){
          	setup(inputBuffer,args,&background,history,&history_size,history_size-1);
          } else if (atoi(args[1])>history_size){
          	printf("out of bounds\n");
          } else {
          	setup(inputBuffer,args,&background,history,&history_size,atoi(args[1]));
          }
       }
       
       /* detecting an in-built yell command */
       if (strcmp(args[0],"yell")==0){
          int j = 1;
          /* traverse through all arguments */
          for (j;j<(MAX_LINE/2);j++){
          	/* Detecting nullptr */
          	if(args[j]==NULL){
          		break;
          	} else {
          	/* printing the phrase character by character due toupper() */
          		int i = 0;
          		char* st = args[j];
          		for (i;i<strlen(st);i++){
          			printf("%c",toupper(st[i]));
          		}
          	}
          	printf(" ");
          }
          
          printf("\n");
          
       /* detecting an in-built exit command */
       } else if (strcmp(args[0],"exit")==0){
          /* getting the pid to plug in the ps command, translating it to string*/
          int pid = getpid();
          char pidstr[10];
          sprintf(pidstr,"%d",pid);
          
          /* the main payload and concatenation of pid to main*/
          char command[100]="ps -o pid,ppid,pcpu,pmem,etime,user,command -p ";
          strcat(command,pidstr);
          
	  system(command);
       	  exit(0);
       } else {
       	  pid_t pid;
       	  /* fork a child process */
       	  pid = fork();
       	  
       	  if (pid<0){
       	  	/* error check */
       	  	printf("Error\n");
       	  } else if(pid==0){
       	  	/* child executes requested program */
       	  	execvp(args[0],args);
       	  }
       	  
       	  /* status message for a process depending on background variable */
       	  if (background==0){
       	  printf("[Child pid = %d, background = FALSE]\n",pid);
       	  } else {
       	  printf("[Child pid = %d, background = TRUE]\n",pid);
       	  }
       	  
       	  if (pid>0 && background==0){
       	  /* parent waits for the specific child to finish */
       	  	waitpid(pid,NULL,0);
       	  	printf("Child process complete\n");
       	  }
       }
            
	/*
	printf("%d History:\n",history_size);
	for(int i = history_size; i>0;i--){
		printf("%s\n",history[MAX_HISTORY-i]);
		
	}
	*/

    }
}

