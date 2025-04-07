#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CMDLINE_MAX 512 // Maximum Length of command line input 

int main(void)
{
        char cmd[CMDLINE_MAX]; //Array that stores the command entered by the user 
        char *eof; //A pointer that will help detect end-of-file conditions

        while(1)
        {
                char *nl; // pointer used to find the newline character
                // int retval; // store the return value of executed commands 
        
                /*Print Prompt*/
                printf("sshell@ucd$ ");
                fflush(stdout); //Flushes the output buffer to ensure its displayed
        
                /*Get Command Line*/
                eof = fgets(cmd, CMDLINE_MAX, stdin); //Reads a line from standard input into the cmd array
                if (!eof) // If fgets returns NULL indicating EOF, then command exits
                {
                        strncpy(cmd, "exit\n", CMDLINE_MAX); 
                }
                /*Print command line if stdin is not provided by terminal*/
                if (!isatty(STDIN_FILENO)) // Checks if the standard input is not coming from a terminal
                {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /*Remove trainiling newline from command line*/
                nl = strchr(cmd, '\n'); //Find newline character using cmd 
                if (nl)
                {
                        *nl = '\0';
                }

                /* Builtin Command */
                if (!strcmp(cmd, "exit"))
                {
                        fprintf(stderr, "Bye...\n");
                }

                pid_t pid = fork();

                if(pid == 0)
                {
                        char *argv[2];
                        argv[0] = cmd;
                        argv[1] = NULL;

                        execvp(argv[0], argv);

                        perror("execvp error");
                        exit(1);
                }
                else if (pid > 0)
                {
                        int status;
                        waitpid(pid, &status, 0);
                        int exit_status = WEXITSTATUS(status);
                        fprintf(stderr, "+ completed '%s' [%d]\n", cmd, exit_status);
                }
                else
                {
                        perror("fork error");
                        exit(1);
                }
        }

        return EXIT_SUCCESS;

}