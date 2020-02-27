
#define SIZE 500

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
 
static unsigned int sig_flag = 0;


typedef struct Node{
	
	char* val;
	struct Node  *next;
	
} Node;

typedef struct List{
	
	Node* head;
	unsigned int length;
	
} List;

List* initList(){
	List* this = malloc(sizeof(List));
	this->length = 0;
	this->head = NULL;
	
	return this;
}

void insert(List* this, char* _val){
	if(this->head == NULL){
		this->head = malloc(sizeof(Node));
		this->head->val = _val;
		this->head->next = NULL;
	}
	else{
		Node* temp = this->head;
		while(temp->next != NULL){
			temp = temp->next;
		}
		temp->next = malloc(sizeof(Node));
		temp->next->val = _val;
		temp->next->next = NULL;
	}
	
	++(this->length);
}

char* pop_front(List* this){
	char* ret = NULL;
	
	if(this->head == NULL){
		printf("ERROR returning null ptr\n");
	}
	else{
		ret = this->head->val;
		Node* temp = this->head;
		this->head = this->head->next;
		free(temp);
		--(this->length);
	}
	return ret;
}


int build_argsarray(char line[], char* ptrArray[]){
	//break the string up into words
  	char *word = strtok(line," ");
    int i = 0;
    while (word) {
    //printf("word: %s\n", word);
    //copy a word to the arg array
    ptrArray[i] = word;
    //get next word
    word = strtok(NULL, " ");
    i = i + 1;
    }
    
    ptrArray[i] = NULL;
    
    return i;
}

int quit(char* line){
	
	char* quit = "exit\0";
	if(strcmp(quit, line) == 0){
		return 1;
	}
	else{
		return 0;
	}
}

void runProg(char* ptrArray[]){
	pid_t pid; int s = 0; int *status = &s;
	if((pid = fork()) != 0){
		waitpid(pid, status, WEXITSTATUS(*status) );
		printf("pid:%d status:%d\n", pid, (*status % 255) );
		return;
	}
	else if( (execvp(ptrArray[0], ptrArray)) < 0){  // put this in an if statement to get the return value
		printf("error invalid program\n");
		exit(-1);
	}	
}

int isProg(char* ptrArray[]){
	if(ptrArray[0][0] == '.' && ptrArray[0][1] == '/'){
		return 1;
	}
	return 0;
}



void runCommand(char* ptrArray[]){
	pid_t pid; int s = 0; int *status = &s;
	if((pid = fork()) != 0){
		waitpid(pid, status, WEXITSTATUS(*status) );
		printf("pid:%d status:%d\n", pid, (*status % 255) );
		return;
	}
	else if( (execvp(ptrArray[0], ptrArray)) < 0){  // put this in an if statement to get the return value
		printf("error invalid command\n");
		exit(-1);
	}	
}
void clean(char* ptrArray[], int count){
	int i;
	for(i = 0; i <= count; i++){
		ptrArray[i] = NULL;
	}
}



int catchColon(List* instance, char* line){
	int ret = 0;
	char* word = strtok(line, ";");
	
	if(word != NULL && strcmp(line, word) == 0)
		ret = 0;
	else
		ret = 1;
	
	while(word){
		//printf("inside parseColon. word is %s with size %ld\n",word, strlen(word));
		insert(instance, word);
		word = strtok(NULL, ";");
	}
	return ret;	
}
void handler(int sig){
	if(sig == SIGINT){
		sig_flag = 1;
		write(STDOUT_FILENO, "\ncaught sigint", 14);
	}
	else if(sig == SIGTSTP){
		sig_flag = 1;
	    write(STDOUT_FILENO, "\ncaught sigtstp", 15);
	}
}

void setup(){
	struct sigaction sa = {};
	sa.sa_handler = handler;
	sigemptyset(&(sa.sa_mask));
	sa.sa_flags = SA_RESTART;
	const int signals[] = {SIGINT, SIGTSTP}; int i = 0;
	sigaction(signals[i++], &sa, NULL);
	sigaction(signals[i], &sa, NULL);
}

int catchPipe(List* instance, char* line){
	int ret = 0;
	char* word = strtok(line, "|");
	
	if(word != NULL && strcmp(line, word) == 0)
		ret = 0;
	else
		ret = 1;
	
	while(word){
		//printf("inside catchPipe. word is %s with size %ld\n",word, strlen(word));
		insert(instance, word);
		word = strtok(NULL, "|");
		++ret;
	}
	//printf("catch pipe found %d words", ret);
	return ret;	
}

void run(char* temp, int count, char* ptrArray[]){
    count = build_argsarray(temp, ptrArray);

    if(count == 0){
        return;
    }

    else if(isProg(ptrArray)){
        runProg(ptrArray);
        clean(ptrArray, count);
    }

    else{ // !isProg() -> is command, therefore runCommand()
        runCommand(ptrArray);
        clean(ptrArray, count);
	}
}

/*void  _sig_int(int sig){
  write(STDOUT_FILENO, "caught sigint\n", 14);
}

void  _sig_tstp(int sig){
  write(STDOUT_FILENO, "caught sigstp\n", 14);
}
*/

int main(){
	List* instance = initList();
	List* pipeSep = initList();
	char line[500]; int count;
	char* ptrArray[21]; char* shellLine = "CS361 > ";
    
    setup();
   
    while(1){
        write(2, shellLine, strlen(shellLine));
        
		fgets(line, 500, stdin);
		line[strlen(line) - 1] = '\0';
		catchColon(instance, line);
		
		while(instance->length != 0){
			char* temp = pop_front(instance);
			
			if(catchPipe(pipeSep, temp) == 2){
				int pipefds[2]; int proc1 = -1, proc2 = -1;
				if(pipe(pipefds) == -1){
					perror("pipe\n");
					exit(EXIT_FAILURE);
				}
				
				char* command1 = pop_front(pipeSep);
				char* command2 = pop_front(pipeSep);
				
				int old_out = dup(STDOUT_FILENO);
				int old_in = dup(STDIN_FILENO);
				
				int s1 = 0; int *status1 = &s1;
				int s2 = 0; int *status2 = &s2;
				
				if((proc1 = fork()) != 0){
					waitpid(proc1, status1, WEXITSTATUS(*status1));
				}
				
				else if(proc1 == 0){
					close(pipefds[0]); // close read end in child 1
					dup2(pipefds[1], STDOUT_FILENO); // redirect out to write end
					count = build_argsarray(command1, ptrArray);
					if( (execvp(ptrArray[0], ptrArray)) < 0){  // put this in an if statement to get the return value
						dup2(old_out, STDOUT_FILENO); // change stdout back after proc1 finishes redirecting io
						printf("in proc 1 error invalid command %s\n", ptrArray[0]);
						exit(-1);
					}
				}
				
				if((proc2 = fork()) != 0){
					close(pipefds[0]); //close read end in parent;
					close(pipefds[1]);  // close write end in parent
					dup2(old_out, STDOUT_FILENO); // change back stdout in parent
					waitpid(proc2, status2, WEXITSTATUS(*status2));
				}
				
				else if(proc2 == 0){
					close(pipefds[1]); // close write end in child 2
					dup2(pipefds[0], STDIN_FILENO); // redirect stdin to get from read end
					count = build_argsarray(command2, ptrArray);
					if( (execvp(ptrArray[0], ptrArray)) < 0){  // put this in an if statement to get the return value
						dup2(old_out, STDOUT_FILENO); // change stdout back after proc1 finishes redirecting io
						printf("in proc 1 error invalid command %s\n", ptrArray[0]);
						exit(-1);
					}
				}
				
				if(proc1 != 0 && proc2 != 0){
					dup2(old_in, STDIN_FILENO); // change stdin back
					printf("pid:%d status:%d\n", proc1, (*status1 % 255) );
					printf("pid:%d status:%d\n", proc2, (*status2 % 255) );
				}
				
				close(old_in); close(old_out);
			}
				
			/*	char* next = pop_front(pipeSep);
				dup2(pipefds[1], STDOUT_FILENO);
				if((proc1 = fork()) != 0){
					int s1 = 0; int *status1 = &s1;
					//printf("next is %s\n", next);     
					//count = build_argsarray(next, ptrArray);
					waitpid(proc1, status1, WEXITSTATUS(*status1));
					//close(pipefds[1]); // close write end of pipe
					dup2(old_out, STDOUT_FILENO); // change stdout back after proc1 finishes redirecting io
					//printf("made it back to parent\n");
					close(pipefds[1]);
					clean(ptrArray, count);
					dup2(pipefds[0], STDIN_FILENO); 
					if((proc2 = fork()) != 0){
						//printf("made it to parent, forkd proc2\n");
						int s2 = 0; int *status2 = &s2;
						waitpid(proc2, status2, WEXITSTATUS(*status2));
						printf("pid 1st:%d status:%d\n", proc1, (*status1 % 255) );
						printf("pid 2nd:%d status:%d\n", proc2, (*status2 % 255) );
						close(pipefds[0]); 
						//close(pipefds[1]);
						dup2(old_in, STDIN_FILENO); 
						//dup2(old_out, STDOUT_FILENO);
					} 
				}
				
				if(proc1 == 0){
					//close(pipefds[0]); // close read end
					//dup2(pipefds[1], STDOUT_FILENO);

				   //printf("in proc1\n");
					//dup2(pipefds[1], STDOUT_FILENO); // change stdout to write end of pipe
					//close read end of pipe
					
					count = build_argsarray(next, ptrArray);
					if( (execvp(ptrArray[0], ptrArray)) < 0){  // put this in an if statement to get the return value
						dup2(old_out, STDOUT_FILENO); // change stdout back after proc1 finishes redirecting io
						printf("in proc 1 error invalid command %s\n", ptrArray[0]);
						exit(-1);
					}	
				}
				
				if(proc2 == 0){
					//printf("in proc2\n");
					//close(pipefds[1]); // close write end of pipe
					//dup2(pipefds[0], STDIN_FILENO); // change stdin to read end of pipe
					//printf("in proc2\n");
					next = pop_front(pipeSep);
					//printf(" in proce 2 im about to %s \n", next);
					clean(ptrArray, count);
					count = build_argsarray(next, ptrArray);
					//printf("in proc2 comand is %s", next);
					if( (execvp(ptrArray[0], ptrArray)) < 0){  // put this in an if statement to get the return value
						dup2(old_in, STDIN_FILENO); // change stdout back after proc1 finishes redirecting io
						printf("error proc 2 invalid command %s\n", ptrArray[0]);
						exit(-1);
					}
				}
				dup2(old_in, STDIN_FILENO); dup2(old_out, STDOUT_FILENO);
				memset(line,0,sizeof(line));
				//continue;	
			}*/
					
				/*if( (proc2 = fork()) == 0){
						next = pop_front(pipeSep);
						close(pipefds[1]); // close write end of pipe
						dup2(pipefds[0], STDIN_FILENO);
						//printf("in child pipe, want to run: %s\n", next);
						close(pipefds[0]);
						count = build_argsarray(next, ptrArray);
						//run(next, count, ptrArray);
						if( (execvp(ptrArray[0], ptrArray)) < 0){  // put this in an if statement to get the return value
							printf("error invalid command\n");
							exit(-1);
						}
						printf("made it here\n");
						//dup2(old_in, STDIN_FILENO);
						//exit(0);	
					}
					dup2(pipefds[1], STDOUT_FILENO);
					count = build_argsarray(next, ptrArray);
					int s = 0; int *status = &s;
					if( (execvp(ptrArray[0], ptrArray)) < 0){  // put this in an if statement to get the return value
						printf("error invalid command\n");
						exit(-1);
					}*/
					
				/*if((who = fork()) != 0){ // parent
				    close(pipefds[0]); // close read end of pipe
					dup2(pipefds[1], STDOUT_FILENO); // redirect stdout to read end
					run(next, count, ptrArray);
					dup2(old_out, STDOUT_FILENO); // change back;
					close(pipefds[1]); // close write end
					int s = 0; int *status = &s;
					waitpid(who, status, WEXITSTATUS(*status));
					//wait(status);
					printf("ppppid:%d status:%d\n", who, (*status % 255) );
					dup2(old_in, STDIN_FILENO);
						
				}
				else if(who == 0){ // child
					next = pop_front(pipeSep);
					close(pipefds[1]); // close write end of pipe
					dup2(pipefds[0], STDIN_FILENO);
					//printf("in child pipe, want to run: %s\n", next);
					close(pipefds[0]);
					count = build_argsarray(next, ptrArray);
					//run(next, count, ptrArray);
					if( (execvp(ptrArray[0], ptrArray)) < 0){  // put this in an if statement to get the return value
						printf("error invalid command\n");
						exit(-1);
					}
					printf("made it here\n");
					//dup2(old_in, STDIN_FILENO);
					//exit(0);	
				}*/	
			
			else{
				
			count = build_argsarray(temp, ptrArray);
			
			if(sig_flag == 1){
				break;
			}
			
			if(count == 0){
				break;
			}
			
			else if(quit(ptrArray[0])){ // test if user wrote quit
				exit(0);
			}
			
			else if(isProg(ptrArray)){
				runProg(ptrArray);
				clean(ptrArray, count);
			}
			
			else{ // !isProg() -> is command, therefore runCommand()
				runCommand(ptrArray);
				clean(ptrArray, count);
			}	
		}
	}
		memset(line,0,sizeof(line));
		
		if(sig_flag == 1){
			sig_flag = 0;
			continue;
		}
			  
	}
	return 0;
}
