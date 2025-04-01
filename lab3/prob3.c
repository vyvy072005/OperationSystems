#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>  //  uint32_t, int8_t


#define PAGE_SIZE 256
#define PHYSICAL_MEMORY_SIZE 65536  // 2^16
#define TLB_SIZE 16
#define PAGE_TABLE_SIZE 256
#define FRAME_SIZE PAGE_SIZE
#define BACKING_STORE_FILE "BACKING_STORE.bin" 


typedef struct {
    int page_number;
    int frame_number;
} TLBEntry;


char physical_memory[PHYSICAL_MEMORY_SIZE];
int page_table[PAGE_TABLE_SIZE]; // Сопоставляет номер страницы с номером кадра (или -1, если его нет в памяти)
TLBEntry tlb[TLB_SIZE];
int tlb_index = 0;
int page_fault_count = 0;
int tlb_hit_count = 0;
int next_available_frame = 0; // Для замены страницы FIFO
int tlb_full = 0;  //Флаг, указывающий на то, что TLB заполнен
int total_addresses = 0;

int translate_address(int logical_address);
int get_frame_from_page_table(int page_number);
int get_frame_from_tlb(int page_number);
void add_to_tlb(int page_number, int frame_number);
int handle_page_fault(int page_number);
void load_page_from_backing_store(int page_number, int frame_number);

//Функция для получения значения в байтах со знаком из физического адреса.
int8_t get_signed_byte(int physical_address) {
    return (int8_t)physical_memory[physical_address];
}

int main(int argc, char* argv[]) {


    if (argc != 2) {
        fprintf(stderr, "Usage: %s <addresses.txt>\n", argv[0]);  
        return 1;
    }

    const char* addresses_file = argv[1]; 

    // Инициализация page table (все страницы изначально не находятся в памяти)
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        page_table[i] = -1;  //-1 означает, что страница не находится в физической памяти
    }

    // Инициализация TLB
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].page_number = -1;  
        tlb[i].frame_number = -1;  
    }

   //Загрузка логического адреса из файла
    FILE* addr_fp = fopen(addresses_file, "r");
    if (addr_fp == NULL) {
        perror("Error opening addresses file");
        return 1;
    }

    char line[10]; // Предполагая, что каждый адрес представляет собой число, состоящее максимум из 5 цифр + нулевой разделитель. !!!!!!!!!!!!!!!!!!!!
    int logical_address;

    while (fgets(line, sizeof(line), addr_fp) != NULL) {
        logical_address = atoi(line); 
        total_addresses++;  //Общее количество адресов

        int physical_address = translate_address(logical_address);

        int8_t signed_byte = get_signed_byte(physical_address);

        printf("Logical address: %d, Physical address: %d, Value: %d\n",
            logical_address, physical_address, signed_byte);
    }

    fclose(addr_fp);

    // Статистика
    double page_fault_rate = (double)page_fault_count / total_addresses * 100.0;
    double tlb_hit_rate = (double)tlb_hit_count / total_addresses * 100.0;

    printf("Page Fault Rate: %.2f%%\n", page_fault_rate);
    printf("TLB Hit Rate: %.2f%%\n", tlb_hit_rate);

    return 0;
}

// Функция преобразования логического адреса в физический
int translate_address(int logical_address) {
    int page_number = (logical_address >> 8) & 0xFF;  // Старшие 8 бит   0xFF - 255
    int offset = logical_address & 0xFF;             // Младшие 8 бит

    // Проверка TLB
    int frame_number = get_frame_from_tlb(page_number);

    if (frame_number != -1) {
        // TLB Hit (наличие)
        tlb_hit_count++;
    }
    else {
        // TLB Miss (отсутствие): обращаемся к таблице страниц
        frame_number = get_frame_from_page_table(page_number);

        if (frame_number == -1) {
            // Обработка ошибки на странице
            frame_number = handle_page_fault(page_number);
        }

        // Добавление в TLB 
        add_to_tlb(page_number, frame_number);
    }

    int physical_address = (frame_number << 8) | offset;
    return physical_address;
}

// Функция для получения номера фрейма из TLB
int get_frame_from_tlb(int page_number) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].page_number == page_number) {
            return tlb[i].frame_number;
        }
    }
    return -1; // TLB Miss (отсутствие)
}

// Функция для получения  frame number из таблицы страниц
int get_frame_from_page_table(int page_number) {
    return page_table[page_number]; // Вернёт -1, если страницы нет в памяти 
}

// Функция для добавления записи в TLB (замена FIFO)
void add_to_tlb(int page_number, int frame_number) {
    tlb[tlb_index].page_number = page_number;
    tlb[tlb_index].frame_number = frame_number;
    tlb_index = (tlb_index + 1) % TLB_SIZE; 

    if (tlb_index == 0) {
        tlb_full = 1; //Установка tlb_full = 1, когда TLB вернется к началу цикла.
    }

}

// Функция обработки ошибки страниц
int handle_page_fault(int page_number) {
    page_fault_count++;

    // Найти свободный фрейм 
    int frame_number = next_available_frame;

    // Если фрейм занят, то использовать замену страницы FIFO 
    if (next_available_frame >= PHYSICAL_MEMORY_SIZE / PAGE_SIZE) {
        // Простая замена по FIFO (найти самый старый кадр)
        // Сделать недействительным замененный кадр в TLB.

        int page_to_replace = -1;  //Инициализировать недопустимым значением
        for (int i = 0; i < PAGE_TABLE_SIZE; ++i) {
            if (page_table[i] != -1) { //Найти правильную запись на странице
                page_to_replace = i;
                break; //Использовать первую найденную цель в качестве заменяющей.
            }
        }

        if (page_to_replace != -1) { //Проверка, что была найдена правильная страница.

            for (int i = 0; i < TLB_SIZE; ++i) {
                if (tlb[i].page_number == page_to_replace) {
                    tlb[i].page_number = -1;
                    tlb[i].frame_number = -1;
                }
            }

            //Получение номера фрейма страницы, которую мы заменяем.
            frame_number = page_table[page_to_replace];
            page_table[page_to_replace] = -1; //Отметила страницу как удаленную из памяти.
        }
        else {
            fprintf(stderr, "Error: No page found to replace during FIFO.\n");
            exit(1);
        }
    }
    else {
        frame_number = next_available_frame;
        next_available_frame++; //Увеличьте номер кадра, если еще есть свободное место.
    }


    // Загрузите страницу из backing store во фрейм
    load_page_from_backing_store(page_number, frame_number);

    // Обновить таблицу страниц
    page_table[page_number] = frame_number;

    return frame_number;
}

// Функция для загрузки страницы из BACKING_STORE_FILE
void load_page_from_backing_store(int page_number, int frame_number) {
    FILE* backing_store_fp = fopen(BACKING_STORE_FILE, "rb"); 
    if (backing_store_fp == NULL) {
        perror("Error opening backing store file");
        exit(1); 
    }

    fseek(backing_store_fp, page_number * PAGE_SIZE, SEEK_SET); //перейдите в нужное исходное местоположение в BACKING_STORE_FILE.
    fread(physical_memory + (frame_number * PAGE_SIZE), sizeof(char), PAGE_SIZE, backing_store_fp); //считывает страницу в физическую память

    fclose(backing_store_fp);
}