#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>

/*
@author xponch0x
@version 4-7-2025
@description: C PROGRAM THAT USES FORK AND PIPE TO PROCCESS DATA FILES FOR UNIX/LINUX SYSTEMS; AND TO TEST EXECUTION TIMES
*/

//STRUCT TO HANDLE SINGLE CHILD PROCESS CASE
typedef struct{
	int fds[2];
	pid_t pid;
	int sum;
} SingleChild;

struct timeval start, end;

int main(){
	int num_child, file_choice;
	const char *filename;
	int total_lines;
	
	//GET INPUT FOR HOW MANY CHILDREN
	printf("ENTER NUMBER OF CHILDREN PROCESSES [1, 2, 4] >> ");
	scanf("%d", &num_child);
	
	//GET INPUT FOR FILE SELECTION
	printf("SELECT FILE [1, 2, 3] >> ");
	scanf("%d", &file_choice);
	
	switch(file_choice){
		case 1: filename = "file1.dat"; total_lines = 1000; break;
		case 2: filename = "file2.dat"; total_lines = 10000; break;
		case 3: filename = "file3.dat"; total_lines = 100000; break;
		default: printf("INVALID FILE CHOICE\n"); return 1;
	}
	
	gettimeofday(&start, NULL); //GET START TIME -> DOCUMENTATION: https://www.qnx.com/developers/docs/8.0/com.qnx.doc.neutrino.lib_ref/topic/g/gettimeofday.html
	pid_t pids[num_child]; //STORES CHILD ID FOR PARENT TO KEEP TRACK
	int lines_per_child_process = total_lines / num_child; //DIVIDES TASKS FOR CHILDREN FOR DATA FILE
	
	if(num_child == 1){
		SingleChild child;
		pipe(child.fds); //CREATE PIPE
		child.pid = fork(); //FORK THE CHILD PROCESSES
		
		if(child.pid == 0){ //CHILD PROCESS
			
			close(child.fds[0]); //CLOSE READ END OF PIPE
			
			//OPEN THE FILE; DISPLAY EROR MESSAGE IF CANNOT OPEN
			FILE *fp = fopen(filename, "r");
			if(!fp){
				perror("ERROR: FILE CONNOT BE OPENED");
				exit(1);
			}
			
			child.sum = 0; 
			int num = 0;
			for(int i = 0; i < total_lines; i++){
				if(fscanf(fp, "%d", &num) == 1){ //CHECKS FOR ONLY VALID INTEGERS TO ADD OR END OF FILE
					child.sum += num;
				}
			}
			
			fclose(fp); //CLOSES FILE
			write(child.fds[1], &child.sum, sizeof(child.sum)); //WRITES SUM TO PIPE FOR PARENT
			exit(0);
		}
		close(child.fds[1]); //PARENT CLOSES THE WRITE END OF THE PIPE
		
		//PARENT COLLECTS DATA FROM CHILD PROCESSES
		int result = 0;
		read(child.fds[0], &result, sizeof(result));
		close(child.fds[0]);
		wait(NULL);
		gettimeofday(&end, NULL);
		double execution_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
		
		printf("CHILD RESULT: %d\n", result); //PRINT CHILD RESULT
		printf("PARENT RESULT: %d\n", result);//PRINT PARENT RESULT
		printf("EXECUTION TIME: %.6f [s]\n", execution_time); //RETRIEVE EXECUTIONN TIME AND PRINT
	}else{
		
		gettimeofday(&start, NULL); //GET START TIME
		int pipes[num_child][2]; //DECLARES A DOUBLE ARRAY FOR PIPE
		
		//FORK THE CHILD PROCESSES
		for(int i = 0; i < num_child; i++){
			pipe(pipes[i]); //CREATE PIPE
			pids[i] = fork(); //CREATE FORK
		
			if(pids[i] == 0){ //CHILD PROCESS
				close(pipes[i][0]); //CLOSE READ END OF PIPE
			
				//OPEN THE FILE; DISPLAY EROR MESSAGE IF CANNOT OPEN
				FILE *fp = fopen(filename, "r");
				if (!fp){
					perror("ERROR: FILE CONNOT BE OPENED");
					exit(1);
				}
			
				//LOCATE CORRECT LINES TO BE PROCESSED BY EACH CHILD PROCESS
				fseek(fp, i * lines_per_child_process * 5, SEEK_SET);
				int sum = 0, num = 0;
				for (int j = 0; j < lines_per_child_process; j++){
					if(fscanf(fp, "%d", &num) == 1){ //CHECKS FOR ONLY VALID INTEGERS TO ADD OR END OF FILE
						sum += num;
					}
				}
				fclose(fp); //CLOSES FILE
				write(pipes[i][1], &sum, sizeof(sum)); //WRITES SUM TO PIPE FOR PARENT
				exit(0);
			}
			close(pipes[i][1]); //PARENT CLOSES THE WRITE END OF THE PIPE
		}
		
		//PARENT COLLECTS DATA FROM CHILD PROCESSES
		int result = 0;
		for(int i = 0; i < num_child; i++){
			int child_sum;
			read(pipes[i][0], &child_sum, sizeof(child_sum));
			close(pipes[i][0]);
			waitpid(pids[i], NULL, 0); //PREVENTS RACE CONDITIONS OF CHILDREN -> DOCUMENTATION: https://www.qnx.com/developers/docs/8.0/com.qnx.doc.neutrino.lib_ref/topic/w/waitpid.html
			printf("CHILD RESULT: [%d] %d\n", i, child_sum);//PRINT CHILD RESULT
			result += child_sum;
		}
		gettimeofday(&end, NULL); //GET END TIME
		double execution_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
		printf("PARENT RESULT: %d\n", result); //PRINT PARENT RESULT
		printf("EXECUTION TIME: %.6f [s]\n", execution_time); //RETRIEVE EXECUTION TIME AND PRINT
	}
	return 0;
}