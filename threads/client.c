#include "MyHeader.h" /* for user-defined constants */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h> /* basic socket definitions */
#include <sys/types.h> /* basic system data types */
#include <sys/un.h> /* for Unix domain sockets */

struct epiloges{		//client choices
	int id;			
	int seatsnumber;	
	int zone;			
	int credit_card;
};

int main( int argc, char **argv )
{
        int sockfd;
        struct sockaddr_un servaddr; /* Struct for the server socket address. */
        struct epiloges choices;
	char menu_text1[] = "\nHow many tickets do you want(max 4)";	//dilwsh mhnymatwn gia to menu
	char menu_text2[] = "\nPreferred zone\n1 for A \n2 for B \n3 for C \n4 for D)";
	char menu_text3[] = "\nGive credit card number:";
	char buff_in[150] ;
	int flag=1;

       

       sockfd = socket( AF_LOCAL, SOCK_STREAM, 0 ); /* Create the client's endpoint. */

       bzero( &servaddr, sizeof( servaddr ) ); /* Zero all fields of servaddr. */
       servaddr.sun_family = AF_LOCAL; /* Socket type is local (Unix Domain). */
       strcpy( servaddr.sun_path, UNIXSTR_PATH ); /* Define the name of this socket. */

/* Connect the client's and the server's endpoint. */
       connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
//********************************************************************************************

	if(argc<=1)	//if 1 argument is given in command line (./client)
	{	//Show MENU
		system("clear");
		printf("%s",menu_text1);
		do{
			scanf("%d",&choices.seatsnumber);
			if(choices.seatsnumber>=1 && choices.seatsnumber<=4) flag=0; 
			else{
				printf("Lathos epilogh");
				sleep(1);
				system("clear");
				printf("%s",menu_text1);
			}
		}while(flag);

		system("clear");
		flag=1;
		
		printf("%s",menu_text2);
		do{
			scanf("%d",&choices.zone);
			if(choices.zone>=1 && choices.zone<=4) flag=0;	
			else{
				printf("Wrong choice");
				sleep(1);
				system("clear");
				printf("%s",menu_text2);
			}
		}while(flag);

		system("clear");
		
		
		printf("%s",menu_text3);
		scanf("%d",&choices.credit_card);
			
		system("clear");
		
	}
	else if(argc==2) //2 arguments (p.x. ./client a)
	{	
		int random_num;
		srand((unsigned int)getpid());
		choices.seatsnumber = rand() % 4 + 1;
		//random choices
		random_num = rand() % 10 + 1;
		if(random_num==1) choices.zone = 1;
		else if(random_num==2 || random_num==3) choices.zone = 2;
		else if(random_num>=4 && random_num<=6) choices.zone = 3;
		else choices.zone = 4;
		
		choices.credit_card= rand() % 99999 + 10000;
		
	}
	else if(argc==4) //3 argumetns ./client num_of_seats zone credit_card_num
		choices.seatsnumber = atoi(argv[1]);
		choices.zone = atoi(argv[2]);
		choices.credit_card = atoi(argv[3]);
	}


	write( sockfd, &choices , sizeof( choices ) ); //send choices to  server
	read( sockfd, buff_in, sizeof(buff_in));	//read server
	printf("%s \n",buff_in);
       close( sockfd );

	
}
