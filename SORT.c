//Amir Kalmoni

#include <stdbool.h>
#include <stdlib.h>
#include "shm_com.h"
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include "semun.h"
#include <sys/time.h>
#include <string.h>

struct shared_use_st *shared_stuff;
bool debugMode = false;
static int arrayId;
static int stateId;

void output_array(int array[Number_Of_Elements]);
void output_states();
void swap(int array[Number_Of_Elements], int index1, int index2);
void initializeState();
bool isitranked();
static int set_semaphore(int sem_id, int index);
static void delete_semaphore(int sem_id);
static int semaphore_p(int sem_id, int index);
static int semaphore_v(int sem_id, int index);

int main() {
	int active_children = 0;
	int processes_created = 0;
	int processes_left_to_create = 4;

	 // Setup shared memory
	 
	void *shared_memory = (void *)0;
	int shmid;
	srand((unsigned int)getpid());
	shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666 | IPC_CREAT);
	if (shmid == -1) {
		fprintf(stderr, "shmget failed\n");
		exit(EXIT_FAILURE);
	}
	shared_memory = shmat(shmid, (void *)0, 0);
	if (shared_memory == (void *)-1) {
		fprintf(stderr, "shmat failed\n");
	exit(EXIT_FAILURE);
	}
	if(debugMode) {
		printf("Memory attached at %X\n", shared_memory);
	}
	shared_stuff = (struct shared_use_st *)shared_memory;
	initializeState();
	arrayId = semget((key_t)1234, 5, 0666 | IPC_CREAT);
	stateId = semget((key_t)2468, 1, 0666 | IPC_CREAT);
	


	//initializing semaphore for array to 0
	int i;
	for(i = 0; i < 5; i++) {
		if(set_semaphore(arrayId, i) == 0){
			fprintf(stderr, "Failed to initialize semaphore\n");
			exit(EXIT_FAILURE);
		}
	}
	//initializing semaphore for state to 0

	if(set_semaphore(stateId, 0) == 0){
		fprintf(stderr, "Failed to initialize semaphore\n");
		exit(EXIT_FAILURE);
	}

	printf("For debug mode, enter 1. For silent mode, enter 0\n");
	int temp;
	scanf("%d", &temp);
	debugMode = temp;

	printf("Please enter 5 integers with a comma after each value\n");
	scanf("%d,%d,%d,%d,%d", &shared_stuff->B[0], &shared_stuff->B[1], &shared_stuff->B[2], &shared_stuff->B[3], &shared_stuff->B[4]);
	if(debugMode) {
		printf("Initial Array: ");
		output_array(shared_stuff->B);
		

	}

	printf("fork program starting\n");	
	int index;
	for (index = 0; index < processes_left_to_create; index++) {
		pid_t pid = fork();
		processes_created++;
		active_children++;
		switch(pid) {
		case -1:
			perror("fork failed");
			exit(1);
		case 0:
		{
			while (!isitranked()) {
				// Acquire the lock for adjacent array indices
				semaphore_p(arrayId, index);
				semaphore_p(arrayId, index + 1);
				//If the left is less than the right, swap 
				if (shared_stuff->B[index] < shared_stuff->B[index + 1]) {
					swap(shared_stuff->B, index, index + 1);
					// Acquire the lock for state array
					semaphore_p(stateId, 0);
					// Mark both indices as either modified/unranked
					shared_stuff->state[index] = UNSORTED;
					shared_stuff->state[index + 1] = UNSORTED;
					if(debugMode) {
						printf("\nProcess %d: performed swapping\n", index);
						output_states();
						output_array(shared_stuff->B);
					}
					// Release the lock for the state array
					semaphore_v(stateId, 0);

				} else {
					// Acquire the lock for the state array
					semaphore_p(stateId, 0);
					// If no change, mark both indices as sorted
					shared_stuff->state[index] = SORTED;
					shared_stuff->state[index + 1] = SORTED;
					if(debugMode) {
						printf("\nProcess %d: No swapping performed\n", index);
						output_states();
						output_array(shared_stuff->B);
					}
					// Release the lock for the state array
					semaphore_v(stateId, 0);
				}
				// Release the lock for the left and right array indices
				semaphore_v(arrayId, index);
				semaphore_v(arrayId, index + 1);
			}

			exit(EXIT_SUCCESS);
		}
		default:
			if(debugMode) {
				printf("Fork Created. PID: %d\n", pid);
			}
			if (processes_created < processes_left_to_create) {
			break;
			}

			// The parent waits for all child processes to finish and decrements active_children each time a child process finishes.
			while (active_children > 0) {
				int stat_val;
				pid_t child_pid;
				child_pid = wait(&stat_val);
				if(debugMode) {
					printf("Child is completed: PID = %d\n", child_pid);
					if(WIFEXITED(stat_val))
						printf("Child exited with code %d\n", WEXITSTATUS(stat_val));
					else
						printf("Child terminated abnormally\n");
				}
				active_children--;
			}
			if(debugMode) {
				printf("All Children terminated\n");
			}
			printf("Sorted Array: ");
			output_array(shared_stuff->B);
			printf("Maximum: %d\n", shared_stuff->B[0]);
			printf("Minimum: %d\n", shared_stuff->B[Number_Of_Elements - 1]);
			printf("Median: %d\n", shared_stuff->B[(Number_Of_Elements - 1) / 2]);
			delete_semaphore(stateId);
			delete_semaphore(arrayId);

			exit(EXIT_SUCCESS);
		}
	}
}

static int set_semaphore(int sem_id, int index)
{
	union semun sem_union;

	sem_union.val = 1;
	if (semctl(sem_id, index, SETVAL, sem_union) == -1) return(0);
	return(1);
}

static void delete_semaphore(int sem_id)
{
	union semun sem_union;
	if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
		fprintf(stderr, "Failed to delete semaphore\n");
}



 // Prints the array values in all indices
 
void output_array(int array[Number_Of_Elements]) {
	int i;
	for (i = 0; i < Number_Of_Elements; i++) {
		printf("%d ", array[i]);
	}
	printf("\n");
}

//Release semaphore key
static int semaphore_v(int sem_id, int index)
{
	struct sembuf sem_b;

	sem_b.sem_num = index;
	sem_b.sem_op = 1; /* V() */
	sem_b.sem_flg = SEM_UNDO;
	if (semop(sem_id, &sem_b, 1) == -1) {
		fprintf(stderr, "semaphore_v failed\n");
		return(0);
	}
	return(1);
}

//Acquire semaphore key
static int semaphore_p(int sem_id, int index)
{
	struct sembuf sem_b;

	sem_b.sem_num = index;
	sem_b.sem_op = -1; 
	sem_b.sem_flg = SEM_UNDO;
	if (semop(sem_id, &sem_b, 1) == -1) {
		fprintf(stderr, "semaphore_p failed\n");
		return(0);
	}
	return(1);
}


//Swaps the array elements
void swap(int array[Number_Of_Elements], int index1, int index2) {
	int temp = array[index1];
	array[index1] = array[index2];
	array[index2] = temp;
}

//Initializes the state for each array index to UNSORTED
void initializeState() {
	int i;
	for (i = 0; i < Number_Of_Elements; i++)
		shared_stuff->state[i] = UNSORTED;
}

// Prints the states for each array index
void output_states() {
	int i;
	for (i = 0; i < Number_Of_Elements; i++) {
		if (shared_stuff->state[i] == SORTED) {
			printf("SORTED ");
		} else {
			printf("UNSORTED ");
		}
	}
	printf("\n");
}


// Checks if all array indices are ranked
bool isitranked() {
	semaphore_p(stateId, 0);
	int i;
	for (i = 0; i < Number_Of_Elements; i++) {
		if (shared_stuff->state[i] != SORTED) {
			semaphore_v(stateId, 0);
			return false;
		}
	}
	semaphore_v(stateId, 0);
	return true;
}

