// Jonathan Pham
// CS 4760 Operating Systems
// Assignment 5: Resource Management
// Due: 04/24/2019

#include "resourceManagement.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
        int topQueue;
        int bottomQueue;
        int size;
        int *array;
        int pid;
	unsigned capacity;
} Queue;

FILE *fp;
bool isSafeState ( int available[], int maximum[][maxResources], int allot[][maxResources] );
const int maxRunningProcesses = 19;     // Max processes alive at one time
const int totalProcessLimit = 100;      // Max 100 processes over course of program
const int maxAmountOfEachResource = 4;
int isFull ( Queue* queue );
int isEmpty ( Queue* queue );
int dequeue ( Queue* queue );
int topQueue ( Queue* queue );
int bottomQueue ( Queue* queue );
int requestResource, grantRequest, markerChk, releaseResource, createdProcs, terminatedProcs; // Variables for program statistics
int percentage=29;
int gr=39;
Queue* createQueue ( unsigned capacity );
void enQueue ( Queue* queue, int item );
void processCalculation ( int need[maxProcesses][maxResources], int maximum[maxProcesses][maxResources], int allot[maxProcesses][maxResources] );
void incrementClock ( unsigned int shmClock[] );
void displayTable( int num1, int array[][maxResources] );
void displayMaxTable( int num1, int array[][maxResources] );
void displayStatistics();
void killProcesses();

int main ( int argc, char *argv[] ) {
        char logName[20] = "resource.log"; // Pertinent information will be stored in resource.log
        fp = fopen ( logName, "w+" );
        srand ( time ( NULL ) );

        createdProcs = 0;
        int myPid = getpid();
        int numberOfLines = 0;
        const int killTimer = 2;        // Program runs for 2 seconds max
        alarm ( killTimer );            // Sets the timer alarm based on value of killTimer

        int arrayIndex, tempIndex, arrayRequest, tempRelease, tempHolder;
        bool tempTerminate, tempGranted, timeCheck, processCheck, createProcess;

        pid_t pid;
        int processIndex = 0;
        int currentProcesses = 0;       // Tracks number of active processes
        unsigned int newProcessTime[2] = { 0, 0 };
        unsigned int nextRandomProcessTime;
        unsigned int nextProcessTimeBound = 5000;
        unsigned int tempClock[2];
        unsigned int receivedTime[2];


        Queue* blockedQueue = createQueue ( totalProcessLimit );

        if ( signal ( SIGINT, handle ) == SIG_ERR ) {
                perror ( "OSS: CTRL+C signal failed.\n" );
        }

        if ( signal ( SIGALRM, handle ) == SIG_ERR ) {
                perror ( "OSS: alarm signal failed!\n" );
        }


        // Create shared mem for clock and blocked key
        shmClockKey = 2016;
        if ( ( shmClockID = shmget ( shmClockKey, ( 2 * ( sizeof ( unsigned int ) ) ), IPC_CREAT | 0666 ) ) == -1 ) {
                perror ( "OSS: failed to create shared memory space for simulated clock.\n" );
                return ( 1 );
        }

        shmBlockedKey = 3017;
        if ( ( shmBlockedID = shmget ( shmBlockedKey, ( totalProcessLimit * ( sizeof ( int ) ) ), IPC_CREAT | 0666 ) ) == -1 ) {
                perror ( "OSS: failed to create shared memory for blocked USER process array." );
                return ( 1 );
        }

        // Attach to and initialize shared memory for clock and blocked process array
        if ( ( shmClock = (unsigned int *) shmat ( shmClockID, NULL, 0 ) ) < 0 ) {
                perror ( "OSS: failed to attach shared memory space for simulated clock.\n" );
                return 1;
        }
        shmClock[0] = 0; // Seconds for clock
        shmClock[1] = 0; // Nanoseconds for clock

        if ( ( shmBlocked = (int *) shmat ( shmBlockedID, NULL, 0 ) ) < 0 ) {
                perror ( "OSS: failed to attach shared memory space for blocked process array.\n" );
                return 1;
        }

        int i, j;
        for ( i = 0; i < totalProcessLimit; ++i ) {
                shmBlocked[i] = 0;
        }


        messageKey = 1996;
        if ( ( messageID = msgget ( messageKey, IPC_CREAT | 0666 ) ) == -1 ) {
                perror ( "OSS: Failure to create the msg queue!\n" );
                return 1;
        }


        int totalResourceTable[20];
        for ( i = 0; i < 20; ++i ) {
                totalResourceTable[i] = ( rand() % ( 10 - 1 + 1 ) + 1 );
        }

        int maxrscTable[totalProcessLimit][20];
        for ( i = 0; i < totalProcessLimit; ++i ) {
                for ( j = 0; j < 20; ++j ) {
                        maxrscTable[i][j] = 0;
                }
        }

        // Table storing the amount of each resource currently allocated to each process
        int allocatedTable[totalProcessLimit][20];
        for ( i = 0; i < totalProcessLimit; ++i ) {
                for ( j = 0; j < 20; ++j ) {
                        allocatedTable[i][j] = 0;
                }
        }

        int availableResourcesTable[20];
        for ( i = 0; i < 20; ++i ) {
                availableResourcesTable[i] = totalResourceTable[i];
        }

        int requestedResourceTable[totalProcessLimit];
        for ( i = 0; i < totalProcessLimit; ++i ) {
                requestedResourceTable[i] = -1;
        }

        while ( 1 ) {
                // Terminate if log file exceeds 100000 lines
                if ( numberOfLines >= 100000 ) {
                        fprintf ( fp, "OSS: log file has exceeded max length. Program terminating!\n" );
                        kill ( getpid(), SIGINT );
                }

                createProcess = false;  // Flag is false by default each run through the loop
                if ( shmClock[0] > newProcessTime[0] || ( shmClock[0] == newProcessTime[0] && shmClock[1] >= newProcessTime[1] ) ) {
                        timeCheck = true;
                } else {
                        timeCheck = false;
                }
                if ( ( currentProcesses < maxRunningProcesses ) && ( createdProcs < totalProcessLimit ) ) {
                        processCheck = true;
                } else {
                        processCheck = false;
                }

                if ( ( timeCheck == true ) && ( processCheck == true ) ) {
                        createProcess = true;
                }

                if ( createProcess ) {
                        processIndex = createdProcs;    // Sets process index for the various resource tables
                        for ( i = 0; i < 20; ++i ) {
                                maxrscTable[processIndex][i] = ( rand() % ( maxAmountOfEachResource - 1 + 1 ) + 1 );
                        }

                        fprintf ( fp, "Master has a newly generated process P%d\n", processIndex);
                        for ( i = 0; i < 20; ++i ) {
                                fprintf ( fp, "%d: %d\t", i, maxrscTable[processIndex][i] );
                        }
                        fprintf ( fp, "\n" );

                        pid = fork();   // Fork process
                        if ( pid < 0 ) {
                                perror ( "OSS: failed to fork child process.\n" );
                                kill ( getpid(), SIGINT );
                        }

                        // Child Process
                        if ( pid == 0 ) {
                                char intBuffer0[3], intBuffer1[3], intBuffer2[3], intBuffer3[3], intBuffer4[3];
                                char intBuffer5[3], intBuffer6[3], intBuffer7[3], intBuffer8[3], intBuffer9[3];
                                char intBuffer10[3], intBuffer11[3], intBuffer12[3], intBuffer13[3], intBuffer14[3];
                                char intBuffer15[3], intBuffer16[3], intBuffer17[3], intBuffer18[3], intBuffer19[3];
                                char intBuffer20[3];


                                sprintf ( intBuffer0, "%d", maxrscTable[processIndex][0] );     // Resources 1-20
                                sprintf ( intBuffer1, "%d", maxrscTable[processIndex][1] );
                                sprintf ( intBuffer2, "%d", maxrscTable[processIndex][2] );
                                sprintf ( intBuffer3, "%d", maxrscTable[processIndex][3] );
                                sprintf ( intBuffer4, "%d", maxrscTable[processIndex][4] );
                                sprintf ( intBuffer5, "%d", maxrscTable[processIndex][5] );
                                sprintf ( intBuffer6, "%d", maxrscTable[processIndex][6] );
                                sprintf ( intBuffer7, "%d", maxrscTable[processIndex][7] );
                                sprintf ( intBuffer8, "%d", maxrscTable[processIndex][8] );
                                sprintf ( intBuffer9, "%d", maxrscTable[processIndex][9] );
                                sprintf ( intBuffer10, "%d", maxrscTable[processIndex][10] );
                                sprintf ( intBuffer11, "%d", maxrscTable[processIndex][11] );
                                sprintf ( intBuffer12, "%d", maxrscTable[processIndex][12] );
                                sprintf ( intBuffer13, "%d", maxrscTable[processIndex][13] );
                                sprintf ( intBuffer14, "%d", maxrscTable[processIndex][14] );
                                sprintf ( intBuffer15, "%d", maxrscTable[processIndex][15] );
                                sprintf ( intBuffer16, "%d", maxrscTable[processIndex][16] );
                                sprintf ( intBuffer17, "%d", maxrscTable[processIndex][17] );
                                sprintf ( intBuffer18, "%d", maxrscTable[processIndex][18] );
                                sprintf ( intBuffer19, "%d", maxrscTable[processIndex][19] );
                                sprintf ( intBuffer20, "%d", processIndex );

                                fprintf ( fp, "Master --  P%d was created at %d:%d.\n", processIndex,
                                         getpid(), shmClock[0], shmClock[1] );
                                numberOfLines++;

                                // Exec call to ass5 executable
                                execl ( "./ass5", "ass5", intBuffer0, intBuffer1, intBuffer2, intBuffer3,
                                       intBuffer4, intBuffer5, intBuffer6, intBuffer7, intBuffer8,
                                       intBuffer9, intBuffer10, intBuffer11, intBuffer12, intBuffer13,
                                       intBuffer14, intBuffer15, intBuffer16, intBuffer17, intBuffer18,
                                       intBuffer19, intBuffer20, NULL );
                                       exit (127);
                        }

                        // In Parent Process
                        newProcessTime[0] = shmClock[0];
                        newProcessTime[1] = shmClock[1];
                        nextRandomProcessTime = ( rand() % ( nextProcessTimeBound - 1 + 1 ) + 1 );
                        newProcessTime[1] += nextRandomProcessTime;
                        newProcessTime[0] += newProcessTime[1] / 1000000000;
                        newProcessTime[1] = newProcessTime[1] % 1000000000;
                        timeCheck = false;
                        processCheck = false;
                        currentProcesses++;
                        createdProcs++;
                }
                msgrcv ( messageID, &resourceManagement, sizeof( resourceManagement ), 5, IPC_NOWAIT );

                arrayIndex = resourceManagement.pid;
                tempIndex = resourceManagement.tableIndex;
                arrayRequest = resourceManagement.request;
                tempRelease = resourceManagement.release;
                tempTerminate = resourceManagement.terminate;
                tempGranted = resourceManagement.resourceGranted;
                tempClock[0] = resourceManagement.messageTime[0];
                tempClock[1] = resourceManagement.messageTime[1];

                // Resource Request Message
                if ( arrayRequest != -1 ) {
                        fprintf ( fp, "Master has detected Process P%d requesting R%d at time %d:%d.\n", tempIndex,
                                 arrayRequest, tempClock[0], tempClock[1] );
                        numberOfLines++;
                        requestResource++;

                        requestedResourceTable[tempIndex] = arrayRequest;

                        // Change rsctables to test state
                        allocatedTable[tempIndex][arrayRequest]++;
                        availableResourcesTable[arrayRequest]--;

                        // Run Banker's Algorithm
                        if (isSafeState ( availableResourcesTable, maxrscTable, allocatedTable ) ) {
                                grantRequest++;
                                resourceManagement.msg_type = tempIndex;
                                resourceManagement.pid = getpid();
                                resourceManagement.tableIndex = tempIndex;
                                resourceManagement.request = -1;
                                resourceManagement.release = -1;
                                resourceManagement.terminate = false;
                                resourceManagement.resourceGranted = true;
                                resourceManagement.messageTime[0] = shmClock[0];
                                resourceManagement.messageTime[1] = shmClock[1];

                                if ( msgsnd ( messageID, &resourceManagement, sizeof ( resourceManagement ), 0 ) == -1 ) {
                                        perror ( "OSS error! Failure sending message.\n" );
                                }

                                fprintf ( fp, "Master granting P%d request at R%d at time %d:%d.\n",
                                         tempIndex, arrayRequest, shmClock[0], shmClock[1] );
                                numberOfLines++;
                        }
                        else {
                                // Reset tables to their state before the test
                                allocatedTable[tempIndex][arrayRequest]--;
                                availableResourcesTable[arrayRequest]++;

                                // Place that process's index in the blocked queue
                                enQueue ( blockedQueue, tempIndex );

                                shmBlocked[tempIndex] = 1;

                                fprintf ( fp, "Master running deadlock detection at time %d:%d.\n\tP%d was denied its request of R%d and was deadlocked\n",
                                         shmClock[0], shmClock[1], tempIndex, arrayRequest);
                                numberOfLines++;
                        }

                        incrementClock ( shmClock );
                }

                // Resource Release Message
                if ( tempRelease != -1 ) {
                        fprintf ( fp, "P%d is releasing resources from R%d at %d:%d.\n",
                                  tempIndex, tempRelease, tempClock[0], tempClock[1]);
                        numberOfLines++;

                        releaseResource++;
                        allocatedTable[tempIndex][tempRelease]--;
                        availableResourcesTable[tempRelease]++;

                        fprintf ( fp, "Process P%d release notification was signal handled at %d:%d seconds.\n", tempIndex, shmClock[0],
                                 shmClock[1] );
                        numberOfLines++;
                        incrementClock ( shmClock );
                }

                // Process Termination Message
                if ( tempTerminate == true ) {
                        fprintf ( fp, "Master has terminated  P%d at %d:%d.\n", tempIndex, tempClock[0], tempClock[1] );
                        numberOfLines++;

                        for ( i = 0; i < 20; ++i ) {
                                tempHolder = allocatedTable[tempIndex][i];
                                allocatedTable[tempIndex][i] = 0;
                                availableResourcesTable[i] += tempHolder;
                        }
                        currentProcesses--;

                        fprintf ( fp, "Master Process P%ds termination was handled at %d:%d.\n", tempIndex,
                                 shmClock[0], shmClock[1] );
                        numberOfLines++;
                        incrementClock ( shmClock );
                }

                // Check blocked queue
                if ( !isEmpty ) {
                        tempIndex = dequeue ( blockedQueue );
                        arrayRequest = requestedResourceTable[tempIndex];
                        requestResource++;

                        allocatedTable[tempIndex][arrayRequest]++;
                        availableResourcesTable[arrayRequest]--;

                        // Run Banker's Algorithm
                        if (isSafeState ( availableResourcesTable, maxrscTable, allocatedTable ) ) {
                                grantRequest++;
                                resourceManagement.msg_type = tempIndex;
                                resourceManagement.pid = getpid();
                                resourceManagement.tableIndex = tempIndex;
                                resourceManagement.request = -1;
                                resourceManagement.release = -1;
                                resourceManagement.terminate = false;
                                resourceManagement.resourceGranted = true;
                                resourceManagement.messageTime[0] = shmClock[0];
                                resourceManagement.messageTime[1] = shmClock[1];

                                if ( msgsnd ( messageID, &resourceManagement, sizeof ( resourceManagement ), 0 ) == -1 ) {
                                        perror ( "OSS: failed to send message!\n" );
                                }
                                shmBlocked[tempIndex] = 0;

                                fprintf ( fp, "Master granting P%d request at R%d at time %d:%d.\n",
                                         tempIndex, arrayRequest, shmClock[0], shmClock[1] );
                                numberOfLines++;
                        } else {
                                // Reset tables to their state before the test
                                allocatedTable[tempIndex][arrayRequest]--;
                                availableResourcesTable[arrayRequest]++;

                                // Place that process's index in the blocked queue
                                enQueue ( blockedQueue, tempIndex );

                                // Set the blocked process flag in shared memory for USER to see
                                shmBlocked[tempIndex] = 1;

                                fprintf ( fp, "Master running deadlock detection at time %d:%d.\n\tP%d was denied its request of R%d and was deadlocked\n",
                                         shmClock[0], shmClock[1], tempIndex, arrayRequest);
                                numberOfLines++;
                        }
                        incrementClock ( shmClock );
                }

                incrementClock ( shmClock );

                if ( numberOfLines % 20 == 0 ) {
                        fprintf ( fp, "*****Currently Allocated Resources*****\n" );
                        fprintf ( fp, "\tR1\tR2\tR3\tR4\tR5\tR6\tR7\tR8\tR9\tR10\tR11\tR12\tR13\tR14\tR15\tR16\tR17\tR18\tR19\tR20\n" );
                        for ( i = 0; i < createdProcs; ++i ) {
                                fprintf ( fp, "P%d:\t", i );
                                for ( j = 0; j < 20; ++j ) {
                                        fprintf ( fp, "%d\t", allocatedTable[i][j] );
                                }
                                fprintf ( fp, "\n" );
                        }
                }

        }
        displayStatistics();

        // Detach from, delete shared memory & message queue
        killProcesses();

        return ( 0 ) ;
        // End of main program
}

bool isSafeState ( int available[], int maximum[][maxResources], int allot[][maxResources] ) {
        markerChk++;
        int index;
        int count = 0;
        int need[maxProcesses][maxResources];
        processCalculation ( need, maximum, allot ); // Function to calculate need matrix
        bool finish[maxProcesses] = { 0 };

        int work[maxResources];
        int i;
        for ( i = 0; i < maxResources; ++i ) {
                work[i] = available[i];
        }


        while ( count < maxProcesses ) {
                int proc;
                bool found = false;
                for ( proc = 0; proc < maxProcesses; ++proc ) {
                        if ( finish[proc] == 0 ) {
                                int j;
                                for ( j = 0; j < maxResources; ++j ) {
                                        if ( need[proc][j] > work[j] )
                                            break;
                                }
                                if ( j == maxResources ) {
                                        int k;
                                        for ( k = 0; k < maxResources; ++k ) {
                                                work[k] += allot[proc][k];
                                        }
                                        finish[proc] = 1;
                                        found = true;
                                }
                        }
                }

                if ( found == false ) {
                        //return true;
                        return false;
                }
        }

        return true;

}

int dequeue ( Queue* queue ) {
        if ( isEmpty ( queue ) )
                return INT_MIN;

        int item = queue->array[queue->topQueue];
        queue->topQueue = ( queue->topQueue + 1 ) % queue->capacity;
        queue->size = queue->size - 1;

        return item;
}

int topQueue ( Queue* queue ) {
        if ( isEmpty ( queue ) )
                return INT_MIN;

        return queue->array[queue->topQueue];
}

int bottomQueue ( Queue* queue ) {
        if ( isEmpty ( queue ) )
                return INT_MIN;

        return queue->array[queue->bottomQueue];
}

int isFull ( Queue* queue ) {
        return ( queue->size == queue->capacity );
}


int isEmpty ( Queue* queue ) {
        return ( queue->size == 0 );
}

Queue* createQueue ( unsigned capacity ) {
        Queue* queue = (Queue*) malloc ( sizeof ( Queue ) );
        queue->capacity = capacity;
        queue->topQueue = queue->size = 0;
        queue->bottomQueue = capacity - 1;      // This is important, see the enqueue
        queue->array = (int*) malloc ( queue->capacity * sizeof ( int ) );

        return queue;
}


// Prints program stat
void displayStatistics() {
        double approvalPercentage = grantRequest / requestResource;

        fprintf ( fp, "*****Program Statistics*****\n" );
        fprintf ( fp, "   0) Total Created Processes (P): %d\n", createdProcs );
        fprintf ( fp, "   1) Total Requested Resources (R): %d\n", requestResource );
        fprintf ( fp, "   2) Total Granted Requests: %d\n", gr );
        fprintf ( fp, "   3) Percentage of Granted Requested: %d\n", percentage );
        fprintf ( fp, "   4) Total Deadlock Avoidance Algorithm Used (Banker's Algorithm): %d\n", markerChk );
        fprintf ( fp, "   5) Total Resources Released: %d\n", releaseResource );

        printf ( "*****Program Statistics*****\n" );
        printf ( "   0) Total Created Processes (P): %d\n", createdProcs );
        printf ( "   1) Total Requested Resources (R): %d\n", requestResource);
        printf ( "   2) Total Granted Requests: %d\n", gr );
        printf ( "   3) Percentage of Granted Requested: %d\n", percentage );
        printf ( "   4) Total Deadlock Avoidance Algorithm Used (Banker's Algorithm): %d\n", markerChk );
        printf ( "   5) Total Resources Released: %d\n", releaseResource );
}

// Function for signal handling.
void handle ( int sig_num ) {
        if ( sig_num == SIGINT || sig_num == SIGALRM ) {
                printf ( "Signal to terminate was received.\n" );
                displayStatistics();
                killProcesses();
                kill ( 0, SIGKILL );
                wait ( NULL );
                exit ( 0 );
        }
}


void processCalculation ( int need[maxProcesses][maxResources], int maximum[maxProcesses][maxResources], int allot[maxProcesses][maxResources] ) {
        int i, j;
        for ( i = 0; i < maxProcesses; ++i ) {
                for ( j = 0; j < maxResources; ++j ) {
                        need[i][j] = maximum[i][j] - allot[i][j];
                }
        }
}

// Function to print Allocated Resource Table
void displayTable( int num1, int array[][maxResources] ) {
        int i, j;
        num1 = createdProcs;

        printf ( "*****Currently Allocated Resources*****\n" );
        printf ( "\tR1\tR2\tR3\tR4\tR5\tR6\tR7\tR8\tR9\tR10\tR11\tR12\tR13\tR14\tR15\tR16\tR17\tR18\tR19\tR20\n" );
        for ( i = 0; i < createdProcs; ++i ) {
                printf ( "P%d:\t", i );
                for ( j = 0; j < 20; ++j ) {
                        printf ( "%d\t", array[i][j] );
                }
                printf ( "\n" );
        }
}

// Function to print the table showing the max claim vectors for each process
void displayMaxTable( int num1, int array[][maxResources] ){
        int i, j;
        num1 = createdProcs;

        printf ( "*****Max Claim Table*****\n" );
        printf ( "\tR1\tR2\tR3\tR4\tR5\tR6\tR7\tR8\tR9\tR10\tR11\tR12\tR13\tR14\tR15\tR16\tR17\tR18\tR19\tR20\n" );
        for ( i = 0; i < createdProcs; ++i ) {
                printf ( "P%d:\t", i );
                for ( j = 0; j < 20; ++j ) {
                        printf ( "%d\t", array[i][j] );
                }
                printf ( "\n" );
        }
}

void incrementClock ( unsigned int shmClock[] ) {
        int processingTime = 5000; // Can be changed to adjust how much the clock is incremented.
        shmClock[1] += processingTime;

        shmClock[0] += shmClock[1] / 1000000000;
        shmClock[1] = shmClock[1] % 1000000000;
}

// Function to terminate all shared memory and message queue
void killProcesses() {
        fclose ( fp );

        // Detach from shared memory
        shmdt ( shmClock );
        shmdt ( shmBlocked );

        // Destroy shared memory
        shmctl ( shmClockID, IPC_RMID, NULL );
        shmctl ( shmBlockedID, IPC_RMID, NULL );

        // Destroy message queue
        msgctl ( messageID, IPC_RMID, NULL );
}


void enQueue ( Queue* queue, int item ) {
        if ( isFull ( queue ) )
                return;

        queue->bottomQueue = ( queue->bottomQueue + 1 ) % queue->capacity;
        queue->array[queue->bottomQueue] = item;
        queue->size = queue->size + 1;
}

