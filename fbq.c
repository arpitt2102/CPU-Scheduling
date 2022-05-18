
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "sch-helpers.h"
#define levels 3
process processes[MAX_PROCESSES+1];
int numberOfProcesses;
int nextP;
int totalWait;
int contextSwitches;
int cpuTime;
int totalTime;
int turnaround;
int timeQ[levels-1];

process_queue readyQ[levels];
process_queue waitQ;

process *cpu[NUMBER_OF_PROCESSORS];

process *temQ[MAX_PROCESSES+1];
int temQlen;


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

int totalIncomingProcesses(void){
	return numberOfProcesses - nextP;
}

int compareArrivalTime(const void *a, const void *b){
	process *first = (process *) a;
	process *second = (process *) b;
	return first->arrivalTime - second->arrivalTime;
}

int compareProcessId(const void *a, const void *b){
	process *first = (process *) a;
	process *second = (process *) b;
	if (first->pid < second->pid) return -1;
    if (first->pid > second->pid) return 1;
	return 0;
}

int runningProcesses(void){
	int runningProcesses = 0;
	int i;
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
		if (cpu[i] != NULL){
			runningProcesses++;
		}
	}
	return runningProcesses;
}

process *nextProcess(void){
    int readyQ1 = readyQ[0].size;
    int readyQ2 = readyQ[1].size;
    int readyQ3 = readyQ[2].size;
    process *nextElement;
    if(readyQ1 != 0){
        nextElement = readyQ[0].front->data;
	    dequeueProcess(&readyQ[0]);
    }
	else if (readyQ2 != 0){
        nextElement = readyQ[1].front->data;
	    dequeueProcess(&readyQ[1]);
    }
    else {
        nextElement = readyQ[2].front->data;
	    dequeueProcess(&readyQ[2]);
    }
	return nextElement;
}

void comingProcesses(void){
	while(nextP < numberOfProcesses && processes[nextP].arrivalTime <= totalTime){
		temQ[temQlen++] = &processes[nextP++];
	}
}

void waitingProcesses(void){
 	int i;
	int waitQSize = waitQ.size;
 	for(i = 0; i < waitQSize; i++){
 		process *nextElement = waitQ.front->data;
        nextElement->priority = 0;
        nextElement->quantumRemaining = 0;
 		dequeueProcess(&waitQ);
 		if(nextElement->bursts[nextElement->currentBurst].step == nextElement->bursts[nextElement->currentBurst].length){
 			nextElement->currentBurst++;
 			nextElement->endTime = totalTime;
 			temQ[temQlen++] = nextElement;
 		}
 		else{
 			enqueueProcess(&waitQ, nextElement);
 		}
 	}
 }

void readyProcesses(void){
 	int i;
 	qsort(temQ, temQlen, sizeof(process*), compareProcessId);
 	for (i = 0; i < temQlen; i++){
 		enqueueProcess(&readyQ[0], temQ[i]);
 	}
	temQlen = 0;
 	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
 		if (cpu[i] == NULL){
 			cpu[i] = nextProcess();
 		}
 	}
 }

void runningProcess(void){
	int readyQ1 = readyQ[0].size;
    int readyQ2 = readyQ[1].size;
	int readyQ3 = readyQ[2].size;
 	int i;
 	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
 		if (cpu[i] != NULL){
 			if (cpu[i]->bursts[cpu[i]->currentBurst].step != cpu[i]->bursts[cpu[i]->currentBurst].length 
 				&& cpu[i]->quantumRemaining != timeQ[0] && cpu[i]->priority == 0){
 				cpu[i]->quantumRemaining++;
 				cpu[i]->bursts[cpu[i]->currentBurst].step++;
 			}
 			else if(cpu[i]->bursts[cpu[i]->currentBurst].step != cpu[i]->bursts[cpu[i]->currentBurst].length 
 				&& cpu[i]->quantumRemaining == timeQ[0] && cpu[i]->priority == 0){
 				cpu[i]->quantumRemaining = 0;
 				cpu[i]->priority = 1;
 				contextSwitches++;
 				enqueueProcess(&readyQ[1], cpu[i]);
 				cpu[i] = NULL;
 			}
 			else if(cpu[i]->bursts[cpu[i]->currentBurst].step != cpu[i]->bursts[cpu[i]->currentBurst].length 
 				&& cpu[i]->quantumRemaining != timeQ[1] && cpu[i]->priority == 1){
 				if (readyQ1 != 0){
 					cpu[i]->quantumRemaining = 0;
 					contextSwitches++;
 					enqueueProcess(&readyQ[1], cpu[i]);
 					cpu[i] = NULL;
 				}
 				else{
 					cpu[i]->bursts[cpu[i]->currentBurst].step++;
 					cpu[i]->quantumRemaining++;
 				}
 			}
 			else if(cpu[i]->bursts[cpu[i]->currentBurst].step != cpu[i]->bursts[cpu[i]->currentBurst].length 
 				&& cpu[i]->quantumRemaining == timeQ[1] && cpu[i]->priority == 1){
 				cpu[i]->quantumRemaining = 0;
 				cpu[i]->priority = 2;
 				contextSwitches++;
 				enqueueProcess(&readyQ[2], cpu[i]);
 				cpu[i] = NULL;
 			}
 			else if(cpu[i]->bursts[cpu[i]->currentBurst].step != cpu[i]->bursts[cpu[i]->currentBurst].length 
 				&& cpu[i]->priority == 2){
 				if (readyQ1 != 0 || readyQ2 != 0){
 					cpu[i]->quantumRemaining = 0;
 					contextSwitches++;
 					enqueueProcess(&readyQ[2], cpu[i]);
 					cpu[i] = NULL;
 				}
 				else{
 					cpu[i]->bursts[cpu[i]->currentBurst].step++;
 				}
 			}
 			// fcfs part of the algorithm
 			else if (cpu[i]->bursts[cpu[i]->currentBurst].step == cpu[i]->bursts[cpu[i]->currentBurst].length){
 				cpu[i]->currentBurst++;
 				cpu[i]->quantumRemaining = 0;
 				cpu[i]->priority = 0;
 				if(cpu[i]->currentBurst < cpu[i]->numberOfBursts){
 					enqueueProcess(&waitQ, cpu[i]);
 				}
 				else{
 					cpu[i]->endTime = totalTime;
 				}
 				cpu[i] = NULL;
 			}
 		}

 		/* when cpu is idle then check quantums and assign cpu to that process */
		
 		else if (cpu[i] == NULL){
 			if(readyQ1 != 0){
 				process *grabNext = readyQ[0].front->data;
 				dequeueProcess(&readyQ[0]);
 				cpu[i] = grabNext;
 				cpu[i]->bursts[cpu[i]->currentBurst].step++;
 				cpu[i]->quantumRemaining++;
			}
			else if(readyQ2 != 0){
 				process *grabNext = readyQ[1].front->data;
 				dequeueProcess(&readyQ[1]);
 				cpu[i] = grabNext;
 				cpu[i]->bursts[cpu[i]->currentBurst].step++;
 				cpu[i]->quantumRemaining++;
 			}
 			else if(readyQ3 != 0){
 				process *grabNext = readyQ[2].front->data;
 				dequeueProcess(&readyQ[2]);
 				cpu[i] = grabNext;
 				cpu[i]->bursts[cpu[i]->currentBurst].step++;
 				cpu[i]->quantumRemaining = 0;
 			}
 		}	
 	}
}

/* Main execution loop */

int main(int argc, char *argv[]){
	int i;
	int readStatus = 0;
	int lastPID;
	timeQ[0] = atoi(argv[1]);
    timeQ[1] = atoi(argv[2]);

	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
		cpu[i] = NULL;
	}
	resetVariables();
	initializeProcessQueue(&readyQ[levels]);
	initializeProcessQueue(&waitQ);
	
	while( readStatus = (readProcess(&processes[numberOfProcesses]))){
		if (readStatus == 1) numberOfProcesses++;
	}

	/* Sorting processes according to arrival time */

	qsort(processes, numberOfProcesses, sizeof(process*), compareArrivalTime);

	while (1){

		/* Calling the functions */

		comingProcesses();
		runningProcess();
		readyProcesses();
		waitingProcesses();

		/* Updating waiting queue */
		int waitQSize = waitQ.size;
		for (i = 0; i < waitQSize; i++){
 			process *nextElement = waitQ.front->data;
 			dequeueProcess(&waitQ);
 			nextElement->bursts[nextElement->currentBurst].step++;
 			enqueueProcess(&waitQ, nextElement);
 		}
		
		/* Updating ready queue */

 		for (i = 0; i < levels; i++){
 			process *nextElement = readyQ[i].front->data;
 			dequeueProcess(&readyQ[i]);
 			nextElement->waitingTime++;
 			enqueueProcess(&readyQ[i], nextElement);
 		}
 		
		if (runningProcesses() == 0 && totalIncomingProcesses() == 0 && waitQ.size == 0){
			break;
		}

		cpuTime += runningProcesses();
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

	/* Printing the report for feedback queue */

    printf("--FeedBack Queue--\n"
		"Average wait time\t\t:%.2f units\n"
		"Average turnaround time\t\t:%.2f units\n"
		"Total time to finish all processes\t:%d\n"
		"Average CPU utilization\t\t:%.1f%%\n"
		"Number of context Switces\t:%d\n" 
		"PID of last process to finish\t:%d\n", 
		totalWait / (double) numberOfProcesses, 
		turnaround / (double) numberOfProcesses, 
		totalTime, 
		(cpuTime * 100.0) / totalTime, 
		contextSwitches, 
		lastPID);	
	return 0;
}
