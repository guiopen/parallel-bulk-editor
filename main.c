#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "ui.h"
#include "img_editing.h"
#include "file_utils.h"

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

ImagePath *get_next_image(SharedState *state)
{
    pthread_mutex_lock(&state->queue.mutex);

    /*
     * ESPERA CONDICIONAL
     * 1. Verifica se fila está vazia (current >= size) ou se programa está terminando
     * 2. Enquanto não houver trabalho E programa não estiver terminando:
     *    - Sinaliza outras threads que podem estar esperando (evita deadlocks)
     *    - Suspende esta thread até que haja trabalho ou programa termine
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

    // Se programa está terminando, retorna NULL para iniciar término da thread
    if (state->queue.should_exit)
    {
        pthread_mutex_unlock(&state->queue.mutex);
        return NULL;
    }

    // Obtém próxima imagem e incrementa contador atomicamente (thread-safe, já que o mutex está bloqueado)
    ImagePath *path = &state->queue.paths[state->queue.current++];

    pthread_mutex_unlock(&state->queue.mutex);

    return path;
}

void *worker_thread(void *arg)
{
    SharedState *state = (SharedState *)arg;

    while (1)
    {
        // Obtém próxima imagem da fila (thread-safe)
        ImagePath *path = get_next_image(state);
        if (!path)  // Retorna NULL quando fila vazia e should_exit=true
            break;

        // Processa imagem - esta é a operação principal de cada thread
        if (transform_image(path->input_path, path->output_path, state->transform))
        {
            pthread_mutex_lock(&state->queue.mutex);
            state->queue.processed++;

            /*
             * SINALIZAÇÃO DE CONDIÇÃO:
             * Se esta foi a última imagem a ser processada (processed == size):
             * - Sinaliza a thread principal através da variável de condição done_cond
             * - A thread principal está esperando este sinal para continuar o fluxo
             * Apenas uma thread fará este if (a que processar a última imagem), evitando sinalizações desnecessárias
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

int reload_queue(SharedState *state, const char *input_dir, const char *output_dir,
                 PixelTransformFunction new_transform)
{
    // Garante que nenhuma thread vai tentar acessar a fila durante a recarga
    pthread_mutex_lock(&state->queue.mutex);

    // Libera recursos da fila anterior se existir
    free(state->queue.paths);

    // Escaneia diretório para encontrar imagens e preencher nova fila
    int count;
    state->queue.paths = scan_directory(input_dir, output_dir, &count);
    if (!state->queue.paths)
    {
        // Em caso de falha, libera mutex e retorna erro (0)
        pthread_mutex_unlock(&state->queue.mutex);
        return 0;
    }

    // Reinicializa estado da fila de forma segura (dentro da seção crítica)
    state->queue.size = count;
    state->queue.current = 0;
    state->queue.processed = 0;
    state->transform = new_transform;

    // BROADCAST: Acorda TODAS as threads trabalhadoras - cada uma irá competir pelo mutex para obter imagens da fila
    pthread_cond_broadcast(&state->queue.queue_cond);

    pthread_mutex_unlock(&state->queue.mutex);

    return 1;
}

int process_directory_parallel(const char *input_dir, int num_threads)
{
    SharedState state = {0};

    /*
     * INICIALIZAÇÃO DE SINCRONIZAÇÃO: Prepara objetos de sincronização
     * 1. mutex: proteger acesso compartilhado à fila e contadores
     * 2. queue_cond: sinalizar quando a fila tem/não tem mais trabalho
     * 3. done_cond: sinalizar quando todo o processamento de um lote terminou
    */
    pthread_mutex_init(&state.queue.mutex, NULL);
    pthread_cond_init(&state.queue.queue_cond, NULL);
    pthread_cond_init(&state.queue.done_cond, NULL);
    state.queue.should_exit = 0;
    state.queue.total_time = 0.0;

    // CRIAÇÃO DO POOL DE THREADS: Inicia threads trabalhadoras
    // Cada thread executará a função worker_thread com o mesmo estado compartilhado
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    for (int i = 0; i < num_threads; i++)
    {
        pthread_create(&threads[i], NULL, worker_thread, &state);
    }

    char *edit_type = NULL;
    char output_dir[256];
    int total_processed = 0;

    edit_type = get_edit_type();

    while (1)
    {
        /*
         * SINALIZAÇÃO DE TÉRMINO:
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
            edit_type = get_edit_type();
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
         * SUSPENSÃO DA THREAD PRINCIPAL:
         * 1. Bloqueia mutex para verificar estado da fila
         * 2. Dorme na variável de condição done_cond até receber sinal
         *    - A última thread a processar uma imagem enviará esse sinal
         *    - Cada filtro só termina quando todas as imagens são processadas
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

        display_processing_result(edit_type, state.queue.processed, elapsed);

        pthread_mutex_unlock(&state.queue.mutex);

        free(edit_type);
        edit_type = get_edit_type();
    }

    // JOIN DE THREADS: Espera todas as threads trabalhadoras terminarem antes de finalziar o programa
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    display_final_statistics(total_processed, state.queue.total_time, num_threads);

    // LIMPEZA DE RECURSOS DE SINCRONIZAÇÃO
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