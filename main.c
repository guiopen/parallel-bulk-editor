#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "img_editing.h"

// ----------------
// Funções E/S
// ----------------

void clear_input_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

char *get_input_directory()
{
    char *dir = malloc(512);
    printf("Digite o caminho do diretório com as imagens: ");
    if (fgets(dir, 512, stdin) != NULL)
    {
        dir[strcspn(dir, "\n")] = 0; // Remove newline
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

char *get_next_edit_type()
{
    char *edit_type = malloc(20);
    printf("\nDeseja aplicar outro filtro? Digite o tipo ou 'sair' para encerrar\n");
    printf("Tipos disponíveis: grayscale, red, green, blue, invert\n> ");
    if (fgets(edit_type, 20, stdin) != NULL)
    {
        edit_type[strcspn(edit_type, "\n")] = 0;
    }
    return edit_type;
}

char *get_first_edit_type()
{
    char *edit_type = malloc(20);
    printf("Digite o tipo de filtro inicial:\n");
    printf("Tipos disponíveis: grayscale, red, green, blue, invert\n> ");
    if (fgets(edit_type, 20, stdin) != NULL)
    {
        edit_type[strcspn(edit_type, "\n")] = 0;
    }
    return edit_type;
}

PixelTransformFunction get_transform_function(const char *edit_type)
{
    if (strcmp(edit_type, "grayscale") == 0)
        return grayscale;
    if (strcmp(edit_type, "red") == 0)
        return filter_red;
    if (strcmp(edit_type, "green") == 0)
        return filter_green;
    if (strcmp(edit_type, "blue") == 0)
        return filter_blue;
    if (strcmp(edit_type, "invert") == 0)
        return invert;
    return NULL;
}

int is_image_file(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if (!ext)
        return 0;
    return (strcasecmp(ext, ".jpg") == 0 ||
            strcasecmp(ext, ".jpeg") == 0 ||
            strcasecmp(ext, ".png") == 0);
}

ImagePath *scan_directory(const char *input_dir, const char *output_dir, int *count)
{
    DIR *dir = opendir(input_dir);
    if (!dir)
        return NULL;

    struct dirent *entry;
    *count = 0;
    while ((entry = readdir(dir)))
    {
        if (entry->d_type == DT_REG && is_image_file(entry->d_name))
        {
            (*count)++;
        }
    }

    ImagePath *paths = malloc(*count * sizeof(ImagePath));
    if (!paths)
    {
        closedir(dir);
        return NULL;
    }

    rewinddir(dir);
    int i = 0;
    while ((entry = readdir(dir)) && i < *count)
    {
        if (entry->d_type == DT_REG && is_image_file(entry->d_name))
        {
            snprintf(paths[i].input_path, sizeof(paths[i].input_path),
                     "%s/%s", input_dir, entry->d_name);
            snprintf(paths[i].output_path, sizeof(paths[i].output_path),
                     "%s/%s", output_dir, entry->d_name);
            i++;
        }
    }

    closedir(dir);
    return paths;
}




// ----------------
// Lógica principal
// ----------------

/**
 * @brief Obtém a próxima imagem da fila para processamento
 * 
 * SINCRONIZAÇÃO:
 * - Usa mutex para proteger acesso à fila compartilhada
 * - Implementa espera em condição quando fila está vazia
 * - Lida com sinais de terminação do programa
 *
 * @param state Estado compartilhado contendo a fila de imagens
 * @return Próxima imagem a ser processada ou NULL, se deve terminar
 */
ImagePath *get_next_image(SharedState *state)
{
    pthread_mutex_lock(&state->queue.mutex);

    /*
     * SUSPENSÃO CONTROLADA: Suspende thread quando não há trabalho
     *
     * 1. Verifica se fila está vazia (current >= size) ou se programa está terminando
     * 2. Enquanto não houver trabalho E programa não estiver terminando:
     *    - Sinaliza outras threads que podem estar esperando (evita deadlocks)
     *    - Suspende esta thread até que haja trabalho ou programa termine
     *
     * Loop while é necessário por causa de "spurious wakeups"
     *       (thread pode acordar sem ter sido explicitamente sinalizada)
     */
    while (state->queue.current >= state->queue.size && !state->queue.should_exit)
    {
        pthread_cond_signal(&state->queue.queue_cond);
        /*
         * pthread_cond_wait automaticamente:
         * 1. Libera o mutex enquanto a thread dorme
         * 2. Reacquire o mutex quando a thread acorda
         */
        pthread_cond_wait(&state->queue.queue_cond, &state->queue.mutex);
    }

    if (state->queue.should_exit)
    {
        pthread_mutex_unlock(&state->queue.mutex);
        return NULL;
    }

    ImagePath *path = &state->queue.paths[state->queue.current++];

    pthread_mutex_unlock(&state->queue.mutex);

    return path;
}

/**
 * @brief Thread trabalhadora do pool de threads
 *
 * SINCRONIZAÇÃO:
 * - Obtém imagens da fila thread-safe
 * - Incrementa contador de imagens processadas
 * - Sinaliza término do processamento quando última imagem é processada
 *
 * @param arg Ponteiro para o estado compartilhado (SharedState)
 * @return NULL após terminar processamento
 */
void *worker_thread(void *arg)
{
    SharedState *state = (SharedState *)arg;

    while (1)
    {
        ImagePath *path = get_next_image(state);
        if (!path) // Retorna NULL quando fila vazia e should_exit=true
            break;

        // Processa imagem
        if (transform_image(path->input_path, path->output_path, state->transform))
        {
            pthread_mutex_lock(&state->queue.mutex);
            state->queue.processed++;

            /*
             * SUSPENSÃO CONTROLADA: Notifica que processamento terminou
             *
             * Se esta foi a última imagem a ser processada (processed == size):
             * - Sinaliza a thread principal através da variável de condição done_cond
             * - A thread principal está esperando este sinal para continuar o fluxo
             *
             * Apenas uma thread fará este if (a que processar a última imagem),
             * evitando sinalizações desnecessárias.
             */
            if (state->queue.processed == state->queue.size)
            {
                pthread_cond_signal(&state->queue.done_cond);
            }

            pthread_mutex_unlock(&state->queue.mutex);
        }
    }
    return NULL;
}

/**
 * @brief Recarrega a fila com um novo conjunto de imagens para processamento
 *
 * SINCRONIZAÇÃO:
 * - Usa mutex para proteger a atualização da fila
 * - Sinaliza threads trabalhadoras que estão esperando por trabalho
 *
 * @param state Estado compartilhado
 * @param input_dir Diretório com de entrada
 * @param output_dir Diretório de saída
 * @param new_transform Função de transformação das imagens
 * @return 1 se sucesso, 0 se falha
 */
int reload_queue(SharedState *state, const char *input_dir, const char *output_dir,
                 PixelTransformFunction new_transform)
{
    pthread_mutex_lock(&state->queue.mutex);

    free(state->queue.paths);

    // Busca imagens no diretório de entrada
    int count;
    state->queue.paths = scan_directory(input_dir, output_dir, &count);
    if (!state->queue.paths)
    {
        pthread_mutex_unlock(&state->queue.mutex);
        return 0;
    }

    // Reinicializa estado
    state->queue.size = count;
    state->queue.current = 0;
    state->queue.processed = 0;
    state->transform = new_transform;

    /*
     * SUSPENSÃO CONTROLADA: Notifica TODAS as threads trabalhadoras
     *
     * Acorda todas threads esperando por trabalho
     */
    pthread_cond_broadcast(&state->queue.queue_cond);

    pthread_mutex_unlock(&state->queue.mutex);

    return 1;
}

/**
 * @brief Processamento paralelo de imagens
 *
 * SINCRONIZAÇÃO:
 * - Inicializa e gerencia o pool de threads trabalhadoras
 * - Coordena os filtros
 * - Mantém estatísticas de tempo e quantidade de processamento
 *
 * @param input_dir Diretório com as imagens originais
 * @param num_threads Número de threads trabalhadoras a serem criadas
 * @return Total de imagens processadas
 */
int process_directory_parallel(const char *input_dir, int num_threads)
{
    SharedState state = {0};
    pthread_mutex_init(&state.queue.mutex, NULL);
    pthread_cond_init(&state.queue.queue_cond, NULL);
    pthread_cond_init(&state.queue.done_cond, NULL);
    state.queue.should_exit = 0;
    state.queue.total_time = 0.0;

    /*
     * POOL DE THREADS:
     *
     * Cada thread executa `worker_thread` com o mesmo estado compartilhado
     */
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    for (int i = 0; i < num_threads; i++)
    {
        pthread_create(&threads[i], NULL, worker_thread, &state);
    }

    char *edit_type = NULL;
    char output_dir[256];
    int total_processed = 0;

    edit_type = get_first_edit_type();

    while (1)
    {
        /*
         * SUSPENSÃO CONTROLADA: Notifica todas as threads para terminar
         *
         * Quando usuário escolhe 'sair':
         * 1. Bloqueia mutex para acessar estado compartilhado
         * 2. Define flag de saída como verdadeira
         * 3. Acorda TODAS as threads que estejam esperando com broadcast
         * 4. Libera mutex para que as threads possam verificar a flag e terminar
         */
        if (strcmp(edit_type, "sair") == 0)
        {
            pthread_mutex_lock(&state.queue.mutex);
            state.queue.should_exit = 1;
            pthread_cond_broadcast(&state.queue.queue_cond);
            pthread_mutex_unlock(&state.queue.mutex);

            free(edit_type);
            break;
        }

        PixelTransformFunction transform = get_transform_function(edit_type);
        if (!transform)
        {
            printf("Tipo de edição inválido: %s\n", edit_type);
            free(edit_type);
            edit_type = get_next_edit_type();
            continue;
        }

        snprintf(output_dir, sizeof(output_dir), "%s_%s", input_dir, edit_type);
        mkdir(output_dir, 0777);

        // Registra tempo de início desta edição
        struct timespec start_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        if (!reload_queue(&state, input_dir, output_dir, transform))
        {
            printf("Erro ao recarregar fila\n");
            free(edit_type);
            break;
        }

        /*
         * SUSPENSÃO CONTROLADA: Thread principal aguarda término de processamento
         *
         * - A última thread a processar uma imagem enviará esse sinal
         * - Cada filtro só termina quando todas as imagens são processadas
         */
        pthread_mutex_lock(&state.queue.mutex);
        while (state.queue.processed < state.queue.size && !state.queue.should_exit)
        {
            pthread_cond_wait(&state.queue.done_cond, &state.queue.mutex);
        }

        // Calcula tempo gasto nesta edição
        struct timespec end_time;
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        double elapsed = (end_time.tv_sec - start_time.tv_sec) +
                         (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

        state.queue.total_time += elapsed; // Acumula para o tempo total
        total_processed += state.queue.processed;

        printf("Processadas %d imagens com filtro %s em %.2f segundos\n",
               state.queue.processed, edit_type, elapsed);
        pthread_mutex_unlock(&state.queue.mutex);

        free(edit_type);
        edit_type = get_next_edit_type();
    }

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("\nEstatísticas finais:\n");
    printf("- Imagens processadas: %d\n", total_processed);
    printf("- Tempo total de processamento: %.2f segundos\n", state.queue.total_time);
    printf("- Velocidade média: %.2f imagens/segundo\n",
           total_processed / (state.queue.total_time > 0 ? state.queue.total_time : 1));
    printf("- Utilizando %d threads\n", num_threads);

    // Destruindo objetos de sincronização
    pthread_mutex_destroy(&state.queue.mutex);
    pthread_cond_destroy(&state.queue.queue_cond);
    pthread_cond_destroy(&state.queue.done_cond);
    free(threads);

    return total_processed;
}

int main()
{
    char *input_dir = get_input_directory();
    int num_threads = get_thread_count();

    int total_processed = process_directory_parallel(input_dir, num_threads);
    printf("\nTotal de imagens processadas: %d\n", total_processed);

    free(input_dir);
    return 0;
}