# Editor de Imagens em Lote Paralelo

Um programa de processamento paralelo de imagens que aplica efeitos a múltiplas imagens simultaneamente.

## Padrões de Programação Paralela Utilizados

### 1. Thread Pool
- Utilizado na criação e gerenciamento das threads responsáveis pela edição das imagens
- Um conjunto fixo de threads (igual ao número de núcleos da CPU) é criado no início
- As threads são reutilizadas para processar diferentes imagens, evitando o overhead de criação/destruição
- Cada thread processa uma imagem por vez, obtendo a próxima da fila quando termina

### 2. Suspensão Controlada
- Aplicado na thread principal do programa
- A thread principal é suspensa após inicializar as threads trabalhadoras
- Permanece suspensa enquanto as imagens estão sendo processadas
- É acordada automaticamente quando todas as imagens forem processadas
- Após acordar, gera o relatório final e encerra o programa

## Estruturas de Sincronização

### 1. Mutex
- Protege o acesso à fila compartilhada de imagens
- Garante que apenas uma thread por vez possa acessar a fila
- Previne condições de corrida no acesso às imagens

### 2. Variável de Condição
- Utilizada para suspender a thread principal
- Permite que as threads trabalhadoras sinalizem quando a última imagem for processada
- Acorda a thread principal de forma segura para gerar o relatório final

## Compilação

Para compilar o programa:
```bash
gcc -o editor main.c -pthread
```

## Uso

```bash
./editor <tipo_edicao> <pasta_entrada> <pasta_saida>
```

Exemplo:
```bash
./editor saturacao ./imagens_entrada ./imagens_saida
```

Tipos de edição suportados:
- saturacao
- contraste
