#include "myshell.h"

// Внутренние команды
void cd(char** args) {
    if (args[1] == NULL) {
        fprintf(stderr, "cd: ожидается аргумент для смены директории\n");
    }
    else {
        if (chdir(args[1]) != 0) {
            //Используем функцию chdir() для смены директории на указанную в args[1]
            perror("cd");
        }
        else {
            // Обновляем текущую рабочую директорию
            if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
                perror("Ошибка при получении текущей директории");
               
            }
        }
    }
}

void dir(char** args) {
    // Объявляем указатель на структуру DIR
    DIR* dir;
    // Объявляем указатель на структуру dirent, который будет хранить информацию о каждой записи в каталоге.
    struct dirent* ent;
    char* dirname = "."; // По умолчанию текущая директория

    if (args[1] != NULL) {
        dirname = args[1]; // Если указан аргумент, используем его как имя директории
    }
   

   
    // Результат (указатель на структуру DIR или NULL в случае ошибки) сохраняем в переменной dir.
    dir = opendir(dirname);

    // Проверка, удалось ли открыть директорию.
    if (dir != NULL) {
        
        while ((ent = readdir(dir)) != NULL) {
            
            // Если ent не равен NULL, значит, запись была успешно прочитана.

            // Выводим имя файла или поддиректории, хранящееся в поле d_name структуры dirent, в стандартный поток вывода.
            printf("%s\n", ent->d_name);
        }

        // Закрываем открытую директорию
        closedir(dir);
    }
    else {     
        // Функция perror() выводит сообщение, связанное с последней произошедшей системной ошибкой (хранится в глобальной переменной errno).
        perror("dir");
    }
    
}

void environ_func() {
    /*Переменная внешяня, определена в системных библиотеках
        environ - это указатель на массив указателей на символы(char**),
            где каждый указатель указывает на строку, представляющую переменную окружения.*/
    extern char** environ;
   
    for (int i = 0; environ[i] != NULL; i++) {
        printf("%s\n", environ[i]);
    }
}


void echo(char** args) {
    //Если есть аргументы, начинаем цикл для их перебора и вывода.
    if (args[1] != NULL) {
        for (int i = 1; args[i] != NULL; i++) {
            printf("%s ", args[i]);
        }
        printf("\n");
    }
    else {
        printf("\n"); // Пустая строка, если нет аргументов
    }
}


//Функция для вывода справочной информации
void help() {
    printf("Список команд:\n");
    printf("  cd [directory] - сменить директорию\n");
    printf("  dir [directory] - показать содержимое директории\n");
    printf("  environ - показать переменные окружения\n");
    printf("  echo [string] - вывести строку\n");
    printf("  help - показать помощь\n");
    printf("  pause - приостановить выполнение\n");
    printf("  quit - выйти из оболочки\n");
    printf("  & - запустить команду в фоновом режиме\n");
}

//Функция паузы
void pause_func() {
    printf("Нажмите Enter для продолжения...\n");
    getchar(); // Ожидаем нажатия Enter
}

//Функция очистики консоли
void clr() {
#ifdef _WIN32
    system("cls"); // Для Windows
#else
    // Для Linux и macOS
    printf("\033[2J");   // Очистка экрана
    printf("\033[H");    // Перемещение курсора в верхний левый угол
#endif

}

//Функция выхода из программы
void quit() {
    exit(0);
}



// Функция для разбора команды и извлечения аргументов, файлов ввода/вывода и флага append
void parse_command(char* command, char** args, char** input_file, char** output_file, int* append_output, int* background) {
    int i = 0;
    *input_file = NULL; //по умолчанию ввод не перенаправляется из файла.
    *output_file = NULL; //по умолчанию вывод не перенаправляется в файл.
    *append_output = 0; // флаг добавления в конец файла
    *background = 0; //флаг фонового выполнения

    // Проверяем, есть ли '&' в конце команды
    char* ampersand = strchr(command, '&');
    if (ampersand != NULL) {
        *background = 1;
        *ampersand = '\0';  // Обрезаем команду, удаляя '&'
    }

    char* token = strtok(command, " "); // Разбиваем команду на токены
    while (token != NULL && i < MAX_ARGUMENTS - 1) {
        // Проверяем, является ли текущий токен символом перенаправления ввода ('<').
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Ошибка: Ожидается имя файла после '<'\n");

                args[0] = NULL;  // Устанавливаем первый аргумент (команду) в NULL, чтобы указать на ошибку.
                return;
            }
            *input_file = token; //сохранение токена в указателе
        }
        else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Ошибка: Ожидается имя файла после '>'\n");
                args[0] = NULL; // Обозначаем ошибку
                return;
            }

            *output_file = token; //сохранение токена в указателе
            *append_output = 0; // Устанавливаем append в 0 для '>'
        }
        else if (strcmp(token, ">>") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Ошибка: Ожидается имя файла после '>>'\n");
                args[0] = NULL; // Обозначаем ошибку
                return;
            }
            *output_file = token;
            *append_output = 1; // Устанавливаем append в 1 для '>>'
        }
        else {
            args[i++] = token; // Добавляем аргумент
        }
        token = strtok(NULL, " ");
    }

    args[i] = NULL; // Завершаем массив аргументов NULL
}


