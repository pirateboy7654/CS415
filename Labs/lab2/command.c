#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

void newLine() { write(STDOUT_FILENO, "\n", 1); }

void lfcat()
{
/* High level functionality you need to implement: */

	/* Get the current directory with getcwd() */
	
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
	/* Open the dir using opendir() */

	DIR *dir;

	dir = opendir(cwd);

	struct dirent *read_dir;

	int c = 1;
	/*
	if ((read_dir = readdir(dir)) == NULL) {
    fprintf(stderr, "Error: read_dir is NULL\n");
	}
	if ((read_dir = readdir(dir))->d_name == NULL) {
    fprintf(stderr, "Error: filename is NULL\n");
	}*/
	/* use a while loop to read the dir with readdir()*/
	while ((read_dir = readdir(dir)) != NULL) {

		if (strcmp(read_dir->d_name, ".") == 0 ||
			strcmp(read_dir->d_name, "..") == 0 || 
			strcmp(read_dir->d_name, "lab2") == 0) 
			{
				continue;
			}
	
		/* You can debug by printing out the filenames here */
		fprintf(stderr, "File %d: %s\n", c, read_dir->d_name);
		c++;
		/* Option: use an if statement to skip any names that are not readable files (e.g. ".", "..", "main.c", "lab2.exe", "output.txt" */
			
			/* Open the file */
		int file_id = open(read_dir->d_name, O_RDONLY);
			/* Read in each line using getline() */
		char buffer[1024];
		size_t bytes;
		do {
			bytes = read(file_id, buffer, sizeof(buffer)); 
			write(STDOUT_FILENO, buffer, bytes);
		}	while (bytes > 0);
				/* Write the line to stdout */
		close(file_id);
			/* write 80 "-" characters to stdout */
		for (int i = 0; i < 80; ++i) {
			write(STDOUT_FILENO, "-", strlen("-"));
		}
		newLine();
			/* close the read file and free/null assign your line buffer */
	}
	
	/*close the directory you were reading from using closedir() */
		closedir(dir);
}
