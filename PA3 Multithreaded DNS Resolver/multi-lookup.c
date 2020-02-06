#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/time.h>
#include <time.h>

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
	char string[1025];
	char* payload;
	bool push = false;
	
	bool worked = false; // check to see if a thread actually pushed anything onto the buffer for a given file
	int servCount = 0;
	int cf = requester->fileNum;

	while(cf < requester->everything_ptr->numFiles) // while the current file is less than the total number of files
	{
		if(requester->everything_ptr->fileArray[cf] != NULL) // check for bogus files
		{
			requester->inputFile = requester->everything_ptr->fileArray[cf]; // get file to be worked
			
			while(fscanf(requester->inputFile, INPUTFILESTRING, string) > 0) // grab a line from the txt file if there is one
			{
				
				while(push == false)
				{		
					
					if(!queue_is_full(&requester->everything_ptr->buffer))
					{
						payload = malloc(BUFFSIZE);					
						strncpy(payload, string, BUFFSIZE);
						pthread_mutex_lock(&requester->everything_ptr->buffMu); // lock the buffer
						queue_push(&requester->everything_ptr->buffer,payload);
						pthread_mutex_unlock(&requester->everything_ptr->buffMu); // unlock the buffer
						push = true; // string was pushed to the buffer
						worked = true; // this thread did something!
					}
				}
				
				push = false;				
			}

			if (worked == true)
			{
				servCount++; // this thread did something so we can count this as a file it worked on
				worked = false;
			}

			// check to see if every file has been looked at before the thread is destroyed
			if (requester->count < requester->everything_ptr->numFiles)
			{
				requester->count++;
				if (cf == requester->everything_ptr->numFiles-1) // loop the current file around if at the end of the array
				{
					cf = 0;
				}
				else
				{
					cf++;
				}
			}
			else
			{
				cf++; // increment to exit while loop
			}
		}
	}
	
	pthread_mutex_lock(&requester->everything_ptr->reqMu); // lock serviced mutex
	fprintf(requester->serviced,"Thread %d serviced %d files\n", requester->id, servCount); // write to serviced.txt
	pthread_mutex_unlock(&requester->everything_ptr->reqMu); // unlock serviced mutex

	return NULL;
}



void* resolve(void* ident)
{
	struct resolver* resolver = ident;
	char string[1024];
	char ipaddress[INET6_ADDRSTRLEN];
	int length = sizeof(ipaddress);
	char* payload;
	int complete = 0;
	
	// check if requesters are done AND queue is empty
	while((!queue_is_empty(&resolver->everything_ptr->buffer)) || (complete != 1))
	{

		complete = resolver->everything_ptr->reqComplete; // check this every time to see if requesters have finished
		
		if(!queue_is_empty(&resolver->everything_ptr->buffer))
		{
			pthread_mutex_lock(&resolver->everything_ptr->buffMu);//BUFFER ACCESS LOCK
			payload = queue_pop(&resolver->everything_ptr->buffer);
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
				fprintf(resolver->results, "%s\n", ipaddress);
			}
			pthread_mutex_unlock(&resolver->everything_ptr->resMu);//DNS LOOKUP & RESULTS UNLOCK
		}

		
	}
	free(payload);
	return NULL;
}


int main (int argc, char * argv[])
{
	/* time function*/
	struct timeval start, stop;
	gettimeofday(&start,NULL);
	
	// check to make sure input files meets constraints
	if (argc < 6)
	{
		fprintf(stderr, "Not enough input parameters \n");
		return -1;
	}
	
	int numRequesters = atoi(argv[1]);
	int numResolvers = atoi(argv[2]);

	int numFiles = argc-5;
	FILE* input[numFiles];
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
			//printf("invalid file\n");
			b1++;
			c1--;
		}
		else
		{
			input[a1] = fopen(argv[b1], "r");
			//printf("file %d opened\n", a1);
			a1++;
			b1++;
		}	
	}

	numFiles = c1;
	tyler.numFiles = numFiles;

	/*for(int i = 0; i < numFiles; i++)
	{
		input[i] = test[i];
	}*/
	
	tyler.fileArray = input;
	
	struct requester reqInfo[numRequesters]; // info
	pthread_t reqArray[numRequesters]; // shell
	
	struct resolver resolveInfo[numResolvers]; // info
	pthread_t resolveArray[numResolvers]; // shell
	
	int EC;
	for(int i = 0; i<numRequesters; i++)
	{
		reqInfo[i].everything_ptr = &tyler;
		if (i > numFiles)
		{
			reqInfo[i].fileNum = 0;
		}
		else
		{
			reqInfo[i].fileNum = i;
		}
		reqInfo[i].id = i;
		reqInfo[i].count = 0;
		reqInfo[i].serviced = serviced;
	
		EC = pthread_create(&(reqArray[i]),NULL,request,&(reqInfo[i]));
		
		if(EC != 0)
		{
			fprintf(stderr,"Requester not created. Error %d\n", EC);
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
		//fprintf(stdout, "Waiting on requesters\n");
		pthread_join(reqArray[i],NULL);
	}
	tyler.reqComplete = 1;
	
	
	for (int i = 0; i < numResolvers; i++)
	{
		//fprintf(stdout, "Waiting on resolvers\n");
		pthread_join(resolveArray[i],NULL);
	}
	
	/* Destroy mutexes */
	pthread_mutex_destroy(&tyler.buffMu);
	pthread_mutex_destroy(&tyler.resMu);
	pthread_mutex_destroy(&tyler.reqMu);

	/* Clean Queue */
	queue_cleanup(&tyler.buffer);
	
	/* Close files */
	fclose(serviced);
	fclose(results);
	for(int i = 0; i < numFiles; i++)
	{
		fclose(input[i]);
	}

	
	/* clock function */
	gettimeofday(&stop,NULL);
	long seconds = (stop.tv_sec - start.tv_sec);
	long micros = ((seconds * 1000000) + stop.tv_usec) - (start.tv_usec);
	printf("multi-lookup ran in %ld.%ld seconds\n", seconds, micros); 

}	