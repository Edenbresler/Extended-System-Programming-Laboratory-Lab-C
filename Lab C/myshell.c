#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/wait.h>
#include "LineParser.h"
#include <fcntl.h>

#define HISTLEN 20

    #define TERMINATED  -1
    #define RUNNING 1
    #define SUSPENDED 0

    typedef struct process{
        cmdLine* cmd;                         /* the parsed command line*/
        pid_t pid;                        /* the process id that is running the command*/
        int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
        struct process *next;                     /* next process in chain */
    } process;


process *process_list = NULL;


char* history_list[HISTLEN];
int newest = -1; 
int oldest  = -1;

int debug_mode =0;



void addToHistory(char* command) {
    char* new_history = (char*)malloc(strlen(command) + 1);
    strcpy(new_history,command);
    
    if(oldest==-1){
        newest=0;
        oldest=0;
        history_list[newest] = new_history;
        
    }
    else{
        newest=(newest+1)%HISTLEN;
        if(newest==oldest){
            free(history_list[oldest]);
            oldest=(oldest+1)%HISTLEN;
            history_list[newest]=new_history;
        }
        else{
            history_list[newest]=new_history;
        }
    }

}

void printHistory() {
    int i;
    int index=1;
    if(oldest !=-1){
        for (i = oldest; i != newest; i = (i + 1) % HISTLEN) {
            if (history_list[i] != NULL) {
                printf("%d %s\n",index, history_list[i]);
                index++;
            }
            
        }
        printf("%d %s\n",index, history_list[i]);
    }
}

cmdLine* getCommandFromHistory(int index) {
    
    
    if(oldest !=-1 && history_list[index]!=NULL){
        
        addToHistory(history_list[index]);
        if(strcmp(&history_list[index][0], "history\n")!=0 ){
           fprintf(stderr,"%s",history_list[index]); 
            return parseCmdLines(history_list[index]);
        }
        else{
         
            printHistory();
        }

    }

    
    return NULL; 
}


cmdLine *deepCopyCmdLine(const cmdLine* original) {
    
    cmdLine *copy = (cmdLine*)malloc(sizeof(cmdLine));
    if (original->inputRedirect != NULL) {
        copy->inputRedirect = strdup(original->inputRedirect);
    } else {
        copy->inputRedirect = NULL;
    }

    if (original->outputRedirect != NULL) {
        copy->outputRedirect = strdup(original->outputRedirect);
    } else {
        copy->outputRedirect = NULL;
    }

    copy->next = NULL; 

   
    copy->argCount = original->argCount;
    copy->blocking = original->blocking;
    copy->idx = original->idx;

    
    for (int i = 0; i < original->argCount; i++) {
       replaceCmdArg(copy, i,original->arguments[i]);
    }

    return copy;
}


    void addProcess(process** process_list, cmdLine* cmd, pid_t pid){
        process *process_to_add = (process*) malloc(sizeof(process));
        process_to_add->cmd = deepCopyCmdLine(cmd);
        process_to_add->pid = pid;
        process_to_add->status = RUNNING;
        process_to_add->next = *process_list;
        *process_list = process_to_add;

    }

        void updateProcessStatus(process* process_list, int pid, int status){
        process* current = process_list;

        while (current != NULL) {
            if (current->pid == pid) {
                current->status = status;
               
                break;  
            }
            current = current->next;
        }

    }


void updateProcessList(process **process_list) {
    process *current = *process_list;
    process *prev = NULL;

    while (current != NULL) {
        int status;
       
        pid_t result = waitpid(current->pid, &status, WNOHANG |WCONTINUED |WUNTRACED);
     
        if(result!=0){
            if (WIFEXITED(status)) {
                updateProcessStatus(*process_list, current->pid, TERMINATED);
            
                
            } if (WIFSIGNALED(status)) {
                updateProcessStatus(*process_list, current->pid, -1);
      
                
            }if (WIFSTOPPED(status)) {
                updateProcessStatus(*process_list, current->pid, SUSPENDED);
          
                
            } if (WIFCONTINUED(status)) {
                updateProcessStatus(*process_list, current->pid, RUNNING);
            
                
            }
        }
            

           
            prev = current;
            current = current->next;

    }
}
 

    void printProcessList(process** process_list){
        updateProcessList(process_list);
        printf("ID\tPID\tCommand\tSTATUS\n");
        
        process *curr_process = *process_list; 
        process *prev = NULL;
        int termin_sin=0;
     

        
       
        int index = 0;
        while (curr_process != NULL) {
           
            printf("%d\t%d\t", index, curr_process->pid );

            
            // Print command and arguments
            for (int i = 0; i < curr_process->cmd->argCount; i++) {
                printf("%s ", curr_process->cmd->arguments[i]);
                
            }

            if (curr_process->status ==RUNNING) {
               
                printf("Running\n");
            }
            else if(curr_process->status ==SUSPENDED){
                
                printf("Suspended\n");
            }
            else if(curr_process->status ==TERMINATED){
                
                termin_sin=1;
                printf("Terminated\n");
            }
            
          
            index++;
            if(termin_sin==1){
                if (prev == NULL) {
                    *process_list = curr_process->next;
                  
                    freeCmdLines(curr_process->cmd);
                    free(curr_process);
                    
                    curr_process = *process_list;
                   
                }
                else {
                    prev->next = curr_process->next;
                    
                    freeCmdLines(curr_process->cmd);
                    free(curr_process);
               
                    curr_process = prev->next;
                }
                termin_sin=0;

            }
            else{
       
            prev=curr_process;
            curr_process = curr_process->next;
         

            }

        }

    }


    void freeProcessList(process **process_list) {
        process *current_process = *process_list;
        process *next;

        while (current_process != NULL) {
            next = current_process->next;
            freeCmdLines(current_process->cmd); 
            free(current_process);
            current_process = next;
        }

        *process_list = NULL;
    }

    


void execute(cmdLine *pCmdLine) {

    
    if (strcmp(pCmdLine->arguments[0], "procs") == 0) {
        printProcessList(&process_list);
    }
    
    else if(strcmp(pCmdLine->arguments[0] ,"cd")==0){
        if (pCmdLine->argCount > 1) {
            if (chdir(pCmdLine->arguments[1])==-1)
            {
                perror("cd operation error");
            }
        }
        
    }

    else if(strcmp(pCmdLine->arguments[0] ,"suspend")==0){
        if (pCmdLine->argCount > 1) {
            pid_t pid = atoi(pCmdLine->arguments[1]);
            if (kill(pid, SIGTSTP) == 0) {
                updateProcessStatus(process_list,pid, 0);
                printf("suspend pid: %d\n",pid);
                
            }
            else{
                printf("suspend error");
            }
        }
    
    }

    else if(strcmp(pCmdLine->arguments[0] ,"wakeup")==0){
        if (pCmdLine->argCount > 1) {
            pid_t pid = atoi(pCmdLine->arguments[1]);
            if (kill(pid, SIGCONT) == 0) {
                updateProcessStatus(process_list,pid, 1);
                printf("wakeup pid: %d\n",pid);
            }
            else{
                printf("wakeup error");
            }
        }
    
    }
    else if(strcmp(pCmdLine->arguments[0] ,"nuke")==0){
        if (pCmdLine->argCount > 1) {
            pid_t pid = atoi(pCmdLine->arguments[1]);
            if (kill(pid, SIGTERM) == 0) { 
                updateProcessStatus(process_list,pid,-1);
                printf("nuke pid: %d\n",pid);
            }
            else{
                printf("nuke error");
            }
        }
    }
    else{

        pid_t new_process;
        int pipe_shell[2];
        cmdLine* next = pCmdLine->next; 
        pid_t child1;
        

        if (next == NULL){
            new_process = fork();
            if (new_process == 0) {
               
            
                if(pCmdLine->inputRedirect != NULL){
               
                    int input_fd = open(pCmdLine->inputRedirect,O_RDONLY,0644);
                    if(input_fd ==-1){
                        perror("error input file");
                        exit(EXIT_SUCCESS);
                    }
                    dup2(input_fd, STDIN_FILENO);
                    close(input_fd);
                }
                if(pCmdLine->outputRedirect != NULL){
             
                    int output_fd =open(pCmdLine->outputRedirect,O_RDWR | O_CREAT |O_TRUNC ,0644); 
                    if(output_fd ==-1){
                        perror("error output file");
                        exit(EXIT_SUCCESS);
                    }
                    dup2(output_fd, 1);
                    close(output_fd);
                }
                if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1) {
                
                    perror("Execv_error");
                    freeCmdLines(pCmdLine);
                    _exit(EXIT_FAILURE);
                }
                
            }
            else if (new_process > 0) {
                 addProcess(&process_list, pCmdLine, new_process);
  
                if (debug_mode==-1){
                    fprintf(stderr , "the PID is : %d\n" , new_process);
                    fprintf(stderr , "the executing command is : %s\n" , pCmdLine->arguments[0]);
                }
                if(pCmdLine->blocking !=0){
             
                    waitpid(new_process,NULL,0);
                }
               
                
            }
        }

        else{
            if(pipe(pipe_shell)==-1){
                perror("Pipe creation failed");
                exit(EXIT_FAILURE);
            }
            child1 = fork();

            if (child1 == 0) {
                
                if (pCmdLine->outputRedirect != NULL) {
                    fprintf(stderr, "Error: Redirection the output of left-hand-side process\n");
                    _exit(EXIT_FAILURE);
                }
                if(pCmdLine->inputRedirect != NULL){
               
                    int input_fd = open(pCmdLine->inputRedirect,O_RDONLY,0644);
                    if(input_fd ==-1){
                        perror("error input file");
                        exit(EXIT_SUCCESS);
                    }
                    dup2(input_fd, STDIN_FILENO);
                    close(input_fd);
                }

              
                close(1);
                dup(pipe_shell[1]);
                close(pipe_shell[1]);
              
               
                if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1) {
                   
                    perror("Execv_error");
                    freeCmdLines(pCmdLine);
                    _exit(EXIT_FAILURE);
                }
              
            }
            else if (child1 > 0) {
                
                addProcess(&process_list, pCmdLine, child1);
             
                waitpid(child1, NULL, 0); 
                close(pipe_shell[1]);
                
                if (pCmdLine->next){ 
               
                    pCmdLine = pCmdLine->next;
            
                    pid_t child2;
                  
                    child2 = fork();
                    if (child2 == 0) { 
                        close(0);
                       
                        if (pCmdLine->inputRedirect != NULL) {
                            fprintf(stderr, "Error: Redirection the input of right-hand-side process\n");
                            _exit(EXIT_FAILURE);
                        }
                
                if(pCmdLine->outputRedirect != NULL){
           
          
                    int output_fd =open(pCmdLine->outputRedirect,O_RDWR | O_CREAT |O_TRUNC ,0644); 
                    if(output_fd ==-1){
                        perror("error output file");
                        exit(EXIT_SUCCESS);
                    }
                    dup2(output_fd, 1);
                    close(output_fd);
                }

                        dup(pipe_shell[0]);
                        close(pipe_shell[0]);
                       
                        if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1) {
                            perror("Execv_error");
                            freeCmdLines(pCmdLine);
                            _exit(EXIT_FAILURE);
                        }
                     

                    }
                    else if(child2>0){
                   
                        addProcess(&process_list, pCmdLine, child2);
                        
                        close(pipe_shell[0]);
                       
                        if (pCmdLine->blocking != 0) {
                            
                            waitpid(new_process, NULL, 0);
                            waitpid(child2, NULL, 0);
                        }
                    }
                }
                else{
                    if (pCmdLine->blocking != 0) {
                    
                    waitpid(new_process, NULL, 0);
                    }
                }

                
                
            }

        }
    }

}

int main(int argc, char **argv) {
    
    char* current_directory  = (char*) malloc(PATH_MAX);
    char* user_input = (char*) malloc(2048);
    cmdLine* command_line = (cmdLine*)malloc(sizeof(cmdLine));
    while (1) {
        for (int i = 1; i < argc; i++){
            if((argv[i][0]=='-') && (argv[i][1]=='d')){
                debug_mode =-1;
            }
        }

        if (getcwd(current_directory, PATH_MAX) != NULL) {
            printf("%s\n ", current_directory);
        }

        if (fgets(user_input, 2048, stdin) == NULL) {
            perror("User inputfgets error"); 
            exit(0);
        }

        if (strcmp(user_input, "quit\n") == 0) {

            if(command_line != NULL){
                freeCmdLines(command_line);
            }

            break;
        }
        if (strcmp(user_input, "history\n") == 0) {
            addToHistory(user_input);
            printHistory();
        }
        else if (strcmp(user_input, "!!\n") == 0) {
            
            cmdLine* last_command = getCommandFromHistory(newest);
            if (last_command != NULL) {

                    execute(last_command);
            
            }

        }
        else if (user_input[0] == '!' && user_input[1] >= '1' && user_input[1] <= '9') {
            int index = atoi(user_input + 1)-1;
            if(oldest!=-1){
               cmdLine* command_from_history = getCommandFromHistory((oldest + index) % HISTLEN);

            if (command_from_history != NULL) {
   
                execute(command_from_history);
                
            } else {
                if((oldest ==-1 || history_list[index-1]==NULL))
                printf("Invalid history index: %d\n", index);
            }
            }
            else{
                printf("Invalid history index: %d\n", index);
            }
        }
        else{


        const char* const_user_input = user_input;
        cmdLine *command_line = parseCmdLines(const_user_input);
       
        execute(command_line);
        
        addToHistory(user_input);
        
        }
        
    }
    if(process_list!=NULL){
       freeProcessList(&process_list);
    }
    free(current_directory);
    free(user_input);
    return 0;
}