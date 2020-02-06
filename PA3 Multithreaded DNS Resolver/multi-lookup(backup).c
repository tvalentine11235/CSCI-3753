#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <semaphore.h>

#include "util.c"
#include "multi-lookup.h"
#include "queue.h"

#define MAXFILES 10
#define MAXREQUEST 5
#define MAXRESOLVE 10
#define BUFFSIZE 1025
#define MAXURL 1024
#define INPUTFILESTRING "%1024s"

void* request(void* id)
{
	struct requester* requester = id;
	printf("requester %d is at top of file %d\n",requester->id,requester->fileNum);		
	char string[1025];
	char* payload;
	int cf = 1;
	while(requester->everything_ptr->completed < requester->everything_ptr->numFiles)
	{
		printf("requester %d is at first(1) while loop %d\n",requester->id,requester->fileNum);
		pthread_mutex_lock(&requester->everything_ptr->buffMu);
		while(fscanf(requester->inputFile, INPUTFILESTRING, string) > 0)
		{
			printf("requester %d is at second(2) while loop %d\n",requester->id,requester->fileNum);
			bool push = false;
			while(push == false)
			{		
				printf("requester %d is at third(3) while loop %d\n",requester->id,requester->fileNum);	
				if(!queue_is_full(&requester->everything_ptr->buffer))
				{
					printf("requester %d has determined the queue is not full %d\n",requester->id,requester->fileNum);
					payload = malloc(BUFFSIZE);					
					strncpy(payload, string, BUFFSIZE);
					
					queue_push(&requester->everything_ptr->buffer,payload);
					//printf("%s has been written to the buffer\n",payload);
					
					push = true;
				}
			}
		}
		pthread_mutex_unlock(&requester->everything_ptr->buffMu);
		printf("requester %d unlock the buffMu %d\n",requester->id,requester->fileNum);
	
		pthread_mutex_lock(&requester->everything_ptr->reqMu);
		if(requester->everything_ptr->AuxTable[requester->fileNum] == 0)
		{
			printf("requester %d is in an unserviced file %d\n",requester->id,requester->fileNum);	
			//cf = fclose(requester->inputFile);
			cf = 0;
			
			
			if (cf == 0)
			{
				printf("requester %d closed file %d\n",requester->id,requester->fileNum);	
				requester->everything_ptr->completed++;
				requester->everything_ptr->AuxTable[requester->fileNum] = 1;
			}
		}
		if (requester->everything_ptr->c < requester->everything_ptr->numFiles)
		{
			printf("requester %d is looking for the next unserviced file %d\n",requester->id,requester->fileNum);
			requester->inputFile = requester->everything_ptr->fileArray[requester->everything_ptr->c];
			requester->everything_ptr->c++;
			cf = 1;
			requester->count++;					
		}
		else
		{
			for(int i = 0; i < requester->everything_ptr->numFiles; i++)
			{
				if(requester->everything_ptr->AuxTable[i] == 0)
				{
					printf("I'm going to help with file %d\n", i);
					requester->inputFile = requester->everything_ptr->fileArray[i];
					requester->fileNum = i;
					cf = 1;
					break;
				}
			}
			requester->count++;
		}
		pthread_mutex_unlock(&requester->everything_ptr->reqMu);
		printf("files completed %d number of files %d\n", requester->everything_ptr->completed, requester->everything_ptr->numFiles);
	}	
	
		
	pthread_mutex_lock(&requester->everything_ptr->reqMu);			
	fprintf(requester->serviced,"Thread %d serviced %d files\n", requester->id, requester->count);
	pthread_mutex_unlock(&requester->everything_ptr->reqMu);

	//free(payload);
	return NULL;

}

void* resolve(void* ident)
{
	struct resolver* resolver = ident;
	char string[1024];
	char ipaddress[INET6_ADDRSTRLEN];
	int length = sizeof(ipaddress);
	char* payload;
	while((!queue_is_empty(&resolver->everything_ptr->buffer)) || (resolver->everything_ptr->reqComplete != 1))
	{
		if(!queue_is_empty(&resolver->everything_ptr->buffer))
		{
			pthread_mutex_lock(&resolver->everything_ptr->buffMu);//BUFFER ACCESS LOCK
			payload = queue_pop(&resolver->everything_ptr->buffer);
			printf("%s grabbed from buffer\n", payload);
			pthread_mutex_unlock(&resolver->everything_ptr->buffMu);//BUFFER ACCESS UNLOCK
			
			strncpy(string, payload, BUFFSIZE);
			
			pthread_mutex_lock(&resolver->everything_ptr->resMu);//DNS LOOKUP & RESULTS LOCK
			fprintf(resolver->results,"%s, ",string);
			if(dnslookup(string,ipaddress,length) == UTIL_FAILURE)
			{		
				fprintf(resolver->results,"\n");
				fprintf(stderr, "Hostname %s is totally bogus dude!\n", string);
			}
			else
			{
				fprintf(resolver->results, "%s made by thread %d\n", ipaddress, resolver->id);
			}
			pthread_mutex_unlock(&resolver->everything_ptr->resMu);//DNS LOOKUP & RESULTS UNLOCK
		}

		
	}
	free(payload);
	return NULL;
}


int main (int argc, char * argv[])
{

	
	//FILE* check;
	int numRequesters = atoi(argv[1]);
	int numResolvers = atoi(argv[2]);
	
	// check to make sure input files meets constraints
	if (argc < 6)
	{
		fprintf(stderr, "Not enough input parameters \n");
		return -1;
	}
	
	if (argc-5 > MAXFILES)
	{
		fprintf(stderr, "Too many input files. Program is limited to %d inputs \n",MAXFILES);
		return -1;
	}

	if (numRequesters > MAXREQUEST)
	{
		fprintf(stderr, "Too many requester threads. Program is limited to %d requesters. Number of requesters set to %d \n", MAXREQUEST, MAXREQUEST);
		numRequesters = 5;
	}

	if (numResolvers > MAXRESOLVE)
	{
		fprintf(stderr, "Too many resolver threads. Program is limited to %d resolvers. Number of resolvers set to %d \n", MAXRESOLVE, MAXRESOLVE);
		numResolvers = 10;
	}

	/*check = fopen(argv[3], "r");
	fclose(check);
	check = fopen(argv[4], "r");
	fclose(check);*/

	int numFiles = argc-5;
	FILE* input[numFiles];
	FILE* test[numFiles];
	FILE* serviced = fopen(argv[3], "w");
	FILE* results = fopen(argv[4], "w");
	
	struct everything tyler;
	pthread_mutex_init(&tyler.buffMu, NULL);
	pthread_mutex_init(&tyler.resMu, NULL);
	pthread_mutex_init(&tyler.reqMu, NULL);
	queue_init(&tyler.buffer, 1025);
	tyler.c = numRequesters;
	tyler.completed = 0;
	tyler.reqComplete = 0;
		
	int a1 = 0;
	int b1 = 5;
	int c1 = numFiles;
	for (int i =0; i < numFiles; i++)
	{
		if(!fopen(argv[5+i], "r"))
		{
			printf("invalid file\n");
			b1++;
			c1--;
		}
		else
		{
			test[a1] = fopen(argv[b1], "r");
			printf("file %d opened\n", a1);
			a1++;
			b1++;
		}	
	}
	printf("%d\n", c1);
	numFiles = c1;
	tyler.numFiles = numFiles;

	for(int i = 0; i < numFiles; i++)
	{
		input[i] = test[i];
	}
	
	int AuxTable[numFiles];
	for (int i = 0; i < numFiles; i++)
	{
		AuxTable[i] = 0;
	}

	tyler.AuxTable = AuxTable;
	
	//sem_init(&tyler.semaphore, 0, 1);
	
	
	tyler.fileArray = input;
	
	struct requester reqInfo[numRequesters]; // info
	pthread_t reqArray[numRequesters]; // shell
	
	struct resolver resolveInfo[numResolvers]; // info
	pthread_t resolveArray[numResolvers]; // shell

	for(int i = 0; i < numFiles && i < numRequesters; i++)
	{
		reqInfo[i].everything_ptr = &tyler;
		reqInfo[i].inputFile = input[i];
		reqInfo[i].serviced = serviced;
		reqInfo[i].count = 0;
		reqInfo[i].fileNum = i;
		reqInfo[i].id = i;
		int Error = pthread_create	(&(reqArray[i]), NULL, request, &(reqInfo[i]));
		if (Error == 0)	
		{	
			printf("requester %d created\n", i);
		}
	}
	
	if(numRequesters > numFiles)
	{
		for(int i = numFiles; i < numRequesters; i++)
		{
			for(int j = 0; j < numFiles; j++)
			{
				if(tyler.AuxTable[j] == 0)
				{
					reqInfo[i].everything_ptr = &tyler;
					reqInfo[i].inputFile = input[j];
					reqInfo[i].count = 0;
					reqInfo[i].fileNum = j;
					reqInfo[i].id = i;
					int Error = pthread_create	(&(reqArray[i]), NULL, request, &(reqInfo[i]));
					if (Error == 0)	
					{	
						printf("requester %d created\n", i);
						j = numFiles;
					}
					
				}				
			}
		}
	}

	for(int i = 0; i < numResolvers; i++)
	{
		resolveInfo[i].everything_ptr = &tyler;
		resolveInfo[i].results = results;
		resolveInfo[i].id = i;
		pthread_create (&(resolveArray[i]), NULL, resolve, &(resolveInfo[i]));
	}
	
	for (int i = 0; i < numRequesters; i++)
	{
		fprintf(stdout, "Waiting on requesters\n");
		fprintf(stdout, "requester %d has finished %d\n", i,pthread_join(reqArray[i],NULL));
	}
	tyler.reqComplete = 1;
	
	
	for (int i = 0; i < numResolvers; i++)
	{
		fprintf(stdout, "Waiting on resolvers\n");
		fprintf(stdout, "resolver %d has finished %d\n", i,pthread_join(resolveArray[i],NULL));
	}

	

}	