// Jonathan Pham
// CS 4760 Operating Systems
// Assignment 5: Resource Management
// Due: 04/24/2019


#ifndef RESOURCE_MANAGEMENT_H
#define RESOURCE_MANAGEMENT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdbool.h>

#define maxProcesses 100
#define maxResources 20

typedef struct {
	long msg_type;		
	int pid;	
	int tableIndex;		
	int request;		
	int release;		
	bool terminate;		
	bool resourceGranted;	
	unsigned int messageTime[2];	
} MessageQueue;

void handle (int sig_num);

MessageQueue resourceManagement;
int messageID;
key_t messageKey;

int shmClockID;
int *shmClock;
key_t shmClockKey;

int shmBlockedID;
int *shmBlocked;
key_t shmBlockedKey;

#endif









 
