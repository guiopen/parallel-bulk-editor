# Editor de Imagens em Lote Paralelo

**Processador paralelo de imagens**, feito em C, que aplica filtros em _múltiplas imagens_ simultaneamente, utilizando threads. 

## Interface
Ao iniciar, o usuário deve informar:
- Diretório de entrada com as imagens (em formatos JPG/JPEG ou PNG)
- Número de threads para processamento
- Filtro a ser aplicado (_grayscale_, _red_, _green_, _blue_ ou _invert_)

Após o término do processamento, é possível verificar as cópias processadas das imagens em um novo diretório `./<DIR_ORIGINAL>_<FILTRO>`. Além disso, o programa fornece dados da quantidade de imagens tratadas e tempo decorrido na operação, permitindo também aplicar outros filtros sequencialmente sobre o diretório original.

Ao optar por "sair", são exibidas estatísticas com medições acumuladas de:
- Imagens (considerando todos filtros utilizados)
- Tempo total
- Velocidade média (imagens/segundo)


## Compilação e Execução

É usado Makefile para reunir a compilação de todos arquivos .c em um único executável binário. Para tanto rode o comando:

```bash
make
```
E execute com:

```bash
./bin/editor
```


## Padrões de Projeto
> Multithreading

### 1. Pool de Threads

As threads são criadas de acordo com a quantidade definida pelo usuário:
- Cada thread obtém uma imagem da fila compartilhada, processa, e repete
- Quando a fila esvazia, são suspensas (não destruídas)
- Ao selecionar um novo filtro, são reativadas

### 2. Suspensão Controlada

Está presente nos casos:
- Threads <u>trabalhadoras</u> são suspensas quando a fila esvazia, acordando somente quando um novo filtro é selecionado
- Thread <u>principal</u> é suspensa enquanto as trabalhadoras processam


## Estruturas de Sincronização

### 1. Mutex (`pthread_mutex_t`)

Protege o acesso à fila compartilhada (`state.queue`) de imagens e aos contadores:
- `current`: próxima imagem a ser processada
- `processed`: quantidade de imagens já processadas
- `should_exit`: flag para encerramento das threads

### 2. Variáveis de Condição (`pthread_cond_t`)

1. `done_cond` para a espera da thread <u>principal</u> durante o processamento
2. `queue_cond` para a espera das threads <u>trabalhadoras</u> quando a fila esvazia