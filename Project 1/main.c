/*
main.c must be able to start in both of the following modes:
a. File mode:
    i. 
        File mode is called by using the -f flag and passing it a filename
        (e.g. pseudo-shell -f <filename>)
    ii. 
        The shell only accepts filenames in file mode.
    iii.
        All input is taken from the file whose name is specified by <filename>
        and all output is written to the file “output.txt”. There should be no
        console output in this mode. Place all output in the file.
b. Interactive mode:
    i. 
        If no command line argument is given the project will start in interactive
        mode.
    ii. 
        The program accepts input from the console and writes output from the
        console.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "command.h"
#include "string_parser.h"

// function declarations
void interactive_mode();
void file_mode(const char *filename);

int main(int argc, char const *argv[])
{
    // identify which mode to open in
    // no arguments = interactive mode
    if (argc == 1) {
        interactive_mode();
    }
    // -f <filename> = file mode
    else if ((argc == 3) && (strcmp(argv[1], "-f") == 0)) {
        file_mode(argv[2]);
    }
    // error check for improper arguments
    else {
        fprintf(stderr, "Error Improper Usage: \n"
                "File Mode: pseudo-shell -f <filename> \n"
                "Interactive Mode: pseudo-shell\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

void interactive_mode() {
    char *line = NULL;
    size_t len = 0;

    printf(">>> ");  // prompt for user input

    while (getline(&line, &len, stdin) != -1) {
        // strip newline character
        line[strcspn(line, "\n")] = 0; 
        //printf("you wrote: %s\n", line);

        // check if user wants to exit
        if (strcmp(line, "exit") == 0) {
            break;
        }

        // parse and execute the command
        // copied from lab 1

        command_line large_token_buffer;
        command_line small_token_buffer;

        large_token_buffer = str_filler (line, ";");

        for (int i = 0; large_token_buffer.command_list[i] != NULL; i++)
		{
            // prints large token number
			//printf ("\tLine segment %d:\n", i + 1);

			//tokenize large buffer
			//smaller token is seperated by " "(space bar)
			small_token_buffer = str_filler (large_token_buffer.command_list[i], " ");

            // identify the command and error check, then preform command
            if (small_token_buffer.command_list[0] != NULL) {
                // execute the command based on the first token

                // exit
                if ((strcmp(small_token_buffer.command_list[0], "exit") == 0)) {
                    // error checking token # 2 (in position 1) != null
                    if (small_token_buffer.command_list[1] != NULL) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    /*
                    printf("compared token: %s\n", small_token_buffer.command_list[1]);
                    printf("number of tokens: %d\n", small_token_buffer.num_token);
                    */
                    else {
                    free_command_line (&large_token_buffer);
                    free_command_line(&small_token_buffer);
                    free(line);  
                    return; }
                }
                // ls 
                else if (strcmp(small_token_buffer.command_list[0], "ls") == 0) {
                    // error checking token # 2 (in position 1) != null
                    if (small_token_buffer.command_list[1] != NULL) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else { listDir();}

                }
                // pwd
                else if (strcmp(small_token_buffer.command_list[0], "pwd") == 0) {
                    // error checking token # 2 (in position 1) != null
                    if (small_token_buffer.command_list[1] != NULL) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else {showCurrentDir();  }// call function for "pwd"
                } 
                // mkdir
                else if (strcmp(small_token_buffer.command_list[0], "mkdir") == 0) {
                    // error checking token # 2 (in position 1) == null and # 3 != null
                    if ((small_token_buffer.command_list[1] == NULL) || 
                        small_token_buffer.command_list[2] != NULL) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else {makeDir(small_token_buffer.command_list[1]);}
                }
                // cd
                else if (strcmp(small_token_buffer.command_list[0], "cd") == 0) {
                    // error checking token # 2 (in position 1) == null and # 3 != null
                    if ((small_token_buffer.command_list[1] == NULL) || 
                        small_token_buffer.command_list[2] != NULL) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else {changeDir(small_token_buffer.command_list[1]);}
                }
                // cp
                else if (strcmp(small_token_buffer.command_list[0], "cp") == 0) {
                    // error checking token # 2 (in position 1) == null and # 3 == null and # 4 != null
                    if ((small_token_buffer.command_list[1] == NULL) || 
                        (small_token_buffer.command_list[2] == NULL) ||
                        (small_token_buffer.command_list[3] != NULL)) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else {copyFile(small_token_buffer.command_list[1], small_token_buffer.command_list[2]);}
                }
                // mv
                else if (strcmp(small_token_buffer.command_list[0], "mv") == 0) {
                    // error checking token # 2 (in position 1) == null and # 3 == null and # 4 != null
                    if ((small_token_buffer.command_list[1] == NULL) || 
                        (small_token_buffer.command_list[2] == NULL) ||
                        (small_token_buffer.command_list[3] != NULL)) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else {moveFile(small_token_buffer.command_list[1], small_token_buffer.command_list[2]);}
                }
                // rm
                else if (strcmp(small_token_buffer.command_list[0], "rm") == 0) {
                    // error checking token # 2 (in position 1) == null and # 3 != null
                    if ((small_token_buffer.command_list[1] == NULL) || 
                        small_token_buffer.command_list[2] != NULL) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else {deleteFile(small_token_buffer.command_list[1]);}
                }
                // cat
                else if (strcmp(small_token_buffer.command_list[0], "cat") == 0) {
                    // error checking token # 2 (in position 1) == null and # 3 != null
                    if ((small_token_buffer.command_list[1] == NULL) || 
                        small_token_buffer.command_list[2] != NULL) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else {displayFile(small_token_buffer.command_list[1]);}
                }
                // error unknown command
                else {
                    printf("Error! Unrecognized command: %s\n", small_token_buffer.command_list[0]);
                }
            }

			//free smaller tokens and reset variable
			free_command_line(&small_token_buffer);
			memset (&small_token_buffer, 0, 0);
		}

		//free smaller tokens and reset variable
		free_command_line (&large_token_buffer);
		memset (&large_token_buffer, 0, 0);
        printf(">>> ");  // prompt again after command execution
    }

    free(line);  // free the allocated memory for input line
    return;
}

// some from lab 2 lfcat and lab 1 string parser
void file_mode(const char *filename) {
    char *line = NULL;
    size_t len = 0;

    char cwd[1024];
	getcwd(cwd, sizeof(cwd));

    FILE *input_file = fopen(filename, "r");
    if (input_file == NULL) {
        perror("Error opening file");
        return;
    }

    FILE *output_file = freopen("output.txt", "w", stdout); // redirect stdout to output file
    freopen("output.txt", "w", stderr);  // redirect stderr to output file
	if (output_file == NULL) {
        perror("freopen");
        fclose(input_file);
        return;
    }
    while (getline(&line, &len, input_file) != -1) {
        // strip newline character
        line[strcspn(line, "\n")] = 0; 
        //printf("you wrote: %s\n", line);

        // check if user wants to exit
        if (strcmp(line, "exit") == 0) {
            break;
        }

        // parse and execute the command
        // copied from lab 1
        

        command_line large_token_buffer;
        command_line small_token_buffer;

        large_token_buffer = str_filler (line, ";");

        for (int i = 0; large_token_buffer.command_list[i] != NULL; i++)
		{
            // prints large token number
			//printf ("\tLine segment %d:\n", i + 1);

			//tokenize large buffer
			//smaller token is seperated by " "(space bar)
			small_token_buffer = str_filler (large_token_buffer.command_list[i], " ");

            // identify the command and error check, then preform command
            if (small_token_buffer.command_list[0] != NULL) {
                // execute the command based on the first token

                // exit
                if ((strcmp(small_token_buffer.command_list[0], "exit") == 0)) {
                    // error checking token # 2 (in position 1) != null
                    if (small_token_buffer.command_list[1] != NULL) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    /*
                    printf("compared token: %s\n", small_token_buffer.command_list[1]);
                    printf("number of tokens: %d\n", small_token_buffer.num_token);
                    */
                    else {
                    free_command_line (&large_token_buffer);
                    free_command_line(&small_token_buffer);
                    free(line);  
                    return; }
                }
                // ls 
                else if (strcmp(small_token_buffer.command_list[0], "ls") == 0) {
                    // error checking token # 2 (in position 1) != null
                    if (small_token_buffer.command_list[1] != NULL) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else { listDir();}

                }
                // pwd
                else if (strcmp(small_token_buffer.command_list[0], "pwd") == 0) {
                    // error checking token # 2 (in position 1) != null
                    if (small_token_buffer.command_list[1] != NULL) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else {showCurrentDir();  }// call function for "pwd"
                } 
                // mkdir
                else if (strcmp(small_token_buffer.command_list[0], "mkdir") == 0) {
                    // error checking token # 2 (in position 1) == null and # 3 != null
                    if ((small_token_buffer.command_list[1] == NULL) || 
                        small_token_buffer.command_list[2] != NULL) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else {makeDir(small_token_buffer.command_list[1]);}
                }
                // cd
                else if (strcmp(small_token_buffer.command_list[0], "cd") == 0) {
                    // error checking token # 2 (in position 1) == null and # 3 != null
                    if ((small_token_buffer.command_list[1] == NULL) || 
                        small_token_buffer.command_list[2] != NULL) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else {changeDir(small_token_buffer.command_list[1]);}
                }
                // cp
                else if (strcmp(small_token_buffer.command_list[0], "cp") == 0) {
                    // error checking token # 2 (in position 1) == null and # 3 == null and # 4 != null
                    if ((small_token_buffer.command_list[1] == NULL) || 
                        (small_token_buffer.command_list[2] == NULL) ||
                        (small_token_buffer.command_list[3] != NULL)) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else {copyFile(small_token_buffer.command_list[1], small_token_buffer.command_list[2]);}
                }
                // mv
                else if (strcmp(small_token_buffer.command_list[0], "mv") == 0) {
                    // error checking token # 2 (in position 1) == null and # 3 == null and # 4 != null
                    if ((small_token_buffer.command_list[1] == NULL) || 
                        (small_token_buffer.command_list[2] == NULL) ||
                        (small_token_buffer.command_list[3] != NULL)) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else {moveFile(small_token_buffer.command_list[1], small_token_buffer.command_list[2]);}
                }
                // rm
                else if (strcmp(small_token_buffer.command_list[0], "rm") == 0) {
                    // error checking token # 2 (in position 1) == null and # 3 != null
                    if ((small_token_buffer.command_list[1] == NULL) || 
                        small_token_buffer.command_list[2] != NULL) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else {deleteFile(small_token_buffer.command_list[1]);}
                }
                // cat
                else if (strcmp(small_token_buffer.command_list[0], "cat") == 0) {
                    // error checking token # 2 (in position 1) == null and # 3 != null
                    if ((small_token_buffer.command_list[1] == NULL) || 
                        small_token_buffer.command_list[2] != NULL) {
                        printf("Error! Unsupported parameters for command: %s\n", small_token_buffer.command_list[0]);
                    }
                    else {displayFile(small_token_buffer.command_list[1]);}
                }
                // error unknown command
                else {
                    printf("Error! Unrecognized command: %s\n", small_token_buffer.command_list[0]);
                }
            }

			//free smaller tokens and reset variable
			free_command_line(&small_token_buffer);
			memset (&small_token_buffer, 0, 0);
		}

		//free smaller tokens and reset variable
		free_command_line (&large_token_buffer);
		memset (&large_token_buffer, 0, 0);
        // finish copy
        
    }
    fclose(stdout);
    free(line);  // free the allocated memory for input line
    return;
}