#ifndef VAR_SOCKET_H
#define VAR_SOCKET_H

#ifdef __WIN32__
 #include <windows.h>
 #include <winsock.h>
#else
 #include <sys/types.h>
 #include <netinet/in.h>
#endif

class TVariavel;
class TVarInfo;
enum TVarTipo : unsigned char;
class TVarSocket;
class TClasse;
class TObjeto;

//----------------------------------------------------------------------------
/** Representa uma conex�o de uma vari�vel Socket */
class TObjSocket /// Conex�o de Socket
{
public:
    TObjSocket();               ///< Construtor
    virtual ~TObjSocket();      ///< Destrutor
    bool Enviar(const char * mensagem, int codigo);
        ///< Envia mensagem conforme protocolo
        /**< @param mensagem Endere�o dos bytes a enviar
         *   @param codigo Tipo de mensagem; usado somente no Papovox
         *   @return true se conseguiu enviar, false se n�o conseguiu */

protected:
    virtual bool EnvMens(const char * mensagem, int codigo)=0;
        ///< Envia mensagem
        /**< A mensagem segue o formato:
         *  - Byte =0 -> fim da mensagem
         *  - Byte =1 -> pr�ximos dois bytes = cor
         *     - Byte 0 Bits 0-3 = cor de fundo
         *     - Byte 0 Bits 4-6 = cor das letras
         *     - Byte 0 Bit 7 =1 se negrito (cor das letras mais forte)
         *     - Byte 1 Bit 0 =1 se sublinhado
         *     .
         *  - Byte =2 -> echo off
         *  - Byte =3 -> echo on
         *  - Byte =4 -> go ahead
         *  - Byte =5 -> beep
         *  - Byte ='\n' -> passar para pr�xima linha
         */
    virtual void Fechar(void)=0; ///< Fecha socket, pode apagar socket
    virtual int  Variavel(char num, int valor)=0;
                    ///< L� ou altera uma vari�vel
                    /**< @param num n�mero da vari�vel
                     *         - 1 = proto
                     *         - 2 = cores
                     *         - 3 = aflooder
                     *         - 4 = eco
                     *  @param valor Novo valor, se for >= 0
                     *  @return valor atual da vari�vel
                     */
    virtual void Endereco(int num, char * mens, int total);
                    ///< Anota uma informa��o da conex�o
                    /**< @param num O que informar
                     *    - 0 = endere�o da conex�o local
                     *    - 1 = endere�o da conex�o remota
                     *    - 2 = assinatura MD5
                     *    - 3 = assinatura SHA1
                     *   @param mens Aonde colocar o texto
                     *   @param total Tamanho do buffer em mens */
    void RetiraVarSocket(); ///< Retira objeto da lista ligada de TVarSocket
    void FuncFechou(const char * txt);
                    ///< Executa fun��o _fechou
                    /**< @param txt Texto que cont�m o motivo
                     *   @note Pode apagar o pr�prio objeto */
    void FuncEvento(const char * evento, const char * texto, int v1=-1, int v2=-1);
                    ///< Executa uma fun��o
                    /**< @param evento Nome do evento (ex. "msg")
                     *   @param texto Texto do primeiro argumento, 0=nenhum texto
                     *   @param v1 Segundo argumento, se menor que 0 n�o tem
                     *   @param v2 Terceiro argumento, se menor que 0 n�o tem
                     *   @note Pode apagar o pr�prio objeto */

// Vari�veis usadas para enviar mensagens
    unsigned short CorEnvia;     ///< Cor atual, ao enviar
    unsigned short ColunaEnvia;  ///< Quantos caracteres j� enviou

private:
// Para saber quando objetos foram apagados
    static TVarSocket * varObj;  ///< Usado para saber se objeto foi apagado

// Lista ligadas
    TVarSocket * Inicio; ///< Lista ligada de TVarSocket
    friend class TVarSocket;
};

//----------------------------------------------------------------------------
class TDNSSocket /// Resolve DNS em segundo plano
{
public:
    TDNSSocket(TVarSocket * var, const char * ender);
            ///< Construtor; faz *nomeini=0 se ocorreu algum erro
    ~TDNSSocket();       ///< Destrutor
    static void Fd_Set(fd_set * set_entrada);
    static void ProcEventos(fd_set * set_entrada);
                         ///< Processa eventos
    char nomeini[256];   ///< Nome a ser pesquisado
    char nome[256];      ///< Nome da m�quina
    char ip[256];        ///< Endere�o IP
private:
    TVarSocket * Socket; ///< Para onde gerar os eventos
    static TDNSSocket * Inicio; ///< Lista ligada de TDNSSocket
    TDNSSocket * Antes;  ///< Objeto anterior
    TDNSSocket * Depois; ///< Pr�ximo objeto
#ifdef __WIN32__
    HANDLE hthread;      ///< Para saber quando a Thread terminou
#else
    int recdescr;        ///< Para receber informa��es do outro processo
#endif
    friend class TVarSocket;
};

//----------------------------------------------------------------------------
/** Trata das vari�veis do tipo Socket */
class TVarSocket /// Vari�veis Socket
{
public:
    static const TVarInfo * Inicializa();
        ///< Inicializa vari�vel e retorna informa��es
    void Apagar();          ///< Apaga objeto
    void MudarSock(TObjSocket * socket); ///< Muda a vari�vel TVarSocket::Socket
    void Mover(TVarSocket * destino); ///< Move TVarSock para outro lugar
    void EndObjeto(TClasse * c, TObjeto * o);
    void Igual(TVarSocket * v);     ///< Operador de atribui��o igual
    bool Func(TVariavel * v, const char * nome); ///< Fun��o da vari�vel
    bool FuncMsg(TVariavel * v);
    bool FuncAbrir(TVariavel * v);
    bool FuncAbrirSsl(TVariavel * v);
    bool FuncFechar(TVariavel * v);
    bool FuncIpLocal(TVariavel * v);
    bool FuncIp(TVariavel * v);
    bool FuncMd5(TVariavel * v);
    bool FuncSha1(TVariavel * v);
    bool FuncAFlooder(TVariavel * v);
    bool FuncCores(TVariavel * v);
    bool FuncOpcTelnet(TVariavel * v);
    bool FuncPosX(TVariavel * v);
    bool FuncProto(TVariavel * v);
    bool FuncEventoIP(TVariavel * v);
    bool FuncNomeIP(TVariavel * v);
    bool FuncIPNome(TVariavel * v);
    bool FuncIPValido(TVariavel * v);
    bool FuncIniSSL(TVariavel * v);
    static int FTamanho(const char * instr);
    static int FTamanhoVetor(const char * instr);
    static void FRedim(TVariavel * v, TClasse * c, TObjeto * o,
            unsigned int antes, unsigned int depois);

    static TVarTipo FTipo(TVariavel * v);
                            ///< Retorna o tipo de vari�vel
    int  getValor(int numfunc);
                            ///< Ler valor num�rico da vari�vel
    void setValor(int numfunc, int valor);
                            ///< Mudar o valor num�rico da vari�vel

    const char * defvar;    ///< Como foi definida a vari�vel
    union {
        TClasse * endclasse;///< Em que classe est� definido
        TObjeto * endobjeto;///< Em que objeto est� definido
    };
    bool b_objeto;          ///< O que usar: true=endobjeto, false=endclasse
    unsigned char indice;   ///< �ndice no vetor

    TObjSocket * Socket;    ///< Conex�o atual
    TVarSocket * Antes;     ///< Objeto anterior da mesma conex�o
    TVarSocket * Depois;    ///< Pr�ximo objeto da mesma conex�o
};

//----------------------------------------------------------------------------

#endif
