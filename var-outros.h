#ifndef VAR_OUTROS_H
#define VAR_OUTROS_H

class TVariavel;

//----------------------------------------------------------------------------
/// Processa fun��es de vetores de txt1 a txt512
bool FuncVetorTxt(TVariavel * v, const char * nome);

/// Processa fun��es de vetores de vari�veis int1
bool FuncVetorInt1(TVariavel * v, const char * nome);

/// L� valor num�rico de vetor de int1
int GetVetorInt1(TVariavel * v);

/// Altera valor num�rico de vetor de int1
void SetVetorInt1(TVariavel * v, int valor);

/// Processa fun��es de vetores de vari�veis int8
bool FuncVetorInt8(TVariavel * v, const char * nome);

/// Processa fun��es de vetores de vari�veis uint8
bool FuncVetorUInt8(TVariavel * v, const char * nome);

/// Processa fun��es de vetores de vari�veis int16
bool FuncVetorInt16(TVariavel * v, const char * nome);

/// Processa fun��es de vetores de vari�veis uint16
bool FuncVetorUInt16(TVariavel * v, const char * nome);

/// Processa fun��es de vetores de vari�veis int32
bool FuncVetorInt32(TVariavel * v, const char * nome);

/// Processa fun��es de vetores de vari�veis uint32
bool FuncVetorUInt32(TVariavel * v, const char * nome);

/// Processa fun��es de vetores de vari�veis real
bool FuncVetorReal(TVariavel * v, const char * nome);

/// Processa fun��es de vetores de vari�veis real2
bool FuncVetorReal2(TVariavel * v, const char * nome);

//----------------------------------------------------------------------------

#endif
