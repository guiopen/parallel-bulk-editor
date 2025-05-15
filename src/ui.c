#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "ui.h"

char *get_input_directory()
{
    char *dir = malloc(512);
    printf("Digite o caminho do diretório com as imagens: ");
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
    printf("Digite o número de threads desejado: ");

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
    printf("Digite o tipo de filtro ou 'sair'\n");
    printf("Tipos disponíveis: grayscale, red, green, blue, invert\n> ");
    if (fgets(edit_type, 20, stdin) != NULL)
    {
        edit_type[strcspn(edit_type, "\n")] = 0;
    }
    return edit_type;
}

void display_processing_result(const char *edit_type, int count, double elapsed){
    printf("Processadas %d imagens com filtro %s em %.2f segundos\n",
               count, edit_type, elapsed);
}

void display_final_statistics(int total_processed, double total_time, int num_threads){
    printf("\nEstatísticas finais:\n");
    printf("- Imagens processadas: %d\n", total_processed);
    printf("- Tempo total de processamento: %.2f segundos\n", total_time);
    printf("- Velocidade média: %.2f imagens/segundo\n",
           total_processed / (total_time > 0 ? total_time : 1));
    printf("- Utilizando %d threads\n", num_threads);
}
