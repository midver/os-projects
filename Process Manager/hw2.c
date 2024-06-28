#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>


#define MAX_LINE 250
#define MAX_PARAMS 100

struct process{ //background process
  int j_id;
  pid_t p_id;
  char* p_status;
  char p_name[MAX_LINE];
};

//globals
struct process fgps;
int amt_fgps = 0; //1 if foreground process exists, 0 otherwise

struct process bgps[5];
int amt_bgps = 0;



void remove_from_jobs(pid_t pid);

pid_t jid_to_pid(int jid){
  extern int amt_bgps;
  extern struct process bgps[5];
  
  for(int i = 0; i < amt_bgps; ++i){
    if(bgps[i].j_id == jid){
      return bgps[i].p_id;
    }
  }
  return 0;
}

void wait_for_pid(pid_t pid){
  extern int amt_fgps;
  
  int waitCondition = WUNTRACED | WCONTINUED;
  int currentState;
  pid_t childpid;
  printf("waiting for foreground\n");
  childpid = waitpid(pid, &currentState, waitCondition);
  if(WIFSIGNALED(currentState)){
    printf("\n current state: child exited\n");
  }
  if(WIFSTOPPED(currentState)){
    printf("\n current state: child stopped\n");
  }
  printf("foreground child caught\n");
  amt_fgps = 0;
  
  return;
}

void sigint_handler(){ //interupting foreground process
  extern struct process fgps;
  extern int amt_fgps;

  printf("sigint recieved\n");
  if(amt_fgps == 1){ 
    kill(fgps.p_id, SIGINT);
    amt_fgps = 0;
  }
  return;
}
void sigtstp_handler(){ //stopping foreground process
  extern struct process fgps;
  extern int amt_fgps;
  extern struct process bgps[5];
  extern int amt_bgps;
  
  if(amt_fgps == 1){
    kill(fgps.p_id, SIGTSTP);
    amt_fgps = 0;

    if(amt_bgps < 5){
      setpgid(fgps.p_id, fgps.p_id);
      fgps.p_status = "Stopped";
      bgps[amt_bgps] = fgps;
      amt_bgps += 1;
    }
    else{
      printf("Stopped foreground job has been discarded, too many jobs already exist\n");
      kill(fgps.p_id, SIGINT);
    }
  }
  return;
}
void sigchld_handler(){
  printf("a child has terminated or stopped, searching for any children\n");
  int currentState;
  int waitCondition = WNOHANG;
  pid_t childpid;
  childpid = waitpid(-1, &currentState, waitCondition);
  printf("end waitpid\n");
  
  if(childpid > 0){
    printf("caught background process: %d\n", childpid);
    if(WIFEXITED(currentState) || WIFSIGNALED(currentState)){
      printf("child has terminated: removing from jobs list\n");      
      remove_from_jobs(childpid);
    }  
  }
  else if(childpid == -1){
    printf("waitpid error \n");
  }
  else{
    printf("failed to catch bg process\n");
  }
}
int index_of_pid(pid_t pid){
  extern int amt_bgps;
  extern struct process bgps[5];

  for(int i = 0; i < amt_bgps; ++i){
    if(bgps[i].p_id == pid){
      return i;
    }
  }
  return -1;
}
void remove_from_jobs(pid_t pid){
  extern int amt_bgps;
  extern struct process bgps[5];
  
  for(int i = 0; i < amt_bgps; i++){
    if(bgps[i].p_id == pid){
      for(int j = i; j < amt_bgps; j++){
        bgps[j] = bgps[j+1];
      }
      amt_bgps -= 1;
      break;
    }
  }
  return;
}
      
int main(){
  extern int amt_fgps;
  extern int amt_bgps;
  extern struct process fgps;
  extern struct process bgps[5];

  int next_j_id = 1;
  int running = 1;
  char input[MAX_LINE];
  char input2[MAX_LINE]; //stores input for process name since tokenizing messes up input1 
  char command[MAX_LINE];
  char* params[MAX_PARAMS+1] = {NULL};
  char cwd[300];
  char* argv[] = {NULL};
  mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;

  int save_stdin = dup(STDIN_FILENO);
  int save_stdout = dup(STDOUT_FILENO);

  int filemode = 0; //0 = no file, 1 = read, 2 = write, 3 = append, 4 = r/w, 5=r/a

  signal(SIGINT, sigint_handler);
  signal(SIGTSTP, sigtstp_handler);
  signal(SIGCHLD, sigchld_handler);

  char in_file_name[MAX_LINE];
  char out_file_name[MAX_LINE];
  
  while(running){

    
    if(filemode == 2){
      dup2(STDOUT_FILENO, save_stdout);
    }

    
    printf("\nprompt > ");

    //parsing

    gets(input);
    strcpy(input2, input);
    
    int i = 0;
    char* tok = strtok(input, " ");
    // printf("input: %s\n", input);
    //printf("input2: %s\n", input2);
    strcpy(command, tok);
    params[i] = tok;

    
    int inp_found = 0;
    int append = 0;
    
    while(tok != NULL){
      //printf("%d, %s\n", i, tok);
      
      
      if(strcmp(tok, "<") == 0){
	//input file
	tok = strtok(NULL, " ");
	if(tok != NULL){
	  strcpy(in_file_name, tok);
	  inp_found = 1;

	  //open in_file and loop into params, then exit and finish these params?
	  
	} 
      }
      else if(strcmp(tok, ">") == 0){
	//output file
	tok = strtok(NULL, " ");
	if(tok != NULL){
	  strcpy(out_file_name, tok);
	  int outfile = (out_file_name, O_WRONLY | O_CREAT | O_TRUNC, mode);
	  dup2(outfile, STDOUT_FILENO);
	  filemode = 2;	  
	}
      }
      else if(strcmp(tok, ">>") == 0){
	tok = strtok(NULL, " ");
	if(tok != NULL){
	  strcpy(out_file_name, tok);
	  append = 1;
	  int outfile = open(out_file_name, O_WRONLY | O_CREAT | O_TRUNC, mode);
	  dup2(outfile, STDIN_FILENO);
	}
      }	
      else{
	params[i++] = tok;
      }
      
      tok = strtok(NULL, " ");
    }
    params[i] = NULL;

    

    //parsing test
    /*
    printf("\ninput: %s", input);
    printf("\ncommand: %s", command);
    printf("\nprinting params:\n");
    i = 0;
    while(params[i] != NULL){
      printf("%s\n", params[i]);
      i++;
    }
    if(params[i] == NULL){
      printf("NULL\n");
    }
    */


    
    //
    //built ins
    //
    
    if(strcmp(command, "quit") == 0){//quit
      
      running = 0;
      for(int i = 0; i < amt_bgps; ++i){
	kill(bgps[i].p_id, SIGINT);
      }
	
    }
    else if(strcmp(command, "pwd") == 0){//pwd
      if(getcwd(cwd, sizeof(cwd)) != NULL){
	printf("%s\n", cwd);
      }
      else{
	printf("too long");
      }
    }
    else if(strcmp(command, "cd") == 0){//cd
      chdir(params[1]);
    }
    else if(strcmp(command, "jobs") == 0){//jobs
      for(int j = 0; j < amt_bgps; j++){
	printf("[%d] (%d) %s %s\n", bgps[j].j_id, (bgps[j]).p_id, bgps[j].p_status, bgps[j].p_name);
      }
    }
    else if(strcmp(command, "kill") == 0){ //kill
      if(params[1][0] == '%'){
	params[1][0] = '0';
	kill(jid_to_pid(atoi(params[1])), SIGINT);
      }
      else{
	kill(atoi(params[1]), SIGINT);
      }
    }
    else if(strcmp(command, "fg") == 0){//fg
      int newfgpid = -1;
      if(params[1][0] == '%'){
	params[1][0] = '0';
	newfgpid = jid_to_pid(atoi(params[1]));
	kill(newfgpid, SIGCONT);
      }
      else{
	newfgpid = atoi(params[1]);
	kill(atoi(params[1]), SIGCONT);
      }
      if(newfgpid == -1){
        printf("process not found\n");
        continue;
      }
      int idx = index_of_pid(newfgpid);

      amt_fgps = 1;
      fgps = bgps[idx];
      remove_from_jobs(newfgpid);
      fgps.p_status = "Running";
      setpgid(newfgpid, getpgrp());
      wait_for_pid(newfgpid);
      
    }
    else if(strcmp(command, "bg") == 0){//bg
      int resumedpid = -1;
      if(params[1][0] == '%'){
	params[1][0] = '0';
	resumedpid = jid_to_pid(atoi(params[1]));
      }
      else{
	resumedpid = atoi(params[1]);
      }
      if(resumedpid == -1){
	printf("process not found\n");
	continue;
      }
      int idx = index_of_pid(resumedpid);
      if(strcmp(bgps[idx].p_status, "Running") == 0){
	printf("process already running in background\n");
	continue;
      }
      bgps[idx].p_status = "Running";
      kill(resumedpid, SIGCONT);
    }
      
      



      
    
    //
    //new process
    //
    
    else{
      pid_t pid;

      int bg = 0;//check if & is last parameter
      for(int j = 0; j < MAX_PARAMS && params[j] != NULL; j++){
	if(strcmp(params[j], "&") == 0 && params[j+1] == NULL){
	  bg = 1;
	  params[j] = NULL;
	}
      }
      /*printf("command: %s\n", command);
      for(int j = 0; j < MAX_PARAMS && params[j] != NULL; j++){
	
	printf("params: %s\n", params[j]);
      }*/
      
      if(bg && amt_bgps >= 5){
	printf("cannot run, there are too many processes!\n");
	continue;
      }
      if(bg){
	printf("bg process\n");
      }
      pid = fork();
      

      if(pid == 0){//child process
	if(bg == 1){
	  
	  if(setpgid(0, pid) == 0){
	    printf("\npgrp: %d\n", getpgrp());
	  }
	  else{
	    printf("group id set failed: %s\n", strerror(errno));
	  }
	}
	printf("starting child process\n");
	if(execvp(command, params) < 0){
	  if(execv(command, params) < 0){
	    printf("executable not found\n");
	    exit(0); //executable not found
	  }
	}
      }
      else{ //parent process is here
	struct process newps;
	newps.p_id = pid;
	newps.j_id = next_j_id;
	next_j_id++;
        newps.p_status = "Running";
	strcpy(newps.p_name, input2);
	printf("new process: %s", newps.p_name);
	
	if(bg == 0){ //foreground process
	  printf("new foreground\n");
	  fgps = newps;
	  amt_fgps = 1;
	  wait_for_pid(fgps.p_id);
	}
	else{ //background process
	  bgps[amt_bgps] = newps;
	  amt_bgps += 1;
	}
      }
      
    }
    
  }

  printf("Exiting\n");
}

