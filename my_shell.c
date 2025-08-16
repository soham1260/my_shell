#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
#define MAX_BG_PROCS 64

int fg_gid=-1;

void sigint_handler(int signo) 
{
    if (fg_gid > 0) 
    {
        kill(-fg_gid, SIGINT);
    }
}

char **tokenize(char *line)
{
    char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
    char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
    int i, tokenIndex = 0, tokenNo = 0;

    for(i =0; i < strlen(line); i++){

        char readChar = line[i];
        
        if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
            token[tokenIndex] = '\0';
            if (tokenIndex != 0){
            	tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
            	strcpy(tokens[tokenNo++], token);
            	tokenIndex = 0; 
            }
        } else {
            token[tokenIndex++] = readChar;
        }
    }
  
    free(token);
    tokens[tokenNo] = NULL ;
    return tokens;
}


int main(int argc, char* argv[]) {

    signal(SIGINT, sigint_handler);

	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;
    char* cwd = (char*)malloc(256);

    int bg_pids[MAX_BG_PROCS];
    for (int i = 0; i < MAX_BG_PROCS; i++) {
        bg_pids[i] = -1;
    }

    int bg_count = 0;

	while(1) {

///////////////////////////////////////////////////////////////////////////////// Reap bg processes
        
        for (int i = 0; i < MAX_BG_PROCS; i++) 
        {
            if (bg_pids[i] == -1) continue;
            int status;
            int res = waitpid(bg_pids[i], &status, WNOHANG);
            if (res > 0) 
            {
                printf("\n[Background Done] PID %d exited with code %d\n", res, WEXITSTATUS(status));
                bg_pids[i] = -1;
                bg_count--;
            }
        }
        
///////////////////////////////////////////////////////////////////////////////// get pwd of shell
 
		bzero(line, sizeof(line));

        if (getcwd(cwd,256) != NULL)
        {
            printf("%s $ ", cwd);
        }
        else
        {
            printf("<getcwd error>%s $ ", cwd);
        }
        
///////////////////////////////////////////////////////////////////////////////// get new command and tokenize

		scanf("%[^\n]", line);
		getchar();

		line[strlen(line)] = '\n';
		tokens = tokenize(line);

///////////////////////////////////////////////////////////////////////////////// handle exit command

        if (tokens[0] != NULL && strcmp(tokens[0], "exit") == 0) 
        {
            for(int i=0;i<MAX_BG_PROCS;i++) 
            {
                if (bg_pids[i] != -1 && kill(bg_pids[i], 0) == 0) kill(bg_pids[i], SIGTERM);
            }
            for(int i=0;i<MAX_BG_PROCS;i++) 
            {
                if (bg_pids[i] != -1) waitpid(bg_pids[i], NULL, 0);
            }
            for (int i = 0; tokens[i] != NULL; i++) 
            {
                free(tokens[i]);
            }
            free(tokens);
            exit(0);
        }

///////////////////////////////////////////////////////////////////////////////// handle cd command

        if (tokens[0] != NULL && strcmp(tokens[0], "cd") == 0) 
        {
            if (tokens[1] == NULL || chdir(tokens[1]) != 0) 
            {
                printf("cd error\n");
            }
            for (int i = 0; tokens[i] != NULL; i++) 
            {
                free(tokens[i]);
            }
            free(tokens);
            continue;
        }

///////////////////////////////////////////////////////////////////////////////// handle &&

        int i = 0;
        int num_serial_commands = 0;

        while(tokens[i] != NULL)
        {
            if(strcmp(tokens[i],"&&") == 0) num_serial_commands++;
            i++;
        }
        
        char*** commands = NULL;
        if(num_serial_commands)
        {
            num_serial_commands++;
            commands = (char ***)malloc(num_serial_commands * sizeof(char **));
            i=0;
            int j=0;
            while(i<num_serial_commands)
            {
                int k=0;
                char **individual_command = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
                while(tokens[j] != NULL && strcmp(tokens[j],"&&") != 0)
                {
                    char *cmd_token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
                    strcpy(cmd_token,tokens[j]);
                    individual_command[k++] = cmd_token;
                    j++;
                }
                individual_command[k] = NULL;
                commands[i]=individual_command;
                if (tokens[j] != NULL && strcmp(tokens[j], "&&") == 0) j++;
                i++;
            }
        }

///////////////////////////////////////////////////////////////////////////////// handle &&&

        i = 0;
        int num_parallel_commands = 0;

        while(tokens[i] != NULL)
        {
            if(strcmp(tokens[i],"&&&") == 0) num_parallel_commands++;
            i++;
        }
        
        if(num_parallel_commands)
        {
            num_parallel_commands++;
            commands = (char ***)malloc(num_parallel_commands * sizeof(char **));
            i=0;
            int j=0;
            while(i<num_parallel_commands)
            {
                int k=0;
                char **individual_command = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
                while(tokens[j] != NULL && strcmp(tokens[j],"&&&") != 0)
                {
                    char *cmd_token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
                    strcpy(cmd_token,tokens[j]);
                    individual_command[k++] = cmd_token;
                    j++;
                }
                individual_command[k] = NULL;
                commands[i]=individual_command;
                if (tokens[j] != NULL && strcmp(tokens[j], "&&&") == 0) j++;
                i++;
            }
        }

///////////////////////////////////////////////////////////////////////////////// check & for bg execution

        int background = 0;
        int last_token = 0;
        while (tokens[last_token] != NULL) last_token++;

        if (last_token > 0 && strcmp(tokens[last_token - 1], "&") == 0) 
        {
            background = 1;
            free(tokens[last_token - 1]);
            tokens[last_token - 1] = NULL;
        }
        
///////////////////////////////////////////////////////////////////////////////// command execution

        if (background && bg_count == MAX_BG_PROCS) 
        {
            printf("Too many background processes!\n");
            background = 0;
            for (int i = 0; tokens[i] != NULL; i++) 
            {
                free(tokens[i]);
            }
            free(tokens);
            continue;
        }
        

///////////////////////////////////////////////////////////////////////////////// command execution serial

        if(num_serial_commands)
        {
            for (int c = 0; c < num_serial_commands; c++) 
            {
                int pid = fork();
                if (pid == 0) 
                {
                    if (setpgid(0, 0) == -1) 
                    {
                        printf("setpgid error");
                        exit(1);
                    }
                    if(execvp(commands[c][0], commands[c]) == -1)
                    {
                        printf("execvp error : Invalid Command\n");
                        exit(1);
                    }
                }
                else 
                {
                    fg_gid = pid;
                    setpgid(pid, pid);
                    int status;
                    waitpid(pid, &status, 0);
                    fg_gid = -1;
                }
            }
            for (int c = 0; c < num_serial_commands; c++) 
            {
                for (int k = 0; commands[c][k] != NULL; k++) 
                {
                    free(commands[c][k]);
                }
                free(commands[c]);
            }
            free(commands);
        }

///////////////////////////////////////////////////////////////////////////////// command execution parallel

        else if(num_parallel_commands)
        {
            int pids[num_parallel_commands];

            for (int c = 0; c < num_parallel_commands; c++) 
            {
                int pid = fork();
                if(pid == 0) 
                {
                    if(fg_gid == -1) 
                    {
                        if (setpgid(0, 0) == -1) 
                        {
                            perror("setpgid error");
                            exit(1);
                        }
                    }
                    else  
                    {
                        if(setpgid(0, fg_gid) == -1)
                        {
                            printf("setpgid error");
                            exit(1);
                        }
                    }
                    if(execvp(commands[c][0], commands[c]) == -1)
                    {
                        printf("execvp error : Invalid Command\n");
                        exit(1);
                    }
                }
                else 
                {
                    if (fg_gid == -1) 
                    {
                        fg_gid = pid;
                    }
                    setpgid(pid, fg_gid);
                    pids[c] = pid;
                }
            }
            int status;
            for(int c = 0; c < num_parallel_commands; c++) 
            {
                waitpid(pids[c], &status, 0);
            }
            fg_gid = -1;
            for(int c = 0; c < num_parallel_commands; c++) 
            {
                for(int k = 0; commands[c][k] != NULL; k++) 
                {
                    free(commands[c][k]);
                }
                free(commands[c]);
            }
            free(commands);
        }

///////////////////////////////////////////////////////////////////////////////// command execution normal

        else
        {
            int pid = fork();
    
            if(pid < 0)
            {
                printf("Error forking");
            }
            else if(pid == 0)
            {
                if (setpgid(0, 0) == -1) 
                {
                    printf("setpgid error");
                    exit(1);
                }
                if(execvp(tokens[0],tokens) == -1)
                {
                    printf("execvp error : Invalid Command\n");
                    exit(1);
                }
            }
            else
            {
                setpgid(pid, pid);
                if (!background) 
                {
                    fg_gid = pid;
                    int status;
                    waitpid(pid, &status, 0);
                    fg_gid = -1;
                } 
                else 
                {
                    printf("[Background] PID: %d\n", pid);
                    int i=0;
                    while(i<MAX_BG_PROCS)
                    {
                        if (bg_pids[i] == -1)
                        {
                            bg_pids[i] = pid;
                            bg_count++;
                            break;
                        }
                        i++;
                    }
                }
            }
        }

///////////////////////////////////////////////////////////////////////////////// free memory    
        
	 	for(i=0;tokens[i]!=NULL;i++){
	 		free(tokens[i]);
	 	}
	 	free(tokens);

	}
    free(cwd);
	return 0;
}
