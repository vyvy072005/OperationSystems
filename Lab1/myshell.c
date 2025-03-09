#include "myshell.h"


char current_directory[256];

int main(int argc, char* argv[]) {
    FILE* batch_file = NULL; //���� �������� ���������
    char command[MAX_COMMAND_LENGTH];
    char shell_path[MAX_COMMAND_LENGTH];

    // �������� ������ ���� � ������������ ����� ��������
    if (realpath(argv[0], shell_path) == NULL) {
        perror("������ ��� ��������� ������� ���� � ��������");
        strncpy(shell_path, "./myshell", sizeof(shell_path) - 1); // �������� �� ���������
        shell_path[sizeof(shell_path) - 1] = '\0'; // ����������� ���������� �����
    }

    // ������������� ���������� ��������� SHELL
    if (setenv("SHELL", shell_path, 1) != 0) {
        perror("������ ��� ��������� ���������� ��������� SHELL");
    }

    // �������� ������� ������� ����������
    if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
        perror("������ ��� ��������� ������� ����������");
        return 1;
    }



    // ���������, ��� �� ������� ���� �������� ���������
    if (argc > 1) {
        batch_file = fopen(argv[1], "r");
        if (batch_file == NULL) {
            perror("������ ��� �������� ��������� �����");
            return 1;
        }
    }

    while (1) {
        if (batch_file != NULL) {
            if (fgets(command, sizeof(command), batch_file) == NULL) {
                fclose(batch_file);
                break; // ����� �����
            }
        }
        else {
            printf("%s> ", current_directory);
            if (fgets(command, sizeof(command), stdin) == NULL) {
                printf("\n"); // ����� �� ��������� � ������������ � ��������� ������
                break; // EOF 
            }
        }

        // ������� ������ ����� ������
        command[strcspn(command, "\n")] = 0;

        // ���������� ������ ������
        if (strlen(command) == 0) continue;

        execute_command(command);

        // ��������� ����������� ������� ��������
        int status;
        pid_t pid;

        //WNOHANG: ����, �����������, ��� ������� waitpid() �� ������ �������������, ���� ��� ����������� �������� ���������.
        // ���� �� ���� �������� ������� �� ����������, waitpid() ������ 0.
        
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) { 
            printf("������� ������� %d ��������.\n", pid);
        }
    }

    return 0;
}

void execute_command(char* command) {
    char* args[MAX_ARGUMENTS];
    char* input_file = NULL;
    char* output_file = NULL;
    int append_output = 0;
    int background = 0;

    parse_command(command, args, &input_file, &output_file, &append_output, &background);

    if (args[0] == NULL) {
        return; // ������ �������
    }



    int input_fd = STDIN_FILENO;    //STDIN_FILENO - ��� �������� ���������� ��� ������������ ������ �����
    int output_fd = STDOUT_FILENO; //STDOUT_FILENO - ��� �������� ���������� ��� ������������ ������ ������

    // ��������� ��������������� �����
    if (input_file != NULL) {
        input_fd = open(input_file, O_RDONLY);
        if (input_fd == -1) {
            perror("������ �������� �������� �����");
            return;
        }
    }

    // ��������� ��������������� ������
    if (output_file != NULL) {
        int flags = O_WRONLY | O_CREAT;

        // O_WRONLY - ������� ���� ������ ��� ������.
        // O_CREAT - ������� ����, ���� �� �� ����������.
        if (append_output) {
            flags |= O_APPEND; //��������� ������ � ����� ����� ��� ������.
        }
        else {
            flags |= O_TRUNC; // �������������� ����
        }

        output_fd = open(output_file, flags, 0644); // 0644 - ����� ������� (rw-r--r--)
        if (output_fd == -1) {
            perror("������ �������� ��������� �����");
            if (input_fd != STDIN_FILENO) close(input_fd); // ������� input_fd, ���� �� ��� ������
            return;
        }
    }

    // ���������� ���������� �������
    if (strcmp(args[0], "cd") == 0 || strcmp(args[0], "dir") == 0 ||
        strcmp(args[0], "environ") == 0 || strcmp(args[0], "echo") == 0 ||
        strcmp(args[0], "help") == 0 || strcmp(args[0], "pause") == 0 ||
        strcmp(args[0], "quit") == 0 || strcmp(args[0], "clr") == 0) {
        int temp_fd = dup(STDOUT_FILENO); // ��������� �������� stdout
        if (temp_fd == -1) {
            perror("dup error");
            if (input_fd != STDIN_FILENO) close(input_fd);
            if (output_fd != STDOUT_FILENO) close(output_fd);
            return;
        }

        if (dup2(output_fd, STDOUT_FILENO) == -1) { // �������������� stdout
            perror("dup2 error");
            close(temp_fd);
            if (input_fd != STDIN_FILENO) close(input_fd);
            if (output_fd != STDOUT_FILENO) close(output_fd);
            return;
        }

        if (strcmp(args[0], "cd") == 0) cd(args);
        else if (strcmp(args[0], "dir") == 0) dir(args);
        else if (strcmp(args[0], "environ") == 0) environ_func();
        else if (strcmp(args[0], "echo") == 0) echo(args);
        else if (strcmp(args[0], "help") == 0) help();
        else if (strcmp(args[0], "pause") == 0) pause_func();
        else if (strcmp(args[0], "quit") == 0) quit();
        else if (strcmp(args[0], "clr") == 0) clr();

        fflush(stdout); // ����� ������ ������, ����� ��� ������ ����� ����������

        if (dup2(temp_fd, STDOUT_FILENO) == -1) { // ��������������� �������� stdout
            perror("dup2 error");
        }
        close(temp_fd);
        if (output_fd != STDOUT_FILENO) close(output_fd); // ������� output_fd, ���� �� ��� ������
        if (input_fd != STDIN_FILENO) close(input_fd);   // ������� input_fd, ���� �� ��� ������
    }
    else {
        // ������� ����� ������� � ������� ������� fork().
        // ������� fork() ������� ������ ����� �������� �������� (������������� ��������).
        // ����� ������ fork() � ��� ���� ��� ��������: ������������ ������� � �������� �������.
        // - � ������������ �������� fork() ���������� PID (������������� ��������) ��������� ��������.
        // - � �������� �������� fork() ���������� 0.
        // - ���� fork() �� ������� ������� ����� �������, ��� ���������� -1.
        
        
        // ���������� ������� �������
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork error");
            if (input_fd != STDIN_FILENO) close(input_fd);
            if (output_fd != STDOUT_FILENO) close(output_fd);
            return;
        }

        if (pid == 0) { // �������� �������
            setpgid(0, 0); // ������� ����� ������ ���������

            //���������������� �����
            if (input_fd != STDIN_FILENO && dup2(input_fd, STDIN_FILENO) == -1) {
                perror("dup2 error");
                exit(1);
            }
            if (output_fd != STDOUT_FILENO && dup2(output_fd, STDOUT_FILENO) == -1) {
                perror("dup2 error");
                exit(1);
            }

            if (input_fd != STDIN_FILENO) close(input_fd);
            if (output_fd != STDOUT_FILENO) close(output_fd);


           // ������� execvp() ��������� ����� ����������� ���� � ������� �������, ������� ���������� ���.
           // ������ �������� ������� execvp() - ��� ������������ �����.
           // ������ �������� - ������ ����������, ������� ����� ������� ����� ���������.

            execvp(args[0], args);
            perror("������ ���������� �������");
            exit(1);
        }
        else { // ������������ �������
            if (input_fd != STDIN_FILENO) close(input_fd);
            if (output_fd != STDOUT_FILENO) close(output_fd);

            if (!background) {
                int status;
                waitpid(pid, &status, 0); // ���� ���������� ��������� ��������, ���� �� background

                if (WIFSIGNALED(status)) {  // �������� �� ������ 
                    fprintf(stderr, "������� �������� �������� %d\n", WTERMSIG(status));
                }
            }
            else {
                printf("�������� ������� ������ � PID %d\n", pid);
            }
        }
    }
}

