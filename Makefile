CC = gcc
CFLAGS = -Wall -pthread -Iinclude -Ilibs -lm
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TARGET = $(BIN_DIR)/editor
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

all: directories $(TARGET) # Define que 'all' depende de 'directories' e do alvo

directories: # Define o alvo 'directories' para criar os diretórios
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(TARGET): $(OBJS) # Define o alvo do executável e suas dependências
	$(CC) -o $@ $^ $(CFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c # Regra para compilar os arquivos .c em .o
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)