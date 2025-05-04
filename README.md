# Editor de Imagens em Lote Paralelo

Processador paralelo de imagens que aplica filtros em múltiplas imagens simultaneamente utilizando threads. O programa permite aplicar diferentes filtros sequencialmente na mesma pasta de imagens, reaproveitando as threads entre as operações.

## Padrões de Programação Paralela

### 1. Thread Pool
- As threads são criadas no início com a quantidade definida pelo usuário
- São reutilizadas para processar diferentes filtros de imagem
- Cada thread obtém uma imagem da fila compartilhada, processa, e repete
- Quando a fila esvazia, as threads são suspensas (não destruídas)
- As threads são reativadas quando um novo filtro é selecionado
- Economiza recursos evitando criação/destruição constante de threads

### 2. Suspensão Controlada
- A thread principal gerencia o ciclo de vida do processamento
- As threads trabalhadoras são suspensas quando a fila esvazia
- A thread principal é suspensa enquanto as trabalhadoras processam
- Todas são acordadas quando um novo filtro é selecionado
- Só são destruídas quando o usuário encerra o programa

## Estruturas de Sincronização

### 1. Mutex (pthread_mutex_t)
- Protege o acesso à fila compartilhada de imagens
- Garante exclusão mútua no acesso aos contadores:
  - current: próxima imagem a ser processada
  - processed: quantidade de imagens já processadas
  - should_exit: flag para encerramento das threads
- Previne condições de corrida no acesso à fila e contadores

### 2. Variáveis de Condição (pthread_cond_t)
- Implementa a suspensão e reativação de todas as threads usando duas variáveis distintas:
  - queue_cond: para controle da fila de trabalho
  - done_cond: para sinalização de conclusão do processamento
- Thread principal espera na variável done_cond durante o processamento
- Threads trabalhadoras esperam na variável queue_cond quando a fila esvazia
- São acordadas quando:
  - Um novo filtro é selecionado (threads trabalhadoras via queue_cond)
  - Todas as imagens foram processadas (thread principal via done_cond)
  - O programa vai encerrar (todas as threads via ambas variáveis)
- Separar em duas variáveis de condição melhora a eficiência da sinalização e reduz falsos acordares
- Permite comunicação direcionada entre grupos específicos de threads

## Compilação

```bash
gcc -o editor main.c img_editing.c -pthread -lm
```

## Uso

```bash
./editor
```

### Fluxo de Execução
1. Programa solicita informações iniciais:
   - Caminho do diretório com as imagens
   - Número de threads desejado para processamento
   - Tipo do primeiro filtro a ser aplicado

2. Para cada filtro:
   - Cria/usa diretório de saída: pasta_entrada_filtro
   - Processa todas as imagens usando as threads definidas
   - Mostra tempo de processamento deste filtro
   - Pergunta se deseja aplicar outro filtro
   - Continua até usuário digitar 'sair'

3. Ao finalizar:
   - Mostra total de imagens processadas
   - Mostra tempo total efetivo de processamento

### Medição de Tempo
- Usa CLOCK_MONOTONIC para precisão de nanosegundos
- Mede apenas o tempo efetivo de processamento
- Não inclui tempo gasto pelo usuário escolhendo opções
- Mostra:
  - Tempo individual de cada filtro
  - Tempo total acumulado ao final

### Filtros e Opções Disponíveis
- grayscale: converte para escala de cinza usando pesos perceptuais (R:21%, G:72%, B:7%)
- red: mantém apenas o canal vermelho
- green: mantém apenas o canal verde
- blue: mantém apenas o canal azul
- invert: inverte as cores (255 - valor de cada canal)
- sair: encerra o programa

### Formato de Saída
O programa cria/usa um diretório para cada tipo de filtro:
pasta_entrada_filtro (ex: imagens_red)

Se o diretório já existe:
- Usa o diretório existente
- Sobrescreve imagens de mesmo nome
- Mantém outras imagens existentes

### Formatos de Imagem Suportados
- JPG/JPEG
- PNG

## Estrutura do Projeto

```
.
├── main.c              # Implementação do processamento paralelo e interface
├── img_editing.h       # Definições das estruturas e funções
├── img_editing.c       # Implementação dos filtros de imagem
└── libs/              
    ├── stb_image.h        # Biblioteca para leitura de imagens
    └── stb_image_write.h  # Biblioteca para escrita de imagens
```

## Detalhes de Implementação

### Estruturas de Dados
- `ImageQueue`: Fila thread-safe de imagens para processamento
  - Mantém arrays com paths de entrada/saída
  - Contadores para controle de progresso
  - Flag de controle de término
  - Mutex e variável de condição para sincronização

### Fluxo de Execução
1. Thread Principal:
   - Obtém configurações do usuário:
     - Diretório de entrada
     - Número de threads
     - Filtro inicial
   - Cria o pool de threads
   - Entra em loop interativo:
     - Processa escolha do usuário
     - Prepara diretório de saída
     - Recarrega a fila
     - Suspende aguardando processamento
     - Pede próximo filtro
   - Ao receber "sair":
     - Sinaliza threads para terminarem
     - Espera todas finalizarem
     - Mostra relatório final

2. Threads Trabalhadoras:
   - Executam em loop:
     - Tentam obter uma imagem da fila
     - Se fila vazia, suspendem
     - Ao obter imagem, aplicam filtro
     - Incrementam contador
     - Última thread acorda a principal
   - Quando sinalizadas para terminar:
     - Saem do loop
     - Encerram execução
