#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "ui.h"

char *get_input_directory()
{
    char *dir = malloc(512);
    printf("Caminho do diretório com as imagens: ");
    if (fgets(dir, 512, stdin) != NULL)
    {
        dir[strcspn(dir, "\n")] = 0;
    }

    DIR *d = opendir(dir);
    while (!d)
    {
        printf("Diretório não encontrado. Digite novamente: ");
        if (fgets(dir, 512, stdin) != NULL)
        {
            dir[strcspn(dir, "\n")] = 0;
        }
        d = opendir(dir);
    }
    closedir(d);
    return dir;
}

int get_thread_count()
{
    int num_threads;
    char buffer[32];
    printf("\nNúmero de threads: ");

    while (fgets(buffer, sizeof(buffer), stdin))
    {
        if (sscanf(buffer, "%d", &num_threads) == 1 && num_threads > 0)
        {
            break;
        }
        printf("Número inválido. Digite um valor positivo: ");
    }
    return num_threads;
}

char *get_edit_type()
{
    char *edit_type = malloc(20);
    printf("\nEscolha um tipo de filtro ou 'sair'\n");
    printf("Tipos disponíveis: grayscale, red, green, blue, invert\n> ");
    if (fgets(edit_type, 20, stdin) != NULL)
    {
        edit_type[strcspn(edit_type, "\n")] = 0;
    }
    return edit_type;
}

void display_processing_result(const char *edit_type, int count, double elapsed){
    printf("Processadas %d imagens com filtro '%s' em %.2f segundos\n",
               count, edit_type, elapsed);
}

void display_final_statistics(int total_processed, double total_time, int num_threads){
    printf("\n======= Estatísticas finais =======\n\n");
    printf("%-25s %d\n", "Imagens processadas:", total_processed);
    printf("%-25s %.2f %s\n", "Tempo total:", total_time, "s");
    printf("%-25s %.2f %s\n", "Velocidade media:", 
        total_processed / (total_time > 0 ? total_time : 1), "imagens/s");

    printf("\n> Utilizando %d threads\n", num_threads);
}
