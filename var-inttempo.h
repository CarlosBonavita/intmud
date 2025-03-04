#ifndef VAR_INTTEMPO_H
#define VAR_INTTEMPO_H

class TVariavel;
class TVarInfo;
class TObjeto;

//----------------------------------------------------------------------------
/** Trata das vari�veis do tipo inttempo */
class TVarIntTempo /// Vari�veis inttempo
{
public:
    static const TVarInfo * Inicializa();
        ///< Inicializa vari�vel e retorna informa��es
    static int TempoEspera();
    static void ProcEventos(int TempoDecorrido);
    int  getValor(int numfunc); ///< Ler valor da vari�vel
    void setValor(int numfunc, int valor); ///< Mudar valor da vari�vel
    void Mover(TVarIntTempo * destino); ///< Move para outro lugar
    void EndObjeto(TClasse * c, TObjeto * o);
    bool Func(TVariavel * v, const char * nome); ///< Fun��es de inttempo
    static bool FuncVetor(TVariavel * v, const char * nome); ///< Fun��es de vetores

    const char * defvar;///< Como foi definida a vari�vel
    union {
        TClasse * endclasse;///< Em que classe est� definido
        TObjeto * endobjeto;///< Em que objeto est� definido
    };
    bool b_objeto;          ///< O que usar: true=endobjeto, false=endclasse
    unsigned char indice;   ///< �ndice no vetor
    bool parado;            ///< Se o contador est� parado

private:
    static int FTamanho(const char * instr);
    static int FTamanhoVetor(const char * instr);
    static void FRedim(TVariavel * v, TClasse * c, TObjeto * o,
            unsigned int antes, unsigned int depois);

    static void DebugVet(bool mostrar); ///< Checa se listas ligadas est�o OK
    unsigned short IndiceMenos;   ///< �ndice em VetMenos
    unsigned short IndiceMais;    ///< �ndice em VetMais
    union {
        TVarIntTempo * Antes;     ///< Objeto anterior na lista ligada
        int ValorParado;          ///< Valor quando contador est� parado
    };
    TVarIntTempo * Depois;        ///< Pr�ximo objeto na lista ligada

    static unsigned int TempoMenos;  ///< Contagem de tempo com VetMenos
    static unsigned int TempoMais;   ///< Contagem de tempo com VetMais
    static TVarIntTempo ** VetMenos; ///< Primeira lista ligada
    static TVarIntTempo ** VetMais;  ///< Segunda lista ligada
};

//----------------------------------------------------------------------------

#endif
