#ifndef VAR_INCDEC_H
#define VAR_INCDEC_H

class TVariavel;

//----------------------------------------------------------------------------
/** Trata das vari�veis intinc e intdec */
class TVarIncDec /// Vari�veis intinc e intdec
{
public:
    int getInc(int numfunc); ///< Obt�m o valor de intinc
    int getDec(int numfunc); ///< Obt�m o valor de intdec
    void setInc(int numfunc, int valor); ///< Muda o valor de intinc
    void setDec(int numfunc, int valor); ///< Muda o valor de intdec
    bool FuncInc(TVariavel * v, const char * nome);
        ///< Fun��es de intinc
    bool FuncDec(TVariavel * v, const char * nome);
        ///< Fun��es de intdec
    bool FuncVetorInc(TVariavel * v, const char * nome);
        ///< Fun��es de vetores de intinc
    bool FuncVetorDec(TVariavel * v, const char * nome);
        ///< Fun��es de vetores de intdec
private:
    int valor; ///< Tempo, usando tanto em intinc quanto em intdec
};

//----------------------------------------------------------------------------

#endif
