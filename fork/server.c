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


/* Size of the request queue. */
#define LISTENQ 20
#define SEM_NAME "4355"
#define MAX_CALLS 1000

struct epiloges{         	 //structure forclient choices
	int id;		 	 
	int seatsnumber; 	 
	int zone;	 	 //zone number (1=A, 2=B, 3=C, 4=D)
	int credit_card;
};

struct seats_counters{		//management seats structure
	int Aseats ; 		//zone a counter
	int Bseats ;	 	//zone b counter
	int Cseats ;		//zone c counter
	int Dseats ;		//zone d counter
	int total ;		//total seats counter
	int planA[100];		//seat - client mapping array for zone a
	int planB[130];		//seat - client mapping array for zone b
	int planC[180];		//seat - client mapping array for zone c
	int planD[230];		//seat - client mapping array for zone d
};

struct center{			//phone center vars structure
	int Phoners;		//number of phoners
	int Terminals;		//number of terminals
	int fail_counter;	//failed transactioms counter
	int counter;		//transactioms counter
	double sum_delay;	//delay total time
	double sum_service;	//service total time
};

struct accounts{		//structure for account management
	int Theater_account;	
	int Bank_account;	
	int trans_counter;	//transactions counter
	int transactions[];	//transcaction array
};

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
//Functions Declaration
void phone_center(int num, int zone , int cred_num, int id); 	//Phone service functiom
int bank_service(int credit_card_num, int price);		//Bank terminal funstion
void quit_process(void);					//Quit procces function
void sigFunc(int sig);						//alarm function
void printResults(void);					//Print function
					
//Var declaration
struct seats_counters *seats_Array; 		//Array of structure for seat management
struct epiloges *epiloges_Array;		//Array of structure for customer choices
struct center *center_Array;			//Array of structure of useful variables for the phone center
struct accounts *account_Array;			//Array of structure for account management
int shmid,shmid2,shmid3,shmid4;
sem_t *my_sem;

//String var
char exit_message[150];
char pelatis[]=" customer";
char seat[]=", your seats: ";
char text[5];
char pricetxt[]="the cost of the transaction is: ";

int main( int argc, char **argv )
{
       int listenfd, connfd; /* Socket descriptors. */
       pid_t childpid;
       socklen_t clilen;
       struct sockaddr_un cliaddr, servaddr; /* Structs for the client and server socket addresses. */
	struct epiloges choices;
       signal( SIGCHLD, sig_chld ); /* Avoid "zombie" process generation. */
	signal( SIGINT, quit_process);
	signal(SIGALRM, sigFunc);

       listenfd = socket( AF_LOCAL, SOCK_STREAM, 0 ); /* Create the server's endpoint */

//**********************************SHARED_MEMORY*****************************************************//
	//initialisation
	key_t key;
	key = 9999;

	shmid = shmget(key, sizeof(seats_Array), IPC_CREAT | 0666);
	seats_Array = shmat(shmid, NULL, 0);

	seats_Array->Aseats = 100; 
	seats_Array->Bseats = 130;
	seats_Array->Cseats = 180;
	seats_Array->Dseats = 230;
	seats_Array->total = 640;

	key_t shm_key;
	shm_key = 4567;

	shmid2 = shmget(shm_key, MAX_CALLS*sizeof(epiloges_Array),IPC_CREAT | 0666);
	epiloges_Array = shmat(shmid2, NULL, 0);

	epiloges_Array->id=1;

	key_t shm_key2;
	shm_key2 = 1111;
	shmid3 = shmget(shm_key2, sizeof(center_Array),IPC_CREAT | 0666);
	center_Array = shmat(shmid3, NULL, 0);

	center_Array->Phoners = 10;	
	center_Array->Terminals = 4;
	center_Array->fail_counter = 0;
	center_Array->counter = 0;
	center_Array->sum_delay = 0;
	center_Array->sum_service = 0;

	key_t shm_key3;
	shm_key3 = 6780;
	shmid4 = shmget(shm_key3, sizeof(account_Array),IPC_CREAT | 0666);
	account_Array = shmat(shmid4, NULL, 0);

	account_Array->Theater_account = 0; 
	account_Array->Bank_account = 0;
	account_Array->trans_counter = 0;

//******************Semaphore init *****************************************************************

	my_sem = sem_open(SEM_NAME, O_CREAT | O_RDWR,S_IRUSR | S_IWUSR, 1);
	if (my_sem == SEM_FAILED)
	exit(1);

//***************************SOCKET INIT**************************************************************


/* ATTENTION!!! THIS ACTUALLY REMOVES A FILE FROM YOUR HARD DRIVE!!! */
       unlink( UNIXSTR_PATH ); /* Remove any previous socket with the same filename. */

       bzero( &servaddr, sizeof( servaddr ) ); /* Zero all fields of servaddr. */
       servaddr.sun_family = AF_LOCAL; /* Socket type is local (Unix Domain). */
       strcpy( servaddr.sun_path, UNIXSTR_PATH ); /* Define the name of this socket. */

/* Create the file for the socket and register it as a socket. */
       bind( listenfd, ( struct sockaddr* ) &servaddr, sizeof( servaddr ) );

       listen( listenfd, LISTENQ ); /* Create request queue. */
	
	alarm(30);
	
       for ( ; ; ) {
              clilen = sizeof( cliaddr );

/* Copy next request from the queue to connfd and remove it from the queue. */
              connfd = accept( listenfd, ( struct sockaddr * ) &cliaddr, &clilen );

              if ( connfd < 0 ) {
                     if ( errno == EINTR ) /* Something interrupted us. */
                            continue; /* Back to for()... */
                     else {
                            fprintf( stderr, "Accept Error\n" );
                            exit( 0 );
                     }
              }
//*********************************** FORK *******************************************************


              childpid = fork(); /* Spawn a child. */

  if ( childpid == 0 ) 
	{
			int id;		
		
				 /* Child process. */
                    // close( listenfd ); /* Close listening socket. */
                    read( connfd, &choices, sizeof(choices) );  //diavasma mhnymatos client
			
			sem_wait(my_sem);//pause proccesses in order to store client choices in the shared mem
				
			id=epiloges_Array->id++; 				//Increment customer id
			epiloges_Array->seatsnumber = choices.seatsnumber;
			epiloges_Array->zone = choices.zone;
			epiloges_Array->credit_card = choices.credit_card;
			epiloges_Array++;					

			
			sem_post(my_sem);//Continue proccesses
			
			if( (seats_Array->total - choices.seatsnumber) > 0) //check for sold-out
			{
				phone_center(choices.seatsnumber, choices.zone, choices.credit_card, id); 
				
			}
			else 
			{

				strcpy(exit_message, "\nNo seats available"); 
				center_Array->fail_counter++;			//count failed transactions
				
			}
			printf("%s",exit_message);
			
			write(connfd, exit_message, sizeof(exit_message)); //send client message
			
                            
		
                            exit( 0 ); /* Terminate child process. */
          }//end_child_process

              close(connfd); /* Parent closes connected socket */
       }//end_for(;;)
}


//********************Phone Center function*****************************

void phone_center(int num , int zone , int cred_num, int id)
{	
	int flag = 1 ; 
	int price,dif;
	int credit_check;
	time_t start_time;	//time vars
	time_t delay_time;
	time_t service_time;
	time(&start_time);	//time of the connection with phone center
	
	
	sem_wait(my_sem);	//pause procceses
	center_Array->counter++;	//increment transactions counter
	if(center_Array->Phoners>0)	//find available phone employee
	{					
		
		center_Array->Phoners--;	//decrement available phoners
		time(&delay_time);		//delay_time time client attached phoner
		center_Array->sum_delay += difftime(delay_time,start_time); 
		sem_post(my_sem); //continue
		sleep(6);	//tseatfind=6 
		center_Array->Phoners++; //increment available phoners
		
		
			if(zone == 1) {					//check for zone A
				if( (seats_Array->Aseats - num) > 0) 	//check for available seats
				{
					price=num*50;			//calculate cost
					credit_check=bank_service(cred_num, price);	
					if(credit_check){				
						sem_wait(my_sem);		//pause
						dif=100-seats_Array->Aseats;	//dif = number of taken seats
						seats_Array->Aseats-=num;	//decrement available seats
						seats_Array->total-=num;	//decrement total seats			
						sprintf(pelatis,"%s %d",pelatis,id); 
						strcat(exit_message,pelatis);
						strcat(exit_message,seat);
						int i;
						for(i=0;i<num;i++)
						{	
							*(seats_Array->planA + dif + i) = id; //set customer id on the taken seat
							sprintf(text," %s%d,","A",dif+i);
							strcat(exit_message,text);
						}
						sprintf(pricetxt,"%s%d",pricetxt,price);
						strcat(exit_message,pricetxt);		
						sem_post(my_sem);	
					}
				}
				else //no available seats
				{
					strcpy(exit_message, "\nNo available seats for zone A"); 
					center_Array->fail_counter++;	//failed transactions
				}
			}
			else if(zone == 2)	//zone B
			{
				if( (seats_Array->Bseats - num) > 0) 
				{	
					
					price=num*40;
					credit_check=bank_service(cred_num, price);
					if(credit_check){
						sem_wait(my_sem);
						dif=130-seats_Array->Bseats;
						seats_Array->Bseats-=num;
						seats_Array->total-=num;
						sprintf(pelatis,"%s %d",pelatis,id);
						strcat(exit_message,pelatis);
						strcat(exit_message,seat);
						int i;
						for(i=0;i<num;i++)
						{
							*(seats_Array->planB + dif + i) = id;
							sprintf(text," %s%d,","B",dif+i);
							strcat(exit_message,text);
						}
						sprintf(pricetxt,"%s%d",pricetxt,price);
						strcat(exit_message,pricetxt);
						sem_post(my_sem);
					}
				}
				else 
				{
					strcpy(exit_message, "\nNo available seats for zone B");
					center_Array->fail_counter++;	
				}
			}
			else if(zone == 3) 		//zone C
			{
				if( (seats_Array->Cseats-num) > 0)
				{	
					price=num*35;
					credit_check=bank_service(cred_num, price);
					if(credit_check){
						sem_wait(my_sem);
						dif=180-seats_Array->Cseats;
						seats_Array->Cseats-=num;
						seats_Array->total-=num;
						sprintf(pelatis,"%s %d",pelatis,id);
						strcat(exit_message,pelatis);
						strcat(exit_message,seat);
						int i;
						for(i=0;i<num;i++)
						{
							*(seats_Array->planC + dif + i) = id;
							sprintf(text," %s%d,","C",dif+i);
							strcat(exit_message,text);
						}
						sprintf(pricetxt,"%s%d",pricetxt,price);
						strcat(exit_message,pricetxt);
						sem_post(my_sem);
					}
				}
				else 
				{
					strcpy(exit_message, "\nNo availablem seats for zone C");
					center_Array->fail_counter++;
				}
			}
			else if(zone == 4)		//periptwsh epiloghs pelati zonh D
			{
				if( (seats_Array->Dseats-num) > 0) 
				{
					price=num*30;
					credit_check=bank_service(cred_num, price);
					if(credit_check){
						sem_wait(my_sem);
						dif=230-seats_Array->Dseats;
						seats_Array->Dseats-=num;
						seats_Array->total-=num;
						sprintf(pelatis,"%s %d",pelatis,id);
						strcat(exit_message,pelatis);
						strcat(exit_message,seat);
						int i;
						for(i=0;i<num;i++)
						{
							*(seats_Array->planD + dif + i) = id;
							sprintf(text," %s%d,","D",dif+i);
							strcat(exit_message,text);
						}
						sprintf(pricetxt,"%s%d",pricetxt,price);
						strcat(exit_message,pricetxt);
						sem_post(my_sem);
					}
				}
				else 
				{
					strcpy(exit_message, "\nNo available seats for zone D");
					center_Array->fail_counter++;
				}
			}
			
		
		
	}
	else{		//no available phoner
		do{	
			if(center_Array->Phoners>0) flag=0;
		}while(flag); //keep searching for phoner
		center_Array->Phoners--; 
		time(&delay_time);	
		center_Array->sum_delay += difftime(delay_time,start_time);	
		sem_post(my_sem); 
		sleep(6);		//tseatfind = 6
		center_Array->Phoners++; 
			
			if(zone == 1) {		//elegxos zonis epiloghs pelati zoni A
				if( (seats_Array->Aseats - num) > 0) 
				{
					price = num*50; //calculate cost
					credit_check=bank_service(cred_num, price); 
					if(credit_check){		
						sem_wait(my_sem);	
						dif=100-seats_Array->Aseats;	
						seats_Array->Aseats-=num;	
						seats_Array->total-=num;	
						sprintf(pelatis,"%s %d",pelatis,id);	
						strcat(exit_message,pelatis);
						strcat(exit_message,seat);
						int i;
						for(i=0;i<num;i++)
						{
							*(seats_Array->planA + dif + i) = id;	
							sprintf(text," %s%d,","A",dif+i);
							strcat(exit_message,text);
						}
						sprintf(pricetxt,"%s%d",pricetxt,price);
						strcat(exit_message,pricetxt);		
						sem_post(my_sem); 			
					}
					
				}
				else
				{
					 strcpy(exit_message, "\nNo available seats for zone A");
					center_Array->fail_counter++;	 
				}
			}
			else if(zone == 2)		//zone B
			{
				if( (seats_Array->Bseats - num) > 0) 
				{
					price = num*40;
					credit_check=bank_service(cred_num, price);
					if(credit_check){
						sem_wait(my_sem);
						dif=130-seats_Array->Bseats;
						seats_Array->Bseats-=num;
						seats_Array->total-=num;
						sprintf(pelatis,"%s %d",pelatis,id);
						strcat(exit_message,pelatis);
						strcat(exit_message,seat);						
						int i;
						for(i=0;i<num;i++)
						{	
							*(seats_Array->planB + dif + i) = id;
							sprintf(text," %s%d,","B",dif+i);
							strcat(exit_message,text);
						}
						sprintf(pricetxt,"%s%d",pricetxt,price);
						strcat(exit_message,pricetxt);
						sem_post(my_sem);
					}
				}
				else 
				{
					strcpy(exit_message, "\nNo available seats for zone B");
					center_Array->fail_counter++;
				}
			}
			else if(zone == 3)		//zone C
			{
				if( (seats_Array->Cseats-num) > 0)
				{
					price = num*35;
					credit_check=bank_service(cred_num, price);
					if(credit_check){
						sem_wait(my_sem);
						dif=180-seats_Array->Cseats;
						seats_Array->Cseats-=num;
						seats_Array->total-=num;
						sprintf(pelatis,"%s %d",pelatis,id);
						strcat(exit_message,pelatis);
						strcat(exit_message,seat);
						int i;
						for(i=0;i<num;i++)
						{
							*(seats_Array->planC + dif + i) = id;
							sprintf(text," %s%d,","C",dif+i);
							strcat(exit_message,text);
						}
						sprintf(pricetxt,"%s%d",pricetxt,price);
						strcat(exit_message,pricetxt);
						sem_post(my_sem);
					}
				}
				else 
				{
					strcpy(exit_message, "\nNo available seats for zone C");
					center_Array->fail_counter++;
				}
			}
			else if(zone ==4)		//zone D
			{
				if( (seats_Array->Dseats-num) > 0) 
				{
					price = num*30;
					credit_check=bank_service(cred_num, price);
					if(credit_check){
						sem_wait(my_sem);
						dif=230-seats_Array->Dseats;
						seats_Array->Dseats-=num;
						seats_Array->total-=num;
						sprintf(pelatis," %s %d",pelatis,id);
						strcat(exit_message,pelatis);
						strcat(exit_message,seat);
						int i;
						for(i=0;i<num;i++)
						{
							*(seats_Array->planD + dif + i) = id;
							sprintf(text," %s%d ","D",dif+i);
							strcat(exit_message,text);
						}
						sprintf(pricetxt,"%s%d",pricetxt,price);
						strcat(exit_message,pricetxt);
						sem_post(my_sem);
					}
				}
				else
				{
					 strcpy(exit_message, "\nNo available seats for zone D");
					 center_Array->fail_counter++;
				}
			}	
		
	}
	time(&service_time);//end of service time
	
	center_Array->sum_service+= difftime(service_time,start_time);	
	
	
}

//*********************************Bank terminal function.**************************
int bank_service(int credit_card_num, int price)
{	
	int random_num,a;		
	int flag=1;
	srand(credit_card_num); 
	random_num = rand() % 10 + 1; 
	sem_wait(my_sem); //pause
	if(center_Array->Terminals >0)	// check for available terminal
	{
		center_Array->Terminals--; //decrement available terminals
		if(random_num==1)	//credit card number is not valid if random num is 1
		{
			strcpy(exit_message, "\nNot valid credit card");
			center_Array->fail_counter++; 
			a=0;	.
		}
		else		
		{
			account_Array->Bank_account += price;	//add cost to theater account		
			strcpy(exit_message, "\nSuccessful reservation. Reservation ID is:");
			a=1; 
		}
		sem_post(my_sem);	
		sleep(4);		//t card check = 4
		center_Array->Terminals++;	
	}
	else //no available terminals
	{
		do{
			if(center_Array->Terminals >0) flag=0;
		}while(flag);	// keep checking for available terminal
		center_Array->Terminals--;	
		if(random_num==1)	
		{
			strcpy(exit_message, "\nNot valid credit card");
			center_Array->fail_counter++;	
			a=0;	
		}	
		else	
		{	
			account_Array->Bank_account += price; //ayxhsh logariamsou ths etairias stn trapeza me to kostos
			strcpy(exit_message,"\nSuccessful reservation. Reservation ID is:");	
			a=1; 
		}
		sem_post(my_sem);	 
		sleep(4);		
		center_Array->Terminals++;	
	}
	return a;
}

//*********************** ALARM *************************************************

void sigFunc(int sig)			 
{		
		*(account_Array->transactions + account_Array->trans_counter) = account_Array->Bank_account; //store transaction in the array
		account_Array->trans_counter++; 
		account_Array->Theater_account += account_Array->Bank_account;	//summarise theater account
		account_Array->Bank_account = 0; 
		alarm(30) ;   /* Renew alarm 30 seconds */
}

//*****************************************Print function*******************
void printResults(void)
{
	system("clear");
	int i;
	printf("Zone A: ");
	for(i=0;i<100-seats_Array->Aseats;i++)
	{
		printf("P%d ",seats_Array->planA[i]);
	}
	printf("\n");
	printf("Zone B: ");
	for(i=0;i<130-seats_Array->Bseats;i++)
	{
		printf("P%d ",seats_Array->planB[i]);
	}
	printf("\n");
	printf("Zone C: ");
	for(i=0;i<180-seats_Array->Cseats;i++)
	{
		printf("P%d ",seats_Array->planC[i]);
	}
	printf("\n");
	printf("Zone D: ");
	for(i=0;i<230-seats_Array->Dseats;i++)
	{
		printf("P%d ",seats_Array->planD[i]);
	}
	printf("\n");
	float failed;	
	failed = center_Array->fail_counter*100 / center_Array->counter; //ypologismos posostou apotyxhmenwn synallagwn
	printf("Successful transactions ratio: %f % \n",failed);
	
	double average_delay;
	average_delay = center_Array->sum_delay / center_Array->counter ;//ypologismos mesou xronou ka8usterhshs
	printf("Average waiting time: %f seconds\n",average_delay);
	double average_service;
	average_service = center_Array->sum_service / center_Array->counter ;//ypologismos mesou xronou exyphrethshs
	printf("Average service time: %f seconds\n",average_service);
	printf("transactiom Array: ");
	for(i=0;i<account_Array->trans_counter;i++)
	{
		printf(" %d |",account_Array->transactions[i]);
	}
	printf("\nFinal amount of money: %d \n",account_Array->Theater_account+account_Array->Bank_account);
}

//***************************************Quit function************************************

void quit_process(void)		
{	
	printResults();
	unlink( UNIXSTR_PATH ); /* delete socket in UNIXSTR_PATH */
	sem_unlink( SEM_NAME ); /* delete sem */
	shmdt(seats_Array); 			/* Detach from shared mem */
	shmctl(shmid, IPC_RMID, NULL); 		/* Delete from shared mem */
	shmdt(epiloges_Array); 			/* Detach from shared mem  */
	shmctl(shmid2, IPC_RMID, NULL); 	/* Delete from shared mem*/
	shmdt(center_Array); 			/* Detach from shared mem  */
	shmctl(shmid3, IPC_RMID, NULL); 	/* Delete from shared mem*/
	shmdt(account_Array);			/* Detach from shared mem  */
	shmctl(shmid4, IPC_RMID, NULL);		/* Delete from shared mem */
	exit(0);
}
