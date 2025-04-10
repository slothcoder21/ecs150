#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512 // Maximum Length of command line input 
#define MAX_TOKENS 17 // 16 arguments

/*
how parse should work in my brain??:

Given: "date -u"
argv[0] = "date"
argv[1] = "-u" 

need a variable to keep track of tokens - argc
*/

int parse(char *str, char *argv[])
{
        int argCount = 0; //keeps track of tokens found
        
        /*
        strtok returns a pointer to the next token found in the string
        strtok modifies the existing string, separating the string into a series of tokens. if a delimiter is spotted
        it replaces it with a '\0' to get rid of the delimiter
        */
        
        char *token = strtok(str, " \t"); //Splits at delimiters and returns a pointer to the first token
        //Loop through the string ensuring we don't go over the maximum number of tokens
        while (token != NULL && argCount < MAX_TOKENS - 1)
        {
                argv[argCount++] = token;
                token = strtok(NULL, " \t"); //stores each token in the array argv[argc++]
        }

        if(token != NULL)
        {
                return -1;
        }

        argv[argCount] = NULL;

        return argCount; //Returns the number of tokens found
}

int main(void)
{
        char cmd[CMDLINE_MAX]; //Array that stores the command entered by the user 
        char unedited_cmd[CMDLINE_MAX];
        char *eof; //A pointer that will help detect end-of-file conditions

        while(1)
        {
                char *nl; // pointer used to find the newline character
                // int retval; // store the return value of executed commands 
        
                /*Print Prompt*/
                printf("sshell@ucd$ ");
                fflush(stdout); //Flushes the output buffer to ensure it's displayed
        
                /*Get Command Line*/
                eof = fgets(cmd, CMDLINE_MAX, stdin); //Reads a line from standard input into the cmd array
                if (!eof) // If fgets returns NULL indicating EOF, then command exits
                {
                        strncpy(cmd, "exit\n", CMDLINE_MAX); 
                        strncpy(unedited_cmd, "exit\n", CMDLINE_MAX); 
                }
                else
                {
                        strncpy(unedited_cmd, cmd , CMDLINE_MAX);
                }
 
                /*Print command line if stdin is not provided by terminal*/
                if (!isatty(STDIN_FILENO)) // Checks if the standard input is not coming from a terminal
                {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /*Remove trailing newline from command line*/
                nl = strchr(cmd, '\n'); //Find newline character using cmd 
                if (nl)
                {
                        *nl = '\0';
                }

                // Removes the newline from unedited_cmd
                nl = strchr(unedited_cmd, '\n');
                if (nl)
                {
                        *nl = '\0';
                }  

                /* Builtin Command */
                if (!strcmp(cmd, "exit"))
                {
                        fprintf(stderr, "Bye...\n");
                        fprintf(stderr,"+ completed 'exit' [0]\n");
                        exit(0);
                }

                

                char *argv[MAX_TOKENS]; //storing arguments for the parsed array
                int argCount = parse(cmd, argv); // parsing command by tokenizing 

                if(argCount == -1)
                {
                        fprintf(stderr, "Error: too many process arguments\n");
                        continue;
                }

                pid_t pid = fork(); //create child process
                if(pid == 0) //checks if child
                {
                        execvp(argv[0], argv);
                        fprintf(stderr, "Error: command not found\n"); // match expected output
                        exit(1);
                }
                else if (pid > 0) //checks if parent
                {
                        int status;
                        waitpid(pid, &status, 0);
                        int exit_status = WEXITSTATUS(status); //exit status from child 
                        fprintf(stderr, "+ completed '%s' [%d]\n", unedited_cmd, exit_status);
                }
                else // if fork fails
                {
                        perror("fork error");
                        exit(1);
                }
        }

        return EXIT_SUCCESS;
}
