
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
		//printf("inside parseColon. word is %s with size %ld\n",word, strlen(word));
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
				int pipefds[2]; int who;
				if(pipe(pipefds) == -1){
					perror("pipe\n");
					exit(EXIT_FAILURE);
				}
				
				int old_out = dup(STDOUT_FILENO);
				int old_in = dup(STDIN_FILENO);
				
				char* next = pop_front(pipeSep);
				if((who = fork()) != 0){ // parent
				    close(pipefds[0]); // close read end of pipe
				    //printf("in parent pipe, want to run: %s\n", next);
					dup2(pipefds[1], STDOUT_FILENO); // redirect stdout to read end
					run(next, count, ptrArray);
					dup2(old_out, STDOUT_FILENO); // change back;
					close(pipefds[1]); // close write end
					int s = 0; int *status = &s;
					waitpid(who, status, WEXITSTATUS(*status));
					printf("pid:%d status:%d\n", who, (*status % 255) );
						
				}
				else if(who == 0){ // child
					next = pop_front(pipeSep);
					close(pipefds[1]); // close write end of pipe
					dup2(pipefds[0], STDIN_FILENO);
					//printf("in child pipe, want to run: %s\n", next);
					close(pipefds[0]);
					run(next, count, ptrArray);	
					dup2(old_in, STDIN_FILENO);	
				}	
				continue;	
			}
			
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
		memset(line,0,sizeof(line));
		
		if(sig_flag == 1){
			sig_flag = 0;
			continue;
		}
			  
	}
	return 0;
}
