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

	write(STDOUT_FILENO, cwd, strlen(cwd));
	newLine();

	FILE *output_file = freopen("my_output.txt", "w", stdout);
	if (output_file == NULL) {
        perror("freopen");
        return;
    }
	/* Open the dir using opendir() */

	printf("Current working directory: %s\n", cwd);
	
	if (strlen(cwd) + strlen("/files") < sizeof(cwd)) {
        strcat(cwd, "/files");
    } 
	
	//freopen("my_output.txt", "w", stdout);
	DIR *dir;

	dir = opendir(cwd);

	
	struct dirent *read_dir;
	//printf("%s", readdir(dir)->d_name);

	int c = 1;
	if (dir == NULL) {
		perror("dir");
	}
	/*
	read_dir = readdir(dir);
	printf("Current working directory: %s\n", cwd);
	return;
	*/
	/* use a while loop to read the dir with readdir()*/
	while ((read_dir = readdir(dir)) != NULL) {
	//read_dir = readdir(dir);
	//for (int i = 0; i < 3; i++, read_dir = readdir(dir)) {
		
		//printf("dir name: %s", read_dir->d_name);
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
		/* Build full path for each file */
		char full_path[1024];  // Buffer for the full path
		snprintf(full_path, sizeof(full_path), "%s/%s", cwd, read_dir->d_name);
		printf("full path %s\n", full_path);
		/* Open the file using the full path */
		int file_id = open(full_path, O_RDONLY);
		if (file_id == -1) {
			perror("open");
			continue;  // Skip this file if it cannot be opened
		}

		//int file_id = open(read_dir->d_name, O_RDONLY);
		/*
		if (file_id == -1) {
    		perror("open");
    		continue;
		}*/
			/* Read in each line using getline() */
		// FILE *out_f = freopen(dest_path, "w", stdout);
		char buffer[1024];
		size_t bytes;
		do {
			bytes = read(file_id, buffer, sizeof(buffer)); 
			fwrite(buffer, 1, bytes, stdout);
		}	while (bytes > 0);
				/* Write the line to stdout */
		close(file_id);
		// freopen("/dev/tty", "a", stdout);
			/* write 80 "-" characters to stdout */
			
		for (int i = 0; i < 80; ++i) {
			write(STDOUT_FILENO, "-", 1);
		}
		newLine();
			/* close the read file and free/null assign your line buffer */
	}
		
	/*close the directory you were reading from using closedir() */
	closedir(dir);
	fclose(stdout);
	freopen("/dev/tty", "w", stdout);
	printf("lfcat is done\n");
}
