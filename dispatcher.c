/**
 * Author: Jacob Wachs
 * Institution: University of Alabama
 * Course: CS 300 Operating Systems
 *
 * Assignment 4
 */

// <Arrival time>, <priority>, <processor time>
// highest priority assigned to system tasks (priority 0)
// user processes can have priorities 1-3
//
// processes will be terminated after 20 seconds if not already terminated by the dispatcher

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "integer.h"
#include "scanner.h"
#include "queue.h"

typedef struct JOB JOB;
struct JOB {
	int arrivalTime;
	int priority;
	char *processorTime;
};

static void displayJOB(FILE *, void *);

// void terminateProcess();
// void suspendProcess();
// void startProcess();
// void restartProcess();		// Required functions^

static void readInFile(FILE *, QUEUE *);
static void dispatchJobs(QUEUE *, QUEUE *, QUEUE *, QUEUE *, QUEUE *);

int main(int argc, char *argv[]) {
	//printf("test\n");
	if (argc < 2) {
		printf("Error: Not enough command line arguments.\n");
		exit(-1);
	}

	/* Open input file for reading */
	FILE *inputFile = fopen(argv[1], "r");

	/* Create all priority queues */
	QUEUE *jobList = newQUEUE(displayJOB);
	QUEUE *sysQueue = newQUEUE(displayJOB);
	QUEUE *p1q = newQUEUE(displayJOB);
	QUEUE *p2q = newQUEUE(displayJOB);
	QUEUE *p3q = newQUEUE(displayJOB);

	/* Read input file into job dispatch list */
	readInFile(inputFile, jobList);
	printf("jobslist: \n");
	displayQUEUE(stdout, jobList);

	int numJobs = sizeQUEUE(jobList);

	/* Send each job in job dispatch list to its proper priority queue */
	dispatchJobs(jobList, sysQueue, p1q, p2q, p3q);

	printf("joblist:");
	displayQUEUE(stdout, jobList);
	printf("\n");
	printf("sysqueue:");
	displayQUEUE(stdout, sysQueue);
	printf("\n");
	printf("p1:");
	displayQUEUE(stdout, p1q);
	printf("\np2:");
	displayQUEUE(stdout, p2q);
	printf("\np3:");
	displayQUEUE(stdout, p3q);
	printf("\n");

	pid_t pid;
/*
	char *args[3] = {"./process", "5", NULL};

	execvp("./process", args);
*/
	char *args[3];
	args[0] = "./process";
	args[2] = NULL;

	if (sizeQUEUE(sysQueue) > 0) {
		int size = sizeQUEUE(sysQueue);
		pid = fork();
		JOB *j = dequeue(sysQueue);

		char *pTime = j->processorTime;
		args[1] = pTime;

		wait(NULL);
		execvp("./process", args);
	}
	else if (sizeQUEUE(p1q) > 0) {
		pid = fork();
		JOB *j = dequeue(p1q);

		char *pTime = j->processorTime;
		args[1] = pTime;

		wait(NULL);
		execvp("./process", args);
	}
	else if (sizeQUEUE(p2q) > 0) {
		pid = fork();
		JOB *j = dequeue(p2q);

		char *pTime = j->processorTime;
		args[1] = pTime;

		wait(NULL);
		execvp("./process", args);
	}
	else if (sizeQUEUE(p3q) > 0) {
		pid = fork();
		JOB *j = dequeue(p3q);

		char *pTime = j->processorTime;
		args[1] = pTime;

		wait(NULL);
		execvp("./process", args);
	}


	// kill(process->pid, SIGINT);

	return 0;
}

/**
 * Starts a process using the fork() command & returns the pid
 */
static int startProcess() {
	return fork();
}

/**
 * Displays the job object in proper format
 * @fp - file printed to
 * @job - job object to be displayed
 */
static void displayJOB(FILE *fp, void *job) {
	JOB *j = job;
	fprintf(fp, "<%d>, <%d>, <%s>\n", j->arrivalTime, j->priority, j->processorTime);
}

/**
 * Reads the input file and places each job in the dispatch list as an object
 * @fp - file to be read from
 * @q - queue to which job objects are enqueued
 */
static void readInFile(FILE *fp, QUEUE *q) {
	char *str = readToken(fp);

	while (str) {
		int arrivalTime = atoi(str);
		int priority = atoi(readToken(fp));
		char *processorTime = readToken(fp);

		/* Create new JOB object and fill it with information */
		JOB *job = malloc(sizeof(struct JOB));
		//printf("flag\n");
		job->arrivalTime = arrivalTime;
		job->priority = priority;
		job->processorTime = processorTime;

		/* Add the new object to the queue */
		enqueue(q, job);

		str = readToken(fp);
	}
}

/**
 * Takes jobs from dispatch list queue and sends them to their appropriate priority queue
 * @jobList - job dispatch list
 * @p1q - Priority queue 1
 * @p2q - Priority queue 2
 * @p3q - Priority queue 3
 */
static void dispatchJobs(QUEUE *jobList, QUEUE *sysQueue, QUEUE *p1q, QUEUE *p2q, QUEUE *p3q) {
	JOB *currJob = dequeue(jobList);

	while (sizeQUEUE(jobList) >= 0) {
		switch (currJob->priority) {
			case (0):
				enqueue(sysQueue, currJob);
				break;
			case (1):
				enqueue(p1q, currJob);
				break;
			case (2):
				enqueue(p2q, currJob);
				break;
			case (3):
				enqueue(p3q, currJob);
				break;
			default:
				enqueue(sysQueue, currJob);		// FIXME: might need to default to something else
				break;
		}

		if (sizeQUEUE(jobList) == 0)
			break;

		if (peekQUEUE(jobList))
			currJob = dequeue(jobList);
	}
}