
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "sch-helpers.h"

process processes[MAX_PROCESSES+1];
process_queue rQueue;
process_queue wQueue;

//Creating temperory queue of struct process
process *tempQ[MAX_PROCESSES+1];
int tempQLength = 0;

//Creating CPU with the numberOfProcesses parameter
process *CPU[NUMBER_OF_PROCESSORS];

//Global variables
int nextP = 0;
int totalTime = 0;
int numberOfProcesses = 0;
int totalCPUUti = 0;
int totalRunProcess = 0;
int totalWait = 0;
int lastProcess = 0;
int totalTurnaround = 0;
int switches = 0;

/* 
We created this helper function named nextProcess. We will use it to check what is the 
next process. If Ready queue is empty then we return null but if no then we assign nextElement of type process
to the front of the queue. We also remove the element that was previously there.
*/
process *nextProcess(){
    if(rQueue.size == 0){ 
        return NULL;
    }
    process *nextElement = rQueue.front->data;
    dequeueProcess(&rQueue);
    return nextElement;
}

int main(){
    int status = 0;
    int i = 0;
    /* 
    We make the CPU processes NULL in order to get the new CPU everytime on the start 
    of new process. That is why we added it to the top in the main menu.
    */
    for(i = 0; i < NUMBER_OF_PROCESSORS; i++){
        CPU[i] = NULL;
    }
    int comingProcess = numberOfProcesses - nextP;
    
    //initializing queues
    initializeProcessQueue(&rQueue);
    initializeProcessQueue(&wQueue);

    while((status = (readProcess(&processes[numberOfProcesses])))){
        if (status == 1){
            numberOfProcesses++;
        }
        //We check if number of processes are equal to 0 or larger than MAX_PROCESSES. 
        //If so then we output the error message.
        if(numberOfProcesses == 0 || 
           numberOfProcesses > MAX_PROCESSES){
            fprintf(stderr, "Either processes are zero or are more than maximum possible processes\n");
            exit(-1);
        }
    }

    qsort(processes,numberOfProcesses,sizeof(process *), compareByArrival);

    while(1){
        
        /* 
       If the run process becomes zero and there is nothing left in waiting queue then we can exit the while loop
       and output the results.
       */
      if(totalRunProcess == 0  && wQueue.size == 0 && comingProcess == 0){
          break;
       }
        //We will first add the processes in the temporary queue that we created. It will be sorted in the next qsort function.
        while(nextP < numberOfProcesses && 
              processes[nextP].arrivalTime <= totalTime){
            tempQ[tempQLength++] = &processes[nextP++];
        }
        qsort(tempQ,tempQLength,sizeof(process *),compareByPID);
        /*  
        First we will move the prcesses that are finished to waiting queue.
        And then the processes which are not finished, those will also move to waiting queue by freeing the CPU
        */
       int i = 0;
       for(i = 0; i<NUMBER_OF_PROCESSORS; i++){
           if(CPU[i] != NULL){
               if(CPU[i]->bursts[CPU[i]->currentBurst].step == 
                  CPU[i]->bursts[CPU[i]->currentBurst].length){
                   (CPU[i])->currentBurst++;
                   if (CPU[i]->currentBurst < CPU[i]->numberOfBursts){
                       enqueueProcess(&wQueue, CPU[i]);
                   }
                   else {
                       CPU[i]->endTime = totalTime;
                   }
                   CPU[i] = NULL;
               }
           }
       }
       /* 
       We have sorted elements in temperory queue and we now enqueue that in ready queue. 
       Then we reset the length of this temporary queue so that it can be used again.
       */
       for(i = 0; i<tempQLength; i++){
           enqueueProcess(&rQueue, tempQ[i]);
       }
       tempQLength = 0; 
       /* 
       Now we find the next process and schedule it to the CPU.
       */
       for(i = 0; i<NUMBER_OF_PROCESSORS; i++){
           if(CPU[i] == NULL) {
               CPU[i] = nextProcess();
           }
       }
       /* 
       Now we will check if the first element in the waiting queue has completed I/O burst or not.
       If yes, then we move that process to the temporary queue that we have created and dequeue the waiting queue.
       Then we update the waiting queue with the burst steo]p of current function.
       */
      for(i = 0; i<wQueue.size; i++){
          process *nextElement = wQueue.front->data;
          dequeueProcess(&wQueue);
          if(nextElement->bursts[nextElement->currentBurst].step 
          == nextElement->bursts[nextElement->currentBurst].length){
              nextElement->currentBurst++;
              tempQ[tempQLength++] = nextElement;
           }
          else {
            enqueueProcess(&wQueue, nextElement);
          }
       }
      /* 
       Now we should update the waiting and running processes as well as the CPU processes. 
       We will use three for loops for updating the processes.
       */
      for(i = 0; i < wQueue.size; i++){
          process *nextElement = wQueue.front->data;
          dequeueProcess(&wQueue);
          nextElement->bursts[nextElement->currentBurst].step++;
          enqueueProcess(&wQueue,nextElement);
      }
      for(i = 0; i < rQueue.size; i++){
          process *nextElement = rQueue.front->data;
          dequeueProcess(&rQueue);
          nextElement->waitingTime++;
          enqueueProcess(&rQueue, nextElement);
      }
      for(i = 0; i < NUMBER_OF_PROCESSORS; i++){
          if(CPU[i] != NULL){
              CPU[i]->bursts[CPU[i]->currentBurst].step++;
          }
      }
      /* 
       Here we find the total running processes on CPU.
       */
      for(i = 0; i < NUMBER_OF_PROCESSORS; i++){
          if(CPU[i] != NULL){
              totalRunProcess++;
          }
      }
      totalCPUUti = totalCPUUti + totalRunProcess;
      totalTime++;
    }
    /* 
    We calculate the total waitime and turnarouund time here.
    WE wll use that in obtaining average waittime and turnaround time.
    */
    for(i = 0; i < numberOfProcesses; i++){
        totalWait = totalWait + processes[i].waitingTime;
        totalTurnaround += processes[i].endTime - processes[i].arrivalTime;
        if(processes[i].endTime == totalTime) {
            lastProcess = processes[i].pid;
        }
    }
    /* 
    We use the value of waiting time and turnaround time to calculate
    the average of values. Also CPU utilization is in % so we multiplied it with 100 
    to obtain that.
    */
    double wait = totalWait / (double) numberOfProcesses;
    double turnaround = totalTurnaround / (double) numberOfProcesses;
    double utilization = (totalCPUUti * 100.0) / totalTime;

    printf("Average waiting time: %.2f\n"
            "Average Turnaround time: %.2f\n"
            "Time taken by CPU to finish all processes: %d\n"
            "Average CPU Utilization: %.1f\n"
            "Context Switches: %d\n"
            "PID of last process: %d\n", wait, turnaround, totalTime, utilization, switches, lastProcess);

    return 0;
}
