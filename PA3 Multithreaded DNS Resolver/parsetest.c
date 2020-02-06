#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "util.c"

#define BUFSIZE 1025

int main(int argc, char* argv[])
{
	int c;
	int i = 0;
	char url[BUFSIZE];
	char ipaddress[INET6_ADDRSTRLEN];
	int length = sizeof(ipaddress);
	memset(url,0,1025);
	memset(ipaddress,0,length);
	FILE* input[9];
	FILE* output = NULL;
	output = fopen("serviced.txt", "w");
	for(int a = 0; a < 10; a++)
	{
		input[a] = fopen(argv[1], "r");
		if(input[a])
		{
			while((c = getc(input[a])) != EOF)
			{
				//printf("%c",c);
				if(c == '\n')
				{
					fprintf(output,"%s,",url);
					printf("%s,",url);
					if(dnslookup(url, ipaddress, length) == UTIL_FAILURE)
					{
						fprintf(output,"\n");
						printf("\n");
						//printf("dnslookup error: %s\n", url);
						//memset(ipaddress,0,length);
					}
					else
					{
						fprintf(output,"%s\n",ipaddress);
						printf("%s\n",ipaddress);
					}
					i = 0;
					memset(url,0,1025);
				}
				else
				{
					url[i] = c;
					i++;
				}
			}
			fclose(input[a]);
			fclose(output);
		}
		else
		{
			break;
		}
	}
}