/* Este arquivo � software livre; voc� pode redistribuir e/ou
 * modificar nos termos das licen�as GPL ou LGPL. Vide arquivos
 * COPYING e COPYING2.
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GPL or the LGP licenses.
 * See files COPYING e COPYING2.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "instr.h"
#include "instr-enum.h"
#include "variavel.h"
#include "var-arqlog.h"
#include "classe.h"
#include "objeto.h"
#include "var-prog.h"
#include "var-debug.h"
#include "mudarclasse.h"
#include "socket.h"
#include "misc.h"

void Termina(); // Encerra o programa

//#define DEBUG_INSTR // Mostra instru��es que ser�o executadas
//#define DEBUG_EXPR  // Mostra valores de Instr::Expressao encontrados
//#define DEBUG_VAR   // Mostra vari�veis no topo da pilha

/** @defgroup exec Instr::Exec - Algoritmo para executar fun��es

@par Algoritmo geral para executar uma fun��o:

@verbatim
Localiza o objeto e a fun��o
Coloca argumentos na pilha
Executa fun��o
Para cada objeto marcado para exclus�o:
    Executa fun��o fim
    Exclui objeto
Para cada modifica��o agendada nas classes:
    Se for excluir classe:
        Enquanto a classe possuir algum objeto:
            Executa fun��o fim do primeiro objeto da classe
            Exclui objeto
        Exclui classe
    Se for criar classe:
        Cria classe
        Executa fun��o ini da classe
        Para cada objeto marcado para exclus�o:
            Executa fun��o fim
            Exclui objeto
    Se for modificar classe:
        Modifica a classe
    Se for salvar mapa:
        Salva o mapa em arquivo
    Retira modifica��o da lista
@endverbatim

@par Valores de Instr::Comando usados somente aqui:

- cVarNome    -> Para obter nome da vari�vel
- cVarInicio  -> Esperando texto logo ap�s ex_varini
- cVarClasse  -> Leu ':', TVariavel::endvar = endere�o do objeto TClasse
- cVarObjeto  -> TVariavel::endvar = endere�o do objeto TObjeto
- cVarIniFunc -> Somente se OTIMIZAR_VAR estiver definido (em instr.h):
    - Internamente funciona como cVarInt (guarda um int)
    - Se o valor for >= 0, � o n�mero da fun��o interna (ex_varfunc)
    - Se o valor for < 0, � uma vari�vel do objeto (ex_varlocal seguido de 0xFF)
    - Quando for vari�vel local (ex_varlocal seguido de um valor != 0xFF),
      a vari�vel local � colocada diretamente na pilha

@par Algoritmo simplificado - apenas a parte de executar uma fun��o:

@verbatim
Limpa pilha de vari�veis
Cria vari�vel cNulo no topo da pilha
Limpa pilha de fun��es
Cria fun��o referente ao que deve ser executado
Cria argumentos da fun��o: vari�veis no topo da pilha
FuncAtual->fimvar = endere�o da primeira vari�vel ap�s VarAtual

(in�cio)
Se FuncAtual->expr == 0:
  *** Est� processando uma linha:
  Se for fim da fun��o ou instru��o ret sem par�metros:
     Apaga vari�veis criadas na fun��o (a partir de FuncAtual->inivar)
     Coloca NULO na pilha de vari�veis
     Se n�o houver fun��o anterior:
       Fim do algoritmo
     Apaga fun��o atual (FuncAtual--)
     Vai para (in�cio)
  Se for vari�vel:
     Cria vari�vel local da fun��o atual
     FuncAtual->fimvar++
  Se linha possui express�o num�rica:
     FuncAtual->expr = in�cio da express�o num�rica
     Vai para (in�cio)
  Processa instru��o que n�o requer express�o num�rica
  Passa para pr�xima instru��o
  Vai para (in�cio)
Se FuncAtual->expr != 0:
  *** Est� processando uma express�o num�rica
  *** FuncAtual->expr[0] cont�m a pr�xima instru��o da express�o num�rica
  Se for ex_fim:
     FuncAtual->expr = 0
     Processa instru��o que depende de express�o num�rica
     Passa para pr�xima instru��o
     Apaga vari�veis de FuncAtual->fimvar em diante
     Vai para (in�cio)
  Se for um valor fixo (n�mero, texto ou NULO):
     Coloca vari�vel na pilha de vari�veis
     Avan�a FuncAtual->expr
     Vai para (in�cio)
  Se for um operador:
     Processa operador conforme as vari�veis do topo da pilha
     Os operadores && e || podem avan�ar na express�o num�rica
     Avan�a FuncAtual->expr
     Vai para (in�cio)
  *** A partir daqui processa fun��es e vari�veis
  *** Cria-se uma vari�vel cVarNome, que cont�m parte do nome da vari�vel
  *** Cria-se uma vari�vel em seguida, que cont�m o resultado anterior
  *** Isso torna poss�vel processar v�rias vari�veis ligadas por ponto
  *** Exemplo: a.b.c
  Se for ex_varini ou ex_varabre:
     Cria vari�vel cVarNome na pilha de vari�veis para armazenar o nome
     Cria vari�vel cVarInicio na pilha de vari�veis
     Avan�a FuncAtual->Expr
     Se era ex_varini:
       Copia o que puder do nome em (um texto) para �ltima vari�vel cVarNome
     Vai para (in�cio)
  Se for ex_arg (argumento da fun��o) ou ex_abre (abre colchetes):
     Avan�a FuncAtual->expr
     Vai para (in�cio)
  Se for ex_fecha (fecha colchetes):
     Avan�a FuncAtual->expr
     Sendo que: X=�ltima vari�vel cVarNome da pilha e Y=vari�vel ap�s X
     Transforma em texto as vari�veis ap�s Y e anota em X
     Apaga vari�veis ap�s Y
     Copia o que puder do nome em FuncAtual->expr (um texto) para X
     Vai para (in�cio)
  Se for ex_varfim:
     Avan�a FuncAtual->expr
     Sendo que: X=�ltima vari�vel cVarNome da pilha e Y=vari�vel ap�s X
     Se vari�vel Y for cVarInicio ou cVarClasse:
       Apaga vari�veis de X em diante
       Cria vari�vel cNulo
     Caso contr�rio:
       Apaga vari�veis ap�s Y
       Apaga vari�vel X
       Vari�vel Y passa a ocupar o lugar da vari�vel X
     Vai para (in�cio)
  Se for ex_doispontos:
     Procura �ltima vari�vel cVarNome da pilha
     Se nome de classe em cVarNome existe e pr�xima vari�vel for cVarInicio:
       Muda a pr�xima vari�vel para cVarClasse
       Anota o endere�o da classe
     Caso contr�rio:
       Apaga da vari�vel cVarNome em diante
       Cria vari�vel cNulo na pilha de vari�veis
       Avan�a FuncAtual->expr at� ex_varfim
     Avan�a FuncAtual->expr
     Vai para (in�cio)
  Se for ex_ponto:
     Avan�a FuncAtual->expr
     Procura �ltima vari�vel cVarNome da pilha
     Checa pr�xima vari�vel e texto em cVarNome:
        cVarInicio -> 1. Verifica se � vari�vel/comando interno do programa
                      2. Verifica se � vari�vel local da fun��o
                      3. Verifica se � vari�vel ou fun��o do objeto
        cVarClasse -> Verifica se � fun��o da classe especificada
        cVarObjeto -> Verifica se � vari�vel ou fun��o do objeto em cVarObjeto
     Depois de decidido que a��o tomar:
       Limpa texto em cVarNome
       Copia o que puder do nome em FuncAtual->expr (um texto) para cVarNome
     Executa a��o, que pode ser:
     1. Processar fun��o interna do programa:
        Apaga vari�veis ap�s cVarNome
        Cria vari�vel com o resultado
     2. Executar uma fun��o de uma classe:
        FuncAtual++
        FuncAtual->nome = linha que cont�m o nome da fun��o
        FuncAtual->linha = primeira linha ap�s o nome da fun��o
        FuncAtual->este = endere�o do objeto "este", ou NULL
        FuncAtual->expr = 0
        FuncAtual->inivar = primeira vari�vel ap�s cVarNome
        FuncAtual->fimvar = endere�o da primeira vari�vel ap�s VarAtual
        FuncAtual->numarg = n�mero de vari�veis ap�s cVarNome
     3. Vari�vel definida em uma classe:
        Apaga vari�veis a partir da anterior a cVarNome
        Anota vari�vel no topo da pilha de vari�veis
     4. Uma vari�vel qualquer:
        Consulta a vari�vel sobre o que fazer; pode cair nos passos 2 ou 3
  Nunca dever� chegar aqui; executar assert(0)
@endverbatim
*/

//------------------------------------------------------------------------------
int Instr::VarExec = 0;
int Instr::VarExecIni = 5000;

// Para gerar evento para avisar sobre erros
static char * erromens = nullptr;

// Pilha de dados - para criar/apagar vari�veis
char * const Instr::DadosPilha = new char[0x10000];
char * const Instr::DadosFim   = Instr::DadosPilha + 0x10000;
char * Instr::DadosTopo  = Instr::DadosPilha;

// Pilha de vari�veis
TVariavel * const Instr::VarPilha   = new TVariavel[500];
TVariavel * const Instr::VarFim     = Instr::VarPilha + 500;
TVariavel * Instr::VarAtual   = Instr::VarPilha;

// Pilha de fun��es
Instr::ExecFunc * const Instr::FuncPilha  = new Instr::ExecFunc[40];
Instr::ExecFunc * const Instr::FuncFim = Instr::FuncPilha + 40;
Instr::ExecFunc * Instr::FuncAtual  = Instr::FuncPilha;

//----------------------------------------------------------------------------
#define INSTR_COMPL (char)0xFF, 0, 0, 0, 0, '+', 0
const char Instr::InstrNulo[] = { 10, 0, Instr::cConstNulo, INSTR_COMPL };
const char Instr::InstrDouble[] = { 10, 0, Instr::cReal2, INSTR_COMPL };
const char Instr::InstrSocket[] = { 10, 0, Instr::cSocket, INSTR_COMPL };
const char Instr::InstrTxtFixo[] = { 10, 0, Instr::cTxtFixo, INSTR_COMPL };
const char Instr::InstrVarNome[] = { 10, 0, Instr::cVarNome, INSTR_COMPL };
const char Instr::InstrVarInicio[] = { 10, 0, Instr::cVarInicio, INSTR_COMPL };
const char Instr::InstrVarIniFunc[] = { 10, 0, Instr::cVarIniFunc, INSTR_COMPL };
const char Instr::InstrVarClasse[] = { 10, 0, Instr::cVarClasse, INSTR_COMPL };
const char Instr::InstrVarObjeto[] = { 10, 0, Instr::cVarObjeto, INSTR_COMPL };
const char Instr::InstrVarInt[] = { 10, 0, Instr::cVarInt, INSTR_COMPL };
const char Instr::InstrVarListaItem[] = { 10, 0, Instr::cListaItem, INSTR_COMPL };
const char Instr::InstrVarTextoPos[] =  { 10, 0, Instr::cTextoPos, INSTR_COMPL };
const char Instr::InstrVarTextoVarSub[] =  { 10, 0, Instr::cTextoVarSub, INSTR_COMPL };
const char Instr::InstrVarTextoObjSub[] =  { 10, 0, Instr::cTextoObjSub, INSTR_COMPL };
const char Instr::InstrDebugFunc[] = { 10, 0, INSTR_COMPL };
const char Instr::InstrAddSub[] = { 9, 0, Instr::cConstExpr, (char)0xFF, 0,
        Instr::exo_add_sub1, Instr::exo_atrib,
        Instr::exo_add_sub2, Instr::ex_fim };

static void ApagaVarEscopo()
{
    using namespace Instr;
    while ((unsigned char)FuncAtual->linha[endAlin] < FuncAtual->indent)
    {
        //printf("APAGOU %d\n", VarAtual-VarPilha); fflush(stdout);
        ApagarVar(VarAtual);
        FuncAtual->fimvar--;
        if (VarAtual < FuncAtual->inivar + FuncAtual->numarg)
            FuncAtual->indent = 0;
        else
            FuncAtual->indent = VarAtual->nomevar[endAlin];
    }
}

//----------------------------------------------------------------------------
/// Prepara para executar
/** @param classe Endere�o da classe (objeto este � NULO)
 *  @param func Nome da fun��o (string ASCIIZ)
 *  @retval true Fun��o encontrada; pode executar
 *  @retval false Fun��o n�o existe, n�o pode executar
 *  @sa exec
 */
bool Instr::ExecIni(TClasse * classe, const char * func)
{
    static char vazio[2] = { 0, 0 };
    int indice = classe->IndiceNome(func);
    if (indice < 0)
        return false;
    const char * nome = classe->InstrVar[indice];
    const char * instr = nome;
    const char * expr = nullptr; // N�o est� processando express�o num�rica
    if (instr[0] == 0 && instr[1] == 0)
        return false;
    switch (instr[2])
    {
    case cFunc:
    case cVarFunc:
        instr += Num16(instr);
        if (instr[0] && instr[1])
            switch (instr[2])
            {
            case cFunc:
            case cVarFunc:
            case cConstNulo:
            case cConstTxt:
            case cConstNum:
            case cConstExpr:
            case cConstVar:
                instr = vazio;
            }
        break;
    case cConstTxt:
    case cConstNum:
    case cConstExpr:
    case cConstVar:
        expr = instr + instr[endIndice];
        break;
    default:
        return false;
    }
    VarExec = VarExecIni;
// Acerta pilhas
    DadosTopo = DadosPilha; // Limpa pilha de dados
    VarAtual = VarPilha;    // Limpa pilha de vari�veis
    VarAtual->Limpar();     // Vari�vel atual � nulo
    VarAtual->defvar = InstrNulo;
    VarAtual->nomevar = InstrNulo;
    FuncAtual = FuncPilha;  // Limpa pilha de fun��es
    FuncAtual->nome = nome;
    FuncAtual->este = nullptr; // Nenhum objeto
    FuncAtual->linha = instr;// Primeira instru��o da fun��o
    FuncAtual->expr = expr;
    FuncAtual->inivar = VarAtual + 1; // Endere�o do primeiro argumento
    FuncAtual->fimvar = VarAtual + 1;
    FuncAtual->numarg = 0;  // N�mero de argumentos da fun��o
    FuncAtual->tipo = 0;
    FuncAtual->indent = 0;
    return true;
}

//----------------------------------------------------------------------------
/// Prepara para executar
/** @param este Endere�o do objeto "este"
 *  @param func Nome da fun��o (string ASCIIZ)
 *  @retval true Fun��o encontrada; pode executar
 *  @retval false Fun��o n�o existe, n�o pode executar
 *  @sa exec
 */
bool Instr::ExecIni(TObjeto * este, const char * func)
{
    if (este == nullptr)
        return false;
    if (ExecIni(este->Classe, func) == false)
        return false;
    FuncAtual->este = este;
    return true;
}

//----------------------------------------------------------------------------
/// Adiciona argumento antes de executar
/** @param txt Texto a adicionar
 *  @note  O texto deve existir quando Instr::ExecX() for executado
 *  @sa exec
 */
void Instr::ExecArg(const char * txt)
{
    VarAtual++;
    VarAtual->Limpar();
    VarAtual->defvar = InstrTxtFixo;
    VarAtual->endfixo = txt;
    FuncAtual->fimvar = VarAtual + 1;
    FuncAtual->numarg++;
}

//----------------------------------------------------------------------------
/// Adiciona argumento antes de executar
/** @param valor N�mero
 *  @sa exec
 */
void Instr::ExecArg(int valor)
{
    VarAtual++;
    VarAtual->Limpar();
    VarAtual->defvar = InstrVarInt;
    VarAtual->setInt(valor);
    FuncAtual->fimvar = VarAtual + 1;
    FuncAtual->numarg++;
}

//----------------------------------------------------------------------------
/// Cria uma vari�vel como argumento antes de executar
/** @param def Defini��o da vari�vel, conforme Instr::Codif()
 *  @sa exec
 */
void Instr::ExecArgCriar(const char * def)
{
    // Nota: CriarVar s� retorna false no caso de mem�ria insuficiente
    // Nesse caso, isso n�o deve acontecer
    CriarVar(def);
    FuncAtual->fimvar = VarAtual + 1;
    FuncAtual->numarg++;
}

//----------------------------------------------------------------------------
/// Usado internamente em Instr::ExecX()
/** Executa procedimentos em caso de mem�ria insuficiente
 *  @param  motivo O motivo para chamar essa fun��o:
 *              - 0 = VarExec chegou a zero
 *              - 1 = Mem�ria insuficiente para chamar fun��o
 *              - 2 = Mem�ria insuficiente para criar vari�vel
 *              - 3 = Erro desconhecido
 *  @return false
 */
static bool RetornoErro(int motivo)
{
// Anota erro
    if (erromens == nullptr)
    {
        char mens[BUF_MENS];
        char * p = mens;
        const char * pfim = mens + sizeof(mens);
        switch (motivo)
        {
        case 0: p = copiastr(p, "Exec chegou a 0"); break;
        case 1: p = copiastr(p, "Atingido limite de fun��es"); break;
        case 2: p = copiastr(p, "Atingido limite de vari�veis"); break;
        default: p = copiastr(p, "Erro desconhecido"); break;
        }

        for (Instr::ExecFunc * f = Instr::FuncPilha; f <= Instr::FuncAtual; f++)
        {
        // Checa mem�ria
            if (pfim-p < CLASSE_NOME_TAM + 30)
                break;
        // Nome da fun��o
            *p++ = Instr::ex_barra_n;
            p = copiastr(p, "Fun��o: ");
            Instr::Decod(p, f->nome, pfim - p);
            while (*p)
                p++;
            if (f->este)
                p = mprintf(p, pfim-p, " (objeto=%s)", f->este->Classe->Nome);
        // Checa mem�ria
            if (pfim-p < CLASSE_NOME_TAM + 30)
                break;
        // Linha da fun��o
            if (f->nome == f->linha)
                continue;
            *p++ = Instr::ex_barra_n;
            p = copiastr(p, "Linha:  ");
            Instr::Decod(p, f->linha, pfim - p);
            while (*p)
                p++;
        }

        erromens = new char[p + 1 - mens];
        memcpy(erromens, mens, p-mens);
        erromens[p - mens] = 0;
    }
// Apaga vari�veis da pilha
    TVarProg::LimparVar(); // Apaga refer�ncias das vari�veis prog
    Instr::ApagarVar(Instr::VarPilha);
// Cria NULO na pilha
    Instr::VarAtual++;
    Instr::VarAtual->Limpar();
    Instr::VarAtual->defvar = Instr::InstrNulo;
    return false;
}

//----------------------------------------------------------------------------
/// Usado internamente em Instr::ExecX()
/** Adiciona texto em vari�vel cVarNome
 */
static const char * CopiaVarNome(TVariavel * v, const char * txt)
{
    if (v->defvar[2] != Instr::cVarNome)
        return txt;
    char * dest = (char*)v->endvar;
    const char * fim = dest + TVariavel::Tamanho(v->defvar) - 1;
    while (*dest)
        dest++;
    for (; *txt; txt++)
    {
        if (*(unsigned char*)txt < Instr::ex_varini)
            continue;
        else if (*(unsigned char*)txt < ' ')
            break;
        else if (dest < fim)
            *dest++ = *txt;
    }
    *dest=0;
    return txt;
}

//----------------------------------------------------------------------------
/// Usado internamente em Instr::ExecX()
/** @return �ltima vari�vel cVarNome da pilha
 */
static TVariavel * EndVarNome(void)
{
    for (TVariavel * v = Instr::VarAtual; v>=Instr::VarPilha; v--)
        if (v->defvar[2] == Instr::cVarNome)
            return v;
    return 0;
}

//----------------------------------------------------------------------------
/// Usado internamente em Instr::ExecX()
/** Avan�a para fim do nome da vari�vel
 */
static void VarInvalido(void)
{
// Obt�m �ltima vari�vel cVarNome da pilha
    TVariavel * v = EndVarNome();
    assert(v != nullptr);
// Apaga �ltima vari�vel cVarNome em diante
    Instr::ApagarVar(v);
// Cria NULO na pilha
    Instr::CriarVar(Instr::InstrNulo);
// Avan�a para o fim do nome da vari�vel
    const char * p = ProcuraExpr(Instr::FuncAtual->expr - 1, Instr::ex_varfim);
    assert(p != nullptr);
    Instr::FuncAtual->expr = p + 1;
}

//----------------------------------------------------------------------------
/// Executa fun��o
/** VarAtual cont�m a vari�vel retornada pela fun��o
 *  @return true se executou normal, false se cancelou por falta de mem�ria
 *  @note Independente de retornar true ou false, executar ExecFim() depois
 */
bool Instr::ExecX()
{
    const char * DebugInstr = nullptr;
    FuncAtual->funcdebug = nullptr;
    while (true)
    {
        if (FuncAtual->expr == nullptr)
        {
    // Est� processando uma linha
            if (FuncAtual->funcdebug && DebugInstr != FuncAtual->linha)
            {
                const char * l = FuncAtual->linha;
                if (l && (l[0] || l[1]) && l[2] != cSenao1 && l[2] != cSenao2)
                    TVarDebug::Exec();
                DebugInstr = FuncAtual->linha;
            }
            if (--VarExec < 0)
                return RetornoErro(0);

        // Retorno sem par�metros
            if (FuncAtual->linha[0] == 0 && FuncAtual->linha[1] == 0)
                FuncAtual->linha = nullptr;
            else
                switch (FuncAtual->linha[2])
                {
                case cRet1:
                case cFunc:
                case cVarFunc:
                case cConstNulo:
                case cConstTxt:
                case cConstNum:
                case cConstExpr:
                case cConstVar:
                    FuncAtual->linha = nullptr;
                }
            if (FuncAtual->linha == nullptr)
            {
                bool b = (FuncAtual->funcdebug == nullptr);
                switch (FuncAtual->tipo)
                {
                case 1: // Ler varfunc
                    ApagarVar(FuncAtual->inivar);
                    if (VarAtual >= VarFim - 1)
                        return RetornoErro(2);
                    VarAtual++;
                    VarAtual->Limpar();
                    VarAtual->defvar = InstrNulo;
                    if (!VarFuncFim())
                        return RetornoErro(1);
                    break;
                case 2: // Mudar varfunc
                    ApagarVar(FuncAtual->inivar);
                    break;
                case 3: // criar()
                    ApagarVar(FuncAtual->inivar - 2);
                    VarAtual++;
                    VarAtual->Limpar();
                    VarAtual->defvar = InstrVarObjeto;
                    VarAtual->endvar = FuncAtual->este;
                    break;
                case 4: // debug.cmd
                    ApagarVar(FuncAtual->inivar - 1);
                    VarAtual++;
                    VarAtual->Limpar();
                    VarAtual->defvar = InstrTxtFixo;
                    VarAtual->endfixo = "";
                    break;
                default: // Fun��o normal
                    ApagarVar(FuncAtual->inivar - 1);
                    VarAtual++;
                    VarAtual->Limpar();
                    VarAtual->defvar = InstrNulo;
                }
            // Apaga fun��o
                FuncAtual--;
                if (FuncAtual >= FuncPilha)
                {
                    if (b)
                        DebugInstr = FuncAtual->linha;
                    continue;
                }
                if (VarFuncIni(VarAtual))
                    continue;
                return true;
            }

        // Mostra o que vai executar
#ifdef DEBUG_INSTR
            {
                char mens[BUF_MENS];
                printf(">>> %d %d  ",
                       static_cast<int>(DadosFim-DadosTopo),
                       static_cast<int>(VarFim-VarAtual));
                printf(" %d %d\n", (unsigned char)FuncAtual->linha[endAlin],
                                FuncAtual->indent);
                if (Instr::Decod(mens, FuncAtual->linha, sizeof(mens)))
                    printf("Exec: %s\n", mens);
                else
                    printf("Erro ao ler instru��o\n");
                fflush(stdout);
            }
#endif

        // Apaga vari�veis que sa�ram do escopo
            ApagaVarEscopo();

        // Vari�vel da fun��o
            if (FuncAtual->linha[2] > cVariaveis)
            {
                if (!CriarVar(FuncAtual->linha))
                    return RetornoErro(2);
                FuncAtual->fimvar++;
                FuncAtual->indent = VarAtual->defvar[endAlin];
                int desloc = FuncAtual->linha[endIndice];
                if (desloc != 0)
                    FuncAtual->expr = FuncAtual->linha + desloc;
                else
                    FuncAtual->linha += Num16(FuncAtual->linha);
                //printf("CRIOU %d\n", VarAtual-VarPilha); fflush(stdout);
                continue;
            }

        // Processa instru��o
            switch (FuncAtual->linha[2])
            {
            case cRefVar: // Refer�ncia a uma vari�vel
                {
                    int desloc = FuncAtual->linha[endIndice];
                    if (desloc != 0)
                        FuncAtual->expr = FuncAtual->linha + desloc;
                    else
                        FuncAtual->linha += Num16(FuncAtual->linha);
                    continue;
                }
            case cSe:   // Processa express�o num�rica
            case cEnquanto:
            case cCasoVar:
            case cSair2:
            case cContinuar2:
                FuncAtual->expr = FuncAtual->linha + endVar + 2;
                break;
            case cEPara:
                FuncAtual->expr = FuncAtual->linha + endVar + 6;
                break;
            case cRet2: // Processa express�o num�rica
            case cExpr:
                FuncAtual->expr = FuncAtual->linha + endVar;
                break;
            case cComent: // Coment�rio
                VarExec++;
                FuncAtual->linha += Num16(FuncAtual->linha);
                break;
            case cSenao1: // Avan�a
            case cSenao2:
            case cSair1:
                {
                    int desvio = Num16(FuncAtual->linha + endVar);
                    if (desvio == 0)
                        FuncAtual->linha += Num16(FuncAtual->linha);
                    else
                        FuncAtual->linha += desvio;
                    break;
                }
            case cEFim:  // Recua
            case cContinuar1:
                {
                    const char * p = FuncAtual->linha;
                    int desvio = Num16(p + endVar);
                    if (desvio == 0)
                        p += Num16(p);
                    else
                    {
                        p -= desvio;
                        if (p[2] == cEPara)
                            FuncAtual->expr = p + Num16(p + endVar + 4);
                    }
                    FuncAtual->linha = p;
                    ApagaVarEscopo();
                    break;
                }
            case cFimSe: // Passa para pr�xima instru��o
            case cCasoSe:
            case cCasoSePadrao:
            case cCasoFim:
                FuncAtual->linha += Num16(FuncAtual->linha);
                break;
            case cTerminar: // Encerra o programa
                TSocket::SairPend();
                TVarArqLog::TempoEspera(1000);
                Termina();
            default:  // Instru��o desconhecida
                assert(0);
            }
            continue;
        }

    // Est� processando express�o num�rica
    // FuncAtual->expr[0] cont�m a pr�xima instru��o da express�o num�rica
#ifdef DEBUG_VAR
        printf("       %p\n", DadosTopo);
        for (int x = 0; x < 5; x++)
        {
            TVariavel * v = VarAtual - x;
            if (v<VarPilha)
                continue;
            printf("    v%d %p m%d/%d l%d ", (int)(v - VarPilha), v->endvar,
                   v->indice, (unsigned char)v->defvar[endVetor], v->tamanho);
            const char * p = NomeComando(v->defvar[2]);
            if (p)
                printf("%s ", p);
            else
                printf("???_%d ", (unsigned char)v->defvar[2]);
            if (v->nomevar)
                printf("[nome=%s] ", v->nomevar + Instr::endNome);
            switch (v->Tipo())
            {
            case varOutros: printf("outros"); break;
            case varInt: printf("int=%d", v->getInt()); break;
            case varDouble: printf("double=%f", v->getDouble()); break;
            case varTxt: printf("txt=%s", v->getTxt()); break;
            case varObj: printf("ref=%s", v->getTxt()); break;
            }
            putchar('\n');
        }
        for (int i = 0; i < 3; i++)
            printf("   %sf%d %d/%d\n",
                   i == FuncAtual-FuncPilha ? ">>" : "  ", i,
                   static_cast<int>(FuncPilha[i].inivar-VarPilha),
                   static_cast<int>(FuncPilha[i].fimvar-VarPilha));
        fflush(stdout);
#endif
#ifdef DEBUG_EXPR
        {
            const char * p = NomeExpr(FuncAtual->expr[0]);
            if (p)
                printf("  %s\n", p);
            else
                printf("  ??? %d\n", (unsigned char)FuncAtual->expr[0]);
            fflush(stdout);
        }
#endif
        switch (FuncAtual->expr[0])
        {
        case ex_fim:
        case ex_coment:
#ifdef DEBUG_INSTR
            {
                char mens[BUF_MENS];
                if (Instr::Decod(mens, FuncAtual->linha, sizeof(mens)))
                    printf("Exec_arg: %s\n", mens);
                else
                    printf("Erro ao ler instru��o\n");
                fflush(stdout);
            }
#endif
            switch (FuncAtual->linha[2])
            {
            case cRet2:
            case cConstExpr:
            case cConstVar:
                FuncAtual->expr = nullptr;
                switch (FuncAtual->tipo)
                {
                case 1: // Ler varfunc
                    ApagarRet(FuncAtual->inivar);
                    if (!VarFuncFim())
                        return RetornoErro(2);
                    break;
                case 2: // Mudar varfunc
                    ApagarVar(FuncAtual->inivar);
                    break;
                case 3: // criar()
                    ApagarVar(FuncAtual->inivar - 2);
                    VarAtual++;
                    VarAtual->Limpar();
                    VarAtual->defvar = InstrVarObjeto;
                    VarAtual->endvar = FuncAtual->este;
                    break;
                case 4: // debug.cmd
                    {
                        assert(VarAtual >= FuncAtual->inivar);
                        char mens[BUF_MENS];
                        char * p = copiastr(mens, VarAtual->getTxt(), sizeof(mens));
                        ApagarVar(FuncAtual->inivar - 1);
                        if (!CriarVarTexto(mens, p - mens))
                        {
                            VarAtual++;
                            VarAtual->Limpar();
                            VarAtual->defvar = InstrNulo;
                        }
                        break;
                    }
                default: // Fun��o normal
                    assert(VarAtual >= FuncAtual->inivar);
                    ApagarRet(FuncAtual->inivar-1);
                }
            // Apaga fun��o
                FuncAtual--;
                if (FuncAtual>=FuncPilha)
                    break;
                if (VarFuncIni(VarAtual))
                    break;
                return true;
            case cRefVar: // Refer�ncia a uma vari�vel
                FuncAtual->expr = nullptr;
                assert(VarAtual >= FuncAtual->fimvar);
                ApagarVar(FuncAtual->fimvar + 1);
                VarAtual->nomevar = FuncAtual->linha;
                FuncAtual->fimvar++;
                FuncAtual->indent = VarAtual->nomevar[endAlin];
                FuncAtual->linha += Num16(FuncAtual->linha);
                break;
            case cSe:
            case cSenao2:
                if (!VarFuncIni(VarAtual))
                {
                    FuncAtual->expr = nullptr;
                    int desvio = 0;
                    if (VarAtual->getBool()==false)
                        desvio = Num16(FuncAtual->linha + endVar);
                    ApagarVar(FuncAtual->fimvar);
                    if (desvio == 0)
                    {
                        FuncAtual->linha += Num16(FuncAtual->linha);
                        break;
                    }
                    FuncAtual->linha += desvio;
                    if (FuncAtual->linha[0] == 0 && FuncAtual->linha[1] == 0)
                        break;
                    if (FuncAtual->linha[2] == cSenao1)
                    {
                        if (FuncAtual->funcdebug == nullptr)
                            FuncAtual->linha += Num16(FuncAtual->linha);
                        else
                        {
                            // Executa fun��o de debug e volta para
                            // case cSenao1 (abaixo)
                            static const char tmp[] = { ex_fim };
                            FuncAtual->expr = tmp;
                            TVarDebug::Exec();
                        }
                    }
                    else if (FuncAtual->linha[2] == cSenao2)
                    {
                        FuncAtual->expr = FuncAtual->linha + endVar + 2;
                        if (FuncAtual->funcdebug)
                            TVarDebug::Exec();
                    }
                }
                break;
            case cSair2:
                if (!VarFuncIni(VarAtual))
                {
                    FuncAtual->expr = nullptr;
                    int desvio = 0;
                    if (VarAtual->getBool())
                        desvio = Num16(FuncAtual->linha + endVar);
                    if (desvio == 0)
                        FuncAtual->linha += Num16(FuncAtual->linha);
                    else
                        FuncAtual->linha += desvio;
                    ApagarVar(FuncAtual->fimvar);
                }
                break;
            case cContinuar2:
                if (!VarFuncIni(VarAtual))
                {
                    const char * p = FuncAtual->linha;
                    FuncAtual->expr = nullptr;
                    if (!VarAtual->getBool() ||
                            (p[endVar] == 0 && p[endVar + 1] == 0))
                        p += Num16(p);
                    else
                    {
                        p -= Num16(p + endVar);
                        if (p[2] == cEPara)
                            FuncAtual->expr = p + Num16(p + endVar + 4);
                    }
                    FuncAtual->linha = p;
                    ApagarVar(FuncAtual->fimvar);
                    ApagaVarEscopo();
                }
                break;
            case cSenao1: // sen�o sem argumentos em modo debug
                FuncAtual->expr = nullptr;
                FuncAtual->linha += Num16(FuncAtual->linha);
                ApagarVar(FuncAtual->fimvar);
                break;
            case cEnquanto:
                if (!VarFuncIni(VarAtual))
                {
                    FuncAtual->expr = nullptr;
                    int desvio = 0;
                    if (VarAtual->getBool()==false)
                        desvio = Num16(FuncAtual->linha + endVar);
                    ApagarVar(FuncAtual->fimvar);
                    if (desvio == 0)
                        FuncAtual->linha += Num16(FuncAtual->linha);
                    else
                        FuncAtual->linha += desvio;
                }
                break;
            case cEPara:
                {
                    const char * p = FuncAtual->linha;
                    FuncAtual->expr = p + Num16(p + endVar + 2);
                    break;
                }
            case cCasoVar:
                if (!VarFuncIni(VarAtual))
                {
                    FuncAtual->expr = nullptr;
                    if (FuncAtual->linha[endVar] == 0 &&
                        FuncAtual->linha[endVar + 1] == 0)
                    {
                        FuncAtual->linha += Num16(FuncAtual->linha);
                        ApagarVar(FuncAtual->fimvar);
                        break;
                    }
                    char texto[512];
                    copiastr(texto, FuncAtual->fimvar->getTxt(), sizeof(texto));
                    ApagarVar(FuncAtual->fimvar);
                    FuncAtual->linha += Num16(FuncAtual->linha + endVar);
                    while (true)
                    {
                        if (FuncAtual->linha[0] == 0 && FuncAtual->linha[1] == 0)
                            break;
                        if (FuncAtual->linha[2] == cCasoSePadrao ||
                            FuncAtual->linha[2] == cCasoFim)
                        {
                            FuncAtual->linha += Num16(FuncAtual->linha);
                            break;
                        }
                        if (FuncAtual->linha[2] != cCasoSe)
                            break;
                        int cmp = strcmp(texto, FuncAtual->linha + endVar + 4);
                        if (cmp == 0)
                        {
                            FuncAtual->linha += Num16(FuncAtual->linha);
                            break;
                        }
                        const char * p = FuncAtual->linha + endVar;
                        if (cmp > 0)
                            p += 2;
                        FuncAtual->linha += ((signed char)p[1] * 0x100)
                                            | (unsigned char)p[0];
                    }
                }
                break;
            default:
                if (FuncAtual->linha[2] > cVariaveis)
                {
                    ApagarVar(FuncAtual->fimvar + 1);
                    if (VarFuncIni(VarAtual))
                        break;
                    switch (VarAtual[-1].Tipo())
                    {
                    case varOutros:
                        // Chama fun��o Igual() se forem vari�veis diferentes,
                        // mas do mesmo tipo
                        if (VarAtual[-1].defvar[2] == VarAtual[0].defvar[2] &&
                                (VarAtual[-1].endvar != VarAtual[0].endvar ||
                                    VarAtual[-1].indice != VarAtual[0].indice) )
                            VarAtual[-1].Igual(VarAtual);
                        break;
                    case varInt:
                        VarAtual[-1].setInt( VarAtual->getInt() );
                        break;
                    case varDouble:
                        VarAtual[-1].setDouble( VarAtual->getDouble() );
                        break;
                    case varTxt:
                        VarAtual[-1].setTxt( VarAtual->getTxt() );
                        break;
                    case varObj:
                        VarAtual[-1].setObj( VarAtual->getObj() );
                        break;
                    }
                }
                FuncAtual->expr = nullptr;
                FuncAtual->linha += Num16(FuncAtual->linha);
                ApagarVar(FuncAtual->fimvar);
            }
            break;
        case ex_nulo:
            FuncAtual->expr++;
            if (VarAtual >= VarFim - 1)
                return RetornoErro(2);
            VarAtual++;
            VarAtual->Limpar();
            VarAtual->defvar = InstrNulo;
            break;
        case ex_txt:
            FuncAtual->expr++;
            if (VarAtual >= VarFim - 1)
                return RetornoErro(2);
            VarAtual++;
            VarAtual->Limpar();
            VarAtual->defvar = InstrTxtFixo;
            VarAtual->endfixo = FuncAtual->expr;
            while (*FuncAtual->expr++);
            break;
        case ex_num0:
            if (!CriarVarInt(0))
                return RetornoErro(2);
            FuncAtual->expr++;
            break;
        case ex_num1:
            if (!CriarVarInt(1))
                return RetornoErro(2);
            FuncAtual->expr++;
            break;
        case ex_num8p:
        case ex_num8hexp:
            if (!CriarVarInt((unsigned char)FuncAtual->expr[1]))
                return RetornoErro(2);
            FuncAtual->expr += 2;
            break;
        case ex_num16p:
        case ex_num16hexp:
            if (!CriarVarInt(Num16(FuncAtual->expr+1)))
                return RetornoErro(2);
            FuncAtual->expr += 3;
            break;
        case ex_num32p:
        case ex_num32hexp:
            {
                unsigned int x = Num32(FuncAtual->expr + 1);
                if (x & 0x80000000)
                {
                    if (!CriarVarDouble(x))
                        return RetornoErro(2);
                }
                else if (!CriarVarInt(x))
                    return RetornoErro(2);
                FuncAtual->expr += 5;
                break;
            }
        case ex_num8n:
        case ex_num8hexn:
            if (!CriarVarInt( -(int)(unsigned char)FuncAtual->expr[1] ))
                return RetornoErro(2);
            FuncAtual->expr += 2;
            break;
        case ex_num16n:
        case ex_num16hexn:
            if (!CriarVarInt( -(int)Num16(FuncAtual->expr+1) ))
                return RetornoErro(2);
            FuncAtual->expr += 3;
            break;
        case ex_num32n:
        case ex_num32hexn:
            if (*((unsigned char*)FuncAtual->expr + 4) & 0x80)
            {
                if (!CriarVarDouble(-(double)Num32(FuncAtual->expr+1)))
                    return RetornoErro(2);
            }
            else if (!CriarVarInt( -(int)Num32(FuncAtual->expr+1) ))
                return RetornoErro(2);
            FuncAtual->expr += 5;
            break;
        case ex_div1:
        case ex_div2:
        case ex_div3:
        case ex_div4:
        case ex_div5:
        case ex_div6:
            {
                double valor = VarAtual->getDouble();
                switch (*FuncAtual->expr++)
                {
                case ex_div1: valor /= 10; break;
                case ex_div2: valor /= 100; break;
                case ex_div3: valor /= 1000; break;
                case ex_div4: valor /= 10000; break;
                case ex_div5: valor /= 100000; break;
                default:      valor /= 1000000; break;
                }
                if (VarAtual->tamanho == 0 || VarAtual->defvar[2] != cReal2)
                {
                    ApagarVar(VarAtual);
                    CriarVarDouble(valor);
                    break;
                }
                VarAtual->setDouble(valor);
                break;
            }
        case exo_virg_expr:  // Operador: V�rgula, para separar express�es
            switch (FuncAtual->linha[2])
            {
            case cRet2:
                if (VarFuncIni(VarAtual))
                    break;
                if (VarAtual->getBool())
                {
                    //ApagarVar(FuncAtual->fimvar);
                    ApagarVar(VarAtual);
                    FuncAtual->expr++;
                    break;
                }
                ApagarVar(FuncAtual->fimvar);
                FuncAtual->linha += Num16(FuncAtual->linha);
                FuncAtual->expr = nullptr;
                break;
            case cEPara:
                // Se completou o primeiro argumento, passa para o segundo
                if (FuncAtual->fimvar == VarAtual)
                {
                    FuncAtual->expr++;
                    //const char * p = FuncAtual->linha;
                    //FuncAtual->expr = p + Num16(p + endVar + 2);
                    break;
                }
                // Fecha o la�o, exatamente como na instru��o "enquanto"
                if (!VarFuncIni(VarAtual))
                {
                    FuncAtual->expr = nullptr;
                    int desvio = 0;
                    if (VarAtual->getBool()==false)
                        desvio = Num16(FuncAtual->linha + endVar);
                    ApagarVar(FuncAtual->fimvar);
                    if (desvio == 0)
                        FuncAtual->linha += Num16(FuncAtual->linha);
                    else
                        FuncAtual->linha += desvio;
                }
                break;
            default:
                FuncAtual->expr++;
            }
            break;
        case exo_virgula:    // Operador: V�rgula, para separar argumentos
        case exo_ini:        // Operador: Marca o in�cio dos operadores
        case exo_fim:        // Operador: Marca o fim dos operadores
            FuncAtual->expr++;
            break;
        case exo_neg:        // Operador: -a
            if (VarFuncIni(VarAtual))
                break;
            FuncAtual->expr++;
            switch (VarAtual->Tipo())
            {
            case varOutros:
            case varObj:
                break;
            case varInt:
                if (VarAtual->tamanho)
                    VarAtual->setInt(-VarAtual->getInt());
                else
                {
                    int valor = -VarAtual->getInt();
                    ApagarVar(VarAtual);
                    if (!CriarVarInt(valor))
                        return RetornoErro(2);
                }
                break;
            case varDouble:
                if (VarAtual->tamanho)
                {
                    VarAtual->setDouble(-VarAtual->getDouble());
                    break;
                }
            default:
                {
                    double valor = -VarAtual->getDouble();
                    ApagarVar(VarAtual);
                    if (!CriarVarDouble(valor))
                        return RetornoErro(2);
                }
            }
            break;
        case exo_exclamacao: // Operador: !a
            if (VarFuncIni(VarAtual))
                break;
            FuncAtual->expr++;
            if (VarAtual->tamanho == 0 || VarAtual->defvar != InstrVarInt)
            {
                bool valor = !VarAtual->getBool();
                ApagarVar(VarAtual);
                if (!CriarVarInt(valor))
                    return RetornoErro(2);
            }
            else
                VarAtual->setInt(!VarAtual->getInt());
            break;
        case exo_b_comp: // Operador: ~a
            if (VarFuncIni(VarAtual))
                break;
            FuncAtual->expr++;
            if (VarAtual->Tipo() == varTxt)
            {
                char mens[BUF_MENS];    // Resultado
                char * d = mens;
                const char * o = VarAtual->getTxt();
                while (d < mens + sizeof(mens))
                {
                    char ch = *o++;
                    if (ch == 0)
                        break;
                    if (ch >= '0' && ch <= '9')
                        ch -= '0';
                    else if (ch >= 'A' && ch <= 'F')
                        ch -= 'A' - 10;
                    else if (ch >= 'a' && ch <= 'f')
                        ch -= 'a' - 10;
                    else
                        continue;
                    if (ch == 15 && d == mens) // Para ignorar zeros no in�cio
                        continue;
                    ch = 15 - ch;
                    *d++ = (ch < 10 ? ch + '0' : ch + 'a' - 10);
                }
                ApagarVar(VarAtual);
                if (!CriarVarTexto(mens, d-mens))
                    return RetornoErro(2);
            }
            else if (VarAtual->tamanho == 0 || VarAtual->defvar != InstrVarInt)
            {
                int valor = ~VarAtual->getInt();
                ApagarVar(VarAtual);
                if (!CriarVarInt(valor))
                    return RetornoErro(2);
            }
            else
                VarAtual->setInt(~VarAtual->getInt());
            break;

// EXO_ADD_SUB tratando varfunc:
// Apaga a varfunc do topo da pilha
// Cria vari�vel double: 1, ou ou -1 (ser� adicionado ao resultado final)
// Cria varfunc
// Cria vari�vel double: 1, ou ou -1 (ser� adicionado � varfunc)
// Cria varfunc
// Cria fun��o que come�a com a primeira vari�vel criada

// Em seguida � chamado exo_add_sub1:
// Resolve a varfunc no topo da pilha
// Obt�m: double valor = VarAtual->getDouble()
// Apaga a vari�vel do topo da pilha
// Soma a vari�vel valor �s duas vari�veis double criadas

// Depois vem exo_atrib, para atribuir o valor num�rico obtido � varfunc

// Depois � chamado exo_add_sub2:
// Apaga a vari�vel do topo da pilha, que � a varfunc
// Desse modo, fica no topo da pilha a primeira vari�vel double criada
// Apaga a fun��o atual (volta para a fun��o anterior)

#define EXO_ADD_SUB(somavar, somaresult)                                      \
            FuncAtual->expr++;                                                \
            if (VarAtual->defvar[2] != cVarFunc &&                            \
                VarAtual->defvar[2] != cConstVar)                             \
            {                                                                 \
                double valor = VarAtual->getDouble();                         \
                VarAtual->setDouble(valor + (somavar));                       \
                ApagarVar(VarAtual);                                          \
                if (!CriarVarDouble(valor + (somaresult)))                    \
                    return RetornoErro(2);                                    \
                break;                                                        \
            }                                                                 \
            else                                                              \
            {                                                                 \
                /* Varfunc:                                                   \
                 * 1. Apaga varfunc                                           \
                 * 2. Cria nova fun��o                                        \
                 * 3. Cria vari�vel double -> adicionado ao resultado final   \
                 * 4. Criar uma c�pia da varfunc                              \
                 * 5. Criar vari�vel double -> adicionado ao varfunc          \
                 * 6. Criar uma c�pia da varfunc                              \
                 */                                                           \
                if (FuncAtual >= FuncFim - 1 || VarAtual >= VarFim-4)         \
                    return RetornoErro(1);                                    \
                assert(VarAtual->tamanho == 0);                               \
                                                                              \
                const char * defvar = InstrAddSub;                            \
                FuncAtual++;                                                  \
                FuncAtual->nome = defvar;                                     \
                FuncAtual->linha = defvar;                                    \
                FuncAtual->este = FuncAtual[-1].este;                         \
                FuncAtual->expr = defvar + endIndice;                         \
                FuncAtual->inivar = VarAtual;                                 \
                FuncAtual->fimvar = VarAtual;                                 \
                FuncAtual->numarg = 0;                                        \
                FuncAtual->tipo = 0;                                          \
                FuncAtual->indent = 0;                                        \
                FuncAtual->objdebug = FuncAtual[-1].objdebug;                 \
                FuncAtual->funcdebug = FuncAtual[-1].funcdebug;               \
                                                                              \
                TVariavel variavel_varfunc = VarAtual[0];                     \
                ApagarVar(VarAtual);                                          \
                if (!CriarVarDouble(somaresult))                              \
                    return RetornoErro(2);                                    \
                VarAtual++;                                                   \
                VarAtual[0] = variavel_varfunc;                               \
                if (!CriarVarDouble(somavar))                                 \
                    return RetornoErro(2);                                    \
                VarAtual++;                                                   \
                VarAtual[0] = variavel_varfunc;                               \
                break;                                                        \
            }

        case exo_add_antes:     // Operador: ++a
            EXO_ADD_SUB(1, 1)
            break;
        case exo_sub_antes:     // Operador: --a
            EXO_ADD_SUB(-1, -1)
            break;
        case exo_add_depois:    // Operador: a++
            EXO_ADD_SUB(1, 0)
            break;
        case exo_sub_depois:    // Operador: a--
            EXO_ADD_SUB(-1, 0)
            break;
        case exo_add_sub1:
            {
                if (VarFuncIni(VarAtual - 1))
                    break;
                FuncAtual->expr++;
                double valor = VarAtual->getDouble();
                ApagarVar(VarAtual);
                VarAtual->setDouble(valor + VarAtual->getDouble());
                VarAtual[-2].setDouble(valor + VarAtual[-2].getDouble());
                break;
            }
        case exo_add_sub2:
            {
                FuncAtual->expr++;
                ApagarVar(VarAtual);
                FuncAtual--;
                break;
            }

        case exo_mul:        // Operador: a*b
            {
                if (VarFuncIni(VarAtual - 1))
                    break;
                FuncAtual->expr++;
                double valor = VarAtual[-1].getDouble() *
                               VarAtual[0].getDouble();
                ApagarVar(VarAtual);
                if (VarAtual->tamanho == 0 || VarAtual->defvar != InstrDouble)
                {
                    ApagarVar(VarAtual);
                    if (!CriarVarDouble(valor))
                        return RetornoErro(2);
                    break;
                }
                VarAtual->setDouble(valor);
                break;
            }
        case exo_div:        // Operador: a/b
            {
                if (VarFuncIni(VarAtual - 1))
                    break;
                FuncAtual->expr++;
                double valor1 = VarAtual[-1].getDouble();
                double valor2 = VarAtual[0].getDouble();
                if (valor2)
                    valor1 /= valor2;
                else
                    valor1 *= 1.0e1000;
                ApagarVar(VarAtual);
                if (VarAtual->tamanho == 0 || VarAtual->defvar != InstrDouble)
                {
                    ApagarVar(VarAtual);
                    if (!CriarVarDouble(valor1))
                        return RetornoErro(2);
                    break;
                }
                VarAtual->setDouble(valor1);
                break;
            }
        case exo_porcent:    // Operador: a%b
            {
                if (VarFuncIni(VarAtual - 1))
                    break;
                FuncAtual->expr++;
                if (VarAtual[0].Tipo() == varInt && VarAtual[-1].Tipo() == varInt)
                {
                    int valor = VarAtual[0].getInt();
                    if (valor)
                        valor = VarAtual[-1].getInt() % valor;
                    else
                        valor = 0;
                    ApagarVar(VarAtual);
                    if (VarAtual->tamanho == 0 || VarAtual->Tipo()!=varInt)
                    {
                        ApagarVar(VarAtual);
                        if (!CriarVarInt(0))
                            return RetornoErro(2);
                    }
                    VarAtual->setInt(valor);
                }
                else
                {
                    double valor = VarAtual[0].getDouble();
                    if (valor)
                        valor = fmod(VarAtual[-1].getDouble(), valor);
                    else
                        valor = 0;
                    ApagarVar(VarAtual);
                    if (VarAtual->tamanho == 0 || VarAtual->defvar != InstrDouble)
                    {
                        ApagarVar(VarAtual);
                        if (!CriarVarDouble(valor))
                            return RetornoErro(2);
                        break;
                    }
                    VarAtual->setDouble(valor);
                }
                break;
            }
        case exo_add:        // Operador: a+b
            {
                if (VarFuncIni(VarAtual - 1))
                    break;
                FuncAtual->expr++;
                int tipo = VarAtual[-1].Tipo();
            // Caso 1: N�o � texto: executa soma num�rica
                if (tipo!=varTxt)
                {
                    double valor = VarAtual[-1].getDouble() +
                                   VarAtual[0].getDouble();
                    ApagarVar(VarAtual);
                    if (VarAtual->tamanho == 0 || VarAtual->defvar != InstrDouble)
                    {
                        ApagarVar(VarAtual);
                        if (!CriarVarDouble(valor))
                            return RetornoErro(2);
                        break;
                    }
                    VarAtual->setDouble(valor);
                    break;
                }
            // Exemplo de comando no intmud.int que usa todos os tipos
            // de concatena��o: ("abc" + ("x")) + ("def" + 1) + 4

            // Caso 2: Duas vari�veis n�o locais
            // Transforma em: Primeira vari�vel � cTxtFixo local
                if (VarAtual[-1].tamanho==0 && VarAtual[0].tamanho==0)
                {
                    const char * origem = VarAtual[-1].getTxt();
                    int total = 1 + strlen(origem);
                    if (DadosTopo + total > DadosFim)
                        return RetornoErro(2);
                    VarAtual[-1].defvar = InstrTxtFixo;
                    VarAtual[-1].nomevar = nullptr;
                    VarAtual[-1].endvar = DadosTopo;
                    VarAtual[-1].tamanho = total;
                    memcpy(DadosTopo, origem, total);
                    DadosTopo += total;
                }
            // Caso 3: Primeira vari�vel n�o � cTxtFixo local
                if (VarAtual[-1].tamanho == 0 ||
                    VarAtual[-1].defvar[2] != cTxtFixo)
                {
                    char mens[BUF_MENS];
                // Monta texto em mens[]
                    char * destino = mens;
                    const char * origem = VarAtual[-1].getTxt();
                    while (*origem && destino < mens + sizeof(mens) - 1)
                        *destino++ = *origem++;
                    origem = VarAtual[0].getTxt();
                    while (*origem && destino < mens + sizeof(mens) - 1)
                        *destino++ = *origem++;
                    *destino++ = 0;
                // Apaga vari�veis locais
                    ApagarVar(VarAtual - 1);
                    int total = destino - mens;
                    if (DadosTopo + total > DadosFim)
                        return RetornoErro(2);
                // Cria vari�vel que conter� o texto
                    VarAtual++;
                    VarAtual->defvar = InstrTxtFixo;
                    VarAtual->nomevar = nullptr;
                    VarAtual->endvar = DadosTopo;
                    VarAtual->tamanho = total;
                // Copia texto para vari�vel
                    memcpy(DadosTopo, mens, total);
                    DadosTopo += total;
                    break;
                }
            // Primeira vari�vel � cTxtFixo local
                char * destino = (char*)VarAtual[-1].endvar +
                        VarAtual[-1].tamanho - 1;
                int total = 0;
            // Caso 4: Segunda vari�vel n�o � local
                if (VarAtual->tamanho == 0)
                {
                    const char * origem = VarAtual->getTxt();
                    total = 1 + strlen(origem);
                    if (destino + total >= DadosFim)
                        return RetornoErro(2);
                    memcpy(destino, origem, total);
                    VarAtual--;
                }
            // Caso 5: Segunda vari�vel � cTxtFixo local
                else if (VarAtual->defvar[2] == cTxtFixo)
                {
                    total = VarAtual->tamanho;
                    VarAtual->MoverEnd(destino, 0, 0);
                    VarAtual--;
                }
            // Caso 6: Segunda vari�vel � local
                else
                {
                    const char * origem = VarAtual->getTxt();
                    total = 1 + strlen(origem);
                    if (destino + total >= DadosFim)
                        return RetornoErro(2);
                    if (total>BUF_MENS)
                    {
                        char * pmens = new char[total];
                        memcpy(pmens, origem, total);
                        ApagarVar(VarAtual);
                        memcpy(destino, pmens, total);
                        delete[] pmens;
                    }
                    else
                    {
                        char mens[BUF_MENS];
                        memcpy(mens, origem, total);
                        ApagarVar(VarAtual);
                        memcpy(destino, mens, total);
                    }
                }
                destino += total;
                VarAtual->tamanho = destino - (char*)VarAtual->endvar;
                DadosTopo = destino;
                break;
            }
        case exo_sub:        // Operador: a-b
            {
                if (VarFuncIni(VarAtual - 1))
                    break;
                FuncAtual->expr++;
                double valor = VarAtual[-1].getDouble() -
                                VarAtual[0].getDouble();
                ApagarVar(VarAtual);
                if (VarAtual->tamanho == 0 || VarAtual->defvar != InstrDouble)
                {
                    ApagarVar(VarAtual);
                    if (!CriarVarDouble(valor))
                        return RetornoErro(2);
                    break;
                }
                VarAtual->setDouble(valor);
                break;
            }

// Aqui a vari�vel rotaciona � quantos bits deve rotacionar
// Valores positivos = para esquerda, valores negativos = para direita
#define EXO_B_ROTACAO                                 \
    char mens[BUF_MENS];                              \
    char * d1 = mens + BUF_MENS;                      \
    const char * i1 = VarAtual[-1].getTxt();          \
    const char * o1 = i1;                             \
    while (*o1)                                       \
        o1++;                                         \
    if (rotaciona >= 4)                               \
    {                                                 \
        int total = rotaciona / 4;                    \
        if (total >= BUF_MENS)                        \
        {                                             \
            ApagarVar(VarAtual - 1);                  \
            if (!CriarVarTexto("0", 1))               \
                return RetornoErro(2);                \
            break;                                    \
        }                                             \
        d1 -= total;                                  \
        memset(d1, '0', total);                       \
        rotaciona %= 4;                               \
    }                                                 \
    while (rotaciona <= -4 && o1 > i1)                \
    {                                                 \
        char ch = *--o1;                              \
        if ((ch >= '0' && ch <= '9') ||               \
            (ch >= 'A' && ch <= 'F') ||               \
            (ch >= 'a' && ch <= 'f'))                 \
            rotaciona += 4;                           \
    }                                                 \
    int bits = 0;                                     \
    while (d1 > mens && o1 > i1)                      \
    {                                                 \
        char ch = *--o1;                              \
        if (ch >= '0' && ch <= '9')                   \
            ch = ch - '0';                            \
        else if (ch >= 'A' && ch <= 'F')              \
            ch = ch - 'A' + 10;                       \
        else if (ch >= 'a' && ch <= 'f')              \
            ch = ch - 'a' + 10;                       \
        else                                          \
            continue;                                 \
        if (rotaciona < 0)                            \
        {                                             \
            rotaciona += 4;                           \
            bits = (ch << rotaciona);                 \
            continue;                                 \
        }                                             \
        bits = (bits >> 4) | (ch << rotaciona);       \
        ch = bits & 15;                               \
        *--d1 = (ch < 10 ? ch+'0' : ch+'a'-10);       \
    }                                                 \
    if (d1 > mens)                                    \
    {                                                 \
        char ch = (bits >> 4) & 15;                   \
        *--d1 = (ch < 10 ? ch+'0' : ch+'a'-10);       \
    }                                                 \
    for (; d1 < mens + BUF_MENS - 1; d1++)            \
        if (*d1 != '0')                               \
            break;                                    \
    ApagarVar(VarAtual - 1);                          \
    if (!CriarVarTexto(d1, mens+BUF_MENS-d1))         \
        return RetornoErro(2);

        case exo_b_shl:  // Operador: a << b
            {
                if (VarFuncIni(VarAtual - 1))
                    break;
                FuncAtual->expr++;
                if (VarAtual[-1].Tipo() == varTxt)
                {
                    int rotaciona = VarAtual[0].getInt();
                    if (rotaciona < 0)
                        rotaciona = 0;
                    EXO_B_ROTACAO
                    break;
                }
                int valor = VarAtual[-1].getInt() << VarAtual[0].getInt();
                ApagarVar(VarAtual);
                if (VarAtual->tamanho == 0 || VarAtual->defvar != InstrVarInt)
                {
                    ApagarVar(VarAtual);
                    if (!CriarVarInt(valor))
                        return RetornoErro(2);
                }
                else
                    VarAtual->setInt(valor);
                break;
            }
        case exo_b_shr:  // Operador: a >> b
            {
                if (VarFuncIni(VarAtual - 1))
                    break;
                FuncAtual->expr++;
                if (VarAtual[-1].Tipo() == varTxt)
                {
                    int rotaciona = VarAtual[0].getInt();
                    rotaciona = (rotaciona < 0 ? 0 : -rotaciona);
                    EXO_B_ROTACAO
                    break;
                }
                int valor = VarAtual[-1].getInt() >> VarAtual[0].getInt();
                ApagarVar(VarAtual);
                if (VarAtual->tamanho == 0 || VarAtual->defvar!=InstrVarInt)
                {
                    ApagarVar(VarAtual);
                    if (!CriarVarInt(valor))
                        return RetornoErro(2);
                }
                else
                    VarAtual->setInt(valor);
                break;
            }

#define EXO_B_TEXTO( cmp1, cmp2 )                     \
    char mens[BUF_MENS];                              \
    char * d1 = mens + BUF_MENS;                      \
    const char * i1 = VarAtual[-1].getTxt();          \
    const char * o1 = i1;                             \
    while (*o1)                                       \
        o1++;                                         \
    while (d1 > mens && o1 > i1)                      \
    {                                                 \
        char ch = *--o1;                              \
        if (ch >= '0' && ch <= '9')                   \
            *--d1 = ch - '0';                         \
        else if (ch >= 'A' && ch <= 'F')              \
            *--d1 = ch - 'A' + 10;                    \
        else if (ch >= 'a' && ch <= 'f')              \
            *--d1 = ch - 'a' + 10;                    \
    }                                                 \
    char * d2 = mens + BUF_MENS;                      \
    const char * i2 = VarAtual[0].getTxt();           \
    const char * o2 = i2;                             \
    while (*o2)                                       \
        o2++;                                         \
    while (d2 > mens && o2 > i2)                      \
    {                                                 \
        char ch = *--o2;                              \
        if (ch >= '0' && ch <= '9')                   \
            ch -= '0';                                \
        else if (ch >= 'A' && ch <= 'F')              \
            ch -= 'A' - 10;                           \
        else if (ch >= 'a' && ch <= 'f')              \
            ch -= 'a' - 10;                           \
        else                                          \
            continue;                                 \
        if (d2 > d1)                                  \
        {                                             \
            d2--;                                     \
            ch = (cmp1);                              \
        }                                             \
        else if (cmp2)                                \
            d2--;                                     \
        else                                          \
            break;                                    \
        *d2 = (ch < 10 ? ch+'0' : ch+'a'-10);         \
    }                                                 \
    if (cmp2)                                         \
        while (d2 > d1)                               \
        {                                             \
            char ch = *--d2;                          \
            *d2 = (ch < 10 ? ch+'0' : ch+'a'-10);     \
        }                                             \
    for (; d2 < mens + BUF_MENS - 1; d2++)            \
        if (*d2 != '0')                               \
            break;                                    \
    if (d2 == mens + BUF_MENS)                        \
        *--d2 = '0';                                  \
    ApagarVar(VarAtual - 1);                            \
    if (!CriarVarTexto(d2, mens+BUF_MENS-d2))         \
        return RetornoErro(2);

        case exo_b_e:   // Operador: a&b
            {
                if (VarFuncIni(VarAtual - 1))
                    break;
                FuncAtual->expr++;
                if (VarAtual[-1].Tipo() == varTxt)
                {
                    EXO_B_TEXTO( ch & *d2 , false )
                    break;
                }
                int valor = VarAtual[-1].getInt() & VarAtual[0].getInt();
                ApagarVar(VarAtual);
                if (VarAtual->tamanho == 0 || VarAtual->defvar != InstrVarInt)
                {
                    ApagarVar(VarAtual);
                    if (!CriarVarInt(valor))
                        return RetornoErro(2);
                }
                else
                    VarAtual->setInt(valor);
                break;
            }
        case exo_b_ouou: // Operador: a^b
            {
                if (VarFuncIni(VarAtual - 1))
                    break;
                FuncAtual->expr++;
                if (VarAtual[-1].Tipo() == varTxt)
                {
                    EXO_B_TEXTO( ch ^ *d2 , true )
                    break;
                }
                int valor = VarAtual[-1].getInt() ^ VarAtual[0].getInt();
                ApagarVar(VarAtual);
                if (VarAtual->tamanho == 0 || VarAtual->defvar != InstrVarInt)
                {
                    ApagarVar(VarAtual);
                    if (!CriarVarInt(valor))
                        return RetornoErro(2);
                }
                else
                    VarAtual->setInt(valor);
                break;
            }
        case exo_b_ou:  // Operador: a|b
            {
                if (VarFuncIni(VarAtual - 1))
                    break;
                FuncAtual->expr++;
                if (VarAtual[-1].Tipo() == varTxt)
                {
                    EXO_B_TEXTO( ch | *d2 , true )
                    break;
                }
                int valor = VarAtual[-1].getInt() | VarAtual[0].getInt();
                ApagarVar(VarAtual);
                if (VarAtual->tamanho == 0 || VarAtual->defvar != InstrVarInt)
                {
                    ApagarVar(VarAtual);
                    if (!CriarVarInt(valor))
                        return RetornoErro(2);
                }
                else
                    VarAtual->setInt(valor);
                break;
            }
        case exo_atrib:      // Operador de atribui��o: a=b
            if (VarAtual[-1].defvar[2] == cVarFunc ||
                VarAtual[-1].defvar[2] == cConstVar)
            {
                if (FuncAtual + 1 >= FuncFim)
                    return RetornoErro(2);
                FuncAtual->expr++;
                FuncAtual++;
                const char * defvar = VarAtual[-1].defvar;
                FuncAtual->nome = defvar;
                if (defvar[2] == cVarFunc)
                {
                    FuncAtual->linha = defvar + Num16(defvar);
                    FuncAtual->expr = nullptr;
                }
                else
                {
                    FuncAtual->linha = defvar;
                    FuncAtual->expr = defvar + defvar[endIndice];
                }
                FuncAtual->este = (TObjeto*)VarAtual[-1].endvar;
                FuncAtual->inivar = VarAtual;
                FuncAtual->fimvar = VarAtual + 1;
                FuncAtual->numarg = 1;
                FuncAtual->tipo = 2;
                FuncAtual->indent = 0;
                FuncAtual->objdebug = FuncAtual[-1].objdebug;
                FuncAtual->funcdebug = FuncAtual[-1].funcdebug;
                break;
            }
            if (VarFuncIni(VarAtual))
                break;
            FuncAtual->expr++;
            switch (VarAtual[-1].Tipo())
            {
            case varOutros:
                // Chama fun��o Igual() se forem vari�veis diferentes,
                // mas do mesmo tipo
                if (VarAtual[-1].defvar[2] == VarAtual[0].defvar[2] &&
                        (VarAtual[-1].endvar != VarAtual[0].endvar ||
                            VarAtual[-1].indice != VarAtual[0].indice) )
                    VarAtual[-1].Igual(VarAtual);
                break;
            case varInt:
                VarAtual[-1].setInt( VarAtual->getInt() );
                break;
            case varDouble:
                VarAtual[-1].setDouble( VarAtual->getDouble() );
                break;
            case varTxt:
                VarAtual[-1].setTxt( VarAtual->getTxt() );
                break;
            case varObj:
                VarAtual[-1].setObj( VarAtual->getObj() );
                break;
            }
            ApagarVar(VarAtual);
            break;
        case exo_menor:      // Operador: a<b
        case exo_menorigual: // Operador: a<=b
        case exo_maior:      // Operador: a>b
        case exo_maiorigual: // Operador: a>=b
        case exo_igual:      // Operador: a==b
        case exo_diferente:  // Operador: a!=b
            {
                if (VarFuncIni(VarAtual - 1))
                    break;
            // Compara valores
                int tipo1 = VarAtual[-1].Tipo();
                int tipo2 = VarAtual[0].Tipo();
                    // N�meros
                switch (tipo1)
                {
                case varInt:
                    if (tipo2 == varInt)
                    {
                        int v1 = VarAtual[-1].getInt();
                        int v2 = VarAtual[0].getInt();
                        tipo1 = (v1 == v2 ? 0 : v1 < v2 ? -1 : 1);
                        break;
                    }
                case varDouble:
                    {
                        double v1 = VarAtual[-1].getDouble();
                        double v2 = VarAtual[0].getDouble();
                        tipo1 = (v1 == v2 ? 0 : v1 < v2 ? -1 : 1);
                        break;
                    }
                case varTxt:
                    tipo1 = comparaZ(VarAtual[-1].getTxt(),
                                    VarAtual->getTxt());
                    break;
                case varObj:
                    if (tipo2 == varObj)
                    {
                        TObjeto * v1 = VarAtual[-1].getObj();
                        TObjeto * v2 = VarAtual[0].getObj();
                        tipo1 = (v1 == v2 ? 0 : v1 < v2 ? -1 : 1);
                        break;
                    }
                default:
                        // Mesmo tipo de vari�vel
                    if (VarAtual[-1].defvar[2] == VarAtual[0].defvar[2])
                    {
                        tipo1 = VarAtual[-1].Compara(VarAtual);
                        break;
                    }
                        // Tipos diferentes de vari�veis
                    tipo1 = (*FuncAtual->expr == exo_diferente);
                    FuncAtual->expr++;
                    ApagarVar(VarAtual - 1);
                    if (!CriarVarInt(tipo1))
                        return RetornoErro(2);
                    goto exo_compara_sair;
                    //const void * v1 = VarAtual[-1].endvar;
                    //const void * v2 = VarAtual[0].endvar;
                    //tipo1 = (v1 == v2 ? 0 : v1 < v2 ? -1 : 1);
                    //break;
                }
            // Avan�a Func->expr, verifica operador
                switch (*FuncAtual->expr++)
                {
                case exo_menor:      tipo1 = (tipo1 < 0);  break;
                case exo_menorigual: tipo1 = (tipo1 <= 0); break;
                case exo_maior:      tipo1 = (tipo1 > 0);  break;
                case exo_maiorigual: tipo1 = (tipo1 >= 0); break;
                case exo_igual:      tipo1 = (tipo1 == 0); break;
                case exo_diferente:  tipo1 = (tipo1 != 0); break;
                }
            // Apaga valores da pilha; cria int32 na pilha
                ApagarVar(VarAtual - 1);
                if (!CriarVarInt(tipo1))
                    return RetornoErro(2);
exo_compara_sair:
                break;
            }
        case exo_igual2:     // Operador: a===b
        case exo_diferente2:  // Operador: a!==b
            {
                if (VarFuncIni(VarAtual - 1))
                    break;
            // Compara valores
                int tipo1 = VarAtual[-1].Tipo();
                int tipo2 = VarAtual[0].Tipo();
                    // N�meros
                switch (tipo1)
                {
                case varInt:
                    tipo1 = 0;
                    if (tipo2 == varInt)
                        tipo1 = (VarAtual[-1].getInt() ==
                                 VarAtual[0].getInt());
                    else if (tipo2==varDouble)
                        tipo1 = (VarAtual[-1].getDouble() ==
                                 VarAtual[0].getDouble());
                    break;
                case varDouble:
                    tipo1 = 0;
                    if (tipo2 == varInt || tipo2==varDouble)
                        tipo1 = (VarAtual[-1].getDouble() ==
                                 VarAtual[0].getDouble());
                    break;
                case varTxt:
                    tipo1 = 0;
                    if (tipo2 == varTxt)
                        tipo1 = (strcmp(VarAtual[-1].getTxt(),
                                        VarAtual[0].getTxt())==0);
                    break;
                case varObj:
                    tipo1 = 0;
                    if (tipo2 == varObj)
                        tipo1 = (VarAtual[-1].getObj() ==
                                 VarAtual[0].getObj());
                    break;
                default:
                    if (tipo1 != tipo2)
                        tipo1 = 0;
                    else if (VarAtual[-1].defvar[2] == VarAtual[0].defvar[2])
                        tipo1 = (VarAtual[-1].Compara(VarAtual) == 0);
                    else
                        tipo1 = 0;
                }
                if (*FuncAtual->expr++ == exo_diferente2)
                    tipo1 = !tipo1;
            // Apaga valores da pilha; cria int32 na pilha
                ApagarVar(VarAtual - 1);
                if (!CriarVarInt(tipo1 != 0))
                    return RetornoErro(2);
                break;
            }
        case exo_ee:         // Operador: In�cio do operador &&
            if (VarFuncIni(VarAtual))
                break;
            FuncAtual->expr++;
            if (VarAtual->getBool() == false)
            {
                const char * p = ProcuraExpr(FuncAtual->expr, exo_e);
                assert(p != nullptr);
                FuncAtual->expr = p + 1;
                ApagarVar(VarAtual);
                if (!CriarVarInt(0))
                    return RetornoErro(2);
                break;
            }
            ApagarVar(VarAtual);
            break;
        case exo_ouou:       // Operador: In�cio do operador ||
            if (VarFuncIni(VarAtual))
                break;
            FuncAtual->expr++;
            if (VarAtual->getBool())
            {
                const char * p = ProcuraExpr(FuncAtual->expr, exo_ou);
                assert(p != nullptr);
                FuncAtual->expr = p + 1;
                ApagarVar(VarAtual);
                if (!CriarVarInt(1))
                    return RetornoErro(2);
                break;
            }
            ApagarVar(VarAtual);
            break;
        case exo_e:          // Operador: a&&b
        case exo_ou:         // Operador: a||b
            {
                if (VarFuncIni(VarAtual))
                    break;
                FuncAtual->expr++;
                bool b = VarAtual->getBool();
                ApagarVar(VarAtual);
                if (!CriarVarInt(b))
                    return RetornoErro(2);
                break;
            }
        case exo_i_add:      // Operador: a += b
            if (VarAtual[-1].Tipo() == varTxt)
            {
                FuncAtual->expr += 3;
                VarAtual[-1].addTxt(VarAtual[0].getTxt());
                ApagarVar(VarAtual);
                break;
            }
        case exo_i_mul:      // Operador: a *= b
        case exo_i_div:      // Operador: a /= b
        case exo_i_porcent:  // Operador: a %= b
        case exo_i_sub:      // Operador: a -= b
        case exo_i_b_shl:    // Operador: a <<= b
        case exo_i_b_shr:    // Operador: a >>= b
        case exo_i_b_e:      // Operador: a &= b
        case exo_i_b_ouou:   // Operador: a ^= b
        case exo_i_b_ou:     // Operador: a |= b
            FuncAtual->expr++;
            if (VarAtual >= VarFim-1)
                return RetornoErro(2);
            VarAtual[1] = VarAtual[0];
            VarAtual[0] = VarAtual[-1];
            VarAtual[0].tamanho = 0;
            VarAtual++;
            break;
        case exo_int1:          // Operador: In�cio do operador "?"
            if (VarFuncIni(VarAtual))
                break;
            FuncAtual->expr++;
            if (VarAtual->getBool()==false)
            {
                const char * p = ProcuraExpr(FuncAtual->expr, exo_int2);
                assert(p != nullptr);
                FuncAtual->expr = p + 1;
                ApagarVar(VarAtual);
                if (!CriarVar(InstrNulo))
                    return RetornoErro(2);
                break;
            }
            ApagarVar(VarAtual);
            break;
        case exo_int2:          // Operador: Fim do operador "?"
            FuncAtual->expr++;
            while (true)
            {
                const char * p = FuncAtual->expr;
                if (*p == exo_int1)
                    p = ProcuraExpr(p + 1, exo_int2);
                else if (*p == exo_dponto1)
                    p = ProcuraExpr(p + 1, exo_dponto2);
                else
                    break;
                assert(p != nullptr);
                FuncAtual->expr = p + 1;
            }
            break;
        case exo_intint1:       // Operador: In�cio do operador "??"
            if (VarFuncIni(VarAtual))
                break;
            FuncAtual->expr++;
            if (VarAtual->getBool())
            {
                const char * p = ProcuraExpr(FuncAtual->expr, exo_intint2);
                assert(p != nullptr);
                FuncAtual->expr = p + 1;
                break;
            }
            ApagarVar(VarAtual);
            break;
        case exo_intint2:       // Operador: Fim do operador "??"
            FuncAtual->expr++;
            break;
        case exo_dponto1:       // Operador: In�cio do operador ":"
            FuncAtual->expr++;
            ApagarVar(VarAtual);
            break;
        case exo_dponto2:       // Operador: Fim do operador ":"
            FuncAtual->expr++;
            break;

    // A partir daqui processa fun��es e vari�veis
    // Cria-se uma vari�vel cVarNome, que cont�m parte do nome da vari�vel
    // Cria-se uma vari�vel em seguida, que cont�m o resultado anterior
    // Isso torna poss�vel processar v�rias vari�veis ligadas por ponto
    // Exemplo: a.b.c
        case ex_varini:     // In�cio do texto
        case ex_varabre:    // In�cio do texto + abre colchetes
            if (!CriarVar(InstrVarNome))
                return RetornoErro(2);
            if (!CriarVar(InstrVarInicio))
                return RetornoErro(2);
            if (*FuncAtual->expr++ == ex_varini)
                FuncAtual->expr = CopiaVarNome(VarAtual - 1, FuncAtual->expr);
            break;
        case ex_varfunc:
            if (!CriarVar(InstrVarNome))
                return RetornoErro(2);
            if (!CriarVar(InstrVarIniFunc))
                return RetornoErro(2);
            VarAtual->valor_int = (unsigned char)FuncAtual->expr[1];
            FuncAtual->expr += 2;
            break;
        case ex_varlocal:
            {
                if (!CriarVar(InstrVarNome))
                    return RetornoErro(2);
                int valor = (unsigned char)FuncAtual->expr[1];
            // Vari�vel do objeto
                if (valor == 0xFF)
                {
                    if (!CriarVar(InstrVarIniFunc))
                        return RetornoErro(2);
                    VarAtual->valor_int = -1;
                    FuncAtual->expr = CopiaVarNome(
                        VarAtual - 1, FuncAtual->expr + 2);
                    break;
                }
            // Vari�vel da fun��o
                if (VarAtual >= VarFim-1)
                    return RetornoErro(2);
                // 2 bytes para ex_varlocal + X bytes para o nome da vari�vel
                const char * p = FuncAtual->expr + 2;
                while (*(const unsigned char*)p >= ' ')
                    p++;
                // Avan�a para o pr�ximo ex_ponto
                p = ProcuraExpr(p, ex_ponto);
                assert(p != nullptr);
                // Copia o nome depois do ex_ponto
                FuncAtual->expr = CopiaVarNome(VarAtual, p + 1);
                // Coloca a vari�vel local na pilha
                TVariavel * var = FuncAtual->inivar +
                                  FuncAtual->numarg + valor;
                VarAtual++;
                *VarAtual = *var;
                VarAtual->tamanho = 0;
                break;
            }
        case ex_arg:        // In�cio da lista de argumentos
        case ex_abre:       // Abre colchetes
            FuncAtual->expr++;
            break;
        case ex_fecha:      // Fecha colchetes
            {
                TVariavel * v = EndVarNome();
                if (VarFuncIni(v + 2))
                    break;
                FuncAtual->expr++;
                char mens[VAR_NOME_TAM];
                char * p = mens;
                char * pfim = p + sizeof(mens);
                *mens = 0;
                assert(v != nullptr);
                for (TVariavel * x = v + 2; x <= VarAtual; x++)
                    p = copiastr(p, x->getTxt(), pfim-p);
                ApagarVar(v + 2);
                CopiaVarNome(v, mens);
                FuncAtual->expr = CopiaVarNome(v, FuncAtual->expr);
                break;
            }
        case ex_varfim:     // Fim do texto
            {
                FuncAtual->expr++;
                TVariavel * v = EndVarNome();
                assert(v != nullptr);
                int tipo = v[1].defvar[2];
            // cVarInicio ou cVarClasse: resultado nulo
                if (tipo == cVarInicio || tipo == cVarClasse)
                {
                    ApagarVar(v);
                    if (!CriarVar(InstrNulo))
                        return RetornoErro(2);
                    break;
                }
            // Outro valor
                ApagarVar(v + 2);
                DadosTopo = (char*)v->endvar;
                if (VarAtual->tamanho)
                {
                    VarAtual->MoverEnd(v->endvar, nullptr, nullptr);
                    DadosTopo += VarAtual->tamanho;
                }
                *v = *VarAtual;
                VarAtual = v;
                break;
            }
        case ex_doispontos:
            {
                FuncAtual->expr++;
                TVariavel * v = EndVarNome();
                assert(v != nullptr);
                if (v[1].defvar[2] == cVarInicio)
                {
                    TClasse * c = TClasse::Procura(v->getTxt());
                    if (c)
                    {
                        v[1].defvar = InstrVarClasse;
                        v[1].nomevar = nullptr;
                        v[1].endvar = c;
                        v->setTxt("");
                        FuncAtual->expr = CopiaVarNome(
                                v, FuncAtual->expr);
                        break;
                    }
                }
                VarInvalido();
                break;
            }
        case ex_ponto:
            {
                FuncAtual->expr++;
                TVariavel * v = EndVarNome();
                const char * nome = v->getTxt(); // Nome encontrado
                TClasse * classe = nullptr; // Procurar vari�vel de classe/objeto
                TObjeto * objeto = nullptr; // Procurar vari�vel de objeto
                assert(v != nullptr);

            // Verifica vari�vel/fun��o da classe
                unsigned char defvar_cod = v[1].defvar[2];
                if (defvar_cod == cVarClasse)
                {
                    classe = (TClasse*)v[1].endvar;
                    objeto = FuncAtual->este;
                }
            // Verifica vari�vel/fun��o da classe
                else if (defvar_cod == cVarInicio)
                {
                // Se come�a com $, verifica se objeto existe
                    if (nome[0] == '$')
                    {
                        TClasse * c = TClasse::Procura(nome+1);
                        if (c == nullptr || c->ObjetoIni == nullptr)
                        {
                            VarInvalido();
                            break;
                        }
                        ApagarVar(v+1);
                        VarAtual++;
                        VarAtual->Limpar();
                        VarAtual->defvar = InstrVarObjeto;
                        VarAtual->endvar = c->ObjetoIni;
                        v->setTxt("");
                        FuncAtual->expr = CopiaVarNome(v, FuncAtual->expr);
                        break;
                    }
                // Verifica se � vari�vel/comando interno do programa
                    int NumFunc = InfoFunc(nome);
                    if (NumFunc >= 0)
                    {
                        const TListaFunc * func = &ListaFunc[NumFunc];
                            // Ler varfunc primeiro
                        ExecFunc * f = FuncAtual;
                        if (VarFuncIni(v + 1))
                        {
                            f->expr--;
                            break;
                        }
                            // Processa fun��o interna
                        if (func->Func(v + 1))
                        {
                            v->setTxt("");
                            f->expr = CopiaVarNome(v, f->expr);
                        }
                        else
                            VarInvalido();
                        break;
                    }
                // Verifica se � vari�vel local da fun��o
                    TVariavel * var = FuncAtual->inivar + FuncAtual->numarg;
                    for (; var < FuncAtual->fimvar; var++)
                      if (var->nomevar)
                        if (comparaVar(nome, var->nomevar + Instr::endNome) == 0)
                            break;
                    if (var < FuncAtual->fimvar)
                    {
                        ApagarVar(v+1);
                        VarAtual++;
                        *VarAtual = *var;
                        VarAtual->tamanho = 0;
                        v->setTxt("");
                        FuncAtual->expr = CopiaVarNome(v, FuncAtual->expr);
                        break;
                    }
                // Verifica se � vari�vel/fun��o do objeto
                    objeto = FuncAtual->este;
                    if (objeto)
                        classe = objeto->Classe;
                }
            // Verifica comando interno do programa (fun��o predefinida)
                else if (defvar_cod == cVarIniFunc)
                {
                    int valor = v[1].valor_int;
                    if (valor >= 0) // Se � fun��o predefinida...
                    {
                        const TListaFunc * func = &ListaFunc[ valor ];
                            // Ler varfunc primeiro
                        ExecFunc * f = FuncAtual;
                        if (VarFuncIni(v + 1))
                        {
                            f->expr--;
                            break;
                        }
                            // Processa fun��o interna
                        if (func->Func(v + 1))
                        {
                            v->setTxt("");
                            f->expr = CopiaVarNome(v, f->expr);
                        }
                        else
                            VarInvalido();
                        break;
                    }
                    // Vari�vel/fun��o do objeto
                    objeto = FuncAtual->este;
                    if (objeto)
                        classe = objeto->Classe;
                }
            // Verifica vari�vel/fun��o de objeto
                else if (v[1].Tipo()==varObj)
                {
                    objeto = v[1].getObj();
                    if (objeto == nullptr) // Objeto inexistente
                    {
                        VarInvalido();
                        break;
                    }
                    classe = objeto->Classe;
                }
            // Consulta a vari�vel sobre o que fazer
                else
                {
                // Ler varfunc primeiro
                    ExecFunc * f = FuncAtual;
                    if (VarFuncIni(v + 1))
                    {
                        f->expr--;
                        break;
                    }
                // Executa fun��o
                    if (v[1].Func(nome))
                    {
                        v->setTxt("");
                        f->expr = CopiaVarNome(v, f->expr);
                    }
                    else
                        VarInvalido();
                    break;
                }

            // Processa classe e objeto
                if (classe == nullptr)
                {
                    VarInvalido();
                    break;
                }
                int indice = classe->IndiceNome(nome);
                if (indice < 0) // Vari�vel/fun��o inexistente
                {
                    VarInvalido();
                    break;
                }
                char * defvar = classe->InstrVar[indice];
                int indvar = classe->IndiceVar[indice];
                v->setTxt("");
                FuncAtual->expr = CopiaVarNome(v, FuncAtual->expr);
            // Fun��o
                if (defvar[2]==cFunc)
                {
                    if (FuncAtual >= FuncFim - 1)
                        return RetornoErro(1);
                    FuncAtual++;
                    FuncAtual->nome = defvar;
                    FuncAtual->linha = defvar + Num16(defvar);
                    FuncAtual->este = objeto;
                    FuncAtual->expr = nullptr;
                    FuncAtual->inivar = v + 2;
                    FuncAtual->fimvar = VarAtual + 1;
                    FuncAtual->numarg = FuncAtual->fimvar - FuncAtual->inivar;
                    FuncAtual->tipo = 0;
                    FuncAtual->indent = 0;
                    FuncAtual->objdebug = FuncAtual[-1].objdebug;
                    FuncAtual->funcdebug = FuncAtual[-1].funcdebug;
                    break;
                }
            // Fun��o varfunc
                if (defvar[2] == cVarFunc || defvar[2] == cConstVar)
                {
                    ApagarVar(v + 1);
                  //  if (objeto==0)
                  //  {
                  //      if (!CriarVar(InstrNulo))
                  //          return RetornoErro(2);
                  //      break;
                  //  }
                    VarAtual++;
                    VarAtual->Limpar();
                    VarAtual->defvar = defvar;
                    VarAtual->nomevar = defvar;
                    VarAtual->endvar = objeto;
                    break;
                }
            // Express�o num�rica
                if (defvar[2] == cConstExpr)
                {
                    if (FuncAtual >= FuncFim - 1)
                        return RetornoErro(1);
                    FuncAtual++;
                    FuncAtual->nome = defvar;
                    FuncAtual->linha = defvar;
                    FuncAtual->este = objeto;
                    FuncAtual->expr = defvar + defvar[endIndice];
                    FuncAtual->inivar = v + 2;
                    FuncAtual->fimvar = VarAtual + 1;
                    FuncAtual->numarg = FuncAtual->fimvar - FuncAtual->inivar;
                    FuncAtual->tipo = 0;
                    FuncAtual->indent = 0;
                    FuncAtual->objdebug = FuncAtual[-1].objdebug;
                    FuncAtual->funcdebug = FuncAtual[-1].funcdebug;
                    break;
                }
            // Vari�vel
                ApagarVar(v+1);
                VarAtual++;
                VarAtual->Limpar();
                VarAtual->defvar = defvar;
                VarAtual->nomevar = defvar;
                VarAtual->numbit = indvar >> 24;
                VarAtual->numfunc = 0;
                VarAtual->indice = (defvar[endVetor] == 0 ? 0 : 0xFF);
                if (defvar[2] == cConstTxt || // Constante
                        defvar[2] == cConstNum)
                    VarAtual->endvar = nullptr;
                else if (indvar & 0x400000) // Vari�vel da classe
                    VarAtual->endvar = classe->Vars +
                            (indvar & 0x3FFFFF);
                else if (objeto &&  // Vari�vel do objeto
                            objeto->Classe == classe)
                        // Nota: se executar algo como "x:y = nulo",
                        // a classe vai ser "x", mas o objeto
                        // pode ser de outra classe
                    VarAtual->endvar = objeto->Vars +
                            (indvar & 0x3FFFFF);
                else // Objeto inexistente
                {
                    FuncAtual->expr++;
                    // Somou 1 para compensar Instr::FuncAtual->expr-1
                    // em VarInvalido()
                    VarInvalido();
                }
                break;
            }
        default:
            assert(0);
        }
    }
    return RetornoErro(3);
}

//----------------------------------------------------------------------------
/// Executa procedimentos ap�s ExecX()
void Instr::ExecFim()
{
    static bool dentro = false;
    if (dentro)
    {
        TVarProg::LimparVar(); // Apaga refer�ncias das vari�veis prog
        ApagarVar(VarPilha); // Apaga vari�veis pendentes
        return;
    }
    dentro = true;
    while (true)
    {
        TVarProg::LimparVar(); // Apaga refer�ncias das vari�veis prog
        ApagarVar(VarPilha); // Apaga vari�veis pendentes

    // Apaga objetos marcados para exclus�o
        while (true)
        {
            TObjeto * obj = TObjeto::IniApagar;
            if (obj == nullptr)
                break;
            if (Instr::ExecIni(obj, "fim"))
            {
                Instr::ExecX();
                TVarProg::LimparVar(); // Apaga refer�ncias das vari�veis prog
                ApagarVar(VarPilha); // Apaga vari�veis pendentes
            }
            obj->Apagar();
        }

    // Altera o programa
        if (TMudarClasse::ExecPasso())
            continue;

    // Avisa sobre erros
        if (erromens)
        {
            TVarDebug::FuncEvento("erro", erromens);
            delete[] erromens;
            erromens = nullptr;
            continue;
        }

        break;
    }
    dentro = false;
}
