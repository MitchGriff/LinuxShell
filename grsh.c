#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

char *env_args[4] = {"PATH=/bin", NULL, NULL};

char *readLine(void){
	char *line = NULL;
	ssize_t bufsize = 0;
	getline(&line, &bufsize, stdin);
	return line;
}

#define GRSH_TOK_BUFSIZE 64
#define GRSH_TOK_DELIM " \t\r\n\a"

char **splitLine(char *line) {
	int bufsize = GRSH_TOK_BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token;

	if(!tokens) {
		fprintf(stderr, "grsh: allocation error\n");
		exit(0);
	}

	token = strtok(line, GRSH_TOK_DELIM);
	while(token != NULL){
		tokens[position] = token;
		position++;

		if (position >= bufsize) {
			bufsize += GRSH_TOK_BUFSIZE;
			tokens = realloc(tokens, bufsize * sizeof(char*));
			if(!tokens){
				fprintf(stderr, "grsh: allocation error\n");
				exit(0);
			}
		}
		token = strtok(NULL, GRSH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

int launch(char **args){
	pid_t pid, wpid;
	int status;

	pid = fork();
	if(pid == 0) {
//		printf("---------------------RIGHT BEFORE EXECVP");
		if(execvp(args[0], args) == -1) {
			perror("grsh");
		}
		exit(0);
	}
	else if(pid < 0){
		perror("grsh");
	}
	else {
		do {
//			printf("----------HELLO-----------\n");
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 1;
}

//Built in functions
int grsh_cd(char **args);
int grsh_exit(char **args);
int grsh_path(char **args);

char *builtin_str[] = {
	"cd",
	"exit"
	,"path"
};

int (*builtin_func[]) (char **) = {
	&grsh_cd,
	&grsh_exit
	,&grsh_path
};

int num_builtins() {
	return sizeof(builtin_str) / sizeof(char *);
}

int grsh_cd(char **args){
	if(args[1] == NULL) {
		fprintf(stderr, "grsh: expected argument to \"cd\"\n");
	}
	else {
		if(chdir(args[1]) != 0) {
			perror("grsh");
		}
	}
	return 1;
}

int grsh_exit(char **args) {
	exit(0);
}

int grsh_path(char **args) {
	if(args[1] == NULL){
		env_args[0] = "";
	}
	else {
		int pos = 1;
		while(args[pos] != NULL){
			env_args[0] = args[pos];
			pos++;
		}
	}
	return 1;
}

//batch mode

int batchMode(char *args){
	FILE *fptr;
	fptr = fopen(args, "r");
//	printf("filename: %s", args);
	char **lineargs;
	char *line_buf = NULL;
	size_t line_buf_size = 0;
	int line_count = 0;
	ssize_t line_size;
	int temp = 0;

	line_size = getline(&line_buf, &line_buf_size, fptr);

	while(line_size >= 0){
//		printf("----------------------------------------HELLO");
		lineargs = splitLine(line_buf);
//        printf("----------------------------------------BEFORE EXECCUTE\n");

		temp = execute(lineargs);
//	printf("-----------------------------------------AFTER EXECUTE\n");
		line_size = getline(&line_buf, &line_buf_size, fptr);
	}
	free(lineargs);
	free(line_buf);
	fclose(fptr);
	exit(1);
}

//executing built in commands

int execute(char **args) {
	int i;

	if(args[0] == NULL) {
		return 1;
	}

	for(i = 0; i < num_builtins(); i++){
		if(strcmp(args[0], builtin_str[i]) == 0){
			return (*builtin_func[i])(args);
		}
	}
	return launch(args);
}

void loop(void) {
	char *line;
	char **args;
	int status;

	do {
		printf("grsh> ");
		line = readLine();
		args = splitLine(line);
		status = execute(args);

		free(line);
		free(args);

	} while(status);
}

int main(int argc, char **argv){


	if(argc == 1){
		loop();
	}
	else {
		batchMode(argv[1]);
	}

	return 0;
}
