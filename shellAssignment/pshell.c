#include <stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<pthread.h>
#define BUFF_SIZE 32

typedef struct {
 char programName[BUFF_SIZE];
 int priority;
 int status; //0 ready 1 running
 char* statusString;
 int pid;
} Process;

Process processTable[5];

void initTable(){
 for(int i =0; i<5; i++){
  processTable[i].pid = -1;
 }
}

void printTable(){
  printf("PID		Priority	Status		Program\n");
  for(int i = 0; i<5; i++){
   if(processTable[i].pid!=-1){
    printf("%d		%d		%s		%s\n", processTable[i].pid, processTable[i].priority, processTable[i].statusString, processTable[i].programName);
   }
  }
}


//Updates the status of the program

void setRunning(int pid){
 for(int i = 0; i<5; i++){
  if(processTable[i].pid == pid){
   processTable[i].status = 1;
   processTable[i].statusString = "Running";
   break;
  }
 }
}

void setReady(int pid){
 for(int i =0; i<5; i++){
  if(processTable[i].pid == pid){
   processTable[i].status = 0;
   processTable[i].statusString = "Ready";
   break;
  }
 }
}


//Returns index of a pid or -1 if pid not present
int findIndex(int pid){
 for(int i = 0; i<5; i++){
  if(processTable[i].pid == pid){
   return i;
  }
 }
 return -1;
}

//Returns pid of process that is running, if one
//otherwise returns -1

int checkIfRunning(){
 for(int i = 0; i < 5; i++){
  if(processTable[i].status == 1){
   return processTable[i].pid;
  }
 }
 return -1;
}


//Finds highest priority pid and returns it

int findNext(){
 int highestPriority = -1;
 int highestPID = 0;
 for(int i = 0; i<5; i++){
  if(processTable[i].priority > highestPriority && processTable[i].pid > -1){
   highestPriority = processTable[i].priority;
   highestPID = processTable[i].pid;
  }
 }
 return highestPID;
}

//Handles checking and finding the process that should run
//stops/starts appropriate process

void runNext(){
 int runningPID = checkIfRunning();
 int nextPID = findNext();

 int nextIndex = findIndex(nextPID);
 if(runningPID>-1){
  int runningIndex = findIndex(runningPID);
  if(processTable[runningIndex].priority<processTable[nextIndex].priority){
   setReady(runningPID);
   kill(runningPID, SIGSTOP);
   setRunning(nextPID);
   kill(nextPID, SIGCONT);
  }
  else if(processTable[runningIndex].priority == processTable[nextIndex].priority)
  {
   if(nextPID != runningPID){
    setReady(nextPID);
    kill(nextPID, SIGSTOP);
  }
  }
 }

 else{
  setRunning(nextPID);
  kill(nextPID, SIGCONT);
 }
}

//Inserts process to table

void insert(Process p){
 for(int i = 0; i<5; i++){
  if(processTable[i].pid==-1){
   processTable[i] = p;
   break;
  }
 }
}

//Deletes process, by setting pid to -1

void setPid(int pid){
 for(int i =0; i<5; i++){
  if(processTable[i].pid == pid){
    processTable[i].pid = -1;
    break;
  }
 }
}

//Handles fork/ execve
void* runProcess(void* args){
 int status = 0;
 Process p =*(Process*) args;
 char* name =  p.programName;
 int priority = p.priority;
 int pid = fork();
 if(pid!=0)
 {
  p.pid = pid;
  insert(p);
  setReady(p.pid);
  kill(p.pid, SIGSTOP);
  runNext();// look at table and say do I have the process with highest priority running
  waitpid(pid, &status, 0);//wait pid pid from child/ p.pid
  //delete the entry
  setPid(p.pid);
  //find next entry
 runNext();
 }
 else
   {
   char * arguments[] = {name,  NULL};
   char * envp[] = {NULL};
   execve(name, arguments, envp);
  }
}

//Main just handles thread creation or I/O
int main(int argc, char * argv[]){
 initTable();
 char buffer[BUFF_SIZE];
 while(1)
 {
 printf("<pshell> ");
 fgets(buffer, BUFF_SIZE, stdin);
 int priority;
 
 //Process through our data
 for(int i = 0; i<BUFF_SIZE; i++)
 {
  if(buffer[i]==' ')
  {
   priority = atoi(&buffer[i+1]);
   buffer[i] = '\0';
   break;
  }
 }
int result = strcmp(buffer, "status\n");
if(result==0){
  printTable();
} 
else{
 pthread_t thread;
  Process p;
  strcpy(p.programName, buffer);
  p.priority = priority;
  pthread_create(&thread, NULL, runProcess,&p);
 }
} 
}
