#include "MyHeader.h" /* for user-defined constants */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> /* basic system data types */
#include <sys/socket.h> /* basic socket definitions */
#include <errno.h> /* for the EINTR constant */
#include <sys/wait.h> /* for the waitpid() system call */
#include <sys/un.h> /* for Unix domain sockets */
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>


/* Size of the request queue. */
#define LISTENQ 20
#define SEM_NAME "4355"
#define MAX_CALLS 1000

struct epiloges{         	 //client choices structure
	int id;		 	 
	int seatsnumber; 	 
	int zone;	 	 //zone number (1=A, 2=B, 3=C, 4=D)
	int credit_card;
};


int planA[100];		//seat - customer mapping array for zone a
int planB[130];		//seat - customer mapping array for zone b
int planC[180];		//seat - customer mapping array for zone c
int planD[230];		//seat - customer mapping array for zone d


// Phone center vars
int Phoners = 10;		//number of phoners
int Terminals = 4;		//number of terminals
int fail_counter = 0;	//failed transactions counter
int counter = 0;		//transactions counter
double sum_delay = 0;	//accumulated time of delay
double sum_service = 0;	//accumulated time of service



int Theater_account = 0;	//theater account
int Bank_account = 0;	//bank local account
int trans_counter = 0;	//transactions counter
int transactions[100];	//array of transactions

int costumerID = 1;

/*
The use of this functions avoids the generation of
"zombie" processes.
*/
void sig_chld( int signo )
{
       pid_t pid;
       int stat;

       while ( ( pid = waitpid( -1, &stat, WNOHANG ) ) > 0 ) {
              printf( "\nChild %d terminated.\n", pid );
       }
}

void quit_process(void);	//Synarthsh termatismou
void sigFunc(int sig);		//Synarthsh exyphrethshs alarm
void printResults(void);	//Synarthsh ektypwshs apotelesmatwn
					

int Aseats = 100; //arxikopoihsh metavlhtwn gia thn diaxeirhsh 8eswn
int Bseats = 130;
int Cseats = 180;
int Dseats = 230;
int Totalseats = 640;


/* mutex static init */	
pthread_mutex_t bank_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t phone_mutex = PTHREAD_MUTEX_INITIALIZER;


pthread_cond_t bank_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t phone_cond = PTHREAD_COND_INITIALIZER;

int bank_service(int credit_card_num, int price)
{	
	int random_num,a;		
	srand(credit_card_num); 
	random_num = rand() % 10 + 1; 
	
	pthread_mutex_lock(&bank_mutex);
	// check for available terminal
	if(Terminals == 0) pthread_cond_wait(&bank_cond, &bank_mutex); //pause if not available terminal
	
	Terminals--; //reduce num of avail. terminals
	
	
	if(random_num==1)	//if rand num = 1 account is invalid
	{
		
		fail_counter++; //increment failed transactions
		a=0;	
	}
	else		//if rand num != 1 account is valid
	{
		
		Bank_account += price;	//raise local bank account		
		a=1; 
	}
	pthread_mutex_unlock(&bank_mutex);
	//continue process
	sleep(4);		
	pthread_mutex_lock(&bank_mutex); //short time pause of thread
	Terminals++;	//increment avail terminals
	pthread_cond_signal(&bank_cond);//aknowledgement that bank_cond has changed
	pthread_mutex_unlock(&bank_mutex); 
	
	return a;
}




void* phone_center(void* sockfd)
{	
	//dilwsh keimenwn gia thn apostolh mhnymatwn
	char exit_message[150];	
	char pelatis[]=" cunstomer";
	char seat[]=", your reserved seats are: ";
	char text[5];
	char pricetxt[]="and the cost of transaction is: ";
	struct epiloges choices;
	
	int price,dif;		
	int credit_check;
	time_t start_time;	//time vars
	time_t delay_time;
	time_t service_time;
	time(&start_time);	//time connecting with phone center
	
	int* connfd = (int*)(sockfd);
	//close(*connfd);

	read(*connfd, &choices, sizeof(choices) );  //read from client
	
	int num = choices.seatsnumber;
	int zone = choices.zone;
	int cred_num = choices.credit_card;
	
	if( (Totalseats - choices.seatsnumber) <= 0) //check if there are available seats
	   {	
		strcpy(exit_message, "\nSOLD OUT "); 
		fail_counter++;			//increment failed transactions
		num = 0;	 
		zone = 0;
		
	   }
	
	
	pthread_mutex_lock(&phone_mutex);//short term pause og thread
	//check for available phoners
	if (Phoners==0) pthread_cond_wait(&phone_cond, &phone_mutex);//pause in case of non available phoners
	int id=costumerID++;
	Phoners--;	//reduce num of available phooners
	time(&delay_time);		
	sum_delay += difftime(delay_time,start_time); 
	pthread_mutex_unlock(&phone_mutex);
	
	sleep(6);	
	pthread_mutex_lock(&phone_mutex); //short term pause og thread
	Phoners++; //increase num of available phoners
	pthread_cond_signal(&phone_cond);	//aknowledgement that num of phoners has changed
	pthread_mutex_unlock(&phone_mutex);	
		
		if(zone == 1) {					//check for zone a A
			if( (Aseats - num) > 0) 	//check if exist requested seats
			{
				price=num*50;			//calculate price
				credit_check = bank_service(cred_num, price);	//terminal call
				if(credit_check){				//check for credit card validation
					pthread_mutex_lock(&phone_mutex);		//pause
					strcpy(exit_message, "\nReservation is successful.Reservatiion id is:");
					dif=100-Aseats;	
					Aseats-=num;	
					Totalseats-=num;				
					sprintf(pelatis,"%s %d",pelatis,id); //string for cleint
					strcat(exit_message,pelatis);
					strcat(exit_message,seat);
					int i;
					for(i=0;i<num;i++)
					{	
						planA[dif+i] = id; //map seats with client id
						sprintf(text," %s%d,","A",dif+i);
						strcat(exit_message,text);
					}
					sprintf(pricetxt,"%s%d",pricetxt,price);
					strcat(exit_message,pricetxt);		h
					pthread_mutex_unlock(&phone_mutex);	
				} else {
					strcpy(exit_message, "\nCredit card is not valid");
				}
			}
			else //periptwsh mh yparxhs apaitoumenwn 8eswn
			{
				strcpy(exit_message, "\nNo available seats for zone A"); //mhnyma pros pelati
				fail_counter++;	//ayxhsh metrhthapotyxhmenwn synallagwn
			}
		}
		else if(zone == 2)	//zone B
		{
			if( (Bseats - num) > 0) 
			{	
				
				price=num*40;
				credit_check=bank_service(cred_num, price);
				if(credit_check){
					pthread_mutex_lock(&phone_mutex);
					strcpy(exit_message, "\nReservation is successful.Reservatiion id is::");
					dif=130-Bseats;
					Bseats-=num;
					Totalseats-=num;
					sprintf(pelatis,"%s %d",pelatis,id);
					strcat(exit_message,pelatis);
					strcat(exit_message,seat);
					int i;
					for(i=0;i<num;i++)
					{
						planB[dif + i] = id;
						sprintf(text," %s%d,","B",dif+i);
						strcat(exit_message,text);
					}
					sprintf(pricetxt,"%s%d",pricetxt,price);
					strcat(exit_message,pricetxt);
					pthread_mutex_unlock(&phone_mutex);
				} else {
					strcpy(exit_message, "\nCredit card is not valid");
				}
			}
			else 
			{
				strcpy(exit_message, "\nNo available seats for zone B");
				fail_counter++;	
			}
		}
		else if(zone == 3) 		//zone C
		{
			if( (Cseats-num) > 0)
			{	
				price=num*35;
				credit_check=bank_service(cred_num, price);
				if(credit_check){
					pthread_mutex_lock(&phone_mutex);
					strcpy(exit_message, "\nReservation is successful.Reservatiion id is:");
					dif=180-Cseats;
					Cseats-=num;
					Totalseats-=num;
					sprintf(pelatis,"%s %d",pelatis,id);
					strcat(exit_message,pelatis);
					strcat(exit_message,seat);
					int i;
					for(i=0;i<num;i++)
					{
						planC[dif + i] = id;
						sprintf(text," %s%d,","C",dif+i);
						strcat(exit_message,text);
					}
					sprintf(pricetxt,"%s%d",pricetxt,price);
					strcat(exit_message,pricetxt);
					pthread_mutex_unlock(&phone_mutex);
				} else {
					strcpy(exit_message, "\nCredit crad not valid");
				}
			}
			else 
			{
				strcpy(exit_message, "\nNoavailable seats for zone C");
				fail_counter++;
			}
		}
		else if(zone == 4)		//zone D
		{
			if( (Dseats-num) > 0) 
			{
				price=num*30;
				credit_check=bank_service(cred_num, price);
				if(credit_check){
					pthread_mutex_lock(&phone_mutex);
					strcpy(exit_message, "\nReservation is successful.Reservatiion id is::");
					dif=230-Dseats;
					Dseats-=num;
					Totalseats-=num;
					sprintf(pelatis,"%s %d",pelatis,id);
					strcat(exit_message,pelatis);
					strcat(exit_message,seat);
					int i;
					for(i=0;i<num;i++)
					{
						planD[dif + i] = id;
						sprintf(text," %s%d,","D",dif+i);
						strcat(exit_message,text);
					}
					sprintf(pricetxt,"%s%d",pricetxt,price);
					strcat(exit_message,pricetxt);
					pthread_mutex_unlock(&phone_mutex);
				} else {
					strcpy(exit_message, "\nCredit card is not valid");
				}
			}
			else 
			{
				strcpy(exit_message, "\nNo available seats for zone D");
				fail_counter++;
			}
		}
			
		
	time(&service_time);
	
	pthread_mutex_lock(&phone_mutex);
	sum_service+= difftime(service_time,start_time);	
	pthread_mutex_unlock(&phone_mutex);	
	
	printf("\nExit message is : %s \n ",exit_message);
	write(*connfd, exit_message, sizeof(exit_message)) ; //send message to client
	close(*connfd);	
	
	pthread_exit(0);
	
}


int main( int argc, char **argv )
{
	int threads_number;      
	 int listenfd , connfd; /* Socket descriptors. */
	int i=0;       
	//pid_t childpid;
       socklen_t clilen;
       struct sockaddr_un cliaddr, servaddr; /* Structs for the client and server socket addresses. */
	
      // signal( SIGCHLD, sig_chld ); /* Avoid "zombie" process generation. */
	signal( SIGINT, quit_process);
	signal(SIGALRM, sigFunc);
	if(argc==2) threads_number=atoi(argv[1]);
	else	threads_number=1000;
	
	printf("\n Max thread number :%d \n",threads_number);


	pthread_t threadid[threads_number];

	//childpid = fork();

       listenfd = socket( AF_LOCAL, SOCK_STREAM, 0 ); /* Create the server's endpoint */
       
       /***************************SOCKET INIT**************************************************************/


	/* ATTENTION!!! THIS ACTUALLY REMOVES A FILE FROM YOUR HARD DRIVE!!! */
       unlink( UNIXSTR_PATH ); /* Remove any previous socket with the same filename. */

       bzero( &servaddr, sizeof( servaddr ) ); /* Zero all fields of servaddr. */
       servaddr.sun_family = AF_LOCAL; /* Socket type is local (Unix Domain). */
       strcpy( servaddr.sun_path, UNIXSTR_PATH ); /* Define the name of this socket. */

/* Create the file for the socket and register it as a socket. */
       bind( listenfd, ( struct sockaddr* ) &servaddr, sizeof( servaddr ) );

       listen( listenfd, LISTENQ ); /* Create request queue. */
	
	if(i==0)
	{
		alarm(30);
	}
	
	printf("\n**Waiting for connection... **\n");	

       for ( ; ; ) {
              clilen = sizeof( cliaddr );

/* Copy next request from the queue to connfd and remove it from the queue. */
              connfd = accept( listenfd, ( struct sockaddr * ) &cliaddr, &clilen );
	      printf("\n*Connection accepted*\n");
              if ( connfd < 0 ) {
                     if ( errno == EINTR ) /* Something interrupted us. */
                            continue; /* Back to for()... */
                     else {
                            fprintf( stderr, "Accept Error\n" );
                            exit( 0 );
                     }
              }
		
		int* sockfd = (int *)malloc(sizeof(int)); 
		*sockfd = connfd;
		if (i<threads_number) {
			if (pthread_create(&threadid[i], NULL, &phone_center, sockfd) != 0)
				fprintf(stderr,"Failed creating of thread for transaction: ");
			if (pthread_detach(threadid[i]) != 0)
				fprintf(stderr,"Failed creating thread for transaction: ");
			i++;
			counter++;
		} 
	}

}

void sigFunc(int sig)			 
{		
		transactions[trans_counter] = Bank_account; //store transaction 
		trans_counter++; //increment transactions
		Theater_account += Bank_account; //accumualate money ammount
		Bank_account = 0; 
		alarm(30) ;   /* renew alarm 30 seconds */
}

//*****************************************Synarthsh ektypwshs apotelesmatwn*******************
void printResults(void)
{
	system("clear");
	int i;
	printf("Zone A: ");
	for(i=0;i<100-Aseats;i++)
	{
		printf("P%d ",planA[i]);
	}
	printf("\n\n");
	printf("Zone B: ");
	for(i=0;i<130-Bseats;i++)
	{
		printf("P%d ",planB[i]);
	}
	printf("\n\n");
	printf("Zone C: ");
	for(i=0;i<180-Cseats;i++)
	{
		printf("P%d ",planC[i]);
	}
	printf("\n\n");
	printf("Zone D: ");
	for(i=0;i<230-Dseats;i++)
	{
		printf("P%d ",planD[i]);
	}
	printf("\n");
	float failed;	
	failed = fail_counter*100 / counter; //ypologismos posostou apotyxhmenwn synallagwn
	printf("\nPercentage of failed transactions : %f \n",failed);
	
	double average_delay;
	average_delay = sum_delay / counter ;//ypologismos mesou xronou ka8usterhshs
	printf("Average delay time: %f seconds\n",average_delay);
	double average_service;
	average_service = sum_service / counter ;//ypologismos mesou xronou exyphrethshs
	printf("Average service time: %f seconds\n",average_service);
	printf("Transactions: ");
	for(i=0;i<trans_counter;i++)
	{
		printf(" %d |",transactions[i]);
	}
	printf("\nTotal ammount of money: %d \n",Theater_account+Bank_account);
}

//***************************************Synarthsh termatismou************************************

void quit_process(void)		
{	
	printResults();
	unlink( UNIXSTR_PATH ); /* delete socket in UNIXSTR_PATH */
	sem_unlink( SEM_NAME ); /* delete semaphore */
	exit(0);
}
