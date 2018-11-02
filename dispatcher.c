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
	int arrivalTime;
	int priority;
	char *processorTime;
	int remainingProcessorTime;
	char **args;
};

JOB *running;

static void displayJOB(FILE *, void *);

static void startProcess(JOB *, char **);
static JOB *restartProcess(JOB *);		// Required functions^
static JOB *terminateProcess(JOB *);
static JOB *suspendProcess(JOB *);

static int priorityQueuesEmpty(QUEUE *, QUEUE *, QUEUE *);
static void incrementPriority(JOB *);
static void readInFile(FILE *, QUEUE *);
static void dispatchJobs(QUEUE *, QUEUE *, QUEUE *, QUEUE *, QUEUE *);

static void dispatcher(int, QUEUE *, QUEUE *, QUEUE *, QUEUE *);

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

	/* Create all priority queues */
	QUEUE *jobList = newQUEUE(displayJOB);
	QUEUE *sysQueue = newQUEUE(displayJOB);
	QUEUE *p1q = newQUEUE(displayJOB);
	QUEUE *p2q = newQUEUE(displayJOB);
	QUEUE *p3q = newQUEUE(displayJOB);

	/* Read input file into job dispatch list */
	readInFile(inputFile, jobList);
/*
testing purposes only
	printf("jobslist: \n");
	displayQUEUE(stdout, jobList);
*/
	int numJobs = sizeQUEUE(jobList);
	printf("numjobs is: %d\n", numJobs); 

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

//	pid_t pid;
/*
	char *args[3] = {"./process", "5", NULL};

	execvp("./process", args);
*/
/*
	char *args[3];
	args[0] = "./process";
	args[1] = "1";
	args[2] = NULL;
*/
	dispatcher(numJobs, sysQueue, p1q, p2q, p3q);

	//execvp("./process", args);

/*
	JOB *j;

	int i;
	for (i = 0; i < numJobs; i++)
	{
		if (sizeQUEUE(sysQueue) > 0)
		{
			j = dequeue(sysQueue);
			startProcess(j, args);
		}
		else if (sizeQUEUE(p1q) > 0)
		{
			j = dequeue(p1q);
			startProcess(j, args);
		}
		else if (sizeQUEUE(p2q) > 0)
		{
			j = dequeue(p2q);
			startProcess(j, args);
		}
		else if (sizeQUEUE(p3q) > 0)
		{
			j = dequeue(p3q);
			startProcess(j, args);
		}
	}
*/

	// kill(process->pid, SIGINT);

	return 0;
}

/**
 * Starts a process using the fork() command & returns the pid
 */
static void startProcess(JOB *j, char **args)
{
	char *processTime = j->processorTime;
	strcpy(args[1], processTime);
	//pid_t pid = fork();
	//printf("pid: %d\n", pid);

	//printf("args: %s %s %s\n", args[0], args[1], args[2]);

	switch (j->pid = fork())
	{
		case -1:
			printf("error\n");
			return;
		case 0:
			printf("exec\n");
			printf("EXECVP(./process, %s,%s,%s)\n", args[0], args[1], args[2]);
			char **argv = malloc(sizeof(char *) * 3);
			strcpy(argv[0], "./process");
			strcpy(argv[1], "1");
			argv[2] = NULL;
			execvp("./process", argv);
			//execvp("./process", NULL);
			return;
		default:
			printf("default pid: %d\n", j->pid);
			return;
	}
}

/**
 * Restarts the process
 * @j - the process to be restarted
 * return the process
 */
static JOB *restartProcess(JOB *j)
{
	kill(j->pid, SIGCONT);
	return j;
}

/**
 * Terminates the process
 * @j - process or job to be terminated
 * return the process
 */
static JOB *terminateProcess(JOB *j)
{
	kill(j->pid, SIGINT);
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
	kill(j->pid, SIGTSTP);
	int status;
	waitpid(j->pid, &status, WUNTRACED);
	return j;
}

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
 * Checks the priority queues to see if they are empty
 * @q1 - first queue
 * @q2 - second queue
 * @q3 - third queue
 * return 1 if all queues empty, 0 if at least one job in at least one queue
 */
static int priorityQueuesEmpty(QUEUE *q1, QUEUE *q2, QUEUE *q3) {
	if (sizeQUEUE(q1) == 0 && sizeQUEUE(q2) == 0 && sizeQUEUE(q3) == 0) {
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
	if (j->priority < 3)
	{
		j->priority += 1;
	}
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
static void dispatchJobs(QUEUE *jobList, QUEUE *sysQueue, QUEUE *p1q, QUEUE *p2q, QUEUE *p3q)
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

static void dispatcher(int size, QUEUE *sysQueue, QUEUE *p1, QUEUE *p2, QUEUE *p3)
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

	if (running)
	{
		if (--running->remainingProcessorTime == 0)
		{
			terminateProcess(running);
			running = NULL;
		}
		else if (sizeQUEUE(sysQueue) > 0 || !priorityQueuesEmpty(p1, p2, p3))
		{
			if (running->priority != 0)
			{
				JOB *ptr = suspendProcess(running);
				incrementPriority(ptr);
				// enqueue ptr to the proper priority queue
				switch (ptr->priority)
				{
					case 0:
						enqueue(sysQueue, ptr);
						break;
					case 1:
						enqueue(p1, ptr);
						break;
					case 2:
						enqueue(p2, ptr);
						break;
					case 3:
						enqueue(p3, ptr);
						break;
					default:
						break;
				}
				running = NULL;
			}
		}

		if (!running && (!priorityQueuesEmpty(p1, p2, p3 || sizeQUEUE(sysQueue) > 0)))
		{
			if (sizeQUEUE(sysQueue) > 0)
			{
				running = dequeue(sysQueue);
			}
			else if (sizeQUEUE(p1) > 0)
			{
				running = dequeue(p1);
			}
			else if (sizeQUEUE(p2) > 0)
			{
				running = dequeue(p2);
			}
			else if (sizeQUEUE(p3) > 0)
			{
				running = dequeue(p3);
			}

			if (running->pid != 0)
			{
				restartProcess(running);
			}
			else
			{
				// init args here
				startProcess(running, args);
			}

			sleep(1);
			++timer;
		}
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
			if (sizeQUEUE(sys) > 0)
			{
				printf("if1.0\n");
				running = dequeue(sys);
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