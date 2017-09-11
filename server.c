#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() and alarm() */
#include <errno.h>      /* for errno and EINTR */
#include <signal.h>     /* for sigaction() */
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>

#define BUFF 2048

int create_socket(int port)
{
	int listenfd;
	struct sockaddr_in dataservaddr;
	//Create a socket for the soclet
	//If sockfd<0 there was an error in the creation of the socket
	if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
		perror("Error acquiring socket");		
		exit(-1);
	}


	//preparation of the socket address
	dataservaddr.sin_family = AF_INET;
	dataservaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	dataservaddr.sin_port = htons(port);

	if ((bind (listenfd, (struct sockaddr *) &dataservaddr, sizeof(dataservaddr))) <0) {
		perror("Error binding socket");
		exit(-1);
	}

	//listen to the socket by creating a connection queue, then wait for clients
	listen (listenfd, 1);
	return(listenfd);
}

int accept_conn(int sock)
{
	int dataconnfd;
	socklen_t dataclilen;
	struct sockaddr_in datacliaddr;

	dataclilen = sizeof(datacliaddr);
	//accept a connection
	if ((dataconnfd = accept (sock, (struct sockaddr *) &datacliaddr, &dataclilen)) <0) {
		perror("Error acquiring socket");		
		exit(-1);
	}

	return(dataconnfd);
}

int main(){
	int listenfd, connfd, n;
	socklen_t clilen;
	char buf[BUFF];
	struct sockaddr_in cliaddr, servaddr;
	int serv_portno;

	printf("Enter the port no:\n");
	scanf("%d",&serv_portno);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	if (listenfd <0) {
		perror("Error acquiring socket");
		exit(-1);
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);	
	servaddr.sin_port = htons(serv_portno);

	if(bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
		perror("error on binding");
	}

	if(listen (listenfd, 6)<0){
		perror("Error on listening");
	}

	printf("Server is running..\n");


	fd_set read_fds,master; // temp file descriptor list for select()
	FD_ZERO(&read_fds);	//initializing read_fds
	FD_ZERO(&master);	//initializing read_fds
	int fdmax,i;

	// add the listener to the master set
	FD_SET(listenfd, &master);
	// keep track of the biggest file descriptor
	fdmax = listenfd; // so far, it's this one

	struct timeval tv;
	tv.tv_sec = 2;
 	tv.tv_usec = 500000;

 	int newsockfd,id;
 	int data_port = serv_portno;
	while(1){
		read_fds = master; // copy it
		if (select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1){
			perror("Error in select");
			exit(-1);
		}
		for(i = 0; i <= fdmax; i++) 
		{
 			if (FD_ISSET(i, &read_fds))  // we got one!!
			{
				if (i == listenfd) 
				{
					// Accept actual connection from the client and now  stores socket descriptor for client
					clilen = sizeof(cliaddr);
					newsockfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
					if (newsockfd < 0) 
					{
						perror("ERROR occurs while accepting\n");
						exit(1);
					}
					else 
					{
						FD_SET(newsockfd, &master); // add to master set
						if (newsockfd > fdmax)  // keep track of the max
							fdmax = newsockfd;
					
						printf("selectserver: new connection\n");
					}

				}
				else
				{
					data_port-=100;						//for data connection

					id=fork();

					if(id==0){
						close(listenfd);
						while (1)
						{
							//fflush(stdin);
							//fflush(stdout);

							char temp_buf[BUFF];
							memset(buf,'\0',sizeof buf);
							while(1){
								n = recv(i, temp_buf, BUFF,0);
								if(n==48 || n==0){
									break;
								}
								strcpy(buf,temp_buf);
								//printf("buf = %s, n=%d\n",buf,n);
							}
							//printf("n = %d, buf = %s\n",n,buf);
							data_port++;
							printf("Command from client received: %s",buf);
							// break the command into tokens
							char *token,*dummy;
							dummy=buf;
							token=strtok(dummy," ");

							if (strcmp("quit\n",buf)==0)  {
								printf("Exiting.. After the quit command\n");
							}
							else if (strcmp("servls\n",buf)==0)  {
								FILE *in;
								char temp[BUFF],port[BUFF],full[BUFF]="";
								int datasock;	
								sprintf(port,"%d",data_port);
								datasock=create_socket(data_port);			//creating socket for data connection
								send(i, port,BUFF,0);				//sending data connection port no. to client
								datasock=accept_conn(datasock);	 			//accepting connection from client

								if(!(in = popen("ls", "r"))){ //creating a pipe
									perror("Error opening the pipe");
								}

								while(fgets(temp, sizeof(temp), in)!=NULL){
									//write(datasock,"1",BUFF);
									//write(datasock, temp, BUFF);
									strcat(full,temp);
								}
								send(datasock, full, BUFF, 0);
								pclose(in);
							}							
							else if (strcmp("servpwd\n",buf)==0)  {
								char curr_dir[BUFF];

								getcwd(curr_dir,BUFF-1);
								send(i, curr_dir, BUFF, 0);
							}

							else if(strcmp("servcd",token)==0)  {
								token = strtok(NULL," \n");
								if(chdir(token)<0){
									printf("chdir failed!\n");
									send(i,"0",BUFF,0);
								}
								else{
									printf("chdir succeeded!\n");
									send(i,"1",BUFF,0);
								}
							}
							else if(strcmp("fput",token)==0){
								char port[BUFF],buffer[BUFF],char_num_blks[BUFF];
								char char_num_last_blk[BUFF],check[BUFF];
								int datasock,num_blks,num_last_blk,j;

								FILE *fp;
								token=strtok(NULL," \n");
								printf("Given file name is: %s\n", token);
								
								sprintf(port,"%d",data_port);
								datasock=create_socket(data_port);				//creating socket for data connection
								send(i,port,BUFF,0);					//sending data connection port to client
								datasock=accept_conn(datasock);					//accepting connection

								recv(i,check,BUFF,0);								
								if(strcmp("1",check)==0){
									if((fp=fopen(token,"w"))==NULL){
										printf("Error opening file..\n");
									}
									else{
										// get the number of blocks in the file
										recv(i, char_num_blks, BUFF,0);
										num_blks=atoi(char_num_blks);
										//
										for(j= 0; j < num_blks; j++) { 
											recv(datasock, buffer, BUFF,0);
											fwrite(buffer,sizeof(char),BUFF,fp);
										}

										recv(i, char_num_last_blk, BUFF,0);
										num_last_blk=atoi(char_num_last_blk);
										if (num_last_blk > 0) { 
											recv(datasock, buffer, BUFF,0);
											fwrite(buffer,sizeof(char),num_last_blk,fp);
										}
										fclose(fp);
										printf("File downloaded successfully!\n");
									}
								}
								else{
									printf("Client couldn't open the file!\n");
								}
							}
							else if(strcmp("fget",token)==0){
								char port[BUFF],buffer[BUFF],char_num_blks[BUFF],char_num_last_blk[BUFF];
								int datasock,lSize,num_blks,num_last_blk,j;
								FILE *fp;

								token=strtok(NULL," \n");
								printf("Input file name: %s\n", token);
		
								sprintf(port,"%d",data_port);
								datasock=create_socket(data_port);				//creating socket for data connection
								send(i,port,BUFF,0);					//sending port no. to client
								datasock=accept_conn(datasock);					//accepting connnection by client

								if ((fp=fopen(token,"r"))!=NULL){
									//send the size of the file
									send(i,"1",BUFF,0); // the file opened successfully
									fseek (fp , 0 , SEEK_END);
									lSize = ftell (fp);
									rewind (fp);
									num_blks = lSize/BUFF;
									num_last_blk = lSize%BUFF; 
									sprintf(char_num_blks,"%d",num_blks);
									send(i, char_num_blks, BUFF, 0);

									for(j= 0; j < num_blks; j++) { 
										fread (buffer,sizeof(char),BUFF,fp);
										send(datasock, buffer, BUFF, 0);
									}
									sprintf(char_num_last_blk,"%d",num_last_blk);
									send(i, char_num_last_blk, BUFF, 0);
									if (num_last_blk > 0) { 
										fread (buffer,sizeof(char),num_last_blk,fp);
										send(datasock, buffer, BUFF, 0);
									}
									fclose(fp);
									printf("File uploaded Successfully!\n");
								}
								else{
									printf("%s: No such file\n", token);
									send(connfd,"0",BUFF,0);
								}
							}
							else{
								printf("An invalid FTP command\n");
							}
							printf("end of while\n");
						}
						if (n < 0){
							printf("Read error occured\n");
						}
						exit(0);
					}// end of child process
					else
					{
						//parent process
						printf("running parent\n");
						close(i);						
						FD_CLR(i, &master);
					}
				}// end of work of a socket	 
			}// we handled the file descriptor
		}// for loop
		
	}
	return 0;
}