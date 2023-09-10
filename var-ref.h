#ifndef VAR_REF_H
#define VAR_REF_H

class TObjeto;

//----------------------------------------------------------------------------
/** Trata das vari�veis do tipo REF */
class TVarRef /// Vari�veis REF
{
public:
    void MudarPont(TObjeto * obj); ///< Muda a vari�vel TVarRef::Pont
    void Mover(TVarRef * destino); ///< Move TVarRef para outro lugar
    TObjeto * Pont;     ///< Objeto atual
    TVarRef * Antes;
    TVarRef * Depois;
};

//----------------------------------------------------------------------------

#endif
