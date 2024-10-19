/*
 * string_parser.c
 *
 *  Created on: Nov 25, 2020
 *      Author: gguan, Monil
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h"

#define _GUN_SOURCE

int count_token (char* buf, const char* delim)
{
	//TODO：
	/*
	*	#1.	Check for NULL string
	*	#2.	iterate through string counting tokens
	*		Cases to watchout for
	*			a.	string start with delimeter
	*			b. 	string end with delimeter
	*			c.	account NULL for the last token
	*	#3. return the number of token (note not number of delimeter)
	*/
	if (buf == NULL) {
		return 0;
	}
	if (strlen(buf) == 0) {
		return 0;
	}
	int counter = 0;
	char* token; 
	char* placemaker = strdup(buf);

	if (placemaker == NULL) {
		printf("Failed to duplicate input string");
		return 0;
	}

	char* rest = placemaker;
	while ((token = strtok_r(rest, delim, &rest))) {
		if (strlen(token) > 0) {
			counter++;
		}
	}
	/*
	if (buf[strlen(buf)-1] == delim[0]) {
    counter++; }  */
	/*
	size_t len = strlen(buf);
    if (len > 0 && strspn(&buf[len - 1], delim) > 0) {
        counter++;  // Increment counter to account for empty token
    }*/

	free(placemaker);
	return counter;
}
command_line str_filler (char* buf, const char* delim) {
	int i;
	char *tkn, *saveptr1, *str1, *placeholder;
	command_line cmd; 
	cmd.num_token = count_token(buf, delim);

	cmd.command_list = (char **)malloc(sizeof(char *) * (cmd.num_token +1));

	for (i = 0, placeholder = str1 = strdup(buf);; i++, str1 = NULL) {
		tkn = strtok_r(str1, delim, &saveptr1);
		if (tkn == NULL) 
			break;
		cmd.command_list[i] = strdup(tkn);
	}

	free(placeholder);
	cmd.command_list[cmd.num_token] = NULL;
	return cmd;
}
/*
command_line str_filler (char* buf, const char* delim)
{
	//TODO：
	/*
	*	#1.	create command_line variable to be filled and returned
	*	#2.	count the number of tokens with count_token function, set num_token. 
    *           one can use strtok_r to remove the \n at the end of the line.
	*	#3. malloc memory for token array inside command_line variable
	*			based on the number of tokens.
	*	#4.	use function strtok_r to find out the tokens 
    *   #5. malloc each index of the array with the length of tokens,
	*			fill command_list array with tokens, and fill last spot with NULL.
	*	#6. return the variable.
	*/ /*

	// 1
	//command_line *commands = malloc(sizeof(command_line));
	command_line commands;
	char *str1;
	char *token;
	char *saveptr1;
	char *placemarker;
	int i;
	int j = 0; // valid tokens
	// 2
	commands.num_token = count_token(buf, delim);
	// 3
	commands.command_list = (char **)malloc((commands.num_token+1) * sizeof(char *));
	if (commands.command_list == NULL) {
        perror("Failed to allocate memory for command_list");
        exit(EXIT_FAILURE);
    }
	// 4
	//int* token_sizes = malloc(sizeof(int) * commands.num_token);
	//for (int i = 0; i < length(token_sizes); i++) {
	placemarker = strdup(buf);
	if (placemarker == NULL) {
        perror("Failed to duplicate input string");
        free(commands.command_list);
        exit(EXIT_FAILURE);
    }
	/*
	for (i = 0; i < commands.num_token; i++, str1 = NULL) {
		token = strtok_r(str1, delim, &saveptr1);
		if ((token == NULL) || (strlen(token) == 0)) {
            continue; 
        } 
		else {
            commands.command_list[j] = strdup(token);  // Store the token
			j++;
        }
	}*/ /*
	// Tokenize the string and store only valid tokens
	char *rest = placemarker;
	while ((token = strtok_r(rest, delim, &rest))) {
		// Skip empty tokens
		if (strlen(token) == 0) {
			continue;
		}

		// Store valid token
		commands.command_list[j] = strdup(token);
		if (commands.command_list[j] == NULL) {
			perror("Failed to allocate memory for token");
			free(placemarker);
			// Free previously allocated tokens in case of failure
			for (int k = 0; k < j; k++) {
				free(commands.command_list[k]);
			}
			free(commands.command_list);
			exit(EXIT_FAILURE);
		}
		j++;  // Increment valid token counter
	}
	commands.num_token = j;
	free(placemarker);
	commands.command_list[commands.num_token] = NULL;
	return commands;	
} */


void free_command_line(command_line* command)
{
	//TODO：
	/*
	*	#1.	free the array base num_token
	*/
	for (int i = 0; i < command->num_token; i++) {
		free(command->command_list[i]);
	}
	free(command->command_list);
}
