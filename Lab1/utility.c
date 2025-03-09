#include "myshell.h"

// ���������� �������
void cd(char** args) {
    if (args[1] == NULL) {
        fprintf(stderr, "cd: ��������� �������� ��� ����� ����������\n");
    }
    else {
        if (chdir(args[1]) != 0) {
            //���������� ������� chdir() ��� ����� ���������� �� ��������� � args[1]
            perror("cd");
        }
        else {
            // ��������� ������� ������� ����������
            if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
                perror("������ ��� ��������� ������� ����������");
               
            }
        }
    }
}

void dir(char** args) {
    // ��������� ��������� �� ��������� DIR
    DIR* dir;
    // ��������� ��������� �� ��������� dirent, ������� ����� ������� ���������� � ������ ������ � ��������.
    struct dirent* ent;
    char* dirname = "."; // �� ��������� ������� ����������

    if (args[1] != NULL) {
        dirname = args[1]; // ���� ������ ��������, ���������� ��� ��� ��� ����������
    }
   

   
    // ��������� (��������� �� ��������� DIR ��� NULL � ������ ������) ��������� � ���������� dir.
    dir = opendir(dirname);

    // ��������, ������� �� ������� ����������.
    if (dir != NULL) {
        
        while ((ent = readdir(dir)) != NULL) {
            
            // ���� ent �� ����� NULL, ������, ������ ���� ������� ���������.

            // ������� ��� ����� ��� �������������, ���������� � ���� d_name ��������� dirent, � ����������� ����� ������.
            printf("%s\n", ent->d_name);
        }

        // ��������� �������� ����������
        closedir(dir);
    }
    else {     
        // ������� perror() ������� ���������, ��������� � ��������� ������������ ��������� ������� (�������� � ���������� ���������� errno).
        perror("dir");
    }
    
}

void environ_func() {
    /*���������� �������, ���������� � ��������� �����������
        environ - ��� ��������� �� ������ ���������� �� �������(char**),
            ��� ������ ��������� ��������� �� ������, �������������� ���������� ���������.*/
    extern char** environ;
   
    for (int i = 0; environ[i] != NULL; i++) {
        printf("%s\n", environ[i]);
    }
}


void echo(char** args) {
    //���� ���� ���������, �������� ���� ��� �� �������� � ������.
    if (args[1] != NULL) {
        for (int i = 1; args[i] != NULL; i++) {
            printf("%s ", args[i]);
        }
        printf("\n");
    }
    else {
        printf("\n"); // ������ ������, ���� ��� ����������
    }
}


//������� ��� ������ ���������� ����������
void help() {
    printf("������ ������:\n");
    printf("  cd [directory] - ������� ����������\n");
    printf("  dir [directory] - �������� ���������� ����������\n");
    printf("  environ - �������� ���������� ���������\n");
    printf("  echo [string] - ������� ������\n");
    printf("  help - �������� ������\n");
    printf("  pause - ������������� ����������\n");
    printf("  quit - ����� �� ��������\n");
    printf("  & - ��������� ������� � ������� ������\n");
}

//������� �����
void pause_func() {
    printf("������� Enter ��� �����������...\n");
    getchar(); // ������� ������� Enter
}

//������� �������� �������
void clr() {
#ifdef _WIN32
    system("cls"); // ��� Windows
#else
    // ��� Linux � macOS
    printf("\033[2J");   // ������� ������
    printf("\033[H");    // ����������� ������� � ������� ����� ����
#endif

}

//������� ������ �� ���������
void quit() {
    exit(0);
}



// ������� ��� ������� ������� � ���������� ����������, ������ �����/������ � ����� append
void parse_command(char* command, char** args, char** input_file, char** output_file, int* append_output, int* background) {
    int i = 0;
    *input_file = NULL; //�� ��������� ���� �� ���������������� �� �����.
    *output_file = NULL; //�� ��������� ����� �� ���������������� � ����.
    *append_output = 0; // ���� ���������� � ����� �����
    *background = 0; //���� �������� ����������

    // ���������, ���� �� '&' � ����� �������
    char* ampersand = strchr(command, '&');
    if (ampersand != NULL) {
        *background = 1;
        *ampersand = '\0';  // �������� �������, ������ '&'
    }

    char* token = strtok(command, " "); // ��������� ������� �� ������
    while (token != NULL && i < MAX_ARGUMENTS - 1) {
        // ���������, �������� �� ������� ����� �������� ��������������� ����� ('<').
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "������: ��������� ��� ����� ����� '<'\n");

                args[0] = NULL;  // ������������� ������ �������� (�������) � NULL, ����� ������� �� ������.
                return;
            }
            *input_file = token; //���������� ������ � ���������
        }
        else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "������: ��������� ��� ����� ����� '>'\n");
                args[0] = NULL; // ���������� ������
                return;
            }

            *output_file = token; //���������� ������ � ���������
            *append_output = 0; // ������������� append � 0 ��� '>'
        }
        else if (strcmp(token, ">>") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "������: ��������� ��� ����� ����� '>>'\n");
                args[0] = NULL; // ���������� ������
                return;
            }
            *output_file = token;
            *append_output = 1; // ������������� append � 1 ��� '>>'
        }
        else {
            args[i++] = token; // ��������� ��������
        }
        token = strtok(NULL, " ");
    }

    args[i] = NULL; // ��������� ������ ���������� NULL
}


