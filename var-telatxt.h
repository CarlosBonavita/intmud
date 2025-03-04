#ifndef VAR_TELATXT_H
#define VAR_TELATXT_H

class TVariavel;
class TVarInfo;
enum TVarTipo : unsigned char;
class TClasse;
class TObjeto;

//---------------------------------------------------------------------------
class TVarTelaTxt
{
private:
    bool FuncMsg(TVariavel * v);    ///< Processa fun��o Msg
    bool FuncPosx(TVariavel * v);   ///< Processa fun��o Posx
    bool FuncTecla(TVariavel * v);  ///< Processa fun��o Tecla
    bool FuncProto(TVariavel * v);  ///< Processa fun��o Proto
    bool FuncLimpa(TVariavel * v);  ///< Processa fun��o Limpa
    bool FuncTexto(TVariavel * v);  ///< Processa fun��o Texto
    bool FuncTotal(TVariavel * v);  ///< Processa fun��o Total
    bool FuncLinha(TVariavel * v);  ///< Processa fun��o Linha

    static void Escrever(const char * texto, int tamanho = -1);
        ///< Envia um texto para o console
    static void CursorEditor();
        ///< Posiciona o cursor na linha de edi��o
    static void ProcTecla(const char * texto);
        ///< Processa uma tecla, recebida com LerTecla()
    static void ProcTeclaCursor(int coluna);
        ///< Move o cursor para uma posi��o na linha de edi��o
        /**< Usado em ProcTecla */

    static unsigned int CorTela;
        ///< Cor dos textos, usada em Escrever()
        /**< - Bits 3-0 = fundo
         *   - Bits 6-4 = frente
         *   - Bit 7 = negrito, 0=desativado */
    static unsigned int CorBarraN;
        ///< Cor, ao processar \\n em Escrever()

    static unsigned int LinhaFinal;
        ///< Linha na tela correspondente ao campo de edi��o
    static unsigned int LinhaPosic;
        ///< Em que linha o usu�rio colocou o cursor, 0=no editor
    static unsigned int ColPosic;
        ///< Em que coluna o usu�rio colocou o cursor, se LinhaPosic!=0
    static unsigned int ColEscreve;
        ///< Coluna do texto sendo inserido na tela
        /**< O caracter \\n faz ColEscreve=0xFFFF para inserir
         *   nova linha quando chegar o pr�ximo caracter */
    static unsigned int ColEditor;
        ///< Coluna do cursor na tela (texto sendo editado)
    static char * TelaBuf;
        ///< Buffer com c�pia do conte�do da tela
    static unsigned int TelaLin;
        ///< N�mero da linha em TelaBuf que � a �ltima linha de texto na tela

    static const char * LerLinha() { txt_linha[tam_linha]=0; return txt_linha; }
        ///< Retorna o conte�do da linha sendo editada
    static unsigned int col_linha;  ///< linha[col_linha] = primeiro caracter na tela
    static unsigned int tam_linha;  ///< Tamanho da linha, sem o byte =0 no final
    static unsigned int max_linha;  ///< Tamanho m�ximo da linha sendo editada
    static char * txt_linha; ///< Linha sendo editada

public:
    static const TVarInfo * Inicializa();
        ///< Inicializa vari�vel e retorna informa��es
    void Criar();           ///< Cria objeto
            /**< Ap�s criado, acertar defvar, indice e chamar EndObjeto() */
    void Apagar();          ///< Apaga objeto
    void Mover(TVarTelaTxt * destino); ///< Move TVarSock para outro lugar
    void EndObjeto(TClasse * c, TObjeto * o);
    bool Func(TVariavel * v, const char * nome); ///< Fun��o da vari�vel

    static int FTamanho(const char * instr);
    static int FTamanhoVetor(const char * instr);
    static void FRedim(TVariavel * v, TClasse * c, TObjeto * o,
            unsigned int antes, unsigned int depois);

    static TVarTipo FTipo(TVariavel * v);
                            ///< Retorna o tipo de vari�vel
    int  getValor(int numfunc);
                            ///< Ler o valor num�rico da vari�vel
    void setValor(int numfunc, int valor);
                            ///< Mudar o valor num�rico da vari�vel
    const char * getTxt(int numfunc);
                            ///< Ler o texto
    void setTxt(int numfunc, const char * txt);
                            ///< Mudar o texto
    void addTxt(int numfunc, const char * txt);
                            ///< Adicionar o texto

    const char * defvar;    ///< Como foi definida a vari�vel
    union {
        TClasse * endclasse;///< Em que classe est� definido
        TObjeto * endobjeto;///< Em que objeto est� definido
    };
    bool b_objeto;          ///< O que usar: true=endobjeto, false=endclasse
    unsigned char indice;   ///< �ndice no vetor

    static bool Inic();     ///< Inicializa consolse se Console!=0
    static void Fim();      ///< Encerra o console
    static void Processa(); ///< Processa teclas pressionadas
    static void ProcFim();  ///< Fim do processamento do console
    static bool FuncEvento(const char * evento, const char * texto);
                    ///< Executa uma fun��o
                    /**< @param evento Nome do evento (ex. "msg")
                     *   @param texto Texto do primeiro argumento, 0=nenhum texto
                     *   @return true se n�o apagou o objeto, false se apagou */
    static TVarTelaTxt * ObjAtual; ///< Objeto atual, usado em FuncEvento()

    static TVarTelaTxt * Inicio; ///< Primeiro objeto
    TVarTelaTxt * Antes;    ///< Objeto anterior
    TVarTelaTxt * Depois;   ///< Pr�ximo objeto
};

//---------------------------------------------------------------------------

#endif
