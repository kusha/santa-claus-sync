/*
FILE:           santa.c
AUTHOR:         Mark Birger, FIT VUTBR
DATE:           10.4.2013 - 30.4.2013
COMPILER:       GCC 4.5.4
DESCRIPTION:    Santa Claus Problem
*/

//LIBRARIES
#include <stdlib.h>     /* exit_success */
#include <stdio.h>      /* printf, stderr, fprintf */
#include <sys/types.h>  /* key_t ... */
#include <sys/shm.h>    /* shared memory */
#include <unistd.h>     /* fork, _Exit, usleep*/
#include <sys/wait.h>   /* wait */
#include <semaphore.h>  /* semget(), semctl(), semop()  */
#include <fcntl.h>      /* O_CREAT, O_EXCL */
#include <time.h>       /* random seed */

//MACROS

//macro for closing POSIX semaphore
//inside each process
//close each used semaphore:
#define CLOSE_SEM \
	do { \
		sem_close(out_sem); \
		sem_close(sem_request); \
		sem_close(sem_queue); \
		sem_close(sem_helper); \
		sem_close(sem_unlock); \
		sem_close(sem_term); \
	} while (0)                     //macro container

//macro for closing all
//resources inside main process
//close output file
//delete shared memory
//close all semaphores above
//and unlink (once) each of them:
#define CLOSE_ALL \
	do { \
		fclose(output); \
		shmctl(shmid, IPC_RMID, NULL); \
		CLOSE_SEM; \
		sem_unlink("/out_sem"); \
		sem_unlink("/sem_request"); \
		sem_unlink("/sem_queue"); \
		sem_unlink("/sem_helper"); \
		sem_unlink("/sem_unlock"); \
		sem_unlink("/sem_term"); \
	} while (0)                             //macro container

//STRUCTURES
struct shared_data {            //structure with all shared data
	unsigned int active_elfs;   //number of active elfs
	unsigned int out_num;       //and output line number
};

//PROTOTYPES
int gen_random (int limit); //generate random at interval function

//CODE
int main (int argc, char *argv[]) {

	//COMMON VARIABLES
	int i;  //counter

	//READ INPUT DATA
	if (argc!=5) {                                          //there is should be only 5(4) arguments
		fprintf(stderr, "Wrong number of arguments.\n");    //print error if it's not true
		return 1;                                           //and finish with exit code 1
	}
	char * convert_end[4];                                      //this end of arguments array for strtol
	int vacation_num = strtol(argv[1], &convert_end[0], 10);    //get number of toys to vacation
	int elf_num = strtol(argv[2], &convert_end[1], 10);         //elf number
	int toy_time = strtol(argv[3], &convert_end[2], 10);        //and time of building one toy
	int support_time = strtol(argv[4], &convert_end[3], 10);    //and santa's help time
	for (i = 0; i < 4; ++i) {                                   //if one of str->int transfomation
		if (*convert_end[i]) {                                  //have a suffix
			fprintf(stderr, "Argument is not a number.\n");     //there is inout error
			return 1;                                           //let's exit with code 1
		}
	}
	if ((!(vacation_num>0))||(!(elf_num>0))||(!(toy_time>=0))||(!(support_time>=0))){   //if data is integer, but not in interval
		fprintf(stderr, "Argument number isn't valid.\n");                              //also error
		return 1;                                                                       //and exit 1
	}

	//OPEN OUTPUT FILE
	FILE *output;                                       //open common file to output
	if ((output = fopen("santa.out", "w")) == 0) {      //open file 
		fprintf(stderr, "Can't open output file\n");    //and if error occurred
		return 2;                                       //exit with code 2 (system func error)
	}
	setbuf(output, NULL);   //turn data buffer off - each operation should be writed to disk

	//SHARED MEMORY
	key_t key;  //key variable
	int shmid;  //shared memory id
	struct shared_data *common;                                                     //sturucture with shared data
	if ((key = ftok(argv[0], 1)) == -1) {                                           //try to generate key
		fprintf(stderr, "Can't generate key for shared memory.\n");                 //if error - print a messsage
		exit(2);                                                                    //and exit with code 2
	}
	if ((shmid = shmget(key, sizeof(struct shared_data), IPC_CREAT | 0666)) < 0) {  //try to create sh memory with size of struct
		fprintf(stderr, "Can't shmget() shared memory.\n");                         //print error message
		exit(2);                                                                    //exit 2
	}
	if ((common = shmat(shmid, NULL, 0)) == (struct shared_data *) -1) {    //and use this shared mem thru 'common'
		shmctl(shmid, IPC_RMID, NULL);                                      //close shared memory if error occured
		fprintf(stderr, "Can't shmat() shared memory.\n");                  //and print msg
		exit(2);                                                            //exit 2
	}

	//SEMAPHORES
	sem_t *out_sem = sem_open("/out_sem", O_CREAT | O_EXCL, 0666, 1);           //semaphore for file writing
	sem_t *sem_request = sem_open("/sem_request", O_CREAT | O_EXCL, 0666, 0);   //semaphore wich allow elf to get request
	sem_t *sem_queue = sem_open("/sem_queue", O_CREAT | O_EXCL, 0666, 0);       //main semaphore with queue of elfs
	sem_t *sem_helper = sem_open("/sem_helper", O_CREAT | O_EXCL, 0666, 0);     //helper semaphore unlock elfs after santa's help
	sem_t *sem_unlock = sem_open("/sem_unlock", O_CREAT | O_EXCL, 0666, 0);     //semaphore wich control santa after changing sh. mem
	sem_t *sem_term = sem_open("/sem_term", O_CREAT | O_EXCL, 0666, 0);         //semaphore to terminate process (except active waiting)

	//START VALUES
	(*common).active_elfs = elf_num;    //put start data
	(*common).out_num = 0;              //to shared memory

	//FORK PROCESSES
	pid_t pid;                                  //fork result variable
	for (i = 1; i <= elf_num+1; ++i) {          //for each elf and santa
		pid = fork();                           //fork main process
		if (pid) {                              //if we have pid
			continue;                           //we are in main - continue
		} else if (pid == 0) {                  //if pid = 0 we are in child
			if (i != elf_num+1) {               //if it's not last member
				//THERE IS ELF
				srand(time(NULL));              //generate random seed

				sem_wait(out_sem);              //block output semaphore
				++(*common).out_num;            //move up counter
				fprintf(output,"%d: elf: %d: started\n",(*common).out_num,i);
				sem_post(out_sem);              //unlock output semaphore

				int worked = 0;

				while (1) {                             //in cycle (break exit)
					usleep(gen_random(toy_time)*1000);  //build a toy (microsek.)

					sem_wait(out_sem);                  //block output semaphore
					++(*common).out_num;                //move up counter
					fprintf(output,"%d: elf: %d: needed help\n",(*common).out_num,i);
					sem_post(out_sem);                  //unlock output semaphore

					sem_wait(sem_request);              //wait to santa's ready
					sem_post(sem_queue);                //send request to queue

					sem_wait(out_sem);                  //block output semaphore
					++(*common).out_num;                //move up counter
					fprintf(output,"%d: elf: %d: asked for help\n",(*common).out_num,i);
					sem_post(out_sem);                  //unlock output semaphore

					sem_wait(sem_helper);               //wait until santa's help

					sem_wait(out_sem);                  //block output semaphore
					++(*common).out_num;                //move up counter
					fprintf(output,"%d: elf: %d: got help\n",(*common).out_num,i);
					sem_post(out_sem);                  //unlock output semaphore

					++worked;                           //another one toy is ready

					if (worked == vacation_num) {       //if elf done his job
						break;                          //break cycle
					}

					sem_post(sem_unlock);               //elf checked status - santa can continue

				}

				--(*common).active_elfs;        //decrment number of active elfs

				sem_post(sem_unlock);           //and unlock santa after this

				sem_wait(out_sem);              //block output semaphore
				++(*common).out_num;            //move up counter
				fprintf(output,"%d: elf: %d: got a vacation\n",(*common).out_num,i);
				sem_post(out_sem);              //unlock output semaphore

				//WAITING
				sem_wait(sem_term);             //wait for exit semaphore
				sem_post(sem_term);             //say to other elf, about end

				sem_wait(out_sem);              //block output semaphore
				++(*common).out_num;            //move up counter
				fprintf(output,"%d: elf: %d: finished\n",(*common).out_num,i);
				sem_post(out_sem);              //unlock output semaphore

				CLOSE_SEM;                      //close all semaphores
				_Exit(0);                       //exit from process
			} else {
				//THERE IS SANTA
				srand(time(NULL));              //generate random seed

				sem_wait(out_sem);              //block output semaphore
				++(*common).out_num;            //move up counter
				fprintf(output,"%d: santa: started\n",(*common).out_num);
				sem_post(out_sem);              //unlock output semaphore

				int queue_length;				//number of elf in queue var

				while ((*common).active_elfs != 0) {	//while there is active elfs
					if ((*common).active_elfs>3) {		//and more than 3
						queue_length = 3;				//queue length is 3 elfs
					} else {							//else
						queue_length = 1;				//one by one
					}
					for (i = 1; i <= queue_length; ++i) {	//for each elf in queue
						sem_post(sem_request);				//santa ready to request
						sem_wait(sem_queue);				//get request
						/* forum discussion - ouput is similar to example */
						if (i==queue_length) {				//if it'is last request
							sem_wait(out_sem);              //block output semaphore
							++(*common).out_num;            //move up counter
							fprintf(output,"%d: santa: checked state: %d: %d\n",(*common).out_num, (*common).active_elfs, i);
							sem_post(out_sem);              //unlock output semaphore
						}
					}

					sem_wait(out_sem);              //block output semaphore
					++(*common).out_num;            //move up counter
					fprintf(output,"%d: santa: can help\n",(*common).out_num);
					sem_post(out_sem);              //unlock output semaphore

					usleep(gen_random(support_time)*1000);	//help to elfs
					for (i = 1; i <= queue_length; ++i) {	//and for each elf
						sem_post(sem_helper);				//say that helped
					}

					for (i = 1; i <= queue_length; ++i) {	//and wait each answer about
						sem_wait(sem_unlock);				//shared memory chnging
					}

					i = 0;	//i is zero for last ouput
				   
				}

				sem_wait(out_sem);              //block output semaphore
				++(*common).out_num;            //move up counter
				fprintf(output,"%d: santa: checked state: %d: %d\n",(*common).out_num, (*common).active_elfs, i);
				sem_post(out_sem);              //unlock output semaphore

				sem_post(sem_term);				//send terminate signal string

				sem_wait(out_sem);              //block output semaphore
				++(*common).out_num;            //move up counter
				fprintf(output,"%d: santa: finished\n",(*common).out_num);
				sem_post(out_sem);              //unlock output semaphore

				CLOSE_SEM;		//close semaphores
				_Exit(0);		//and exit ok
			}
			break;	//stop cycle
		} else {
			CLOSE_ALL;									//free resources
			fprintf(stderr, "Fork error occurred.\n");	//say about fork error
			exit(2);									//and exit with code 2
		}
	}

	for (i = 0; i <= elf_num; ++i) {	//for each child
		wait(NULL);						//main process is waiting
	}

	//FREE RESOURCES
	CLOSE_ALL;

	return 0;
}

//FUNCTIONS
int gen_random (int limit) {			//get random [0,limit]
	int divisor = RAND_MAX/(limit+1);	//get a divisor
	int res;							//result variable
	do { 
		res = rand() / divisor;	//get smaller number
	} while (res > limit);		//while result is too huge
	return res;					//reuturn result
}

