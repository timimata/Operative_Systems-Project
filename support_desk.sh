#!/bin/bash
#./support_desk.sh 100 10 10 5    
 

# Compilação dos programas C necessários
gcc -o support_agent support_agent.c -lpthread
gcc -o student student.c
gcc -o admin admin.c

# Verifica se o número correto de argumentos foi fornecido
if [ "$#" -ne 4 ]; then
    echo "Uso: $0 <NALUN> <NDISCIP> <NLUG> <NSTUD>"
    exit 1
fi

# Atribui os parâmetros de entrada para variáveis
NALUN=$1       # Número de alunos
NDISCIP=$2     # Número de disciplinas
NLUG=$3        # Número de lugares por horário
NSTUD=$4       # Número de processos de estudantes

# Calcula o número de horários necessários para todos os alunos
NHOR=$(( ($NALUN + $NLUG - 1) / $NLUG ))

# Define os nomes dos pipes
PIPE_SUPORTE="/tmp/suporte"
PIPE_ADMIN="/tmp/admin"

# Cria o pipe para o agente de suporte, se não existir
if [ ! -p "$PIPE_SUPORTE" ]; then
    mkfifo "$PIPE_SUPORTE"
fi

# Cria o pipe para o administrador, se não existir
if [ ! -p "$PIPE_ADMIN" ]; then
    mkfifo "$PIPE_ADMIN"
fi

# Inicia o agente de suporte em segundo plano
echo "Iniciando o Support Agent..."
./support_agent $NALUN & 
SUPPORT_AGENT_PID=$!  # Salva o PID do agente de suporte para aguardá-lo no final

# Dá um pequeno delay para garantir que o agente de suporte foi inicializado
sleep 1

# Inicia os processos de estudantes
echo "Iniciando $NSTUD processos Student..."
for (( i=1; i<=$NSTUD; i++ )); do
    # Calcula o intervalo de alunos que cada processo student irá gerenciar
    NUM_INICIAL=$(( ($i - 1) * ($NALUN / $NSTUD) )) 
    NUM_ALUNOS=$(( $NALUN / $NSTUD ))             
    if [ $i -eq $NSTUD ]; then
        NUM_ALUNOS=$(( $NUM_ALUNOS + ($NALUN % $NSTUD) )) 
    fi

    # Inicia o processo do student
    ./student $i $NUM_INICIAL $NUM_ALUNOS "$PIPE_SUPORTE" & 
    STUDENT_PIDS+=($!)  # Adiciona o PID do student para aguardar depois
done

# Aguarda todos os processos de estudantes terminarem
for PID in "${STUDENT_PIDS[@]}"; do
    wait $PID
done

echo "Todos os Students terminaram."

# Inicia o menu do administrador
echo "Iniciando o menu do Admin. Pode interagir diretamente com o pipe $PIPE_ADMIN."
while true; do
    echo -e "\nOpções disponíveis:
    1. Pedir horários de um aluno.
    2. Gravar informações em ficheiro.
    3. Terminar o Support Agent."
    echo -n "Escolha uma opção: "
    read COD_OP

    if [ "$COD_OP" -eq 1 ]; then
        # Solicitar horários de um aluno
        echo -n "Digite o ID do aluno: "
        read ALUNO_ID
        echo -n "Digite o nome do pipe de resposta (padrão: /tmp/admin_resposta): "
        read ADMIN_PIPE
        ADMIN_PIPE=${ADMIN_PIPE:-"/tmp/admin_resposta"}  # Define um pipe padrão, se não fornecido

        # Cria o pipe de resposta se não existir
        if [ ! -p "$ADMIN_PIPE" ]; then
            mkfifo "$ADMIN_PIPE"
        fi

        # Envia a solicitação ao Support Agent através do pipe
        echo "1,$ALUNO_ID,$ADMIN_PIPE" > "$PIPE_ADMIN"

        # Aguarda a resposta do Support Agent
        if [ -p "$ADMIN_PIPE" ]; then
            RESPOSTA=$(cat "$ADMIN_PIPE")
            echo "Resposta do Support Agent: $RESPOSTA"
            rm -f "$ADMIN_PIPE"  # Remove o pipe após uso
        fi

    elif [ "$COD_OP" -eq 2 ]; then
        # Solicitar gravação de informações em arquivo
        echo -n "Digite o nome do ficheiro de saída (ex.: resultados.txt): "
        read FICHEIRO
        echo -n "Digite o nome do pipe de resposta (padrão: /tmp/admin_resposta): "
        read ADMIN_PIPE
        ADMIN_PIPE=${ADMIN_PIPE:-"/tmp/admin_resposta"}  # Define um pipe padrão, se não fornecido

        # Cria o pipe de resposta se não existir
        if [ ! -p "$ADMIN_PIPE" ]; then
            mkfifo "$ADMIN_PIPE"
        fi

        # Envia a solicitação ao Support Agent através do pipe
        echo "2,$FICHEIRO,$ADMIN_PIPE" > "$PIPE_ADMIN"

        # Aguarda a resposta do Support Agent
        if [ -p "$ADMIN_PIPE" ]; then
            RESPOSTA=$(cat "$ADMIN_PIPE")
            echo "Resposta do Support Agent: Código=$RESPOSTA (0=sucesso, -1=erro)"
            rm -f "$ADMIN_PIPE"  # Remove o pipe após uso
        fi

    elif [ "$COD_OP" -eq 3 ]; then
        # Solicita ao Support Agent terminar a execução
        echo "3" > "$PIPE_ADMIN"
        break  # Sai do loop e encerra o menu

    else
        echo "Opção inválida. Tente novamente."
    fi
done

# Aguarda o encerramento do Support Agent
echo "Aguardando o encerramento do Support Agent..."
wait $SUPPORT_AGENT_PID

# Remove os pipes e encerra o processo
rm -f "$PIPE_SUPORTE" "$PIPE_ADMIN"
echo "Support Agent encerrado. Execução concluída!"
