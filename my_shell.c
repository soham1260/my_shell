#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

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

	while(1) {			
        
		bzero(line, sizeof(line));

        if (getcwd(cwd,256) != NULL)
        {
            printf("%s $ ", cwd);
        }
        else
        {
            printf("<getcwd error>%s $ ", cwd);
        }
		scanf("%[^\n]", line);
		getchar();

		line[strlen(line)] = '\n';
		tokens = tokenize(line);
        
        if (tokens[0] != NULL && strcmp(tokens[0], "cd") == 0) 
        {
            if (tokens[1] == NULL || chdir(tokens[1]) != 0) 
            {
                printf("cd error\n");
            }
            continue;
        }
        else
        {
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
                int status;
                wait(&status);
                if (WIFEXITED(status))
                {
                    printf("EXITSTATUS: %d\n", WEXITSTATUS(status));
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