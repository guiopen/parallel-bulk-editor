#ifndef IMG_EDITING_H
#define IMG_EDITING_H

#include <stdint.h>
#include <pthread.h>
#include <time.h>

// Estrutura que representa um pixel RGB
// Cada componente (r,g,b) tem 8 bits (0-255)
typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Pixel;

// Tipo de função que realiza transformação em pixels
typedef Pixel (*PixelTransformFunction)(Pixel);

// Estrutura que mantém os caminhos de entrada e saída de uma imagem
// Usada para organizar o processamento em lote das imagens
typedef struct
{
    char input_path[512];
    char output_path[512];
} ImagePath;

/*
 * Estrutura thread-safe que implementa uma fila de imagens para processamento
 * Usa dos seguintes mecanismos para sincronização entre múltiplas threads:
 * 1. mutex - Garante exclusão mútua ao acessar a fila
 * 2. queue_cond - Variável de condição para sinalizar quando há/não há trabalho
 * 3. done_cond - Variável de condição para indicar conclusão do processamento
 * Além disso, há contadores (size, current, processed) que são protegidos pelo mutex e usados para controlar o fluxo de processamento
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

    // MUTEX: Protege o acesso de todas as variáveis compartilhadas acima
    pthread_mutex_t mutex;

    /*
     * VARIÁVEL DE CONDIÇÃO:
     * - Propósito: Suspender threads trabalhadoras quando não há trabalho
     * - Sinalização: Quando a fila é recarregada (nova edição começa)
     * - Espera: Threads trabalhadoras esperam quando não há mais imagens para pegar
     * - Particularidade: Pode ter sinais espúrios (spurious wakeups) - sempre usar em conjunto com loop while
    */
    pthread_cond_t queue_cond;

    /*
     * VARIÁVEL DE CONDIÇÃO:
     * - Propósito: Comunicar à thread principal que um lote de processamento terminou
     * - Sinalização: Quando a última imagem de um lote termina de ser processada
     * - Espera: Thread principal espera nesta condição até todo processamento terminar
     * = Particularidade: Pode ter sinais espúrios (spurious wakeups) - sempre usar em conjunto com loop while
    */
    pthread_cond_t done_cond;
} Queue;

// Estado compartilhado entre todas as threads do pool (e pela principal também)
// Contém a fila de imagens e a função de transformação a ser aplicada
typedef struct
{
    Queue queue;
    PixelTransformFunction transform;
    int total_processed;
} SharedState;

// Função de transformação de imagem
int transform_image(const char *input_path, const char *output_path, PixelTransformFunction transform);

// Filtros disponíveis
Pixel grayscale(Pixel pixel);
Pixel filter_red(Pixel pixel);
Pixel filter_green(Pixel pixel);
Pixel filter_blue(Pixel pixel);
Pixel invert(Pixel pixel);

#endif