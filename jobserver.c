#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <signal.h>
#include "socket.h"
#include "jobprotocol.h"
#include <errno.h>

#define QUEUE_LENGTH 5

#ifndef JOBS_DIR
    #define JOBS_DIR "jobs/"
#endif



    /* TODO: Initialize job and client tracking structures, start accepting
     * connections. Listen for messages from both clients and jobs. Execute
     * client commands if properly formatted. Forward messages from jobs
     * to appropriate clients. Tear down cleanly.
     */


    /* Here is a snippet of code to create the name of an executable to execute:
     * char exe_file[BUFSIZE];
     * sprintf(exe_file, "%s/%s", JOBS_DIR, <job_name>);
     */

int activeJobs=0;
int t=1;
struct JobInfo {
    int pid;
    int watch[100];
};

struct JobInfo jobs[32];


void remove_job(int pid,int client, struct JobInfo *jobs) {

    for (int index = 0; index < 32; index++) {
        	if (jobs[index].pid == pid){
			jobs[index].pid = -1;
        		for(int i=0;i<100;i++){
				jobs[index].watch[i] = -1;
			}
		}
	}


}


void setup_new_job(int pid,int client, struct JobInfo *jobs) {
    int job_index = 0;
    while (job_index < 32 && jobs[job_index].pid != -1) {
        job_index++;
    }

    if (job_index > 32) {
        fprintf(stderr, "server: max concurrent jobs\n");
    }

    jobs[job_index].pid = pid;
    int watch_index = 0;
    while (watch_index < 100 && jobs[job_index].watch[watch_index] != -1) {
        watch_index++;
    }
    jobs[job_index].watch[watch_index] = client;

}

void handler(int code) {
    printf("\n");
    t=0;
}

void handler2(int sig) {
	pid_t pid;
	int status;
 	pid = wait(&status);

	for (int index = 0; index < 32; index++) {
        	if (jobs[index].pid == pid){
			jobs[index].pid = -1;
        		for(int i=0;i<100;i++){
				jobs[index].watch[i] = -1;
			}
		}
	}
	activeJobs-=1;
	printf("[JOB %d] Exited with status %d.\n",pid,status);
	return;
}


int setup_new_client(int fd, int *clients) {
    int client_index = 0;
    while (client_index < 100 && clients[client_index] != -1) {
        client_index++;
    }

    int client_fd = accept_connection(fd);
    if (client_fd < 0) {
        return -1;
    }

    if (client_index > 100) {
        fprintf(stderr, "server: max concurrent connections\n");
        close(client_fd);
        return -1;
    }

    clients[client_index] = client_fd;
    return client_fd;
}

int main(void) {
    // This line causes stdout and stderr not to be buffered.
    // Don't change this! Necessary for autotesting.
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    struct sockaddr_in *self = init_server_addr(PORT);
    int sock_fd = setup_server_socket(self, QUEUE_LENGTH);
    int clients[100];


    struct sigaction newact;
    newact.sa_handler = handler;
    newact.sa_flags = SA_RESTART;
    sigemptyset(&newact.sa_mask);
	

    if (sigaction(SIGINT, &newact, NULL) < 0){
        perror("sigaction");
        exit(1);
    }

    struct sigaction newact1;
    newact1.sa_handler = handler2;
    newact1.sa_flags = SA_RESTART;
    sigemptyset(&newact1.sa_mask);
	

    if (sigaction(SIGCHLD, &newact1, NULL) < 0){
        perror("sigaction");
        exit(1);
    }

    	for (int index = 0; index < 32; index++) {
        	jobs[index].pid = -1;
        	for(int i=0;i<101;i++){
			jobs[index].watch[i] = -1;}

    }


	for (int i = 0; i < 100; i++) {  
        	clients[i] = -1;  } 

        int max_fd = sock_fd;
        fd_set all_fds, listen_fds;
        FD_ZERO(&all_fds);
        FD_SET(sock_fd, &all_fds);
	int child;

	int pipe_fd[2];

	if (pipe(pipe_fd) == -1){
		perror("pipe");
	}
	if (pipe_fd[0] > max_fd) {
                max_fd = pipe_fd[0];
           		 }

	FD_SET(pipe_fd[0], &all_fds);


	char buf[BUFSIZE] = {'\0'};
        int inbuf = 0;           // How many bytes currently in buffer?
        int room = sizeof(buf);  // How many bytes remaining in buffer?
        char *after = buf;       // Pointer to position after the data in buf

        int nbytes;
	while (t) {

		listen_fds = all_fds;
        	int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
		if (errno == EINTR) {
			if(t==0){break;}
			}
        	if (nready == -1) {
            		continue;
        			}
		if (FD_ISSET(sock_fd, &listen_fds)) {
			int fd = setup_new_client(sock_fd, clients);
        		if (fd < 0) {
            			continue;
        		}
			if (fd > max_fd) {
                		max_fd = fd;
           		 }
            		FD_SET(fd, &all_fds);
            		//printf("Accepted connection\n");
        	}

		for (int index = 0; index < 32; index++) {
			if (jobs[index].pid > -1 && FD_ISSET(pipe_fd[0], &listen_fds)) {
				
				
        			nbytes = read(pipe_fd[0], after, room);

					inbuf += nbytes;

            				int where;

				while ((where = find_network_newline(buf, inbuf)) > 0) {
					
					if(buf[where-2] == '\r'){
						buf[where-2]='\0';
					}
					else{
						buf[where-1]='\0';
					}

					printf("Next message: %s\n", buf);
					for(int i=0;i<100;i++){
						int jpid = jobs[index].pid;
						if (jobs[index].watch[i] != -1){
							int fd = jobs[index].watch[i];
							char cmessage[256];
							sprintf(cmessage,"[JOB %d] %s\r\n",jpid, buf);
							if ((write(fd, cmessage, strlen(cmessage))) != strlen(cmessage)){
								perror("server: write");
								exit(1); 
							}

					
						}
					}	

					inbuf -= where;

					memmove(buf, buf + where, inbuf);

				    }
					room = sizeof(buf) - inbuf;
					after = buf + inbuf;
				}

			}		
		


		for (int index = 0; index < 100; index++) {
			if (clients[index] > -1 && FD_ISSET(clients[index], &listen_fds)) {

				int fd = clients[index];
				
				char command[256];
        			int nbytes;
        			nbytes = read(fd, command, sizeof(command) - 1);

				int where = find_network_newline(command, nbytes);
           			command[where-2] = '\0';


				if (nbytes == 0) {
					FD_CLR(clients[index], &all_fds);
                    			close(clients[index]);
					printf("[CLIENT %d] Connection closed\n", clients[index]);
        				clients[index] = -1;
					continue;
				}

				int check = commandCheck(command);
				if (check == 1){
					printf("Invalid command: %s\n",command);
					char *message= malloc(sizeof(char)*50);
					sprintf(message,"[SERVER] Invalid command: %s\r\n",command);

					if ((write(fd, message, strlen(message))) != strlen(message)){
						perror("server: write");
						exit(1); 
					}
					
					continue;
				}
			

				char cfd[20];
				sprintf(cfd, "[CLIENT %d]",clients[index]);
	
				// JOBS
				if (strcmp("jobs",command)==0){
					printf("%s %s\n",cfd,command);

					if (activeJobs == 0){
						char *message = "[SERVER] No currently running jobs\r\n";
						if ((write(fd, message, strlen(message))) != strlen(message)){
							perror("server: write");
							exit(1); 
						}
					}


					else  {
						int i=0;
						char *p=malloc(sizeof(char)*256);
						char spid[10];
						char *message=malloc(sizeof(char)*256);
						for (int index = 0; index < 32; index++) {
        						if (jobs[index].pid != -1){
								sprintf(spid, "%d ", jobs[index].pid);
								if (i==0){
									strcpy(p, spid); i++;			
								}
								else{
									strcat(p,spid);
								}
							}	
						}
						
						printf("%s\n",p);
						sprintf(message, "[SERVER] %s\r\n", p);
						//printf("%s\n",message);
						if ((write(fd, message, strlen(message))) != strlen(message)){
							perror("server: write");
							exit(1); 
						}
					}

				}// JOBS

				//RUN
				char run[500];
				strcpy(run, command);
				strtok(run," ");
				if (strcmp("run",run)==0) {
					printf("%s %s\n",cfd, command);
				
					if (activeJobs > 32){
						char *message = "[SERVER] MAXJOBS exceeded\r\n";
						if((write(fd, message, sizeof(char)*50)) != sizeof(char)*50) {
							perror("server: write");
							exit(1); }
						printf("%s", message);
					} //activeJobs > 32

					//ACTIVEJOBS < 32
					else {
						child = fork();
						activeJobs+=1;
						if (child < 0) {
        						perror("fork");
							char *message = "[SERVER] Fork error Job cannot be created\r\n";
							if((write(fd, message, sizeof(char)*50)) != sizeof(char)*50) {
								perror("server: write");
								activeJobs-=1;
								exit(1); }
							printf("%s",message);
						}//child==-1

					if (child > 0){

						setup_new_job(child ,fd, jobs);	

						}

					//CHILD
					if (child==0) {

						strtok(command," ");
						char *job_name = strtok(NULL," ");
						char *args = strtok(NULL,"");
		
						char exe_file[BUFSIZE];
     						sprintf(exe_file, "%s%s", JOBS_DIR, job_name);

						char args2[500];
						sprintf(args2, "%s %s",job_name, args);

						char *arguments[50];
						arguments[0] = strtok(args2," ");

						close(pipe_fd[0]);
						dup2(pipe_fd[1],1);
						close(pipe_fd[1]);

						int i=0;
						int c=getpid();
						while (arguments[i]!=NULL){
							i += 1;
							arguments[i] = strtok(NULL," "); }//while for arguments

						//activeJobs+=1;
						if ((execvp(exe_file, arguments)) == -1){
							perror("exec");
							char *message = "[SERVER] exec error Job cannot be created\r\n";
							if((write(fd, message, strlen(message))) != strlen(message)) {
								perror("server: write");

								remove_job(c,fd,jobs);
								activeJobs-=1;
								exit(1); }
						}

					}//CHILD

				}//activeJobs < 32

			}//RUN

			char kil[500];
			strcpy(kil,command);
			strtok(kil," ");
			if (strcmp("kill",kil)==0){
				printf("%s %s\n",cfd, command);
				int killed=0;
				char *pid = strtok(NULL,"");
				int kill_pid= atoi(pid);
				for (int index = 0; index < 32; index++) {
        				if (jobs[index].pid == kill_pid){
						kill(kill_pid, SIGKILL);
						killed=1;
						break;
					}
				}
				if(killed==0){
					char message[256];
					char message1[256];
					sprintf(message,"Job %d not found",kill_pid);
					printf("%s\n",message);
					sprintf(message1,"[SERVER] %s\r\n",message);
					if((write(fd, message1, strlen(message1))) != strlen(message1)) {
						perror("server: write");
						exit(1); }
					
					}
			
			}//kill

			char watch[500];
			strcpy(watch,command);
			strtok(watch," ");
			if (strcmp("watch",watch)==0){
				printf("%s %s\n",cfd, command);
				char *pid= strtok(NULL,"");	
				int job_pid= atoi(pid);
				int watched = 0;
				int found = 0;
				for (int index = 0; index < 32; index++) {
        				if (jobs[index].pid == job_pid){
						found = 1;
						for(int i=0; i < 32; i++ ){
							if(jobs[index].watch[i] == fd){
								watched=1;
								jobs[index].watch[i] = -1;

								char message[256];
								char message1[256];

								sprintf(message,"No longer watching job %d",job_pid);
								printf("%s\n",message);
								sprintf(message1,"[SERVER] %s\r\n",message);

								if((write(fd, message1, strlen(message1))) != strlen(message1)) {
									perror("server: write");
									exit(1); }
							}
							

						}if(watched==0){

							for(int i=0; i < 32; i++ ){
								if(jobs[index].watch[i] == -1){

									jobs[index].watch[i] = fd;
									char message[256];
									char message1[256];

									sprintf(message,"Watching job %d",job_pid);
									printf("%s\n",message);
									sprintf(message1,"[SERVER] %s\r\n",message);

									if((write(fd, message1, strlen(message1))) != strlen(message1)) {
										perror("server: write");
										exit(1); }
									break;
								}	

							}
						}




					break;
					}
				}
				if(found==0){
					char message[256];
					char message1[256];
					sprintf(message,"Job %d not found",job_pid);
					printf("%s\n",message);
					sprintf(message1,"[SERVER] %s\r\n",message);
					if((write(fd, message1, strlen(message1))) != strlen(message1)) {
						perror("server: write");
						exit(1); }
					
					}
			}
		}//for isset

	}//FOR index
	
}//while(1)
	for (int index = 0; index < 100; index++) {
			if (clients[index] > -1) {
				char *message = "[SERVER] Shutting down\r\n";
				if((write(clients[index], message, strlen(message))) != strlen(message)) {
					perror("server: write");
					exit(1); }
				FD_CLR(clients[index], &all_fds);
                    		close(clients[index]);
				printf("[CLIENT %d] Connection closed\n", clients[index]);
        			clients[index] = -1;}

}
				

    free(self);
    close(sock_fd);
    return 0;
}

