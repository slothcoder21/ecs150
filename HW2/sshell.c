#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512 //maximum command line length
#define ARGS_MAX 17 // maximum number of arguments
#define CMDS_MAX 8 // Piping worst case scenario (pause)

struct command { //just for processing each command
        char *argv[ARGS_MAX];
};

struct chain { // struct for storing piping commands
        int count; 
        struct command commands[CMDS_MAX]; 
};

// gotta remove all the whitespaces
char *trim(char* str)
{
        while(*str && isspace((unsigned char) *str)) //skip the leading whitespace
        {
                str++;
        }
        if(!*str) //retunr an empty string if the whole string is spaces
        {
                return str;
        }

        char *end = str + strlen(str) - 1; // this is the ending whitespace we need to remove

        while (end > str && isspace((unsigned char) *end))
        {
                *end-- = '\0';
        }

        return str; //returns the trimmed command string
}

int parse(char *str, struct command *command) {
        int argCount = 0;
        char *token = strtok(str, " \t"); // Splits at delimiters and returns a pointer to the first token

        // Loop through the string ensuring we don't go over the maximum number of tokens
        while (token != NULL && argCount < ARGS_MAX - 1)
        {
                command->argv[argCount++] = token;
                token = strtok(NULL, " \t"); // Stores each token in the array argv[argc++]
        }

        if(token != NULL)
        {
                return -1;
        }

        command->argv[argCount] = NULL;
        return argCount; //Returns the number of tokens found
}

// goal is to split the input between '|' into chain->commands
int parse_chain(char *input, struct chain *chain) {
        chain->count = 0;
        char *lasttok = NULL;
        char *token = strtok_r(input, "|", &lasttok); //finds the first segment before the pipe

        while (token && chain->count < CMDS_MAX)
        {
                char *cmdstr = trim(token);
                int args_count = parse(cmdstr, &chain->commands[chain->count]);

                if (args_count < 0) //if there are too many args
                {
                        return -1;
                }
                if (args_count == 0) //if empty
                {
                        return 0;
                }

                chain->count++;
                token = strtok_r(NULL, "|", &lasttok);
        }
        if(token) //too many arguments
        {
                return -1;
        }
        return chain->count; //returns the number of args
}

int main(void)
{       //I think there are going to have to be static global variables since background is keeping track of 
        // which background jobs are running
        // Background jobs 
        static pid_t bg_pids[CMDS_MAX]; //keeps track of pids in the background
        static int bg_count = 0; //number of processes in the background
        static int bg_status[CMDS_MAX]; //exit statuses
        static char bg_cmd[CMDLINE_MAX]; 
        static int bg_active = 0; // bg flag

        char cmd[CMDLINE_MAX];
        char unedited_cmd[CMDLINE_MAX];
        char *eof;

        while (1) {
                char *nl;

                if (bg_active == 1)
                {
                        int all_done = 1;
                        int status;

                        for (int i = 0; i < bg_count; i++)
                        {
                                pid_t running = waitpid(bg_pids[i], &status, WNOHANG);

                                if(running == 0) //still running flag
                                {
                                        all_done = 0; // still running flag
                                }
                                else if (running > 0) //child has exited and we remember the exit code
                                {
                                        bg_status[i] = WEXITSTATUS(status); // exit code 
                                }
                        }

                        if (all_done) //check if all background processes have been completed done
                        {
                                fprintf(stderr, "+ completed '%s'", bg_cmd);
                                for (int i = 0; i < bg_count; i++)
                                {
                                        fprintf(stderr, "[%d]", bg_status[i]); 
                                }
                                fprintf(stderr, "\n");
                                bg_active = 0; // Reset background flag
                        }
                }

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

                //ignores blank lines
                if (cmd[0] == '\0')
                {
                        continue;
                }


                // detecting background &
                int background = 0;
                size_t len = strlen(cmd);
                if (len > 0 && cmd[len-1] == '&') 
                {
                        cmd[--len] = '\0'; // Remove '&'
                        while (len && isspace((unsigned char)cmd[len-1]))
                        {
                                cmd[--len] = '\0'; // Trim trailing spaces, got this from stack overflow https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
                        }
                        if (strchr(cmd, '&')) 
                        { // Extra '&' creates an error
                            fprintf(stderr, "Error: mislocated background sign\n");
                            continue;
                        }
                        background = 1;            // Mark as background job
                }
                
                // Prevent exit if background job still running
                if (!strcmp(cmd, "exit") && bg_active) {
                        fprintf(stderr, "Error: active job still running\n");
                        fprintf(stderr, "* completed '%s' [1]\n", cmd);
                        continue;
                }

                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        fprintf(stderr,"+ completed '%s' [0]\n", cmd);
                        break;
                }

                char *input_redir = strchr(cmd , '<'); 
                char *input_target = NULL;

                if(input_redir != NULL)
                {
                        *input_redir = '\0'; 
                        input_redir++; 

                        while (*input_redir == ' ' || *input_redir == '\t') 
                        {
                                input_redir++;
                        }
                        if (*input_redir == '\0') 
                        {
                                fprintf(stderr, "Error: no output file\n");
                                continue;
                        }

                        input_target = input_redir; 

                        if(strchr(input_redir, '|') != NULL) 
                        {
                                fprintf(stderr, "Error: mislocated output redirection\n");
                                continue;
                        }    
                }

                char *output_redir = strchr(cmd , '>'); //first occurance of > 
                char *output_target = NULL;

                if(output_redir != NULL)
                {
                        *output_redir = '\0'; // just means null
                        output_redir++; // this now points it to the output file name

                        while (*output_redir == ' ' || *output_redir == '\t') // checks for white spaces
                        //It'll continue to skip all the white spaces until it points at the first occurance of a character
                        // aka the first character of the file destination
                        {
                                output_redir++;
                        }
                        if (*output_redir == '\0') // checks if null aka if the first character is null then there is no file destination
                        {
                                fprintf(stderr, "Error: no output file\n");
                                continue;
                        }

                        output_target = output_redir; //sets the output file target to the right side of '>'

                        if(strchr(output_redir, '|') != NULL) //Checks if there are any misc. garbage on the right side
                        {
                                fprintf(stderr, "Error: mislocated output redirection\n");
                                continue;
                        }    
                }

                struct chain chaining;
                char pipeline_buffer[CMDLINE_MAX]; // creates a buffer for parsing
                strncpy(pipeline_buffer, cmd, CMDLINE_MAX); // cmd is being copied into the buffer

                int command_count = parse_chain(pipeline_buffer, &chaining); // get the number of commands from the buffer

                if(command_count < 0) 
                {
                        fprintf(stderr, "Error: too many process arguments\n");
                        continue;
                }
                if(command_count == 0)
                {
                        fprintf(stderr, "Error: missing command\n");
                        continue;
                }

                // handle cd and pwd commands
                // error seems to be based on where they were placed
                //need this for single commands too
                if(command_count == 1 && input_target == NULL  && output_target == NULL)
                {
                        struct command *cmdptr = &chaining.commands[0];
                        if(strcmp(cmdptr->argv[0], "cd") == 0) //checks cd
                        {
                                int exit_status = 0;
                                if (chdir(cmdptr->argv[1]) != 0)
                                {
                                        fprintf(stderr, "Error: cannot cd into directory\n");
                                        exit_status = 1;
                                }
                                fprintf(stderr, "+ completed '%s' [%d]\n", unedited_cmd, exit_status);
                                continue;
                        }
                        else if (strcmp(cmdptr->argv[0], "pwd") == 0) //checks for pwd
                        {
                                char cwd[CMDLINE_MAX];
                                if (getcwd(cwd, sizeof(cwd)) != NULL)
                                {
                                        printf("%s\n", cwd);
                                        fflush(stdout);
                                        fprintf(stderr, "+ completed '%s' [0]\n", unedited_cmd);
                                }
                                continue;
                        }
                }

                int pipes[CMDS_MAX - 1][2]; // holds FDs
                pid_t pids[CMDS_MAX]; //holds child PIDs

                for(int i = 0; i < command_count - 1; i++) // This initializes the pipes between each command
                {
                        if(pipe(pipes[i]) < 0)
                        {
                                perror("pipe");
                                exit(1);
                        }

                }

                for (int i = 0; i < command_count; i++)
                {
                        if((pids[i] = fork()) == 0) //Child
                        {
                                if(i > 0) // grabs the previous command if it is not the first command
                                {
                                        dup2(pipes[i-1][0], STDIN_FILENO);
                                }
                                // checks if it  is the last command
                                // if not then go to the next pipe
                                if(i < command_count - 1)
                                {
                                        dup2(pipes[i][1], STDOUT_FILENO);
                                }

                                // have to handle redirection
                                if(i == command_count - 1 && input_target != NULL)
                                {
                                        int fd_input = open(input_target, O_RDONLY);
                                        if (fd_input < 0)
                                        {
                                                fprintf(stderr, "Error: cannot open input file\n");
                                                exit(1);
                                        }
                                        dup2(fd_input, STDIN_FILENO);
                                        close(fd_input); 
                                }
                                //output redirection
                                if(i == command_count - 1 && output_target != NULL)
                                {
                                        int fd_output = open(output_target, O_WRONLY | O_CREAT | O_TRUNC, 0666); // Found this on stack overflow https://stackoverflow.com/questions/36253331/writing-my-own-linux-shell-i-o-redirect-function
                                        if (fd_output < 0)
                                        {
                                                fprintf(stderr, "Error: cannot open output file\n"); 
                                                exit(1);
                                        }
                                        dup2(fd_output, STDOUT_FILENO); //puts the output file descriptor into standard out
                                        close(fd_output); 
                                }
                                // close all the pipes and fds in child process
                                for(int j = 0; j < command_count - 1; j++)
                                {
                                        close(pipes[j][0]);
                                        close(pipes[j][1]);
                                }

                                execvp(chaining.commands[i].argv[0], chaining.commands[i].argv);
                                fprintf(stderr, "Error: command not found\n");
                                exit(1);
                        }
                }

                // parent process

                //close parent process pipes and fd
                for(int i = 0; i < command_count - 1; i++)
                {
                        close(pipes[i][0]);
                        close(pipes[i][1]);
                }

                //status handling
                if(!background)
                {
                        int statuses[CMDS_MAX]; //creates an array keeping track of all the child processes
                        int status;
                        for(int i = 0; i < command_count; i++)
                        {
                                waitpid(pids[i], &status, 0);
                                statuses[i] = WEXITSTATUS(status); //exit status code
                        }

                        if(bg_active) //checks if there are any bacgkround tasks
                        {
                                int done = 1;
                                int status2;
                                for (int j = 0; j < bg_count; j++)
                                {
                                        pid_t running = waitpid(bg_pids[j], &status2, WNOHANG); // waiting for every bgpid to finish
                                                                                                //WNOHANG - Flag that waitpid should return immediately 
                                                                                                // source: https://stackoverflow.com/questions/33508997/waitpid-wnohang-wuntraced-how-do-i-use-these
                                        if(running == 0)
                                        {
                                                done = 0;
                                        }
                                        else if (running > 0)
                                        {
                                                bg_status[j] = WEXITSTATUS(status2);
                                        }
                                }
                                if (done) //when it is done
                                {
                                        fprintf(stderr, "+ completed '%s' ", bg_cmd);
                                        for (int j = 0; j < bg_count; j++)
                                        {
                                                fprintf(stderr, "[%d]", bg_status[j]);
                                        }
                                        fprintf(stderr, "\n");
                                        bg_active=0;
                                }
                        }

                        //prints this when pid are done
                        fprintf(stderr, "+ completed '%s' ", unedited_cmd);
                        for (int i = 0; i < command_count; i++)
                        {
                                fprintf(stderr, "[%d]", statuses[i]);
                        }
                        fprintf(stderr, "\n");       
                }
                else
                {
                        // adds a new background job
                        bg_active = 1;
                        bg_count = command_count; //stores the number of commands in the background
                        strncpy(bg_cmd, unedited_cmd, CMDLINE_MAX); //copies the unedited command to the background
                        for(int i = 0; i < command_count; i++)
                        {
                                bg_pids[i] = pids[i]; //stores background job pids
                        }
                }
                continue; //continue onto next command loop.
        
        }

        return EXIT_SUCCESS;
}
