#ifndef IMG_EDITING_H
#define IMG_EDITING_H

#include <stdint.h>
#include <pthread.h>
#include <time.h>

/*
 * Pixel (r,g,b) de 8 bits (0-255)
 */
typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Pixel;

/* Tipo de função que realiza transformação em pixels */
typedef Pixel (*PixelTransformFunction)(Pixel);

/*
 * Estrutura que mantém os caminhos de entrada e saída de uma imagem
 */
typedef struct
{
    char input_path[512];
    char output_path[512];
} ImagePath;

/*
 * Fila de imagens thread-safe com sincronização de processamento entre múltiplas threads
 *
 * Utiliza os seguintes mecanismos:
 * 1. mutex - Garante exclusão mútua ao acessar a fila
 * 2. queue_cond - Variável de condição para sinalizar quando há/não há trabalho
 * 3. done_cond - Variável de condição para indicar conclusão do processamento
 */
typedef struct
{
    ImagePath *paths;   // Array com os caminhos das imagens
    int size;           // Número de imagens
    int current;        // Índice da próxima imagem a ser processada
    int processed;      // Contador de imagens já processadas
    int should_exit;    // Flag para indicar que as threads devem terminar
    int active_threads; // Número de threads ativas
    double total_time;  // Tempo total acumulado em segundos

    pthread_mutex_t mutex;
    pthread_cond_t queue_cond;
    pthread_cond_t done_cond;
} Queue;

/*
 * Estado compartilhado entre todas as threads do pool
 */
typedef struct
{
    Queue queue;
    PixelTransformFunction transform;
    int total_processed; // Novo contador global
} SharedState;

/* Funções de transformação de imagens */
int transform_image(const char *input_path, const char *output_path, PixelTransformFunction transform);

// Filtros disponíveis

Pixel grayscale(Pixel pixel);
Pixel filter_red(Pixel pixel);
Pixel filter_green(Pixel pixel);
Pixel filter_blue(Pixel pixel);
Pixel invert(Pixel pixel);

#endif