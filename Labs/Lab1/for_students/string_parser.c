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
	char* rest = placemaker;
	while ((token = strtok_r(rest, delim, &rest))) {
		if (token == NULL) {
			break;
		}
		counter++; 
	}
	if (buf[strlen(buf)-1] == delim[0]) {
    counter++; }
	free(placemaker);
	return counter;
}

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
	*/

	// 1
	//command_line *commands = malloc(sizeof(command_line));
	command_line commands;
	char *str1;
	char *token;
	char *saveptr1;
	char *placemarker;
	int i;
	// 2
	commands.num_token = count_token(buf, delim);
	// 3
	commands.command_list = (char **)malloc((commands.num_token+1) * sizeof(char *));
	// 4
	int* token_sizes = malloc(sizeof(int) * commands.num_token);
	for (int i = 0; i < commands.num_token; i++) {
	//for (i = 0, placemarker = str1 = strdup(buf);; i++, str1 = NULL) {
		token = strtok_r(str1, delim, &saveptr1);
		if (token == NULL) {
			break;
		}
		commands.command_list[i] = strdup(token);
	}
	free(placemarker);
	commands.command_list[commands.num_token] = NULL;
	return commands;	
}


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
