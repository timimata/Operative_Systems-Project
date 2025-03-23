#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

int main(int argc, char *argv[]) {
    // Verifica se o número de argumentos fornecido é correto
    if (argc < 4 || argc > 5) {
        fprintf(stderr, "Uso: %s <numero_estudante> <aluno_inicial> <num_alunos> [<pipe_suporte>]\n", argv[0]);
        exit(1);  // Sai do programa em caso de erro
    }

    // Converte os argumentos de entrada para inteiros e strings
    int numero_estudante = atoi(argv[1]);  // Número do estudante
    int aluno_inicial = atoi(argv[2]);     // ID do primeiro aluno
    int num_alunos = atoi(argv[3]);        // Número de alunos a processar
    const char *pipe_suporte = argc == 5 ? argv[4] : "/tmp/suporte";  // Caminho do pipe para comunicação com o "suporte"

    // Cria um pipe nomeado único para cada estudante para receber respostas
    char pipe_resposta[128];
    snprintf(pipe_resposta, sizeof(pipe_resposta), "/tmp/student_%d", numero_estudante);

    // Tenta criar o pipe de resposta, se falhar e o pipe já existir, ignora o erro
    if (mkfifo(pipe_resposta, 0666) < 0 && errno != EEXIST) {
        perror("Erro ao criar o pipe de resposta");
        exit(1);  // Sai do programa em caso de erro na criação do pipe
    }

    // Exibe informações sobre o estudante e os parâmetros
    printf("student %d: aluno inicial=%d, número de alunos=%d\n", numero_estudante, aluno_inicial, num_alunos);

    // Semente para a geração de números aleatórios, garantindo resultados diferentes por estudante
    srand(time(NULL) + numero_estudante);

    // Processa cada aluno
    for (int i = 0; i < num_alunos; i++) {
        int aluno_id = aluno_inicial + i;  // Calcula o ID do aluno

        // Vetor para armazenar as disciplinas escolhidas para o aluno
        int disciplinas[5];

        // Para cada aluno, escolhemos 5 disciplinas aleatórias
        for (int j = 0; j < 5; j++) {
            int disciplina_aleatoria;
            int ja_escolhida;

            // Garante que a disciplina não seja repetida
            do {
                disciplina_aleatoria = rand() % 10;  // Gera uma disciplina aleatória entre 0 e 9
                ja_escolhida = 0;
                // Verifica se a disciplina já foi escolhida
                for (int k = 0; k < j; k++) {
                    if (disciplinas[k] == disciplina_aleatoria) {
                        ja_escolhida = 1;
                        break;
                    }
                }
            } while (ja_escolhida);  // Repete até encontrar uma disciplina não repetida

            // Armazena a disciplina escolhida
            disciplinas[j] = disciplina_aleatoria;

            // Prepara a mensagem para enviar ao processo de suporte
            char pedido[256];
            snprintf(pedido, sizeof(pedido), "%d %d %s", aluno_id, disciplina_aleatoria, pipe_resposta);

            // Abre o pipe para enviar a solicitação ao processo de suporte
            int fd_suporte = open(pipe_suporte, O_WRONLY);
            if (fd_suporte < 0) {
                perror("Erro ao abrir o pipe principal");
                unlink(pipe_resposta);  // Remove o pipe de resposta antes de sair
                exit(1);
            }

            // Envia a solicitação para o processo de suporte
            write(fd_suporte, pedido, strlen(pedido) + 1);
            close(fd_suporte);  // Fecha o pipe de escrita

            // Abre o pipe de resposta para ler a resposta do processo de suporte
            int fd_resposta = open(pipe_resposta, O_RDONLY);
            if (fd_resposta < 0) {
                perror("Erro ao abrir o pipe de resposta");
                unlink(pipe_resposta);  // Remove o pipe de resposta antes de sair
                exit(1);
            }

            // Lê a resposta do processo de suporte (horário para a disciplina)
            char resposta[256];
            int bytes_lidos = read(fd_resposta, resposta, sizeof(resposta));
            close(fd_resposta);  // Fecha o pipe de leitura

            int horario;
            if (bytes_lidos > 0) {
                resposta[bytes_lidos] = '\0';  // Adiciona o terminador de string
                horario = atoi(resposta);  // Converte a resposta (horário) para inteiro
            } else {
                perror("Erro ao ler a resposta");
                horario = -1;  // Marca erro se não houver resposta
            }

            // Exibe a disciplina e o horário para o aluno, formatando a saída
            if (j == 0) printf("student %d, aluno %d:", numero_estudante, aluno_id);
            printf(" %d/%d%s", disciplina_aleatoria, horario, (j < 4 ? "," : "\n"));
        }
    }

    // Remove o pipe de resposta do estudante após o uso
    unlink(pipe_resposta);
    return 0;  // Finaliza o programa com sucesso
}
