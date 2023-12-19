#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <assert.h>
#include <fcntl.h>
#include <ctype.h>

char *new_file = NULL;
char *path_directories[10] = {"/bin/", "/usr/bin/", NULL};
bool white_space = false;

void add_path(char **arguments)
{
    if (arguments[1] == NULL)
    {
        for (int i = 0; i < 10; i++)
        {
            path_directories[i] = NULL;
        }
    }
    else
    {
        for (int i = 1; arguments[i] != NULL; i++)
        {
            if (i - 1 < 10)
            {
                char *ptr = arguments[i];
                bool closed = false;
                while (*ptr != '\0')
                {
                    closed = false;
                    ptr++;
                    if (*ptr == '/')
                    {
                        closed = true;
                        ptr++;
                        if (*ptr == '\0')
                        {
                            break;
                        }
                    }
                }
                if (!closed)
                {
                    char *temp = strdup(arguments[i]);
                    path_directories[i - 1] = strdup(strcat(temp, "/"));
                }
                else
                {
                    path_directories[i - 1] = strdup(arguments[i]);
                }
            }
        }
    }
}

void error()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

char *find(char *string)
{
    char *ptr = string;
    int length = 0;
    while (*ptr != '\0')
    {
        ptr++;
        length++;
        if (*ptr == '>')
        {
            char *substring = (char *)malloc(length + 1);
            strncpy(substring, string + (length + 1), strlen(string) - length);
            new_file = strdup(substring);
            strncpy(substring, string, length);
            return substring;
        }
    }
    return NULL;
}

void run_commands(char input[])
{
    size_t length = strlen(input);
    if (length > 0 && input[length - 1] == '\n') {
        input[length - 1] = '\0';
        length--;
    }
    if(length == 0){
        return;
    }
    else{

        if (strcspn(input, "\n") == 0)
        {
            error();
            return;
        }
        char *white = input;
        if (isspace(*white))
        {
            if (white_space)
            {
                return;
            }
            printf("test variable whitespace!\n");
            white_space = true;
            return;
        }
        input[strcspn(input, "\n")] = '\0';

        char **command_groups = (char **)malloc(24 * sizeof(char *));
        int num_command_groups = 0;
        char *token = strtok(input, "&");
        while (token != NULL)
        {
            command_groups[num_command_groups] = token;
            num_command_groups++;
            token = strtok(NULL, "&");
        }
        for (int com_number = 0; com_number < num_command_groups; com_number++)
        {
            char *command = strtok(command_groups[com_number], " "); 
            char **arguments = (char **)malloc(21 * sizeof(char *)); 
            arguments[0] = command;

            for (int i = 1; i < 20; i++)
            {
                arguments[i] = strtok(NULL, " ");
                if (arguments[i] == NULL)
                {
                    break;
                }
                if (strcmp(arguments[i], ">") == 0)
                {
                    arguments[i] = NULL;
                    new_file = strtok(NULL, " ");
                    if (new_file == NULL)
                    {
                        error();
                        return;
                    }
                    arguments[i] = strtok(NULL, " ");
                    if (arguments[i] != NULL)
                    {
                        error();
                        return;
                    }
                    break;
                }
                char *new_a = find(arguments[i]);
                if (new_a != NULL)
                {
                    arguments[i] = new_a;
                }
            }
            arguments[20] = NULL;

            // built in commands

            if (strcmp(input, "exit") == 0)
            {
                if (arguments[1] != NULL)
                {
                    error();
                    return;
                }
                else
                {
                    exit(0);
                }
            }
            else if (strcmp(input, "cd") == 0)
            {
                if (arguments[2] != NULL)
                {
                    error();
                }
                else if (chdir(arguments[1]) == -1)
                {
                    error();
                }
            }
            else if (strcmp(input, "path") == 0)
            {
                add_path(arguments);
            }

            // path commands
            else if (path_directories[0] != NULL)
            {
                for (int i = 0; path_directories[i] != NULL; i++)
                {
                    int length = strlen(path_directories[i]) + strlen(command) + 1;
                    char *access_ = (char *)malloc(length);
                    strcpy(access_, path_directories[i]);
                    strcat(access_, command);
                    if (access(access_, X_OK) == 0)
                    {
                        pid_t child_pid = fork();
                        if (child_pid == -1)
                        {
                            error();
                            return;
                        }
                        if (child_pid == 0)
                        {
                            if (new_file != NULL)
                            {
                                int output_fd = open(new_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                                if (output_fd == -1)
                                {
                                    error();
                                    exit(1);
                                }
                                dup2(output_fd, 1);
                                dup2(output_fd, 2);
                                close(output_fd);
                            }
                            execvp(access_, arguments);
                        }
                        else
                        {
                            int status;
                            wait(&status);
                            new_file = NULL;
                        }
                        break;
                    }
                    else if (path_directories[i + 1] == NULL){
                        error();
                        break;
                    }
                    else{
                        continue;
                    }
                    free(access_);
                }
            }
            else
            {
                error();
            }
            free(arguments);
        }
    }
}

void with_arguments(char *filename)
{

    if (access(filename, F_OK) == -1)
    {
        error();
        return;
    }
    else
    {
        FILE *file;
        file = fopen(filename, "r");
        if (file == NULL)
        {
            error();
        }
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), file) != NULL)
        {
            run_commands(buffer);
        }
        fclose(file);
    }
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            char *filename = argv[i];
            with_arguments(filename);
        }
    }
    else
    {
        while (1)
        {
            char input[256];
            printf("witshell> ");
            if (fgets(input, sizeof(input), stdin) == NULL)
            {
                exit(0);
            }
            run_commands(input);
        }
    }
    return 0;
}