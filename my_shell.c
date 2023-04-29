#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
#define MAX_NUM_COMMANDS 64
#define MAX_BG_PROCESSES 50

pid_t bg_processes[MAX_BG_PROCESSES];
int num_bg_processes = 0;
extern char **environ;

// FUNCTIONS FOR CMD
void clear() {
	printf("\e[1;1H\e[2J");
}

void processQuit() {
	pid_t pid;
    int status;

    // send SIGTERM signal to all child processes
    for(int i=0; i<num_bg_processes; i++) {
        pid = bg_processes[i];
        if(pid > 0) {
            kill(pid, SIGTERM);
        }
    }

    // wait for all child processes to terminate
    while(wait(&status) > 0);
exit(0);
}

void env(){
    int i;
    for (i=0; environ[i] != NULL; i++)
        printf("%s\n", environ[i]);
}

void dir (char **tokens) {
	char a[150]="ls -la ";
    system(strcat(a,tokens[1]));
}
void pwd() {
	char cwd[MAX_INPUT_SIZE];
	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		fprintf(stderr, "Error: Failed to get current working directory\n");
		exit(1);
	}
}

void cd(char *fpath) { 
	char *env = (char *)malloc(sizeof(char)*150);
    getcwd(env,150);
    if(fpath && chdir(fpath)!=0){
        printf("Error: Invalid Path\n");
        return;
    }
    if(fpath){
        setenv("Old", env,1);
        getcwd(env,150);
        setenv("pwd", env,1);
        
    }
    else {
		setenv("Old",env,1);
	}
    free(env);
}

void io_Redirect(char **tokens, int n)
{
	FILE *f_p;
	char **args = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	int j = 0;
	for (int i = 0; strcmp(tokens[i], ">>") && strcmp(tokens[i], ">") && strcmp(tokens[i], "<"); i++)
	{
		args[j] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
		strcpy(args[j++], tokens[i]);
	}
	args[j] = NULL;
	char *env = (char *)malloc(sizeof(char) * 150);
	switch(fork()) {
		case -1:
			break;
		case 0:
			if (!strcmp(tokens[n - 2], ">>"))
				f_p = freopen(tokens[n - 1], "a", stdout);
			else if (!strcmp(tokens[n - 2], ">"))
				f_p = freopen(tokens[n - 1], "w", stdout);
			else
				f_p = freopen(tokens[n - 1], "r", stdin);
			strcpy(env, "/bin/");
			strcat(env, args[0]);
			if (execv(env, args) == -1)
			{
				strcpy(env, "/usr/bin");
				strcat(env, args[0]);
				execv(env, args);
			}
			free(env);

	}
	wait(NULL);
	for (int i = 0; args[i] != NULL; i++)
		free(args[i]);

	free(args);
}

// & case
void background(char **tokens, int n) {
	char **args = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	int k=0,fork_calls=0;
	for(int i=0 ; i<n; i++){
		while(tokens[i]!=NULL && strcmp(tokens[i],"&")!=0){
				args[k] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
				strcpy(args[k++], tokens[i++]);
		}
		args[k] = NULL;
		fork_calls++;
		char *env = (char *)malloc(sizeof(char)*150);

		pid_t pid = fork();
		bg_processes[num_bg_processes++] = pid;
		if (pid == -1) {
			num_bg_processes--;
			// remove pid from bg_processes array
			break;
		}
		else if (pid == 0) {
			num_bg_processes--;
			// remove pid from bg_processes array
			printf("Shell: Background process finished");
			strcpy(env,"/usr/");
			strcat(env, args[0]);
			if (execv(env, args) == -1)
			{
				strcpy(env,"/usr/local/bin/");
				strcat(env, args[0]);
				execv(env, args);
			}
			free(env);
	}

		k=0;
	}
	for (int i = 0; args[i] != NULL; i++)
	{
		free(args[i]);
	}
	free(args);
}

// &&
void serial(char **tokens,int n){
    char **args = (char **)malloc(MAX_NUM_TOKENS*sizeof(char *));
	int count=0;
	for(int i=0;i<n;i++){
		int io=0;
		while(tokens[i]!=NULL && strcmp(tokens[i],"&&")!=0){
			if(!io)
				io=strcmp(tokens[i],">>")==0 || strcmp(tokens[i],">")==0 || strcmp(tokens[i],"<")==0;
			args[count] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
			strcpy(args[count++],tokens[i++]);
		}
		args[count]=NULL;
		char *env = (char *)malloc(sizeof(char)*150);

		if(!io){
			pid_t pid = fork();
			if (pid == 0) {
				strcpy(env,"/bin/");
				strcat(env,args[0]);
				if(execv(env,args)==-1){
					strcpy(env,"/usr/bin/");
					strcat(env,args[0]);
					execv(env,args);
				}
				free(env);
			}
			else if (pid == -1) {
				// error occurred
			}
			else {
				wait(NULL);
			}
		}
		else{
			io_Redirect(args, count);
		}
		count=0;
	}
	for(int i=0;args[i]!=NULL;i++) {
		free(args[i]);
	}
		

	free(args);

}

// &&&
void parallel(char **tokens,int n){
	char **args = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	int j = 0;
	for (int i = 0; i < n; i++) {
		int io_flag = 0;
		while (tokens[i] != NULL && strcmp(tokens[i], "&&&") != 0) {
			if (!io_flag) 
				io_flag = strcmp(tokens[i], ">>") == 0 || strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "<") == 0;
			args[j] = (char*)malloc(MAX_TOKEN_SIZE * sizeof(char));
			strcpy(args[j++], tokens[i++]);
		}
		args[j] = NULL;
		char *env = (char *)malloc(sizeof(char) * 150);
		if (!io_flag) {
			pid_t pid = fork();
			if (pid == 0) {
					strcpy(env, "/bin/");
					strcat(env, args[0]);
					if (execv(env, args) == -1) {
						strcpy(env, "/usr/bin/");
						strcat(env, args[0]);
						execv(env, args);
					}
					free(env);
			} else if(pid == -1) {
				break;
			}
		} else {
			io_Redirect(args, j);
		}
		j = 0;
	}
	for (int i = 0; i < MAX_NUM_TOKENS; i++) {
		if (args[i] != NULL) {
			free(args[i]);
		}
	}
	free(args);

}
/* Splits the string by space and returns the array of tokens
 *
 */
char **tokenize(char *line)
{
	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
	int i, tokenIndex = 0, tokenNo = 0;

	for (i = 0; i < strlen(line); i++)
	{

		char readChar = line[i];

		if (readChar == ' ' || readChar == '\n' || readChar == '\t')
		{
			token[tokenIndex] = '\0';
			if (tokenIndex != 0)
			{
				tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0;
			}
		}
		else
		{
			token[tokenIndex++] = readChar;
		}
	}

	free(token);
	tokens[tokenNo] = NULL;
	return tokens;
}

// my original idea for commands, but not used anymore
/* void execute_commands(char **tokens)
{
	char **prev_tokens = NULL;
	int i = 0;
	int j = 0;
	int background = 0;

	for (j = 0; tokens[j] != NULL; j++)
	{
		if (strcmp(tokens[j], "&") == 0) {
			background = 1;
		}
	}
	for (i = 0; tokens[i] != NULL; i++)
	{
		// Check if a token is '&', '&&', or '&&&'
		// Execute the previous command(s)
		if (prev_tokens != NULL)
		{
			if (background == 1)
			{
				// Background execution
				pid_t pid = fork();
				if (pid < 0)
				{
					fprintf(stderr, "Error: Failed to fork process\n");
					exit(1);
				}
				else if (pid == 0)
				{
					// Child process
					execvp(prev_tokens[0], prev_tokens);
					fprintf(stderr, "Error: Failed to execute command '%s'\n", prev_tokens[0]);
					exit(1);
				}
				else
				{
					// Parent process
					printf("Background process started with PID %d\n", pid);
					prev_tokens = NULL;
				}
			}
			
		}
		else
		{
			// Add token to previous command
			prev_tokens = tokens + i;
		}
		}
	}
*/


void sigintHandler(int sig) {
    processQuit();
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigintHandler);

    char line[MAX_INPUT_SIZE];
    char **tokens = NULL;
    int i;
    FILE *fp;
    if (argc == 2)
    {
        fp = fopen(argv[1], "r");
        if (fp < 0)
        {
            printf("File doesn't exists.");
            return -1;
        }
    }

    while (1)
    {
        int i_o, prl, srl, bg;
        /* BEGIN: TAKING INPUT */
        memset(line, 0, MAX_INPUT_SIZE); //// clears the line
        if (argc == 2)
        { // batch mode
            if (fgets(line, sizeof(line), fp) == NULL)
            { // file reading finished
                return 0;
            }
            line[strlen(line) - 1] = '\0';
        }
        else
        { // interactive mode
            pwd();
            printf("$ ");
            scanf("%[^\n]", line);
            getchar();
        }
        /* END: TAKING INPUT */

        line[strlen(line)] = '\n'; // terminate with new line
        tokens = tokenize(line);
        //execute_commands(tokens);
        // do whatever you want with the commands, here we just print them

        for(i=0;tokens[i]!=NULL;i++){
            //printf("found token %s (remove this debug output later)\n", tokens[i]);
            if(!bg)
                bg=strcmp(tokens[i],"&")==0;
            if (!srl)
                srl = strcmp(tokens[i], "&&") == 0;
            if(!prl)
                prl=strcmp(tokens[i],"&&&")==0;
            if(!i_o)
                i_o=strcmp(tokens[i],">")==0 || strcmp(tokens[i], "<")==0 || strcmp(tokens[i],">>")==0;
        }
        if(prl)
            parallel(tokens, i);
        else if(srl)
            serial(tokens, i);
        else if(bg)
            background(tokens, i);  // pass counter variable by reference
        else if(i_o)
            io_Redirect(tokens, i);
        else{
            if(!strcmp(tokens[0],"dir")){
                dir(tokens);
            }
            else if(!strcmp(tokens[0], "cd")){
                cd(tokens[1]);
            }
            else if(!strcmp(tokens[0],"clear")){
                clear();
            }
            else if (!strcmp(tokens[0], "quit"))
            {
                processQuit();
                num_bg_processes = 0;  // reset counter variable
            }
            else if(!strcmp(tokens[0],"env")){
                env();
            }
            else{
                system(line);
            }
        }
       
        // Freeing the allocated memory
        for(i=0;tokens[i]!=NULL;i++){
            free(tokens[i]);
        }
        free(tokens);

    }
    return 0;
}

