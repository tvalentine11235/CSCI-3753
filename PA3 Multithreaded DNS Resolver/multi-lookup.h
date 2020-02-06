#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

struct everything
{
	FILE** fileArray;
	queue buffer;
	char* payload[1024];
	int numFiles;
	int c;
	int completed;
	pthread_mutex_t buffMu;
	pthread_mutex_t resMu;
	pthread_mutex_t reqMu;
	int reqComplete;
	
};

struct requester
{
	struct everything* everything_ptr;	
	FILE* inputFile;
	FILE* serviced;
	int fileNum;
	int count;
	int id;
};

struct resolver
{
	struct everything* everything_ptr;
	FILE* results;
	int id;
};



