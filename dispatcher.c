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
//
// Need to use:
//		kill(process->pid, SIGINT) for signal interrupt
//		kill(process->pid, SIGTSTP) for signal suspend
//		kill(process->pid, SIGCONT) for signal continue
//		waitpid(process->pid, &status, WUNTRACED) to retain synchronization of output between your dispatcher and child process
//			|--> your dispatcher should wait for the process to respond SIGTSTP or SIGINT before continuing
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "integer.h"
#include "scanner.h"
#include "queue.h"

typedef struct JOB JOB;
struct JOB
{
	pid_t pid;
	char *args[3];
	int arrivalTime;
	int remainingProcessorTime;
	int priority;
	char *processorTime;
};

/* Global Variables */
JOB *running;
QUEUE *sysQueue;
QUEUE *p1q;
QUEUE *p2q;
QUEUE *p3q;
QUEUE *jobList;
int timer;


/* Required functions */
static JOB *startProcess(JOB *);
static JOB *restartProcess(JOB *);
static JOB *terminateProcess(JOB *);
static JOB *suspendProcess(JOB *);

/* Utility functions */
static void displayJOB(FILE *, void *);
static void initialize(void);
static int complete(void);
static int priorityQueuesEmpty(void);
static void incrementPriority(JOB *);									// Safe method for incrementing process priority
static void readInFile(FILE *, QUEUE *);
static void dispatchJobs(void);
static void enqToPriority(JOB *);
static QUEUE *getHighestPriorityQ(void);
static void dispatcher(void);

int main(int argc, char *argv[])
{
	//printf("test\n");
	if (argc < 2)
	{
		printf("Error: Not enough command line arguments.\n");
		exit(-1);
	}

	/* Open input file for reading */
	FILE *inputFile = fopen(argv[1], "r");

	/* Set all vars to base values, initialize all queue data structs */
	initialize();
printf("initialized\n");
	/* Read input file into job dispatch list */
	readInFile(inputFile, jobList);
	fclose(inputFile);

	/* Send each job in job dispatch list to its proper priority queue */
	dispatchJobs();

	dispatcher();

	//execvp("./process", args);

	return 0;
}


/************************
Required functions
************************/
/**
 * Starts a process using the fork() command & returns the pid
 * @j - job to start
 * return the job
 */
static JOB *startProcess(JOB *j)
{
	switch (j->pid = fork())
	{
		case -1:
			return NULL;
		case 0:
			execvp(j->args[0], j->args);
			return NULL;
		default:
			return j;
	}
}

/**
 * Restarts the process
 * @j - the process to be restarted
 * return the process
 */
static JOB *restartProcess(JOB *j)
{
	if (kill(j->pid, SIGCONT))
	{
		printf("Error: Restart process error pid: %d\n", j->pid);
		return NULL;
	}
	return j;
}

/**
 * Terminates the process
 * @j - process or job to be terminated
 * return the process
 */
static JOB *terminateProcess(JOB *j)
{
	if (kill(j->pid, SIGINT))
	{
		printf("Error: Terminate process error pid: %d\n", j->pid);
		return NULL;
	}
	int status;
	waitpid(j->pid, &status, WUNTRACED);
	return j;
}

/**
 * Suspends the process
 * @j - the process or job to be suspended
 * return the process
 */
static JOB *suspendProcess(JOB *j)
{
	if (kill(j->pid, SIGTSTP))
	{
		printf("Error: Suspend process error pid: %d\n", j->pid);
		return NULL;
	}
	int status;
	waitpid(j->pid, &status, WUNTRACED);
	return j;
}


/************************
Utility functions
************************/
/**
 * Displays the job object in proper format
 * @fp - file printed to
 * @job - job object to be displayed
 */
static void displayJOB(FILE *fp, void *job)
{
	JOB *j = job;
	fprintf(fp, "<%d>, <%d>, <%s>\n", j->arrivalTime, j->priority, j->processorTime);
}

/**
 * Initializes variables to base values and initializes queue structs
 */
static void initialize(void)
{
	running = NULL;
	timer = 0;

	/* Initialize all queues */
	jobList 	= newQUEUE(displayJOB);
	sysQueue 	= newQUEUE(displayJOB);
	p1q 		= newQUEUE(displayJOB);
	p2q 		= newQUEUE(displayJOB);
	p3q 		= newQUEUE(displayJOB);
}

/**
 * Checks to see if there is a running process or any jobs in any queue
 * return 1 if complete, 0 if still jobs to do
 */
static int complete(void)
{
	if (running || !priorityQueuesEmpty() || sizeQUEUE(sysQueue) > 0 || sizeQUEUE(jobList) > 0)
		return 0;
	else
		return 1;
}

/**
 * Checks the priority queues to see if they are empty
 * @q1 - first queue
 * @q2 - second queue
 * @q3 - third queue
 * return 1 if all queues empty, 0 if at least one job in at least one queue
 */
static int priorityQueuesEmpty(void) {
	if (sizeQUEUE(p1q) == 0 && sizeQUEUE(p2q) == 0 && sizeQUEUE(p3q) == 0)
	{
		return 1;
	}
	else
		return 0;
}

/**
 * Incremements the priority of the given job
 * @j - job of which priority is to be incremented
 */
static void incrementPriority(JOB *j)
{
	if (j->priority < 3) j->priority += 1;
}

/**
 * Reads the input file and places each job in the dispatch list as an object
 * @fp - file to be read from
 * @q - queue to which job objects are enqueued
 */
static void readInFile(FILE *fp, QUEUE *q)
{
	char *str = readToken(fp);

	while (str)
	{
		int arrivalTime = atoi(str);
		int priority = atoi(readToken(fp));
		char *processorTime = readToken(fp);

		/* Create new JOB object and fill it with attribute data */
		JOB *job = malloc(sizeof(struct JOB));

		job->arrivalTime = arrivalTime;
		job->priority = priority;
		job->processorTime = processorTime;
		job->remainingProcessorTime = atoi(processorTime);
		job->args[0] = "./process";
		job->args[1] = processorTime;
		job->args[2] = NULL;

		/* Add the new object to the job dispatch list queue */
		enqueue(q, job);

		str = readToken(fp);
	}
}

/**
 * Takes jobs from job dispatch list queue and sends them to their appropriate priority queue
 * @jobList - job dispatch list
 * @p1q - Priority queue 1
 * @p2q - Priority queue 2
 * @p3q - Priority queue 3
 */
static void dispatchJobs(void)
{
	JOB *currJob = dequeue(jobList);

	while (sizeQUEUE(jobList) >= 0)
	{
		switch (currJob->priority)
		{
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

/**
 * Enqueues the job to the correct priority queue
 * @j - the job to be enqueued
 */
static void enqToPriority(JOB *j)		// FIXME: the priorities might be reversed
{
	switch (j->priority)
	{
		case 1:
			enqueue(sysQueue, j);
			break;
		case 2:
			enqueue(p1q, j);
			break;
		case 3:
			enqueue(p2q, j);
			break;
	}
}

/**
 * Send job to proper queue
 * @j - job to be sent
 */
static void sendToQueue(JOB *j)
{
	switch (j->priority)
	{
		case 0:
			enqueue(sysQueue, j);
			break;
		case 1:
			enqueue(p1q, j);
			break;
		case 2:
			enqueue(p2q, j);
			break;
		case 3:
			enqueue(p3q, j);
			break;
		default:
			break;
	}

	return;
}

/**
 * Returns the highest priority queue that is not null or sysQueue
 * return - highest priority non-null queue
 */
static QUEUE *getHighestPriorityQ(void)
{
	if (sizeQUEUE(p1q) > 0)
		return p1q;
	else if (sizeQUEUE(p2q) > 0)
		return p2q;
	else if (sizeQUEUE(p3q) > 0)
		return p3q;

	return NULL;
}

static void dispatcher(void)
{
	// while (still things in any queues)
	//		1. enqueue to appropriate queues
	//		2. if (--remainingProcessorTime of process is 0)
	//			a. terminate the process
	//		   else if (there's another process waiting in any queue)
	//			if (priority is not 0)
	//				a. suspend the process
	//				b. increment its priority (as long as not greater than 3 result)
	//		3. if process is not current running process but still in queues
	//			if (in sysqueue)
	//				set running to dequeue of sysqueue
	//			else
	//				set to one of the other queues
	//		   if processs has been suspended (running->pid != 0)
	//				restart the running process
	//		   else
	//				start new process
	//				print status of process
	//		4. sleep, increment timer

	while (!complete())
	{
/*
		JOB *next = dequeue(jobList);
		printf("flag\n");
		while (sizeQUEUE(jobList) > 0)
		{
			printf("flag2\n");
			if (next->arrivalTime <= timer)
			{
				// enqueue next to the appropriate queue
				sendToQueue(next);
			}

			next = dequeue(jobList);
		}
*/
		if (running)
		{
			if (--running->remainingProcessorTime == 0)
			{
				terminateProcess(running);
				running = NULL;
			}
			else if (!priorityQueuesEmpty() || sizeQUEUE(sysQueue) > 0)				// FIXME: Might need to be another condition in the elif statement
			{
				if (running->priority != 0)
				{
					JOB *j = suspendProcess(running);
					incrementPriority(j);
					enqToPriority(j);
					running = NULL;
				}
			}
		}

		if (!running && (!priorityQueuesEmpty() || sizeQUEUE(sysQueue) > 0))
		{
			//printf("no running processes, setting running to ...\n");
			if (sizeQUEUE(sysQueue) > 0)
			{
			//	printf("running is a system process\n");
				running = dequeue(sysQueue);
			}
			else if (!priorityQueuesEmpty())
			{
			//	printf("running is a priority queue process\n");
				running = dequeue(getHighestPriorityQ());
			}

			if (running->pid != 0)
			{
				restartProcess(running);
			}
			else
			{
				startProcess(running);
				//print the process... needed???
			}
		}

		sleep(1);
		++timer;
	}
/*
	char *args[3];
	args[0] = "./process";
	args[2] = NULL;

	int i;
	for (i = 0; i < size; i++)
	{
		if (running)
		{
			printf("if0\n");
			if (--running->remainingProcessorTime == 0)
			{
				printf("if0.0\n");
				terminateProcess(running);
				running = NULL;
			}
			else if (!priorityQueuesEmpty(p1, p2, p3))
			{
				if (running->priority != 0)
				{
					printf("if0.1\n");
					suspendProcess(running);
					incrementPriority(running);
					running = NULL;
					// put running in correct priority queue
				}
			}
		}

		if (!running && !priorityQueuesEmpty(p1, p2, p3))
		{
			printf("if1\n");
			if (sizeQUEUE(sysQueue) > 0)
			{
				printf("if1.0\n");
				running = dequeue(sysQueue);
			}
			else if (!priorityQueuesEmpty(p1, p2, p3))
			{
				printf("if1.1\n");
				if (sizeQUEUE(p1) > 0)
					running = dequeue(p1);
				else if (sizeQUEUE(p2) > 0)
					running = dequeue(p2);
				else if (sizeQUEUE(p3) > 0)
					running = dequeue(p3);
			}

			if (running->pid != 0)
			{
				printf("if1.2\n");
				restartProcess(running);
			}

			else
			{
				printf("if1.3\n");	
				args[1] = running->processorTime;
				startProcess(running, args);
				// print_process(running);... needed?		//FIXME: this output needed?
			}
		}

		// sleep(1);
		// ++timer;		//FIXME: is this needed?
	}
*/
}