#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "socket.h"
#include "jobprotocol.h"
#include <errno.h>

#define BUF_SIZE 128

int main(int argc, char **argv) {
    // This line causes stdout and stderr not to be buffered.
    // Don't change this! Necessary for autotesting.
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc != 2) {
        fprintf(stderr, "Usage: jobclient hostname\n");
        exit(1);
    }

    int soc = connect_to_server(PORT, argv[1]);

    /* TODO: Accept commands from the user, verify correctness 
     * of commands, send to server. Monitor for input from the 
     * server and echo to STDOUT.
     */
	int max_fd = soc;
    	fd_set all_fds, listen_fds;
    	FD_ZERO(&all_fds);
    	FD_SET(STDIN_FILENO , &all_fds);
    	FD_SET(soc, &all_fds);

	char line[BUF_SIZE + 1];
	//printf("Enter a Valid Command for the server\n");
	while(1) {
		//printf("Enter a Valid Command for the server\n");
		listen_fds = all_fds;
        	int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        	if (nready == -1) {
            		perror("server: select");
            		exit(1);
        		}

		if (FD_ISSET(STDIN_FILENO, &listen_fds)) {
	    		int num_read = read(STDIN_FILENO, line, BUF_SIZE);
            		if (num_read == 0) {
               		break;
           		}

			line[num_read-1] = '\0';
			int check = commandCheck(line);
			if (check == 1){

				printf("Command not found\n");
				continue;
			}


			//jobs
			if (strcmp("jobs",line)==0){
				strcat(line,"\r\n");
				if ((write(soc, line, strlen(line))) != strlen(line)){
					perror("client: write");
		    			close(soc);
					exit(1);
				}
			
			}

			//RUN
			char run[500];
			strcpy(run, line);
			strtok(run," ");
			if (strcmp("run",run)==0){
				strcat(line,"\r\n");
				if ((write(soc, line, strlen(line))) != strlen(line)){
					perror("client: write");
		    			close(soc);
					exit(1);
			}

			}//run


			//exit 
			if (strcmp("exit",line)==0){

				close(soc);
				exit(0);	
		
			}//if jobs

			//kill
			char kill[500];
			strcpy(kill,line);
			strtok(kill," ");
			if (strcmp("kill",kill)==0){
				strcat(line,"\r\n");
				if ((write(soc, line, strlen(line))) != strlen(line)){
					perror("client: write");
		    			close(soc);
					exit(1);
				}	
		
			}//if kill

			char watch[500];
			strcpy(watch,line);
			strtok(watch," ");
			if (strcmp("watch",watch)==0){
				strcat(line,"\r\n");
				if ((write(soc, line, strlen(line))) != strlen(line)){
					perror("client: write");
		    			close(soc);
					exit(1);
				}		
		
			} 

		}
		// if it is socket
		if (FD_ISSET(soc, &listen_fds)){

        		char buf[BUFSIZE] = {'\0'};
        		int inbuf = 0;           // How many bytes currently in buffer?
        		int room = sizeof(buf);  // How many bytes remaining in buffer?
        		char *after = buf;       // Pointer to position after the data in buf

        		int nbytes;
        		nbytes = read(soc, after, room);

			if (nbytes == 0) {
				FD_CLR(soc, &all_fds);
				FD_CLR(STDIN_FILENO, &listen_fds);
                    		close(soc);
				exit(0);
				}
            		// Step 1: update inbuf (how many bytes were just added?)
				inbuf += nbytes;

            			int where;

            			while ((where = find_network_newline(buf, inbuf)) > 0) {

					buf[where-2]='\0';
					//buf[where-1] = '\0';


                			printf("%s\n", buf);

					inbuf -= where;

					memmove(buf, buf + where, inbuf);


            		}
            			// Step 5: update after and room, in preparation for the next read.
				room = sizeof(buf) - inbuf;
				after = buf + inbuf;

        		} 
	

}//while(1)

    close(soc);
    return 0;
}
