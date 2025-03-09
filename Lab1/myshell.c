#include "myshell.h"


char current_directory[256];

int main(int argc, char* argv[]) {
    FILE* batch_file = NULL; //файл пакетной обработки
    char command[MAX_COMMAND_LENGTH];
    char shell_path[MAX_COMMAND_LENGTH];

    // Получаем полный путь к выполняемому файлу оболочки
    if (realpath(argv[0], shell_path) == NULL) {
        perror("Ошибка при получении полного пути к оболочке");
        strncpy(shell_path, "./myshell", sizeof(shell_path) - 1); // Значение по умолчанию
        shell_path[sizeof(shell_path) - 1] = '\0'; // Гарантируем завершение нулем
    }

    // Устанавливаем переменную окружения SHELL
    if (setenv("SHELL", shell_path, 1) != 0) {
        perror("Ошибка при установке переменной окружения SHELL");
    }

    // Получаем текущую рабочую директорию
    if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
        perror("Ошибка при получении текущей директории");
        return 1;
    }



    // Проверяем, был ли передан файл пакетной обработки
    if (argc > 1) {
        batch_file = fopen(argv[1], "r");
        if (batch_file == NULL) {
            perror("Ошибка при открытии пакетного файла");
            return 1;
        }
    }

    while (1) {
        if (batch_file != NULL) {
            if (fgets(command, sizeof(command), batch_file) == NULL) {
                fclose(batch_file);
                break; // Конец файла
            }
        }
        else {
            printf("%s> ", current_directory);
            if (fgets(command, sizeof(command), stdin) == NULL) {
                printf("\n"); // Чтобы не сливалось с приглашением в следующей строке
                break; // EOF 
            }
        }

        // Удаляем символ новой строки
        command[strcspn(command, "\n")] = 0;

        // Игнорируем пустые строки
        if (strlen(command) == 0) continue;

        execute_command(command);

        // Проверяем завершенные фоновые процессы
        int status;
        pid_t pid;

        //WNOHANG: Флаг, указывающий, что функция waitpid() не должна блокироваться, если нет завершенных дочерних процессов.
        // Если ни один дочерний процесс не завершился, waitpid() вернет 0.
        
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) { 
            printf("Фоновый процесс %d завершен.\n", pid);
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
        return; // Пустая команда
    }



    int input_fd = STDIN_FILENO;    //STDIN_FILENO - это файловый дескриптор для стандартного потока ввода
    int output_fd = STDOUT_FILENO; //STDOUT_FILENO - это файловый дескриптор для стандартного потока вывода

    // Обработка перенаправления ввода
    if (input_file != NULL) {
        input_fd = open(input_file, O_RDONLY);
        if (input_fd == -1) {
            perror("Ошибка открытия входного файла");
            return;
        }
    }

    // Обработка перенаправления вывода
    if (output_file != NULL) {
        int flags = O_WRONLY | O_CREAT;

        // O_WRONLY - открыть файл только для записи.
        // O_CREAT - создать файл, если он не существует.
        if (append_output) {
            flags |= O_APPEND; //добавлять данные в конец файла при записи.
        }
        else {
            flags |= O_TRUNC; // Перезаписываем файл
        }

        output_fd = open(output_file, flags, 0644); // 0644 - права доступа (rw-r--r--)
        if (output_fd == -1) {
            perror("Ошибка открытия выходного файла");
            if (input_fd != STDIN_FILENO) close(input_fd); // Закрыть input_fd, если он был открыт
            return;
        }
    }

    // Выполнение внутренней команды
    if (strcmp(args[0], "cd") == 0 || strcmp(args[0], "dir") == 0 ||
        strcmp(args[0], "environ") == 0 || strcmp(args[0], "echo") == 0 ||
        strcmp(args[0], "help") == 0 || strcmp(args[0], "pause") == 0 ||
        strcmp(args[0], "quit") == 0 || strcmp(args[0], "clr") == 0) {
        int temp_fd = dup(STDOUT_FILENO); // Сохраняем исходный stdout
        if (temp_fd == -1) {
            perror("dup error");
            if (input_fd != STDIN_FILENO) close(input_fd);
            if (output_fd != STDOUT_FILENO) close(output_fd);
            return;
        }

        if (dup2(output_fd, STDOUT_FILENO) == -1) { // Перенаправляем stdout
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

        fflush(stdout); // Сброс буфера вывода, чтобы все данные точно записались

        if (dup2(temp_fd, STDOUT_FILENO) == -1) { // Восстанавливаем исходный stdout
            perror("dup2 error");
        }
        close(temp_fd);
        if (output_fd != STDOUT_FILENO) close(output_fd); // Закрыть output_fd, если он был открыт
        if (input_fd != STDIN_FILENO) close(input_fd);   // Закрыть input_fd, если он был открыт
    }
    else {
        // Создаем новый процесс с помощью функции fork().
        // Функция fork() создает точную копию текущего процесса (родительского процесса).
        // После вызова fork() у нас есть два процесса: родительский процесс и дочерний процесс.
        // - В родительском процессе fork() возвращает PID (идентификатор процесса) дочернего процесса.
        // - В дочернем процессе fork() возвращает 0.
        // - Если fork() не удается создать новый процесс, она возвращает -1.
        
        
        // Выполнение внешней команды
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork error");
            if (input_fd != STDIN_FILENO) close(input_fd);
            if (output_fd != STDOUT_FILENO) close(output_fd);
            return;
        }

        if (pid == 0) { // Дочерний процесс
            setpgid(0, 0); // Создаем новую группу процессов

            //перенапраавление ввода
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


           // Функция execvp() загружает новый исполняемый файл в текущий процесс, заменяя предыдущий код.
           // Первый аргумент функции execvp() - имя исполняемого файла.
           // Второй аргумент - массив аргументов, который будет передан новой программе.

            execvp(args[0], args);
            perror("Ошибка выполнения команды");
            exit(1);
        }
        else { // Родительский процесс
            if (input_fd != STDIN_FILENO) close(input_fd);
            if (output_fd != STDOUT_FILENO) close(output_fd);

            if (!background) {
                int status;
                waitpid(pid, &status, 0); // Ждем завершения дочернего процесса, если не background

                if (WIFSIGNALED(status)) {  // Проверка на сигнал 
                    fprintf(stderr, "Команда прервана сигналом %d\n", WTERMSIG(status));
                }
            }
            else {
                printf("Запущена фоновая задача с PID %d\n", pid);
            }
        }
    }
}

