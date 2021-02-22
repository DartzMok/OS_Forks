#include "wc.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main(int argc, char **argv)
{
		long fsize;
		FILE *fp;
		count_t count, buf;
		struct timespec begin, end;
		int nChildProc = 0, i, j, k, l, m, fpos = 0, len, bytesread, bytesleft, status;
		bool crash = false;
		
		/* 1st arg: filename */
		if(argc < 2) {
				printf("usage: wc <filname> [# processes] [crash rate]\n");
				return 0;
		}
		
		/* 2nd (optional) arg: number of child processes */
		if (argc > 2) {
				nChildProc = atoi(argv[2]);
				if(nChildProc < 1) nChildProc = 1;
				if(nChildProc > 10) nChildProc = 10;
		}

		/* 3rd (optional) arg: crash rate between 0% and 100%. Each child process has that much chance to crash*/
		if(argc > 3) {
				crashRate = atoi(argv[3]);
				if(crashRate < 0) crashRate = 0;
				if(crashRate > 50) crashRate = 50;
				printf("crashRate RATE: %d\n", crashRate);
		}
		
		printf("# of Child Processes: %d\n", nChildProc);
		printf("crashRate RATE: %d\%\n", crashRate);	      
		
		count.linecount = 0;
		count.wordcount = 0;
		count.charcount = 0;

		// start to measure time
		clock_gettime(CLOCK_REALTIME, &begin);

		// Open file in read-only mode
		fp = fopen(argv[1], "r");

		if(fp == NULL) {
				printf("File open error: %s\n", argv[1]);
				printf("usage: wc <filname>\n");
				return 0;
		}
		
		// get a file size
		fseek(fp, 0L, SEEK_END);
		fsize = ftell(fp);
		
		int p[2*nChildProc][2];
		int offset[nChildProc];
		int pid[nChildProc];
	    
		for(k = 0; k < 2*nChildProc; k++){
		  if(pipe(p[k]) != 0) {exit(1);}
		}

		//splits the process into child processes and reads from the file
		for(i = 0; i < nChildProc; i++) {
 	  
		  pid[i] = fork();

		  if(pid[i] < 0){
		    printf("Fork failed.\n");
		    exit(1);
		  }
		  else if(pid[i] == 0){
		    len = read(p[i][0], &fpos, sizeof(fpos));
		    if(len > 0){

		      fp = fopen(argv[1], "r");
		      fseek(fp, fpos, SEEK_SET);
		      bytesread = fsize/nChildProc;

		      if(i == nChildProc - 1){
			bytesleft = fsize % nChildProc;
			count = word_count(fp, fpos, bytesread + bytesleft);
		      }
		      else{
			count = word_count(fp, fpos, bytesread);
		      }
       
		      long bytes_sent;

		      if((bytes_sent = write(p[i+nChildProc][1], &count, sizeof(count))) == -1) {
			printf("Writing into pipe failed.\n");
			exit(1);
		      }
		      
		      fclose(fp);
		      close(p[i][0]);
		      close(p[i][1]);
		      exit(0);
		    }
		  }
		}

		//Calculates the offset and sends value to child prcoess
		for(l = 0; l < nChildProc; l++){
		  fpos = fsize/nChildProc;
		  fpos = (l * fpos);
		  write(p[l][1], &fpos, sizeof(fpos));
		  offset[l] = fpos;
		}

		//Checks to see if a child process crashes and if it does creates
		//a new child process to take over the job
		for(m = 0; m < nChildProc; m++){
		    waitpid(pid[m], &status, 0);

		    if(WIFSIGNALED(status)){
		      crash = true;
		    }


		    while(crash){
		      pid[m] = fork();

		      if(pid[m] < 0){
			printf("Fork failed.\n");
			exit(1);
		      }
		      else if(pid[m] == 0){
		     
			fp = fopen(argv[1], "r");
			fseek(fp, offset[m], SEEK_SET);
			bytesread = fsize/nChildProc;

			if(m == nChildProc - 1){
			  bytesleft = fsize % nChildProc;
			  count = word_count(fp, offset[m], bytesread + bytesleft);
			}
			else{
			  count = word_count(fp, offset[m], bytesread);
			}
       
			long bytes_sent;

			if((bytes_sent = write(p[m+nChildProc][1], &count, sizeof(count))) == -1) {
			  printf("Writing into pipe failed.\n");
			  exit(1);
			}
		      
			fclose(fp);
			close(p[m][0]);
			close(p[m][1]);
			exit(0);
      	
		      }
		      else{
			waitpid(pid[m], &status, 0);	
			if(WIFEXITED(status)){
			  crash = false;
			}
		      }
		    }	   	   
		}
		  
		  
		long bytes_read;

		for(j = 0; j < nChildProc; j++){
		  if((bytes_read = read(p[j+nChildProc][0], &buf, sizeof(buf))) == -1){
		    printf("Reading from pipe failed.\n");
		    exit(1);
		  };
		    
		  if(bytes_read){
		    count.linecount += buf.linecount;
		    count.wordcount += buf.wordcount;
		    count.charcount += buf.charcount;
		  }

		  close(p[j+nChildProc][0]);
		  close(p[j+nChildProc][1]);
	     
		}
      
	       		
		/* word_count() has 3 arguments.
		 * 1st: file descriptor
		 * 2nd: starting offset
		 * 3rd: number of bytes to count from the offset
		 */

		if(nChildProc == 0){
		  count = word_count(fp, 0, fsize);

		  fclose(fp);
		}

		clock_gettime(CLOCK_REALTIME, &end);
		long seconds = end.tv_sec - begin.tv_sec;
		long nanoseconds = end.tv_nsec - begin.tv_nsec;
		double elapsed = seconds + nanoseconds*1e-9;

		printf("\n========= %s =========\n", argv[1]);
		printf("Total Lines : %d \n", count.linecount);
		printf("Total Words : %d \n", count.wordcount);
		printf("Total Characters : %d \n", count.charcount);
		printf("======== Took %.3f seconds ========\n", elapsed);

		return(0);		    		  
		  
		
		

}

