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

#define MAXLINE 2096

int create_socket(int port,char *addr)
{
	int sockfd;
	struct sockaddr_in servaddr;
	//Create a socket for the client
	//If sockfd<0 there was an error in the creation of the socket
	if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
		perror("Error creating socket");
		exit(-1);
	}

	//Creation of the socket
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr= inet_addr(addr);
	servaddr.sin_port =  htons(port); //convert to big-endian order

	//Connection of the client to the socket
	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
		perror("Error connecting to server");
		exit(-1);
	}
	return(sockfd);
}

int main(){
	int sockfd;
	struct sockaddr_in servaddr;
	char sendline[MAXLINE], recvline[MAXLINE];
	char ip_add[] = "10.105.70.35"; //you can hardcode the ip_address of the server here
	int serv_portno=3000;
	//Create a socket for the client
	//If sockfd<0 there was an error in the creation of the socket
	if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
		perror("Error creating socket");
		exit(-1);
	}

	printf("Enter the IP-Address of the server:\n");
	scanf("%s",ip_add);
	fflush(stdout);
	fflush(stdin);
	printf("Enter the port-no of the server:\n");
	scanf("%d",&serv_portno);
	fflush(stdout);
	fflush(stdin);


	//fflush(stdin);
	//fflush(stdout);
	//Creation of the socket
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr= inet_addr(ip_add);
	servaddr.sin_port =  htons(serv_portno); //convert to big-endian order

	//Connection of the client to the socket
	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
		perror("Error connecting to server");
		exit(-1);
	}
	printf("myftp>");
	while (fgets(sendline, MAXLINE, stdin) != NULL){
		
		send(sockfd, sendline, sizeof(sendline), 0);

  		char *token,*dummy;
  		dummy=sendline;
  		token=strtok(dummy," ");


  		if(strcmp("quit\n",sendline)==0){
		   	close(sockfd);
			return 0;
		}
		else if(strcmp("servls\n",sendline)==0)  {
			char buff[MAXLINE],check[MAXLINE]="1",port[MAXLINE];
			int data_port,datasock;

			recv(sockfd, port, MAXLINE,0);				//reciening data connection port
			data_port=atoi(port);
			printf("data_port = %d\n", data_port);
			datasock=create_socket(data_port,ip_add);

			/*while(strcmp("1",check)==0){ 				//to indicate that more blocks are coming
				recv(datasock,check,MAXLINE,0);
				if(strcmp("0",check)==0)			//no more blocks of data
					break;
				recv(datasock, buff, MAXLINE,0);
				printf("%s\t", buff);
			}*/
			recv(datasock, buff, MAXLINE,0);
			printf("%s", buff);
			close(datasock);
		}
		else if(strcmp("clils\n",sendline)==0)  {
   			system("ls");
   		}

		else if(strcmp("servpwd\n",sendline)==0)  {
			char curr_dir[MAXLINE];
			recv(sockfd, curr_dir, MAXLINE,0);
			printf("%s\n", curr_dir);
		}

		else if(strcmp("clipwd\n",sendline)==0)  {
			system("pwd");
		}

		else if(strcmp("servcd",token)==0)  {
			char check[MAXLINE];
			token=strtok(NULL," \n");
			printf("Given Path: %s\n", token);
			recv(sockfd,check,MAXLINE,0);
			if(strcmp("0",check)==0){
				printf("The given directory does not exist!\n");
			}
		}

		else if (strcmp("clicd",token)==0)  {
			token=strtok(NULL," \n");
			printf("Given Path: %s\n", token);
			if(chdir(token)<0){
				printf("The given directory does not exist!\n");
			}
		}
		else if (strcmp("fput",token)==0)  {
			char port[MAXLINE], buffer[MAXLINE],char_num_blks[MAXLINE],char_num_last_blk[MAXLINE];
			int data_port,datasock,lSize,num_blks,num_last_blk,i;
			FILE *fp;
			recv(sockfd, port, MAXLINE,0);				//receiving the data port
			data_port=atoi(port);
			datasock=create_socket(data_port,ip_add);
			token=strtok(NULL," \n");
			if ((fp=fopen(token,"r"))!=NULL){
				//size of file
				send(sockfd,"1",MAXLINE,0);
				fseek (fp , 0 , SEEK_END);
				lSize = ftell (fp);
				rewind (fp);
				num_blks = lSize/MAXLINE;
				num_last_blk = lSize%MAXLINE; 
				sprintf(char_num_blks,"%d",num_blks);
				send(sockfd, char_num_blks, MAXLINE, 0);

				for(i= 0; i < num_blks; i++) { 
					fread (buffer,sizeof(char),MAXLINE,fp);
					send(datasock, buffer, MAXLINE, 0);
				}
				sprintf(char_num_last_blk,"%d",num_last_blk);
				send(sockfd, char_num_last_blk, MAXLINE, 0);
				if (num_last_blk > 0) { 
					fread (buffer,sizeof(char),num_last_blk,fp);
					send(datasock, buffer, MAXLINE, 0);
				}
				fclose(fp);
				printf("File uploaded Successfully\n");
			}
			else{
				send(sockfd,"0",MAXLINE,0);
				printf("%s: No such file\n", token);				
			}
		}
		else if (strcmp("fget",token)==0)  {
			char port[MAXLINE], buffer[MAXLINE],char_num_blks[MAXLINE],char_num_last_blk[MAXLINE],message[MAXLINE];
			int data_port,datasock,lSize,num_blks,num_last_blk,i;
			FILE *fp;

			recv(sockfd, port, MAXLINE,0);
			data_port=atoi(port);
			datasock=create_socket(data_port,ip_add);
			token=strtok(NULL," \n");
			recv(sockfd,message,MAXLINE,0);

			if(strcmp("1",message)==0){
				if((fp=fopen(token,"w"))==NULL){
					printf("Error opening file\n");
				}
				else
				{
					recv(sockfd, char_num_blks, MAXLINE,0);
					num_blks=atoi(char_num_blks);
					for(i= 0; i < num_blks; i++) { 
						recv(datasock, buffer, MAXLINE,0);
						fwrite(buffer,sizeof(char),MAXLINE,fp);
					}
					recv(sockfd, char_num_last_blk, MAXLINE,0);
					num_last_blk=atoi(char_num_last_blk);
					if (num_last_blk > 0) { 
						recv(datasock, buffer, MAXLINE,0);
						fwrite(buffer,sizeof(char),num_last_blk,fp);
					}
					fclose(fp);
					printf("File downloaded Successfully!");
				}
			}
			else{
				printf("The server couldn't open the file!!\n");
			}
		}
		else{
			printf("An invalid FTP command\n");
		}
		printf("myftp>");
	}
	return 0;
}