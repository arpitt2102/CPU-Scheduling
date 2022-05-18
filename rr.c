
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "sch-helpers.h"

/* Global variables */
/* processes array to store data */
process processes[MAX_PROCESSES+1];		
/* total number of processes */	
int numberOfProcesses;
/* next process in the queue */					
int nextP;		
/* Total wait time */						
int totalWait;			
/* Total context switches */				
int contextSwitches;
/* Total CPU utilization time */					
int cpuTime;
/* Total time to finish all processes */
int totalTime;
/* Total turnaround time */
int turnaround;
/* Time Quantum */
int timeQ;

/* Ready and Waiting Queue of struct process_queue */

process_queue readyQ;
process_queue waitQ;

/* CPU pointer */

process *cpu[NUMBER_OF_PROCESSORS];

/* Temporary Queue for storing and sorting data */

process *temQ[MAX_PROCESSES+1];
int temQlen;

/* Variables reset values */

void resetVariables(void){
	numberOfProcesses = 0;
	nextP = 0;
	totalWait = 0;
	contextSwitches = 0;
	cpuTime = 0;
	totalTime = 0;
	turnaround = 0;
	temQlen = 0;
}

/* Counts the total incoming processes */

int totalIncomingProcesses(void){
	return numberOfProcesses - nextP;
}

/* Comparing process id */

int compareProcessId(const void *a, const void *b){
	process *first = (process *) a;
	process *second = (process *) b;
	if (first->pid < second->pid) return -1;
    if (first->pid > second->pid) return 1;
	return 0;
}

/* Currently runnning processes in the CPU */

int totalRunningProcesses(void){
	int runningProcesses = 0;
	int i;
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
		if (cpu[i] != NULL) runningProcesses++;
	}
	return runningProcesses;
}

/* Gets the next process from ready queue and then dequeue it 
from the ready queue and returns it */

process *nextProcess(void){
	if (readyQ.size == 0){
		return NULL;
	}
	process *nextElement = readyQ.front->data;
	dequeueProcess(&readyQ);
	return nextElement;
}

/* Adding the new coming processes to the temporary queue that we created earlier */

void comingProcesses(void){
	while(nextP < numberOfProcesses && processes[nextP].arrivalTime <= totalTime){
		
		/* Getting the address of next process */

		temQ[temQlen] = &processes[nextP];
		temQ[temQlen]->quantumRemaining = timeQ;
		
		/* Incrementing length of temporary queue and of next process */
		
		temQlen++;
		nextP++;
	}
}

/* Move the data from waiting queue to ready queue */

void waitingProcesses(void){
 	int i;
	int waitQSize = waitQ.size;
 	for(i = 0; i < waitQSize; i++){
 		process *nextElement = waitQ.front->data;

		/* Dequeing elements from waiting queue */

 		dequeueProcess(&waitQ);

		/* If current I/O burst is complete then transfer it to 
		temporary queue else enqueue it again to waiting queue */

 		if(nextElement->bursts[nextElement->currentBurst].step == nextElement->bursts[nextElement->currentBurst].length){
 			nextElement->currentBurst++;
 			nextElement->quantumRemaining = timeQ;
 			nextElement->endTime = totalTime;
 			temQ[temQlen++] = nextElement;
 		}
 		else{
 			enqueueProcess(&waitQ, nextElement);
 		}
 	}
}

/* Moving the ready process into idle CPUs */

void readyProcesses(void){
 	int i;
 	qsort(temQ, temQlen, sizeof(process*), compareProcessId);
 	for (i = 0; i < temQlen; i++){
 		enqueueProcess(&readyQ, temQ[i]);
 	}
	temQlen = 0;

	/* For each cpu check if it is free or not. If free, 
	then assign it to next process in the queue. */

 	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
 		if (cpu[i] == NULL) cpu[i] = nextProcess();
	}
}

/* Move processes which just completed their last burst to termination and 
if it is not the last burst then move it to waiting queue */

void runningProcess(void){
	int num = 0;
	process *preemptive[NUMBER_OF_PROCESSORS];
 	int i,j;
 	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
 		if (cpu[i] != NULL){

			/* if current burst is completed then move to next burst */

 			if (cpu[i]->bursts[cpu[i]->currentBurst].step == cpu[i]->bursts[cpu[i]->currentBurst].length){
 				cpu[i]->currentBurst++;

				/* If the cpu I/O burst is less than total number of 
				burst of that cpu then add it to waiting queue to be exectued afterwards 
				else it will exit and calculate the total time */

 				if (cpu[i]->currentBurst < cpu[i]->numberOfBursts){
 					enqueueProcess(&waitQ, cpu[i]);
 				}
				
				/* else terminate it */

 				else{
 					cpu[i]->endTime = totalTime;
 				}
 				cpu[i] = NULL;
 			}
 			else if(cpu[i]->quantumRemaining == 0){
 				preemptive[num] = cpu[i];
 				preemptive[num]->quantumRemaining = timeQ;
 				num++;
 				contextSwitches++;
 				cpu[i] = NULL;
 			}
 		}	
 	}

	/* Sort the preemptive array by its process id and 
	then adding it to the ready queue .*/

 	qsort(preemptive, num, sizeof(process*), compareProcessId);
 	for (j = 0; j < num; j++){
 		enqueueProcess(&readyQ, preemptive[j]);
 	}
}

int main(int argc, char *argv[]){
	int i;
	int readStatus = 0;
	int lastPID;

	/* argv[1] would be the time quantum that 
	we are going to input along with our execution file*/

	timeQ = atoi(argv[1]);

	/* Initializing cpu's to empty */

	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
		cpu[i] = NULL;
	}

	/* Initializing variables to 0 and initializing process queues */

	resetVariables();
	initializeProcessQueue(&readyQ);
	initializeProcessQueue(&waitQ);

	while( (readStatus = (readProcess(&processes[numberOfProcesses]))) ){
		if (readStatus == 1){
			numberOfProcesses++;
		}
	}
	
	/* Sorting array of processes by its arrival time */

	qsort(processes, numberOfProcesses, sizeof(process*), compareByArrival);

	while (1){

		/* Calling all the functions here */

		comingProcesses();
		runningProcess();
		readyProcesses();
		waitingProcesses();
		
		/* Updating the waiting queue */

		for (i = 0; i < waitQ.size; i++){
 			process *nextElement = waitQ.front->data;
 			dequeueProcess(&waitQ);
 			nextElement->bursts[nextElement->currentBurst].step++;
 			enqueueProcess(&waitQ, nextElement);
 		}

		/* Updating the ready queue */

 		for (i = 0; i < readyQ.size; i++){
 			process *nextElement = readyQ.front->data;
 			dequeueProcess(&readyQ);
 			nextElement->waitingTime++;
 			enqueueProcess(&readyQ, nextElement);
 		}

		/* Updating the CPU */

 		for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
 			if(cpu[i] != NULL){
 				cpu[i]->bursts[cpu[i]->currentBurst].step++;
 				cpu[i]->quantumRemaining--;
 			}
 		}

		/* While loop will exit when total running process, total coming 
		processes will be zero. And will also check if the size of waiting 
		queue is zero or not. If yes then the loop will break otherwise not. */
		
		if (totalRunningProcesses() == 0 && totalIncomingProcesses() == 0 && waitQ.size == 0){
			break;
		}

		/* CPU time will be incremented with running processes */

		cpuTime += totalRunningProcesses();

		/* Increments total time by 1 */

		totalTime++;
	}

	/* Calculate turnaround time and total wait time of the processes 
	and at last we are calculating the PID of the last process */

	for(i = 0; i < numberOfProcesses; i++){
		turnaround +=processes[i].endTime - processes[i].arrivalTime;
		totalWait += processes[i].waitingTime;
		if (processes[i].endTime == totalTime){
			lastPID = processes[i].pid;
		}
	}

	/* Printing the report for Round Robin Algorithm */

    printf("\n<--RR-Algorithm Report-->\n"
		"Time Quantum\t\t\t:%d\n"
		"Average wait time\t\t:%.2f units\n"
		"Average turnaround time\t\t:%.2f units\n"
		"Total time to finish processes\t:%d\n"
		"Average CPU utilization\t\t:%.1f%%\n"
		"Number of context Switces\t:%d\n" 
		"PID of last process to finish\t:%d\n\n", 
		timeQ,
		totalWait / (double) numberOfProcesses, 
		turnaround / (double) numberOfProcesses, 
		totalTime, 
		(cpuTime * 100.0) / totalTime, 
		contextSwitches, 
		lastPID);	
	
	return 0;
}
