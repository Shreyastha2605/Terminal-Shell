#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>   
#include <fcntl.h>
#include <errno.h>

// Structure to hold parsed command information
typedef struct {
    char **argv;
    int argc;
    char *in_file;
    char *out_file;
} ShellCommand;

/* Function Prototypes */
void print_prompt();
ShellCommand parse_command(char *input);
void execute_command(ShellCommand *cmd);
void free_command(ShellCommand *cmd);


/* MAIN FUNCTION */
int main() {
    char input[1024];

    while (1) {
        print_prompt();

        // Read user input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;  // Handle Ctrl+D (EOF)
        }

        // Remove trailing newline
        input[strcspn(input, "\n")] = 0;

        // Skip empty input
        if (strlen(input) == 0)
            continue;

        // Parse the input into command structure
        ShellCommand cmd = parse_command(input);

        // If no valid command, skip
        if (cmd.argv[0] == NULL) {
            free_command(&cmd);
            continue;
        }

        // Execute the parsed command
        execute_command(&cmd);

        // Free allocated memory
        free_command(&cmd);
    }

    return 0;
}


/*
 * Prints the shell prompt
 */
void print_prompt() {
    printf("TechShell> ");
    fflush(stdout);
}


/** Parses input string into:*/

ShellCommand parse_command(char *input) {
    ShellCommand cmd;

    cmd.argv = malloc(sizeof(char*) * 100);
    cmd.argc = 0;
    cmd.in_file = NULL;
    cmd.out_file = NULL;

    char *token = strtok(input, " ");

    while (token != NULL) {

        // Handle input redirection
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            if (token)
                cmd.in_file = strdup(token);
        }
        // Handle output redirection
        else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            if (token)
                cmd.out_file = strdup(token);
        }
        // Normal argument
        else {
            cmd.argv[cmd.argc++] = strdup(token);
        }

        token = strtok(NULL, " ");
    }

    cmd.argv[cmd.argc] = NULL;  // Required for execvp
    return cmd;
}


/*
 * Executes a parsed command.
 */
void execute_command(ShellCommand *cmd) {

    // Exit command to quit techshell
    if (strcmp(cmd->argv[0], "exit") == 0) {
        exit(0);
    }

    // cd must run in parent process
    if (strcmp(cmd->argv[0], "cd") == 0) {
        if (cmd->argv[1] == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } else {
            if (chdir(cmd->argv[1]) != 0) {
                fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
            }
        }
        return;
    }

    // Creates child process
    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
        return;
    }

    if (pid == 0) {
        // CHILD PROCESS

        // Handle input redirection
        if (cmd->in_file) {
            int fd = open(cmd->in_file, O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
                exit(1);
            }

            // Redirect STDIN to file
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        // Handle output redirection
        if (cmd->out_file) {
            int fd = open(cmd->out_file,
                          O_WRONLY | O_CREAT | O_TRUNC,
                          0644);
            if (fd < 0) {
                fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
                exit(1);
            }

            // Redirect STDOUT to file
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // Replace child process with requested program
        execvp(cmd->argv[0], cmd->argv);

        // If execvp fails
        fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
        exit(1);
    }
    else {
        // PARENT PROCESS waits for child to finish
        waitpid(pid, NULL, 0);
    }
}


/*
 * Frees dynamically allocated memory
 */
void free_command(ShellCommand *cmd) {

    for (int i = 0; i < cmd->argc; i++) {
        free(cmd->argv[i]);
    }

    free(cmd->argv);

    if (cmd->in_file)
        free(cmd->in_file);

    if (cmd->out_file)
        free(cmd->out_file);
}
