#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "var-arqprog.h"
#include "variavel.h"
#include "arqmapa.h"
#include "instr.h"
#include "misc.h"

char * TArqIncluir::arqnome = nullptr;
TArqIncluir * TArqIncluir::Inicio = nullptr;
TArqIncluir * TArqIncluir::Fim = nullptr;

//----------------------------------------------------------------------------
TArqIncluir::TArqIncluir(const char * nome)
{
    copiastr(Padrao, nome, sizeof(Padrao));
    Anterior = Fim, Proximo = nullptr;
    (Fim ? Fim->Proximo : Inicio) = this;
    Fim = this;
}

//----------------------------------------------------------------------------
TArqIncluir::~TArqIncluir()
{
    (Anterior ? Anterior->Proximo : Inicio) = Proximo;
    (Proximo ? Proximo->Anterior : Fim) = Anterior;
}

//----------------------------------------------------------------------------
bool TArqIncluir::ProcPadrao(const char * nome)
{
    if (*nome == 0)
        return true;
    for (TArqIncluir * obj = Inicio; obj; obj = obj->Proximo)
        if (strcmp(nome, obj->Padrao) == 0)
            return true;
    return false;
}

//----------------------------------------------------------------------------
bool TArqIncluir::ProcArq(const char * nome)
{
    for (TArqIncluir * obj = Inicio; obj; obj=obj->Proximo)
    {
        int tam = strlen(obj->Padrao);
        if (memcmp(nome, obj->Padrao, tam) != 0)
            continue;
        const char * p = nome + tam;
        while (*p && *p != '\\' && *p != '/')
            p++;
        if (*p == 0)
            return true;
    }
    return false;
}

//----------------------------------------------------------------------------
void TArqIncluir::ArqNome(const char * nome)
{
    if (arqnome)
    {
        delete[] arqnome;
        arqnome = nullptr;
    }
    if (nome == nullptr)
        return;
    int tam = strlen(nome) + 1;
    arqnome = new char[tam];
    memcpy(arqnome, nome, tam);
}

//----------------------------------------------------------------------------
const char * TArqIncluir::ArqNome()
{
    return arqnome;
}

//----------------------------------------------------------------------------
const TVarInfo * TVarArqProg::Inicializa()
{
    static const TVarInfo var(
        FTamanho,
        FTamanhoVetor,
        TVarInfo::FTipoOutros,
        FRedim,
        TVarInfo::FFuncVetorFalse);
    return &var;
}

//----------------------------------------------------------------------------
void TVarArqProg::Criar()
{
#ifdef __WIN32__
    wdir = INVALID_HANDLE_VALUE;
#else
    dir = nullptr;
#endif
    Incluir = nullptr;
    *Arquivo = 0;
}

//----------------------------------------------------------------------------
void TVarArqProg::Apagar()
{
    Fechar();
}

//----------------------------------------------------------------------------
void TVarArqProg::Abrir()
{
    Fechar();
    copiastr(Arquivo, TArqIncluir::ArqNome(), sizeof(Arquivo));
    Incluir = TArqIncluir::Inicio;
}

//----------------------------------------------------------------------------
void TVarArqProg::Fechar()
{
    *Arquivo = 0;
#ifdef __WIN32__
    if (wdir != INVALID_HANDLE_VALUE)
        FindClose(wdir);
    wdir = INVALID_HANDLE_VALUE;
#else
    if (dir)
        closedir(dir);
    dir = nullptr;
#endif
}

//----------------------------------------------------------------------------
void TVarArqProg::Proximo()
{
    char mens[ARQINCLUIR_TAM]; // Para procurar arquivos em um diret�rio
    const char * nomearq = nullptr; // Nome do arquivo encontrado

    while (true)
    {

// Se diret�rio fechado: come�a a busca e obt�m a primeira entrada
#ifdef __WIN32__
        if (wdir == INVALID_HANDLE_VALUE)
#else
        if (dir == nullptr)
#endif
        {
        // Se n�o h� pr�ximo diret�rio: fim da busca
            if (Incluir == nullptr)
            {
                *Arquivo = 0;
                return;
            }
        // Acerta vari�veis Arquivo, ArqBarra e ArqPadrao
            copiastr(Arquivo, Incluir->Padrao, sizeof(Arquivo));
            ArqBarra = 0;
            const char * p;
            for (p = Arquivo; *p; p++)
                if (*p == '/' || *p == '\\')
                    ArqBarra = p + 1 - Arquivo;
            ArqPadrao = p - Arquivo;
#ifdef __WIN32__
        // Obt�m o nome a procurar
            copiastr(mens, Incluir->Padrao, sizeof(mens) - 1);
            strcat(mens, "*");
            for (char * p = mens; *p; p++)
                if (*p == '/')
                    *p = '\\';
        // Passa para o pr�ximo Incluir
            Incluir = Incluir->Proximo;
        // Abre o diret�rio e obt�m a primeira entrada
            WIN32_FIND_DATA ffd;
            wdir = FindFirstFile(mens, &ffd);
            if (wdir == INVALID_HANDLE_VALUE)
                continue;
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;
            nomearq = ffd.cFileName;
#else
        // Obt�m o nome a procurar
            copiastr(mens, Incluir->Padrao, sizeof(mens) - 1);
            strcat(mens, "*");
            char * p2 = mens;
            for (char * p = mens; *p; p++)
                switch (*p)
                {
                case '\\': *p = '/';
                case '/':  p2 = p;
                }
            *p2 = 0;
            if (p2 == mens)
                strcpy(mens, ".");
        // Passa para o pr�ximo Incluir
            Incluir = Incluir->Proximo;
        // Abre o diret�rio e obt�m a primeira entrada
            dir = opendir(mens);
            if (dir == nullptr)
                continue;
            struct dirent * sdir = readdir(dir);
            if (sdir == nullptr)
            {
                closedir(dir);
                dir = nullptr;
                continue;
            }
            if (sdir->d_type != DT_REG)
                continue;
            nomearq = sdir->d_name;
#endif
        }
        else

// Diret�rio est� aberto: obt�m a pr�xima entrada
        {
#ifdef __WIN32__
            WIN32_FIND_DATA ffd;
            if (FindNextFile(wdir, &ffd) == 0)
            {
                FindClose(wdir);
                wdir = INVALID_HANDLE_VALUE;
                continue;
            }
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;
            nomearq = ffd.cFileName;
#else
            struct dirent * sdir = readdir(dir);
            if (sdir == nullptr)
            {
                closedir(dir);
                dir = nullptr;
                continue;
            }
            if (sdir->d_type != DT_REG)
                continue;
            nomearq = sdir->d_name;
#endif
        }

// Ignora entradas "." e ".."
        //if (strcmp(nomearq, ".")==0 || strcmp(nomearq, "..")==0)
        //    continue;

        if (*nomearq == '.') // Ignora arquivos cujo nome come�a com um ponto
            continue;

// Checa in�cio do nome do arquivo
        if (memcmp(nomearq, Arquivo+ArqBarra, ArqPadrao-ArqBarra) != 0)
            continue;

// Checa fim do nome do arquivo
        int tam = strlen(nomearq) - 4;
        if (tam < ArqPadrao - ArqBarra)
            continue;
        if (strcmp(nomearq + tam, ".int") != 0)
            continue;

// Checa se nome v�lido para arquivo ".int"
        if (!TArqMapa::NomeValido(nomearq))
            continue;

// Anota nome do arquivo
        if (tam > static_cast<int>(sizeof(Arquivo)) - ArqBarra + 1)
            continue;
        memcpy(Arquivo + ArqBarra, nomearq, tam);
        Arquivo[ArqBarra + tam] = 0;

// Checa arquivo inicial
        if (strcmp(Arquivo + ArqBarra, TArqIncluir::ArqNome()) == 0)
            continue;
        return;
    }
}

//------------------------------------------------------------------------------
bool TVarArqProg::Func(TVariavel * v, const char * nome)
{
// Lista das fun��es de arqprog
// Deve obrigatoriamente estar em letras min�sculas e ordem alfab�tica
    static const struct {
        const char * Nome;
        bool (TVarArqProg::*Func)(TVariavel * v); } ExecFunc[] = {
        { "abrir",        &TVarArqProg::FuncAbrir },
        { "depois",       &TVarArqProg::FuncDepois },
        { "fechar",       &TVarArqProg::FuncFechar },
        { "lin",          &TVarArqProg::FuncLin },
        { "texto",        &TVarArqProg::FuncTexto } };
// Procura a fun��o correspondente e executa
    int ini = 0;
    int fim = sizeof(ExecFunc) / sizeof(ExecFunc[0]) - 1;
    char mens[80];
    copiastrmin(mens, nome, sizeof(mens));
    while (ini <= fim)
    {
        int meio = (ini + fim) / 2;
        int resultado = strcmp(mens, ExecFunc[meio].Nome);
        if (resultado == 0) // Se encontrou...
            return (this->*ExecFunc[meio].Func)(v);
        if (resultado < 0) fim = meio - 1; else ini = meio + 1;
    }
    return false;
}

//------------------------------------------------------------------------------
bool TVarArqProg::FuncAbrir(TVariavel * v)
{
    Fechar();
    Abrir();
    Instr::ApagarVar(v);
    return Instr::CriarVarTexto("");
}
//------------------------------------------------------------------------------
bool TVarArqProg::FuncFechar(TVariavel * v)
{
    Fechar();
    return false;
}

//------------------------------------------------------------------------------
bool TVarArqProg::FuncDepois(TVariavel * v)
{
    Proximo();
    return false;
}

//------------------------------------------------------------------------------
bool TVarArqProg::FuncLin(TVariavel * v)
{
    bool b = (*Arquivo != 0);
    Instr::ApagarVar(v);
    return Instr::CriarVarInt(b);
}

//------------------------------------------------------------------------------
bool TVarArqProg::FuncTexto(TVariavel * v)
{
    char arq[ARQINCLUIR_TAM];
    copiastr(arq, Arquivo, sizeof(arq));
    Instr::ApagarVar(v);
    return Instr::CriarVarTexto(arq);
}

//------------------------------------------------------------------------------
int TVarArqProg::FTamanho(const char * instr)
{
    return sizeof(TVarArqProg);
}

//------------------------------------------------------------------------------
int TVarArqProg::FTamanhoVetor(const char * instr)
{
    int total = (unsigned char)instr[Instr::endVetor];
    return (total ? total : 1) * sizeof(TVarArqProg);
}

//------------------------------------------------------------------------------
void TVarArqProg::FRedim(TVariavel * v, TClasse * c, TObjeto * o,
        unsigned int antes, unsigned int depois)
{
    TVarArqProg * ref = reinterpret_cast<TVarArqProg*>(v->endvar);
    for (; antes < depois; antes++)
        ref[antes].Criar();
    for (; depois < antes; depois++)
        ref[depois].Apagar();
}
