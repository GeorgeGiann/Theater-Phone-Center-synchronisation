# Theater-Phone-Center-synchronisation

A phone center of a theater is receiving calls for reservations. Suppose there k employees and every call takes t1 time. 
Also customers are able to buy tickets with credit card via a bank terminal. Suppose there are n bank terminals and every bank trade takes t2 time. Also the theater has 4 zones seats with different amount of seats and different price for every zone.
The requested are:
The synchroniztion of the proccesses of the phone center and the bank terminals.
The calculation of the amount of money of the theater account.
The calculation of :
	the ratio of successful trades (Unsuccessful trades are supposed those trades: 
						1)with unavailable requested seats
						2)with not valid credit card number (10%))
	the average wait time
	the average service time
  
  Project contents:
  
  folder: /fork
  Files: server.c, client.c, makefile, run_test.sh
  Description: Fork and semaphores implementaion.
  Compile: run make command
  Run:  server ./server
        client ./client args[] or run_test.sh
        
  folder: /threads
  Files: server.c, client.c, makefile, run_test.sh
  Description: Thraeds and mutex implementation
  Compile: run make command
  Run:  server ./server
        client ./client args[] or run_test.sh
  
