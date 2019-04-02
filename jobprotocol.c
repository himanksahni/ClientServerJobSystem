// TODO: Use this file for helper functions (especially those you want available
// to both executables.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <string.h>
#include <arpa/inet.h>

#include "socket.h"

#ifndef PORT
  #define PORT 56730
#endif


#ifndef MINCHARS
    #define MINCHARS 3
#endif
#ifndef MAXCHARS
    #define MAXCHARS 7
#endif
#ifndef MAXDELAY
    #define MAXDELAY 200000000
#endif

#define BUFSIZE 256

/*
 * Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 */
int find_network_newline(const char *buf, int inbuf) {

	for (int i = 0; i < inbuf; i++){
		if (buf[i] == '\n'){
			return i + 1; 
			}
		}
    return -1;
}

int commandCheck(char *message){

	if (strcmp("jobs",message)==0){

		return 0;
	}

	char run[500];
	strcpy(run, message);
	strtok(run," ");

	if (strcmp("run",run)==0){
		return 0;
	}


	if (strcmp("exit",message)==0){
		return 0;	
		
	}//if jobs

	//kill
	char kill[500];
	strcpy(kill,message);
	strtok(kill," ");

	if (strcmp("kill",kill)==0){
		return 0;	
		
	}//if kill

	char watch[500];
	strcpy(watch,message);
	strtok(watch," ");
	if (strcmp("watch",watch)==0){
		return 0;	
	}

	return 1;
}


