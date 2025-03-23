#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// Definição dos pipes nomeados para comunicação com o processo ADMIN
#define ADMIN_PIPE "/tmp/admin"               // Pipe para enviar comandos para o ADMIN
#define ADMIN_RESPOSTA_PIPE "/tmp/admin_resposta" // Pipe para receber respostas do ADMIN

// Declaração das funções
void pedir_horarios(int aluno_id);
void gravar_em_ficheiro(const char *nome_ficheiro);
void terminar_support_agent();

int main() {
    char opcao;

    // Loop de interação com o usuário
    while (1) {
        // Exibe as opções disponíveis para o usuário
        printf("\nOpções disponíveis:\n");
        printf("1. Pedir horários de um aluno\n");
        printf("2. Gravar informações em ficheiro\n");
        printf("3. Terminar o Support Agent\n");
        printf("Digite a sua escolha: ");

        // Lê a opção escolhida pelo usuário
        scanf(" %c", &opcao);

        // Verifica qual opção foi escolhida e executa a ação correspondente
        if (opcao == '1') {
            int aluno_id;
            printf("Digite o ID do aluno: ");
            scanf("%d", &aluno_id);
            pedir_horarios(aluno_id);  // Chama a função para pedir os horários do aluno
        } else if (opcao == '2') {
            char ficheiro[128];
            printf("Digite o nome do ficheiro para salvar as informações: ");
            scanf("%s", ficheiro);
            gravar_em_ficheiro(ficheiro);  // Chama a função para gravar informações no ficheiro
        } else if (opcao == '3') {
            terminar_support_agent();  // Chama a função para terminar o Support Agent
            break;  // Encerra o loop e termina o programa
        } else {
            printf("Opção inválida. Tente novamente.\n");
        }
    }
    return 0;
}

// Função para pedir os horários de um aluno
void pedir_horarios(int aluno_id) {

    // Tenta criar o pipe de resposta (caso não exista)
    if (mkfifo(ADMIN_RESPOSTA_PIPE, 0666) < 0 && errno != EEXIST) {
        perror("Erro ao criar o pipe de resposta");
        return;  // Retorna em caso de erro na criação do pipe
    }

    // Abre o pipe ADMIN para escrita (envio de comando)
    int admin_fd = open(ADMIN_PIPE, O_WRONLY);
    if (admin_fd < 0) {
        perror("Erro ao abrir o pipe ADMIN");
        return;
    }

    // Formata o comando para enviar ao ADMIN (ID do aluno e pipe de resposta)
    char comando[256];
    snprintf(comando, sizeof(comando), "1,%d,%s", aluno_id, ADMIN_RESPOSTA_PIPE);

    // Escreve o comando no pipe ADMIN
    write(admin_fd, comando, strlen(comando) + 1);
    close(admin_fd);  // Fecha o pipe de escrita

    // Abre o pipe de resposta para leitura
    int resposta_fd = open(ADMIN_RESPOSTA_PIPE, O_RDONLY);
    if (resposta_fd < 0) {
        perror("Erro ao abrir o pipe de resposta");
        return;
    }

    // Lê a resposta do ADMIN (horários do aluno)
    char resposta[256];
    int bytes_lidos = read(resposta_fd, resposta, sizeof(resposta));
    if (bytes_lidos > 0) {
        resposta[bytes_lidos] = '\0';  // Adiciona o terminador de string
        printf("Horários para o aluno %d: %s\n", aluno_id, resposta);  // Exibe os horários
    } else {
        perror("Erro ao ler a resposta");
    }

    close(resposta_fd);  // Fecha o pipe de leitura
        unlink(ADMIN_RESPOSTA_PIPE);  // Remove o pipe de resposta após o uso
}

// Função para gravar informações em ficheiro
void gravar_em_ficheiro(const char *nome_ficheiro) {

    // Tenta criar o pipe de resposta (caso não exista)
    if (mkfifo(ADMIN_RESPOSTA_PIPE, 0666) < 0 && errno != EEXIST) {
        perror("Erro ao criar o pipe de resposta");
        return;
    }

    // Abre o pipe ADMIN para escrita (envio de comando)
    int admin_fd = open(ADMIN_PIPE, O_WRONLY);
    if (admin_fd < 0) {
        perror("Erro ao abrir o pipe ADMIN");
        return;
    }

    // Formata o comando para enviar ao ADMIN (nome do ficheiro e pipe de resposta)
    char comando[256];
    snprintf(comando, sizeof(comando), "2,%s,%s", nome_ficheiro, ADMIN_RESPOSTA_PIPE);

    // Escreve o comando no pipe ADMIN
    write(admin_fd, comando, strlen(comando) + 1);
    close(admin_fd);  // Fecha o pipe de escrita

    // Abre o pipe de resposta para leitura
    int resposta_fd = open(ADMIN_RESPOSTA_PIPE, O_RDONLY);
    if (resposta_fd < 0) {
        perror("Erro ao abrir o pipe de resposta");
        return;
    }

    // Lê a resposta do ADMIN
    char resposta[16];
    int bytes_lidos = read(resposta_fd, resposta, sizeof(resposta));
    if (bytes_lidos > 0) {
        resposta[bytes_lidos] = '\0';  // Adiciona o terminador de string
        int resultado = atoi(resposta);  // Converte a resposta para inteiro
        if (resultado == 0) {
            printf("Informações gravadas com sucesso no ficheiro '%s'.\n", nome_ficheiro);
        } else {
            printf("Erro ao gravar as informações no ficheiro '%s'.\n", nome_ficheiro);
        }
    } else {
        perror("Erro ao ler a resposta");
    }

    close(resposta_fd);  // Fecha o pipe de leitura
    unlink(ADMIN_RESPOSTA_PIPE);  // Remove o pipe de resposta após o uso
}

// Função para terminar o Support Agent
void terminar_support_agent() {

    // Abre o pipe ADMIN para escrita (envio de comando)
    int admin_fd = open(ADMIN_PIPE, O_WRONLY);
    if (admin_fd < 0) {
        perror("Erro ao abrir o pipe ADMIN");
        return;
    }

    // Envia o comando para terminar o Support Agent
    char comando[] = "3";
    write(admin_fd, comando, strlen(comando) + 1);
    close(admin_fd);  // Fecha o pipe de escrita

    // Exibe a mensagem de término
    printf("Comando enviado para terminar o Support Agent.\n");
}
