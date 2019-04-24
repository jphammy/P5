// Jonathan Pham
// CS 4760 Operating Systems
// Assignment 5: Resource Management
// Due: 04/24/2019

#include "resourceManagement.h"

bool hasResourcesToRelease ( int arr[] );
bool canRequestMore ( int arr1[], int arr2[] );

int main ( int argc, char *argv[] ) {
            
        int totalProcessLimit = 100;    
        int myPid = getpid();           
        int ossPid = getppid();         
        int processIndex;               
                                        
                                        
        int arrayNumber[20];            
        int allocatedVector[20];        

        if ( signal ( SIGINT, handle ) == SIG_ERR ) {
                perror ( "USER: signal failed." );
        }

        // Access shared memory segments
        shmClockKey = 2016;
        if ( ( shmClockID = shmget ( shmClockKey, ( 2 * ( sizeof ( unsigned int ) ) ), 0666 ) ) == -1 ) {
                perror ( "USER: Failure to find shared memory space for simulated clock." );
                return 1;
        }

        if ( ( shmClock = (unsigned int *) shmat ( shmClockID, NULL, 0 ) ) < 0 ) {
                perror ( "USER: Failure to attach to shared memory space for simulated clock." );
                return 1;
        }

        shmBlockedKey = 3017;
        if ( ( shmBlockedID = shmget ( shmBlockedKey, ( totalProcessLimit * ( sizeof ( int ) ) ), 0666 ) ) == -1 ) {
                perror ( "USER: Failure to find shared memory space for blocked USER process array." );
                return 1;
        }

        if ( ( shmBlocked = (int *) shmat ( shmBlockedID, NULL, 0 ) ) < 0 ) {
                perror ( "USER: Failure to attach to shared memory space for blocked USER process array." );
                return 1;
        }
        // Access message queue
        messageKey = 1996;
        if ( ( messageID = msgget ( messageKey, IPC_CREAT | 0666 ) ) == -1 ) {
                perror ( "USER: Failure to create the message queue." );
                return 1;
        }

        time_t processSeed;
        srand ( ( int ) time ( &processSeed ) % getpid() );

        // Values can be tuned to get desired output
        const int probUpper = 100;
        const int probLower = 1;
        const int requestProb = 100;
        const int releaseProb = 55;
        const int terminateProb = 10;

        arrayNumber[0] = atoi ( argv[1] );
        arrayNumber[1] = atoi ( argv[2] );
        arrayNumber[2] = atoi ( argv[3] );
        arrayNumber[3] = atoi ( argv[4] );
        arrayNumber[4] = atoi ( argv[5] );
        arrayNumber[5] = atoi ( argv[6] );
        arrayNumber[6] = atoi ( argv[7] );
        arrayNumber[7] = atoi ( argv[8] );
        arrayNumber[8] = atoi ( argv[9] );
        arrayNumber[9] = atoi ( argv[10] );
        arrayNumber[10] = atoi ( argv[11] );
        arrayNumber[11] = atoi ( argv[12] );
        arrayNumber[12] = atoi ( argv[13] );
        arrayNumber[13] = atoi ( argv[14] );
        arrayNumber[14] = atoi ( argv[15] );
        arrayNumber[15] = atoi ( argv[16] );
        arrayNumber[16] = atoi ( argv[17] );
        arrayNumber[17] = atoi ( argv[18] );
        arrayNumber[18] = atoi ( argv[19] );
        arrayNumber[19] = atoi ( argv[20] );
        processIndex = atoi ( argv[21] );

	int i, j;
        for ( i = 0; i < 20; ++i ) {
                allocatedVector[i] = 0;
        }


        bool waitingOnRequest = false;  // Flag to prevent USER from requesting another resource
                                        //   while still waiting on a previous request.
        int randomAction;       // Will store the random number to decide what action to take
        int selectedResource;   // Will store the resource that USER wants to request or release
        bool validResource;     // Flag to indicate if the resource is okay to request or release

        // Enter main loop
        while ( 1 ) {

                // If the process is not in the blocked queue in OSS (indicated by shared memory) and
                //   the process is not waiting on OSS to respond to a resource request
                if ( shmBlocked[processIndex] == 0 && waitingOnRequest == false ) {

                        // Check to see if it still needs to resources. If has been allocated enough resources
                        //   to match the max claim vector, then it can do it task and terminate.
                        if ( !canRequestMore ( arrayNumber, allocatedVector ) ) {
                                // Set message outgoing message information and send message
                                resourceManagement.msg_type = 5;
                                resourceManagement.pid = myPid;
                                resourceManagement.tableIndex = processIndex;
                                resourceManagement.request = 0;
                                resourceManagement.release = 0;
                                resourceManagement.terminate = true;
                                resourceManagement.resourceGranted = false;
                                resourceManagement.messageTime[0] = shmClock[0];
                                resourceManagement.messageTime[1] = shmClock[1];

                                if ( msgsnd ( messageID, &resourceManagement, sizeof ( resourceManagement ), 0 ) == -1 ) {
                                        perror ( "USER: Failure to send message." );
                                }


                                exit ( EXIT_SUCCESS );
                        }

                        // If process can still request resources, continue...
                        randomAction = ( rand() % ( probUpper - probLower + 1 ) + probLower );

                        // Request Resource
                        if ( randomAction > releaseProb && randomAction <= requestProb ) {
                                validResource = false;

                                // Check to make sure the selected resource isn't already maxed out. If it is,
                                //   select a different resource to request.
                                while ( validResource == false ) {
                                        selectedResource = ( rand() % ( 19 - 0 + 1 ) + 0 );
                                        if ( allocatedVector[selectedResource] < arrayNumber[selectedResource] ) {
                                                validResource = true;
                                        }
                                } // End of select a valid resource loop

                                // Set message outgoing message information and send message
                                resourceManagement.msg_type = 5;
                                resourceManagement.pid = myPid;
                                resourceManagement.tableIndex = processIndex;
                                resourceManagement.request = selectedResource;
                                resourceManagement.release = -1;
                                resourceManagement.terminate = false;
                                resourceManagement.resourceGranted = false;
                                resourceManagement.messageTime[0] = shmClock[0];
                                resourceManagement.messageTime[1] = shmClock[1];

                                if ( msgsnd ( messageID, &resourceManagement, sizeof ( resourceManagement ), 0 ) == -1 ) {
                                        perror ( "USER: Failure to send message." );
                                }


                                waitingOnRequest = true;
                        } // End of request resource

                        // Release Resource
                        if ( randomAction > terminateProb && randomAction <= releaseProb ) {
                                validResource = false;

                                // Check to make sure that USER currently has resources to release.
                                // If it does, check to make sure it selects a valid resource.
                                if ( hasResourcesToRelease ( allocatedVector ) ) {
                                        while ( validResource == false ) {
                                                selectedResource = ( rand() % ( 19 - 0 + 1 ) + 0 );
                                                if ( allocatedVector[selectedResource] > 0 ) {
                                                        validResource = true;
                                                }
                                        } // End of selected a valid resource loop

                                        // Set message outgoing message information and send message
                                        resourceManagement.msg_type = 5;
                                        resourceManagement.pid = myPid;
                                        resourceManagement.tableIndex = processIndex;
                                        resourceManagement.request = -1;
                                        resourceManagement.release = selectedResource;
                                        resourceManagement.terminate = false;
                                        resourceManagement.resourceGranted = false;
                                        resourceManagement.messageTime[0] = shmClock[0];
                                        resourceManagement.messageTime[1] = shmClock[1];

                                        if ( msgsnd ( messageID, &resourceManagement, sizeof ( resourceManagement ), 0 ) == -1 ) {
                                                perror ( "USER: Failure to send message." );
                                        }

                                        allocatedVector[selectedResource]--;
                                }
                        }

                        // Terminate
                        if ( randomAction > 0 && randomAction <= terminateProb ) {
                                // Set message outgoing message information and send message
                                resourceManagement.msg_type = 5;
                                resourceManagement.pid = myPid;
                                resourceManagement.tableIndex = processIndex;
                                resourceManagement.request = -1;
                                resourceManagement.release = -1;
                                resourceManagement.terminate = true;
                                resourceManagement.resourceGranted = false;
                                resourceManagement.messageTime[0] = shmClock[0];
                                resourceManagement.messageTime[1] = shmClock[1];

                                if ( msgsnd ( messageID, &resourceManagement, sizeof ( resourceManagement ), 0 ) == -1 ) {
                                        perror ( "USER: Failure to send message." );
                                }



                                exit ( EXIT_SUCCESS );
                        }
                }

                // Check to see if OSS has responded to any resource requests for this process
                msgrcv ( messageID, &resourceManagement, sizeof ( resourceManagement ), myPid, IPC_NOWAIT );

                // If a resource request was granted by OSS, reset waitingOnRequest flag and increase
                //   the amount of that resource in the allocated resource vector.
                if ( resourceManagement.resourceGranted == true ) {
                        waitingOnRequest = false;
                        allocatedVector[selectedResource]++;
                }

        }

        return 0;
}

void handle ( int sig_num ) {
        if ( sig_num == SIGINT ) {
                printf ( "%d: Signal to terminate process was received.\n", getpid() );
                exit ( 0 );
        }
}

// Returns true is allocated resource vector has less resources in total than the max claim vector allows
bool canRequestMore ( int arr1[], int arr2[] ) {
        int i;
        int sum1 = 0;
        int sum2 = 0;

        for ( i = 0; i < 20; ++i ) {
                sum1 += arr1[i];        // Total resources in max claim vector
                sum2 += arr2[i];        // Total resources in allocated resource vector
        }

        if ( sum2 < sum1 )
                return true;
        else
                return false;
}

// Returns true if allocated resource vector isn't empty
bool hasResourcesToRelease ( int arr[] ) {
        int i;
        int sum = 0;
        for ( i = 0; i < 20; ++i ) {
                sum += arr[i];
        }

        if ( sum > 0 )
                return true;
        else
                return false;
}
