#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
#define MAX_BG_PROCS 64

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
/////////////////////////////////////////////////////////////////////////////////
 
		bzero(line, sizeof(line));

        if (getcwd(cwd,256) != NULL)
        {
            printf("%s $ ", cwd);
        }
        else
        {
            printf("<getcwd error>%s $ ", cwd);
        }
        
/////////////////////////////////////////////////////////////////////////////////

		scanf("%[^\n]", line);
		getchar();

		line[strlen(line)] = '\n';
		tokens = tokenize(line);

/////////////////////////////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////////////////////////////

        int background = 0;
        int last_token = 0;
        while (tokens[last_token] != NULL) last_token++;

        if (last_token > 0 && strcmp(tokens[last_token - 1], "&") == 0) 
        {
            background = 1;
            free(tokens[last_token - 1]);
            tokens[last_token - 1] = NULL;
        }

/////////////////////////////////////////////////////////////////////////////////

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
        
        int pid = fork();
    
        if(pid < 0)
        {
            printf("Error forking");
        }
        else if(pid == 0)
        {
            if(execvp(tokens[0],tokens) == -1)
            {
                printf("execvp error : Invalid Command\n");
                exit(1);
            }
        }
        else
        {
            if (!background) 
            {
                int status;
                waitpid(pid, &status, 0);
                // if (WIFEXITED(status)) {
                //     printf("EXITSTATUS: %d\n", WEXITSTATUS(status));
                // }
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
        
	 	for(i=0;tokens[i]!=NULL;i++){
	 		free(tokens[i]);
	 	}
	 	free(tokens);

	}
    free(cwd);
	return 0;
}
