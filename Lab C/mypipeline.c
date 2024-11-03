#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
int main(int argc, char **argv) {
    int my_pipe[2];
    char *ls_arr[] = {"ls", "-l", NULL};
    char *tail_arr[] = {"tail", "-n", "2", NULL};
    if (pipe(my_pipe) == -1) { 
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr , "(parent_process>forking...)\n");
    pid_t pid_child1 = fork();
    if (pid_child1 == -1) { 
        perror("Fork failed");
        fprintf(stderr , "(parent_process>exiting...)\n");
        exit(EXIT_FAILURE);
    }
    else if (pid_child1 == 0) {
        
        fprintf(stderr , "(child1>redirecting stdout to the write end of the pipe...)\n"); 
        close(1);
        dup(my_pipe[1]);
        close(my_pipe[1]);
        fprintf(stderr , "(child1>going to execute cmd:...)\n"); 
        execvp(ls_arr[0],ls_arr);

        
    }
    else{
        
        fprintf(stderr ,"(parent_process>created process with id: %d )\n",pid_child1);

        fprintf(stderr , "(parent_process>closing the write end of the pipe...)\n");
        close(my_pipe[1]);
        fprintf(stderr , "(parent_process>forking...)\n");
        pid_t pid_child2 = fork();
        
        if (pid_child2 == -1) { 
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
        else if (pid_child2 == 0) {
            
            fprintf(stderr , "(child2>redirecting stdin to the read end of the pipe...)\n"); 
            close(0);
            dup(my_pipe[0]);
            close(my_pipe[0]);
            fprintf(stderr , "(child2>going to execute cmd:...)\n");
            execvp(tail_arr[0], tail_arr);

        }
        else{
        fprintf(stderr ,"(parent_process>created process with id: %d)\n",pid_child2);
        fprintf(stderr , "(parent_process>closing the read end of the pipe...)\n");
        close(my_pipe[0]);
        fprintf(stderr , "(parent_process>waiting for child processes to terminate...)\n");
        waitpid(pid_child1, NULL, 0);
        fprintf(stderr , "(parent_process>waiting for child processes to terminate....)\n");
        waitpid(pid_child2, NULL, 0);

        fprintf(stderr, "(parent_process>exiting...)\n");
        exit(0); 
        }
    }

    return 0;
}