#ifndef VAR_REF_H
#define VAR_REF_H

class TVarInfo;
class TClasse;
class TObjeto;

//----------------------------------------------------------------------------
/** Trata das vari�veis do tipo REF */
class TVarRef /// Vari�veis REF
{
public:
    static const TVarInfo * Inicializa();
        ///< Inicializa vari�vel e retorna informa��es
    void MudarPont(TObjeto * obj); ///< Muda a vari�vel TVarRef::Pont
    void Mover(TVarRef * destino); ///< Move TVarRef para outro lugar
    TObjeto * Pont;     ///< Objeto atual
    TVarRef * Antes;
    TVarRef * Depois;
private:
    static int FTamanho(const char * instr);
    static int FTamanhoVetor(const char * instr);
    static void FRedim(TVariavel * v, TClasse * c, TObjeto * o,
            unsigned int antes, unsigned int depois);
};

//----------------------------------------------------------------------------

#endif
