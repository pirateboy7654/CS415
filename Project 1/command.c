#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

/*for the ls command*/
void listDir() {
    //printf("ls command\n");
    DIR *d;
    struct dirent *dir;

    // open the current directory
    d = opendir(".");
    if (d == NULL) { // error check if cant open
        perror("opendir");
        return;
    }

    // read directory entries
    while ((dir = readdir(d)) != NULL) {
        // print file names
        if (dir->d_name[0] != '.') {  
            write(STDOUT_FILENO, dir->d_name, strlen(dir->d_name));
            write(STDOUT_FILENO, "\t", 1); // is newline needed here? 
        }
    }

    // close directory with error check
    if (closedir(d) == -1) {
        perror("closedir");
    }
}

/*for the pwd command*/
void showCurrentDir() {
    size_t size = 1024;  // initial buffer size
    char *buffer;  // initial buffer for cwd

    // allocate memory for the buffer
    buffer = (char *)malloc(size);
    if (buffer == NULL) {
        perror("Unable to allocate buffer"); // error if cant allocate buffer
        return;
    }

    // get the cwd
    if (getcwd(buffer, size) != NULL) {
        // output the cwd
        write(STDOUT_FILENO, buffer, strlen(buffer));
        write(STDOUT_FILENO, "\n", 1); // newline
    } else {
        // error if getcwd() fails
        perror("getcwd() error");
    }

    // free the buffer
    free(buffer);
}

/*for the mkdir command*/
void makeDir(char *dirName) {
    //printf("mkdir command\n");
    // creates a dir with r/w/x perms
    if (mkdir(dirName, 0755) == -1) {
        perror("make dir"); // error check
    }
}
/*for the cd command*/
void changeDir(char *dirName) {
    //printf("cd command\n");
    // changes dir to dirname
    if (chdir(dirName) == -1) {
        perror("change dir"); // error check 
    }
}

/*for the cp command*/
void copyFile(char *sourcePath, char *destinationPath) {
    //printf("cp command\n");
    int src, dst;
    char buffer[1024];
    ssize_t bytes_read;

    // open src file for reading
    src = open(sourcePath, O_RDONLY);
    if (src == -1) {
        perror("open (source)"); // error if cant open src file
        return;
    }

    // handle case where dst path is '.' ie cant write to a directory need to write to a file
    char full_dest_path[1024];
    char *src_name = strrchr(sourcePath, '/'); // gets file name
    if (strcmp(destinationPath, ".") == 0) {
        
        strcpy(full_dest_path, ".");  // copy "./" to the dest path
        strcat(full_dest_path, src_name);  // append file name to "./"
    } 
    else {
        strcpy(full_dest_path, destinationPath); // copy the destination path
    }

    // open dst file for writing (create if doesn't exist)
    dst = open(full_dest_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst == -1) {
        perror("open (destination)"); // error if cant open dst file
        close(src); // close file if error
        return;
    }

    // copy file contents from src to dst
    while ((bytes_read = read(src, buffer, sizeof(buffer))) > 0) {
        if (write(dst, buffer, bytes_read) != bytes_read) {
            perror("write"); // error if cant copy
            close(src); // close files if error
            close(dst);
            return;
        }
    }

    // close files
    close(src);
    close(dst);
}

/*for the mv command*/
void moveFile(char *sourcePath, char *destinationPath) {
    //printf("mv command\n");
    // moves or renames the file
    if (rename(sourcePath, destinationPath) == -1) {
        perror("rename");
    }
}

/*for the rm command*/
void deleteFile(char *filename) {
    //printf("rm command\n");
    // deletes the file
    if (remove(filename) == -1) {
        perror("remove");
    }
}

/*for the cat command*/
void displayFile(char *filename) {
    //printf("cat command\n");
    int file_des;
    char buffer[1024];
    ssize_t bytes_read;

    // open the file for reading
    file_des = open(filename, O_RDONLY);
    if (file_des == -1) {
        perror("open");
        return;
    }

    // read and display the file contents
    while ((bytes_read = read(file_des, buffer, sizeof(buffer))) > 0) {
        if (write(STDOUT_FILENO, buffer, bytes_read) != bytes_read) {
            perror("write");
            close(file_des);
            return;
        }
    }

    // close the file
    close(file_des);
}