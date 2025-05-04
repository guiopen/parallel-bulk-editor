/**
 * img_editing.h
 *
 * Definições de estruturas e funções para processamento paralelo de imagens
 */

#ifndef IMG_EDITING_H
#define IMG_EDITING_H

#include <stdint.h>
#include <pthread.h>
#include <time.h>

/*
 * Estrutura que representa um pixel RGB
 * Cada componente (r,g,b) tem 8 bits (0-255)
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
 * Usada para organizar o processamento em lote das imagens
 */
typedef struct
{
    char input_path[512];
    char output_path[512];
} ImagePath;

/**
 * Estrutura thread-safe que implementa uma fila de imagens para processamento
 * com mecanismos completos de sincronização entre múltiplas threads
 *
 * SINCRONIZAÇÃO:
 * Esta estrutura implementa uma fila thread-safe usando os seguintes mecanismos:
 * 1. mutex - Garante exclusão mútua ao acessar a fila
 * 2. queue_cond - Variável de condição para sinalizar quando há/não há trabalho
 * 3. done_cond - Variável de condição para indicar conclusão do processamento
 *
 * Os contadores (size, current, processed) são protegidos pelo mutex
 * e usados para controlar o fluxo de processamento
 */
typedef struct
{
    ImagePath *paths;   // Array com os caminhos das imagens
    int size;           // Quantidade total de imagens
    int current;        // Índice da próxima imagem a ser processada
    int processed;      // Contador de imagens já processadas
    int should_exit;    // Flag para indicar que as threads devem terminar
    int active_threads; // Número de threads ativas
    double total_time;  // Tempo total acumulado em segundos

    /**
     * MUTEX: Protege o acesso a todas as variáveis compartilhadas acima
     *
     * Propósito: Garante exclusão mútua ao acessar/modificar variáveis compartilhadas
     * Uso: Deve ser bloqueado antes de ler/modificar qualquer variável compartilhada
     * Cuidados: Sempre desbloquear após o uso para evitar deadlocks
     */
    pthread_mutex_t mutex;

    /**
     * VARIÁVEL DE CONDIÇÃO: Sinaliza quando fila está vazia/cheia
     *
     * Propósito: Suspender threads trabalhadoras quando não há trabalho
     * Sinalização: Quando a fila é recarregada (nova edição começa)
     * Espera: Threads trabalhadoras esperam quando não há mais imagens para pegar
     * Particularidade: Pode ter sinais espúrios (spurious wakeups) - sempre usar em conjunto com loop while
     */
    pthread_cond_t queue_cond;

    /**
     * VARIÁVEL DE CONDIÇÃO: Sinaliza quando todas as imagens foram processadas
     *
     * Propósito: Comunicar à thread principal que um lote de processamento terminou
     * Sinalização: Quando a última imagem de um lote termina de ser processada
     * Espera: Thread principal espera nesta condição até todo processamento terminar
     * Particularidade: Pode ter sinais espúrios (spurious wakeups) - sempre usar em conjunto com loop while
     */
    pthread_cond_t done_cond;
} Queue;

/*
 * Estado compartilhado entre todas as threads do pool
 * Contém a fila de imagens e a função de transformação a ser aplicada
 *
 * SINCRONIZAÇÃO:
 * Esta estrutura é compartilhada entre todas as threads trabalhadoras
 * e a thread principal. O acesso a ela é sempre protegido pelo mutex da fila.
 */
typedef struct
{
    Queue queue;
    PixelTransformFunction transform;
    int total_processed; // Novo contador global
} SharedState;

/* Funções de transformação de imagens */
int transform_image(const char *input_path, const char *output_path, PixelTransformFunction transform);

/*
 * Filtros disponíveis:
 * grayscale - Converte para escala de cinza usando pesos R(0.21) G(0.72) B(0.07)
 * filter_red/green/blue - Mantém apenas o canal de cor especificado
 * invert - Inverte todos os canais de cor (255 - valor)
 */
Pixel grayscale(Pixel pixel);
Pixel filter_red(Pixel pixel);
Pixel filter_green(Pixel pixel);
Pixel filter_blue(Pixel pixel);
Pixel invert(Pixel pixel);

#endif