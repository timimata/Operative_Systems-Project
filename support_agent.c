#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_DISCIPLINAS 10  // Número máximo de disciplinas
#define MAX_ALUNOS 100      // Número máximo de alunos
#define MAX_HORARIOS 5      // Número máximo de horários por disciplina

// Estrutura para representar um horário
typedef struct {
    int vagas;                // Número de vagas disponíveis
    int capacidade;           // Capacidade do horário
    int alunos[MAX_ALUNOS];   // IDs dos alunos inscritos nesse horário
    int num_inscritos;        // Número de alunos inscritos no horário
} Horario;

// Estrutura para representar uma disciplina
typedef struct {
    Horario horarios[MAX_HORARIOS];  // Array de horários disponíveis para a disciplina
    int num_horarios;               // Número total de horários disponíveis para a disciplina
} Disciplina;

// Array de disciplinas global
Disciplina disciplinas[MAX_DISCIPLINAS];
// Número total de alunos
int num_alunos;
// Mutex para garantir acesso exclusivo à manipulação de dados compartilhados
pthread_mutex_t lock;    

// Função para inicializar as disciplinas com capacidade
void inicializar_disciplinas(int capacidade);
// Função para processar pedidos dos estudantes
void *processar_pedidos_student(void *arg);
// Função para processar pedidos do administrador
void *processar_pedidos_admin(void *arg);
// Função para gravar dados em um arquivo
void gravar_em_ficheiro(const char *nome_ficheiro, int *resposta);
// Função para obter os horários de um aluno
void obter_horarios_aluno(int aluno_id, char *resposta);

void inicializar_disciplinas(int capacidade) {
    // Inicializa todas as disciplinas com a capacidade fornecida
    for (int i = 0; i < MAX_DISCIPLINAS; i++) {
        disciplinas[i].num_horarios = MAX_HORARIOS;
        for (int j = 0; j < MAX_HORARIOS; j++) {
            disciplinas[i].horarios[j].capacidade = capacidade;
            disciplinas[i].horarios[j].vagas = capacidade;
            disciplinas[i].horarios[j].num_inscritos = 0;
        }
    }
}

// Função que processa os pedidos dos estudantes
void *processar_pedidos_student(void *arg) {
    const char *pipe_nome = "/tmp/suporte";
    mkfifo(pipe_nome, 0666);  // Cria o pipe nomeado para comunicação com estudantes

    int pipe_fd = open(pipe_nome, O_RDONLY);
    if (pipe_fd < 0) {
        perror("Erro ao abrir o pipe dos students");
        return NULL;  
    }

    char buffer[256];
    while (1) {
        int bytes_lidos = read(pipe_fd, buffer, sizeof(buffer));  // Lê a solicitação do estudante
        if (bytes_lidos > 0) {
            buffer[bytes_lidos] = '\0';

            int aluno_id, disciplina;
            char nome_pipe[128];
            sscanf(buffer, "%d %d %s", &aluno_id, &disciplina, nome_pipe);

            int horario_inscrito = -1;  // Inicializa como -1 (indica que o aluno não foi inscrito)

            // Bloqueia o mutex para acesso exclusivo aos dados compartilhados
            pthread_mutex_lock(&lock);
            // Procura por um horário disponível para a disciplina solicitada
            for (int i = 0; i < MAX_HORARIOS; i++) {
                if (disciplinas[disciplina].horarios[i].vagas > 0) {
                    // Inscreve o aluno no horário
                    disciplinas[disciplina].horarios[i].vagas--;
                    disciplinas[disciplina].horarios[i].alunos[disciplinas[disciplina].horarios[i].num_inscritos++] = aluno_id;
                    horario_inscrito = i;
                    break;
                }
            }
            // Libera o mutex
            pthread_mutex_unlock(&lock);

            // Envia a resposta para o estudante com o horário inscrito
            int fd_resposta = open(nome_pipe, O_WRONLY);
            if (fd_resposta > 0) {
                char resposta[16];
                snprintf(resposta, sizeof(resposta), "%d", horario_inscrito);
                write(fd_resposta, resposta, strlen(resposta) + 1);
                close(fd_resposta);
            }
        }
    }
    
    close(pipe_fd);
    unlink(pipe_nome);  // Remove o pipe após o uso
    return NULL;  // Em vez de pthread_exit(NULL), retorna NULL para encerrar a thread
}

// Função que processa os pedidos do administrador
void *processar_pedidos_admin(void *arg) {
    const char *pipe_nome = "/tmp/admin";
    mkfifo(pipe_nome, 0666);  // Cria o pipe nomeado para comunicação com o administrador

    int pipe_fd = open(pipe_nome, O_RDONLY);
    if (pipe_fd < 0) {
        perror("Erro ao abrir o pipe do admin");
        return NULL;  
    }

    char buffer[256];
    while (1) {
        int bytes_lidos = read(pipe_fd, buffer, sizeof(buffer));  // Lê a solicitação do administrador
        if (bytes_lidos > 0) {
            buffer[bytes_lidos] = '\0';
            
            int codop;  // Código da operação solicitada
            sscanf(buffer, "%d", &codop);

            if (codop == 1) {
                // Solicitação de horários de um aluno
                int aluno_id;
                char nome_pipe[128], resposta[256] = "";
                sscanf(buffer, "%d,%d,%s", &codop, &aluno_id, nome_pipe);
                obter_horarios_aluno(aluno_id, resposta);

                int fd_resposta = open(nome_pipe, O_WRONLY);
                if (fd_resposta > 0) {
                    write(fd_resposta, resposta, strlen(resposta) + 1);
                    close(fd_resposta);
                }
            } else if (codop == 2) {
                // Solicitação para gravar dados em um arquivo
                char nome_ficheiro[128];
                char nome_pipe[128];
                int resposta;
                sscanf(buffer, "%d,%127[^,],%s", &codop, nome_ficheiro, nome_pipe);
                gravar_em_ficheiro(nome_ficheiro, &resposta);

                int fd_resposta = open(nome_pipe, O_WRONLY);
                if (fd_resposta > 0) {
                    char resposta_str[16];
                    snprintf(resposta_str, sizeof(resposta_str), "%d", resposta);
                    write(fd_resposta, resposta_str, strlen(resposta_str) + 1);
                    close(fd_resposta);
                }
            } else if (codop == 3) {
                // Encerrar o agente de suporte
                printf("Terminando Support Agent.\n");
                break;
            }
        }
    }

    close(pipe_fd);
    unlink(pipe_nome);  // Remove o pipe após o uso
    return NULL;  // Em vez de pthread_exit(NULL), retorna NULL para encerrar a thread
}

// Função que grava os dados de inscrição dos alunos em um arquivo
void gravar_em_ficheiro(const char *nome_ficheiro, int *resposta) {
    FILE *file = fopen(nome_ficheiro, "w");
    if (!file) {
        perror("Erro ao abrir ficheiro para escrita");
        *resposta = -1;  // Retorna erro se não puder abrir o arquivo
        return;
    }

    // Bloqueia o mutex para garantir acesso exclusivo aos dados
    pthread_mutex_lock(&lock);
    // Itera sobre todos os alunos e disciplinas para registrar os horários
    for (int i = 0; i < num_alunos; i++) {
        fprintf(file, "%d", i);
        for (int j = 0; j < MAX_DISCIPLINAS; j++) {
            int horario = -1;
            for (int k = 0; k < disciplinas[j].num_horarios; k++) {
                for (int l = 0; l < disciplinas[j].horarios[k].num_inscritos; l++) {
                    if (disciplinas[j].horarios[k].alunos[l] == i) {
                        horario = k;
                        break;
                    }
                }
            }
            if (horario >= 0) {
                fprintf(file, ",%d", horario);
            } else {
                fprintf(file, ",");
            }
        }
        fprintf(file, "\n");
    }
    pthread_mutex_unlock(&lock);  // Libera o mutex

    fclose(file);
    *resposta = num_alunos;  // Retorna o número de alunos como resposta
}

// Função que obtém os horários de um aluno específico
void obter_horarios_aluno(int aluno_id, char *resposta) {
    // Bloqueia o mutex para garantir acesso exclusivo aos dados
    pthread_mutex_lock(&lock);
    sprintf(resposta, "%d,", aluno_id);
    for (int i = 0; i < MAX_DISCIPLINAS; i++) {
        int horario = -1;
        for (int j = 0; j < MAX_HORARIOS; j++) {
            for (int k = 0; k < disciplinas[i].horarios[j].num_inscritos; k++) {
                if (disciplinas[i].horarios[j].alunos[k] == aluno_id) {
                    horario = j;
                    break;
                }
            }
            if (horario != -1) break;
        }
        if (horario != -1) {
            char temp[16];
            sprintf(temp, "%d/%d,", i, horario);
            strcat(resposta, temp);
        } else {
            strcat(resposta, "-1,");
        }
    }
    pthread_mutex_unlock(&lock);  // Libera o mutex
}

// Função principal
int main(int argc, char *argv[]) {
    // Verifica se o número de argumentos é válido
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <numero_alunos>\n", argv[0]);
        exit(1);
    }

    // Inicializa o número de alunos
    num_alunos = atoi(argv[1]);
    int capacidade = 10;  // Capacidade inicial dos horários

    pthread_mutex_init(&lock, NULL);  // Inicializa o mutex

    // Inicializa as disciplinas com a capacidade fornecida
    inicializar_disciplinas(capacidade);

    // Criação das threads para processar os pedidos de estudantes e administradores
    pthread_t thread_student, thread_admin;
    pthread_create(&thread_student, NULL, processar_pedidos_student, NULL);
    pthread_create(&thread_admin, NULL, processar_pedidos_admin, NULL);

    // Aguarda a finalização das threads
    pthread_join(thread_student, NULL);  // Aguarda a finalização da thread do estudante
    pthread_join(thread_admin, NULL);    // Aguarda a finalização da thread do administrador

    pthread_mutex_destroy(&lock);  // Destrói o mutex
    printf("Support Agent encerrado.\n");
    return 0;
}
