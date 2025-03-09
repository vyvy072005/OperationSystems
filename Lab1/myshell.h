#ifndef MYSHELL_H
#define MYSHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#endif

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGUMENTS 64

// Прототипы функций
void execute_command(char* command);
void parse_command(char* command, char** args, char** input_file, char** output_file, int* append_output, int* background);
void cd(char** args);
void dir(char** args);
void environ_func();
void echo(char** args);
void help();
void pause_func();
void quit();
void clr();


extern char current_directory[256];

#endif