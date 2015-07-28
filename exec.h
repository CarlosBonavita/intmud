#ifndef EXEC_H
#define EXEC_H

#ifdef __WIN32__
#include <windows.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#endif

//------------------------------------------------------------------------------
class TExec
{
public:
    static void Inicializa();
        ///< Deve ser chamado uma vez, no in�cio do programa

    TExec();
    ~TExec();
    const char * Abrir(const char * nomeprog);
        ///< Roda um programa
    void Fechar();
        ///< Fecha o programa
    int InfoProg();
        ///< Checa se o programa est� rodando
        /**< @return 0=parado, 1=rodando, 2=terminou agora */
    int Ler(char * destino, int total);
        ///< L� do STDOUT do programa
    int Escrever(const char * destino, int total);
        ///< Escreve no STDIN do programa
    int CodRetorno;
        ///< C�digo de retorno, quando InfoProg() retorna 2
#ifdef __WIN32__
    HANDLE pipein() { return pipe_in; }
    HANDLE pipeout() { return pipe_out; }
#else
    int pipein() { return pipe_in; }
    int pipeout() { return pipe_out; }
#endif

private:
    void FecharPipe();
    static TExec * Inicio; ///< Lista ligada de TDNSSocket
    TExec * Antes;  ///< Objeto anterior
    TExec * Depois; ///< Pr�ximo objeto
    bool  Rodando; // Verdadeiro se tem programa rodando

#ifdef __WIN32__
    HANDLE pipe_in;     // Para ler mensagens do programa
    HANDLE pipe_out;    // Para enviar mensagens para o programa
    PROCESS_INFORMATION     ProcessInfo;
    SECURITY_ATTRIBUTES     SecurityAttributes;
    STARTUPINFO             StartupInfo;
#else
    static void proc_sigchld_handler(int signum);
    int pipe_in;        // Para ler mensagens do programa
    int pipe_out;       // Para enviar mensagens para o programa
    pid_t pid;          // Identifica��o do processo, -1 se o processo terminou
    int retorno;        // Valor de retorno do processo, quando Rodando=true e pid<0
#endif
};

#endif
