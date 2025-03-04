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
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <assert.h>
#include <vector>
#ifdef __WIN32__
 #include <windows.h>
#endif
#include "var-arqsav.h"
#include "var-listaobj.h"
#include "var-texto.h"
#include "var-textovar.h"
#include "var-textoobj.h"
#include "var-datahora.h"
#include "variavel.h"
#include "classe.h"
#include "objeto.h"
#include "arqler.h"
#include "instr.h"
#include "instr-enum.h"
#include "misc.h"
#include "random.h"
#include "sha1.h"

//#define DEBUG // Mostrar diret�rios e arquivos acessados pela fun��o LIMPAR

#define MAX_OBJ 0xFFFF // N�mero de objetos por arquivo

bool TVarSav::InicVar = false;
int TVarSav::HoraReg = 0;
//int TVarSav::Dia = 0;
//int TVarSav::Hora = 0;
//int TVarSav::Min = 0;
int TVarSav::TempoSav = 0;
TVarSavArq * TVarSavArq::Inicio = nullptr;
TVarSavArq * TVarSavArq::Fim = nullptr;

//----------------------------------------------------------------------------
const TVarInfo * TVarSav::Inicializa()
{
    static const TVarInfo var(
        TVarInfo::FTamanho0,
        TVarInfo::FTamanho0,
        TVarInfo::FTipoOutros,
        TVarInfo::FRedim0,
        TVarInfo::FFuncVetorFalse);
    return &var;
}

//----------------------------------------------------------------------------
// Acerta vari�veis
void TVarSav::ProcEventos(int tempoespera)
{
// Obt�m data e hora para o registro
#ifdef __WIN32__
    SYSTEMTIME lt = {};
    GetLocalTime(&lt);
    int dia = numdata(lt.wYear, lt.wMonth, lt.wDay) - 584389;
    HoraReg = dia * 1440 + lt.wHour * 60 + lt.wMinute;
#else
    // Nota: localtime() Converte para representa��o local de tempo
    struct tm * tempolocal;
    time_t tempoatual;
    time(&tempoatual);
    tempolocal = localtime(&tempoatual);
    int dia = numdata(tempolocal->tm_year + 1900, 
                     tempolocal->tm_mon + 1,
                     tempolocal->tm_mday) - 584389;
    HoraReg = dia * 1440 + tempolocal->tm_hour * 60 + tempolocal->tm_min;
#endif
    //printf("%d\n", (int)HoraReg); fflush(stdout);
// Checa arquivos SAV pendentes
    TempoSav += tempoespera;
    if (TempoSav < 0)
        TempoSav = 0;
    if (TempoSav >= 10)
    {
        TempoSav = 0;
        TVarSavDir::Checa();
    }
}

//----------------------------------------------------------------------------
void TVarSav::Senha(char * senhacodif, const char * senha, char fator)
{
// Checa se fator nulo
    *senhacodif++ = fator;
    if (fator == 0)
    {
        *senhacodif = 0;
        return;
    }
// Prepara a senha
    unsigned char mens[512];
    unsigned int tam = strlen(senha);
    if (tam > sizeof(mens) - 1)
        tam = sizeof(mens) - 1;
    mens[0] = fator;
    memcpy(mens + 1, senha, tam);
// Codifica
    unsigned char digest[20];
    SHA_CTX shaInfo;
    SHAInit(&shaInfo);
    SHAUpdate(&shaInfo, mens, tam + 1);
    SHAFinal(digest, &shaInfo);
// Anota na string
    for (int x = 0; x < 20; x += 4)
    {
        unsigned int valor = digest[x]   * 0x1000000+
                             digest[x + 1] * 0x10000+
                             digest[x + 2] * 0x100+
                             digest[x + 3];
        for (int b = 0; b < 5; valor /= 90, b++)
            *senhacodif++ = (char)(valor % 90 + 33);
    }
    *senhacodif = 0;
}

//----------------------------------------------------------------------------
int TVarSav::Tempo(const char * arqnome)
{
    char mens[300];
    TArqLer arqler;
    if (!arqler.Abrir(arqnome))
        return -1;
    while (arqler.Linha(mens, sizeof(mens), false) > 0)
    {
        if (strcmp(mens, "+++") == 0)
            break;
        if (compara(mens, "data=",5) != 0)
            continue;
        int tempo = TxtToInt(mens + 5) - HoraReg;
        return (tempo < 0 ? 0 : tempo);
    }
    return -1;
}

//----------------------------------------------------------------------------
bool TVarSav::Func(TVariavel * v, const char * nome)
{
// Lista das fun��es de arqsav
// Deve obrigatoriamente estar em letras min�sculas e ordem alfab�tica
    static const struct {
        const char * Nome;
        bool (*Func)(TVariavel * v); } ExecFunc[] = {
        { "apagar",       &TVarSav::FuncApagar },
        { "dias",         &TVarSav::FuncDias },
        { "existe",       &TVarSav::FuncExiste },
        { "ler",          &TVarSav::FuncLer },
        { "limpar",       &TVarSav::FuncLimpar },
        { "limpou",       &TVarSav::FuncLimpou },
        { "salvar",       &TVarSav::FuncSalvar },
        { "salvarcod",    &TVarSav::FuncSalvarCod },
        { "senha",        &TVarSav::FuncSenha },
        { "senhacod",     &TVarSav::FuncSenhaCod },
        { "valido",       &TVarSav::FuncValido }  };
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
            return (*ExecFunc[meio].Func)(v);
        if (resultado < 0) fim = meio - 1; else ini = meio + 1;
    }
    return false;
}

//------------------------------------------------------------------------------
bool TVarSav::FuncLimpar(TVariavel * v)
{
    if (!InicVar) // Inicializa vari�veis se necess�rio
    {
        ProcEventos(0);
        InicVar = true;
    }
    if (Instr::VarAtual < v + 1) // Menos de 1 argumento
    {
        TVarSavDir::ChecaTudo();
        return false;
    }
    char mens[512];
    copiastr(mens, v[1].getTxt(), sizeof(mens));
    if (*mens == 0)
        strcpy(mens, ".");
    Instr::ApagarVar(v);
// Se inv�lido: retorna 0
    if (!arqvalido(mens))
        return Instr::CriarVarInt(0);
// V�lido: coloca na lista de pendentes e retorna 1
    TVarSavDir::NovoDir(mens);
    return Instr::CriarVarInt(1);
}

//------------------------------------------------------------------------------
bool TVarSav::FuncLimpou(TVariavel * v)
{
    if (!InicVar) // Inicializa vari�veis se necess�rio
    {
        ProcEventos(0);
        InicVar = true;
    }
    char mens[BUF_MENS];
    char * p = mens;
    *p++ = (TVarSavDir::ChecaPend() ? '1' : '0');
    *p = 0;
    while (TVarSavArq::Inicio)
    {
        p = mprintf(p, mens + sizeof(mens) - p, "%c%s",
                Instr::ex_barra_n, TVarSavArq::Inicio->Nome);
        delete TVarSavArq::Inicio;
    }
    Instr::ApagarVar(v);
    return Instr::CriarVarTexto(mens);
}

//------------------------------------------------------------------------------
bool TVarSav::FuncSenhaCod(TVariavel * v)
{
    if (!InicVar) // Inicializa vari�veis se necess�rio
    {
        ProcEventos(0);
        InicVar = true;
    }
    if (Instr::VarAtual > v + 1) // Mais de um argumento
    {
        char mens2[100];
        char fator = v[2].getTxt()[0];
        Senha(mens2, v[1].getTxt(), fator);
        int result = strcmp(v[2].getTxt(), mens2);
        Instr::ApagarVar(v);
        return Instr::CriarVarInt(result == 0);
    }
    char mens[256];
    *mens = 0;
    if (Instr::VarAtual == v + 1) // 1 argumento
        Senha(mens, v[1].getTxt(), circle_random() % 90 + 33);
    Instr::ApagarVar(v);
    return Instr::CriarVarTexto(mens);
}

//------------------------------------------------------------------------------
// Checa se nome de arquivo � v�lido
bool TVarSav::FuncValido(TVariavel * v)
{
    if (!InicVar) // Inicializa vari�veis se necess�rio
    {
        ProcEventos(0);
        InicVar = true;
    }
    int result = 0;
    if (Instr::VarAtual >= v + 1)
    {
        char arqnome[512];
        copiastr(arqnome, v[1].getTxt(), sizeof(arqnome) - 4);
        if (arqvalido(arqnome, false)) // Verifica se nome permitido
            result = 1;
    }
    Instr::ApagarVar(v);
    return Instr::CriarVarInt(result);
}

//------------------------------------------------------------------------------
// Checa se arquivo existe
bool TVarSav::FuncExiste(TVariavel * v)
{
    if (!InicVar) // Inicializa vari�veis se necess�rio
    {
        ProcEventos(0);
        InicVar = true;
    }
    int result = 0;
    if (Instr::VarAtual >= v + 1)
    {
        char arqnome[512];
        struct stat buf;
        copiastr(arqnome, v[1].getTxt(), sizeof(arqnome) - 4);
        if (arqvalido(arqnome, false) && stat(arqnome, &buf) >= 0)
            result = 1;
    }
    Instr::ApagarVar(v);
    return Instr::CriarVarInt(result);
}

//------------------------------------------------------------------------------
bool TVarSav::FuncSenha(TVariavel * v)
{
    if (!InicVar) // Inicializa vari�veis se necess�rio
    {
        ProcEventos(0);
        InicVar = true;
    }
    if (Instr::VarAtual < v + 2)
    {
        Instr::ApagarVar(v);
        return Instr::CriarVarInt(0);
    }
    TArqLer arqler;
    char arqnome[512];
    copiastr(arqnome, v[1].getTxt(), sizeof(arqnome) - 4);
    if (!arqvalido(arqnome, false) || !arqler.Abrir(arqnome))
    {
        Instr::ApagarVar(v);
        return Instr::CriarVarInt(0);
    }
    char mens[512];
    while (arqler.Linha(mens, sizeof(mens), false) > 0)
    {
        if (strcmp(mens, "+++") == 0)
            break;
        if (compara(mens, "senha=", 6) != 0)
            continue;
        char mens2[100];
        Senha(mens2, v[2].getTxt(), mens[6]);
        if (strcmp(mens + 6, mens2) == 0)
            break;
        Instr::ApagarVar(v);
        return Instr::CriarVarInt(0);
    }
    Instr::ApagarVar(v);
    return Instr::CriarVarInt(1);
}

//------------------------------------------------------------------------------
bool TVarSav::FuncDias(TVariavel * v)
{
    if (!InicVar) // Inicializa vari�veis se necess�rio
    {
        ProcEventos(0);
        InicVar = true;
    }
    int result = 0;
    if (Instr::VarAtual >= v + 1)
    {
        char arqnome[512];
        copiastr(arqnome, v[1].getTxt(), sizeof(arqnome) - 4);
        if (arqvalido(arqnome, false))
        {
            result = Tempo(arqnome);
            if (result > 0)
                result = (result + 1439) / 1440;
        }
    }
    Instr::ApagarVar(v);
    return Instr::CriarVarInt(result);
}


//------------------------------------------------------------------------------
bool TVarSav::FuncLer(TVariavel * v)
{
    if (!InicVar) // Inicializa vari�veis se necess�rio
    {
        ProcEventos(0);
        InicVar = true;
    }
    int result = 0;
    if (Instr::VarAtual >= v + 1)
    {
        char arqnome[512];
        copiastr(arqnome, v[1].getTxt(), sizeof(arqnome) - 4);
        if (arqvalido(arqnome, false))
            result = Ler(v, arqnome);
    }
    Instr::ApagarVar(v);
    return Instr::CriarVarInt(result);
}

//------------------------------------------------------------------------------
bool TVarSav::FuncSalvar(TVariavel * v)
{
    if (!InicVar) // Inicializa vari�veis se necess�rio
    {
        ProcEventos(0);
        InicVar = true;
    }
    int result = 0;
    if (Instr::VarAtual >= v + 1)
    {
        char arqnome[512];
        copiastr(arqnome, v[1].getTxt(), sizeof(arqnome) - 4);
        if (arqvalido(arqnome, true))
            result = Salvar(v, arqnome, false);
    }
    Instr::ApagarVar(v);
    return Instr::CriarVarInt(result);
}

//------------------------------------------------------------------------------
bool TVarSav::FuncSalvarCod(TVariavel * v)
{
    if (!InicVar) // Inicializa vari�veis se necess�rio
    {
        ProcEventos(0);
        InicVar = true;
    }
    int result = 0;
    if (Instr::VarAtual >= v + 1)
    {
        char arqnome[512];
        copiastr(arqnome, v[1].getTxt(), sizeof(arqnome) - 4);
        if (arqvalido(arqnome, true))
            result = Salvar(v, arqnome, true);
    }
    Instr::ApagarVar(v);
    return Instr::CriarVarInt(result);
}

//------------------------------------------------------------------------------
bool TVarSav::FuncApagar(TVariavel * v)
{
    if (!InicVar) // Inicializa vari�veis se necess�rio
    {
        ProcEventos(0);
        InicVar = true;
    }
    int result = 0;
    if (Instr::VarAtual >= v + 1)
    {
        char arqnome[512];
        copiastr(arqnome, v[1].getTxt(), sizeof(arqnome) - 4);
        if (arqvalido(arqnome, true))
        {
            remove(arqnome);
            struct stat buf;
            if (stat(arqnome, &buf) < 0) // se arquivo n�o existe: sucesso
                result = 1;
        }
    }
    Instr::ApagarVar(v);
    return Instr::CriarVarInt(result);
}

//----------------------------------------------------------------------------
bool TVarSav::ObterVar(TVariavel * var, TObjeto * obj, const char * nomevar)
{
    char nome[CLASSE_NOME_TAM+2];
    char * d = nome;
    int indvar = 0;

// Separa o nome da vari�vel
    while (*nomevar && *nomevar!='.')
        if (d < nome + sizeof(nome) - 1)
            *d++ = *nomevar++;
    *d = 0;

// Obt�m o �ndice o vetor
    if (*nomevar=='.')
        for (nomevar++; *nomevar>='0' && *nomevar<='9'; nomevar++)
            indvar = indvar * 10 + *nomevar - '0';

// Obt�m a posi��o da vari�vel na classe
    TClasse * c = obj->Classe;
    int indice = c->IndiceNome(nome);
    if (indice < 0)
    {
        var->defvar = nullptr;
        return false;
    }

// Prepara objeto TVariavel
    var->defvar = c->InstrVar[indice];
    indice = c->IndiceVar[indice];
    var->numbit = indice >> 24;
    var->indice = indvar;
    if (TVariavel::Tamanho(var->defvar) == 0 ||
            indice & 0x400000) // Vari�vel da classe
    {
        var->defvar = nullptr;
        return false;
    }
    // Vari�vel do objeto
    var->endvar = obj->Vars + (indice & 0x3FFFFF);

// Verifica se vari�vel est� marcada como "sav"
// Verifica o �ndice da vari�vel
    if ((var->defvar[Instr::endProp] & 2) == 0 ||
            (indvar && indvar >=
            (unsigned char)var->defvar[Instr::endVetor]))
    {
        var->defvar = nullptr;
        return false;
    }
    //printf(" [%s] [%d]\n", var.defvar + Instr::endNome, indvar); fflush(stdout);
    return true;
}

//----------------------------------------------------------------------------
int TVarSav::Ler(TVariavel * v, const char * arqnome)
{
    unsigned int quantidade = MAX_OBJ; // N�mero m�ximo de objetos
    char mens[8192];
    TArqLer arqler;
    if (Instr::VarAtual >= v + 3)
        quantidade = v[2].getInt();
    if (*arqnome == 0 ||  // Arquivo inv�lido
            Instr::VarAtual < v + 2 || // Menos de dois argumentos
            quantidade <= 0 ||  // N�mero de objetos <=0
            v[2].defvar[2] != Instr::cListaObj || // N�o � listaobj
            !arqler.Abrir(arqnome))  // N�o conseguiu abrir arquivo
        return 0;
// Prepara vari�veis
    TListaObj * listaobj = v[2].end_listaobj + v[2].indice;
    TListaX * listaitem = listaobj->Inicio;
    std::vector<TObjeto *> bufobj;
    if (quantidade > MAX_OBJ)
        quantidade = MAX_OBJ;
// Avan�a at� a lista dos tipos de objetos
    while (arqler.Linha(mens, sizeof(mens), false) > 0)
        if (strcmp(mens, "+++") == 0)
            break;
// Cria objetos na listaobj se necess�rio
    while (arqler.Linha(mens, sizeof(mens), false) > 0)
    {
        //printf("1> %s\n", mens); fflush(stdout);
        if (strcmp(mens, "+++") == 0)
            break;
        if (bufobj.size() >= quantidade)
            continue;
    // Objeto est� na lista
        if (listaitem)
        {
            bufobj.push_back(listaitem->Objeto);
            listaitem = listaitem->ListaDepois;
            continue;
        }
    // Obt�m a classe
        TClasse * c = TClasse::Procura(mens);
    // Classe inv�lida: n�o cria objeto
        if (c == nullptr)
        {
            bufobj.push_back(nullptr);
            continue;
        }
    // Classe v�lida: cria objeto e adiciona na lista
        TObjeto * obj = TObjeto::Criar(c);
        listaobj->AddFim(obj);
        bufobj.push_back(obj);
    }
// L� objetos
    TListaX * listaitem_fim = nullptr;
    TTextoTxt * textotxt_obj = nullptr; // textotxt que falta anota \n no final
    TTextoVar * textovar_obj = nullptr; // Para anotar texto em textovar
    TTextoObj * textoobj_obj = nullptr; // Para anotar texto em textoobj
    char mensvar[65100];       // Texto que ser� anotado
    char * pmensvar = mensvar; // Aonde adicionar texto
    TVariavel var;
    var.defvar = nullptr;
    quantidade = bufobj.size();
    unsigned int numobj = 0;
    while (arqler.Linha(mens, sizeof(mens), false) > 0)
    {
    // Pr�ximo objeto
        if (strcmp(mens, "+++") == 0)
        {
            var.defvar = nullptr;
            numobj++;
            if (numobj >= quantidade)
                break;
            continue;
        }
    // Verifica se objeto v�lido
        if (bufobj[numobj] == 0)
            continue;
    // Separa nome da vari�vel, �ndice e valor
        char * p = mens;
        while (*p && *p != '=')
            p++;
        if (*p != '=')
            continue;
        *p++ = 0;
    // Obt�m a vari�vel
        if (mens[0] != '.' || mens[1] != 0)
        {
            ObterVar(&var, bufobj[numobj], mens);
            listaitem_fim = nullptr;
            if (textotxt_obj)
            {
                textotxt_obj->AddTexto("\n", 1);
                textotxt_obj = nullptr;
            }
        }
        if (var.defvar == nullptr)
            continue;
    // Anota valor na vari�vel
        switch (var.Tipo())
        {
    // Vari�veis num�ricas
        case varInt:
            var.setInt(TxtToInt(p));
            break;
    // Ponto flutuante
        case varDouble:
            var.setDouble(atof(p));
            break;
    // Texto
        case varTxt:
            {
                char * d = mens;
                for (; *p; p++)
                {
                    if (*p != '\\' || p[1] == 0)
                        *d++ = *p;
                    else switch (*++p)
                    {
                    case 'n':
                    case 'N': *d++ = Instr::ex_barra_n; break;
                    case 'b':
                    case 'B': *d++ = Instr::ex_barra_b; break;
                    case 'c':
                    case 'C': *d++ = Instr::ex_barra_c; break;
                    case 'd':
                    case 'D': *d++ = Instr::ex_barra_d; break;
                    case '\\': *d++ = '\\'; break;
                    }
                }
                *d = 0;
                var.setTxt(mens);
            }
            break;
    // Refer�ncia a objetos
        case varObj:
            {
                unsigned int indice = (p[0] - 0x21) * 0x40 + (p[1] - 0x21);
                if (p[0] < 0x21 || p[0] >= 0x61 || p[1] < 0x21 || p[1] >= 0x61 ||
                        indice >= quantidade)
                    var.setObj(nullptr);
                else
                    var.setObj(bufobj[indice]);
                break;
            }
    // Checa outros tipos de vari�veis
        default:
    // ListaObj
            if (var.defvar[2] == Instr::cListaObj)
            {
                TListaObj * lobj = var.end_listaobj + var.indice;
            // Limpa a lista
                if (*p=='\'')
                {
                    p++;
                    while (lobj->Inicio)
                        lobj->Inicio->Apagar();
                }
            // Muda a lista
                while (*p)
                {
                // Obt�m n�mero do objeto
                    unsigned int subobj = 0;
                    for (; *p >= '0' && *p <= '9'; p++)
                        subobj = subobj * 10 + *p - '0';
                // Verifica se adicionar novo objeto
                    if (*p != '.')
                    {
                        if (subobj < quantidade && bufobj[subobj])
                            listaitem_fim = lobj->AddFim(bufobj[subobj]);
                        else
                            listaitem_fim = nullptr;
                        while (*p && *p!='\'')
                            p++;
                        if (*p)
                            p++;
                        continue;
                    }
                // Adicionar refer�ncia a objeto
                    p++;
                    char * nomeobj = p;
                    while (*p && *p!='\'')
                        p++;
                    char ch = *p;
                    *p++ = 0;
                    TVariavel varitem;
                    if (subobj < quantidade && bufobj[subobj])
                      if (ObterVar(&varitem, bufobj[subobj], nomeobj))
                        if (varitem.defvar[2] == Instr::cListaItem)
                        varitem.end_listaitem[varitem.indice].MudarRef(listaitem_fim);
                    if (ch == 0)
                        break;
                }
                break;
            }
    // TextoTxt
            if (var.defvar[2] == Instr::cTextoTxt)
            {
                TTextoTxt * txtobj = var.end_textotxt + var.indice;
            // Outro textotxt: anota \n no fim do anterior
                if (textotxt_obj && textotxt_obj != txtobj)
                {
                    textotxt_obj->AddTexto("\n", 1);
                    textotxt_obj = nullptr;
                }
            // Limpar textotxt
                if ((*p | 0x20) == 'l')
                {
                    p++;
                    txtobj->Limpar();
                    textotxt_obj = nullptr;
                }
            // Anotar textopos
                if ((*p | 0x20) == 'p')
                {
                // Obt�m n�mero do objeto
                    TVariavel varitem;
                    unsigned int subobj = 0;
                    for (p++; *p >= '0' && *p <= '9'; p++)
                        subobj = subobj * 10 + *p - '0';
                // Obt�m a vari�vel textopos
                    if (*p != '.' ||
                        subobj > quantidade || bufobj[subobj] == nullptr ||
                        ObterVar(&varitem, bufobj[subobj], p + 1) == 0 ||
                        varitem.defvar[2] != Instr::cTextoPos)
                        break;
                    TTextoPos * pos = varitem.end_textopos + varitem.indice;
                // Adicina \n no final de textotxt se necess�rio
                    if (textotxt_obj)
                    {
                        textotxt_obj->AddTexto("\n", 1);
                        textotxt_obj = nullptr;
                    }
                // Aponta textopos para o final de textotxt
                    pos->MudarTxt(txtobj);
                    pos->Bloco = txtobj->Fim;
                    pos->PosicBloco = (pos->Bloco ? pos->Bloco->Bytes : 0);
                    pos->PosicTxt = txtobj->Bytes;
                    pos->LinhaTxt = txtobj->Linhas;
                    break;
                }
            // Anotar texto em textotxt
                if ((*p | 0x20) == 't')
                {
                    char * d = mens;
                    for (p++; *p; p++)
                    {
                        if (*p != '\\' || p[1] == 0)
                            *d++ = *p;
                        else switch (*++p)
                        {
                        case 'n':
                        case 'N': *d++ = Instr::ex_barra_n; break;
                        case 'b':
                        case 'B': *d++ = Instr::ex_barra_b; break;
                        case 'c':
                        case 'C': *d++ = Instr::ex_barra_c; break;
                        case 'd':
                        case 'D': *d++ = Instr::ex_barra_d; break;
                        case '\\': *d++ = '\\'; break;
                        }
                    }
                    if (d != mens)
                    {
                        textotxt_obj = (d[-1] == Instr::ex_barra_n ?
                                        nullptr : txtobj);
                        txtobj->AddTexto(mens, d - mens);
                    }
                    break;
                }
                break;
            }
    // TextoVar
            if (var.defvar[2] == Instr::cTextoVar)
            {
                TTextoVar * txtvar = var.end_textovar + var.indice;
            // Outro textovar: anota texto e inicializa textovar
                if (txtvar != textovar_obj)
                {
                    txtvar->Limpar();
                    textovar_obj = txtvar;
                    pmensvar = mensvar;
                }
            // Acrescenta texto
                for (; *p; p++)
                {
                    if (*p != '\\' || p[1] == 0)
                    {
                        *pmensvar = *p;
                        if (pmensvar < mensvar + sizeof(mensvar))
                            pmensvar++;
                        continue;
                    }
                    bool completo = false;
                    switch (*++p)
                    {
                    case 'n':
                    case 'N': *pmensvar++ = Instr::ex_barra_n; break;
                    case 'b':
                    case 'B': *pmensvar++ = Instr::ex_barra_b; break;
                    case 'c':
                    case 'C': *pmensvar++ = Instr::ex_barra_c; break;
                    case 'd':
                    case 'D': *pmensvar++ = Instr::ex_barra_d; break;
                    case '\\': *pmensvar++ = '\\'; break;
                    case '=': *pmensvar++ = 0x1F; break;
                    case '0': completo = true; break;
                    }
                    if (pmensvar >= mensvar + sizeof(mensvar))
                        pmensvar--;
                    if (!completo)
                        continue;
                // Acerta o texto
                    *pmensvar = 0;
                    pmensvar = mensvar;
                // Procura um sinal de igual
                    char * texto = mensvar;
                    while (*texto && *texto != '=')
                    {
                        if (*texto == 0x1F)
                            *texto = '=';
                        texto++;
                    }
                    if (*texto == 0)
                        continue;
                // Marca o fim do nome da vari�vel
                    *texto++ = 0;
                    for (char * p1 = texto; *p1; p1++)
                        if (*p1 == 0x1F)
                            *p1 = '=';
                // Cria a vari�vel
                    TTextoVarSub sub;
                    sub.Criar(txtvar, mensvar, true);
                    if (sub.TipoVar != TextoVarTipoRef)
                        sub.setTxt(texto);
                    else
                    {
                        unsigned int subobj = atoi(texto);
                        if (*texto && subobj < quantidade)
                            if (bufobj[subobj])
                                sub.setObj(bufobj[subobj]);
                    }
                    sub.Apagar();
                }
                break;
            }
    // TextoObj
            if (var.defvar[2] == Instr::cTextoObj)
            {
                TTextoObj * txtobj = var.end_textoobj + var.indice;
            // Outro textoobj: inicializa textoobj
                if (txtobj != textoobj_obj)
                {
                    txtobj->Limpar();
                    textoobj_obj = txtobj;
                }
            // Acrescenta as vari�veis
                while (*p)
                {
                // Procura um sinal de igual
                    char * nomevar = p;
                    while (*p && *p != '=')
                        p++;
                    if (*p == 0)
                        break;
                // Marca o fim do nome da vari�vel
                    *p++ = 0;
                // Obt�m n�mero do objeto
                    unsigned int subobj = 0;
                    for (; *p >= '0' && *p <= '9'; p++)
                        subobj = subobj * 10 + *p - '0';
                // Adiciona vari�vel
                    if (*nomevar && subobj < quantidade)
                        if (bufobj[subobj])
                            txtobj->Mudar(nomevar, bufobj[subobj]);
                // Passa para a pr�xima vari�vel
                    if (*p == '\'')
                        p++;
                }
                break;
            }
    // DataHora
            if (var.defvar[2] == Instr::cDataHora)
                var.end_datahora[var.indice].LerSav(p);
        } // switch
    } // while
    if (textotxt_obj)
        textotxt_obj->AddTexto("\n", 1);

    return quantidade;
}

//----------------------------------------------------------------------------
int TVarSav::Salvar(TVariavel * v, const char * arqnome, bool senhacod)
{
    if (*arqnome == 0 ||  // Arquivo inv�lido
            Instr::VarAtual < v + 4 || // Menos de 4 argumentos
            v[2].defvar[2] != Instr::cListaObj) // N�o � listaobj
        return 0;
// Cria arquivo
    FILE * arq = fopen("intmud-temp.txt", "w");
    if (arq == nullptr)
        return 0;
// Anota a quantidade de dias para expirar
    int valor = v[3].getInt();
    if (valor >= 1)
        fprintf(arq, "data=%d\n", HoraReg + valor * 1440);
// Anota a senha
    const char * senha = v[4].getTxt();
    if (*senha)
    {
        char mens[256];
        if (senhacod)
        {
            char * p = mens;
            for (; p < mens + sizeof(mens) - 1 && *senha; senha++)
                if (*senha >= 33 && *senha < 128)
                    *p++ = *senha;
            *p = 0;
        }
        else
            Senha(mens, senha, circle_random() % 90 + 33);
        fprintf(arq, "senha=%s\n", mens);
    }
// Anota os tipos de objetos
    TListaObj * listaobj = v[2].end_listaobj + v[2].indice;
    std::vector<TObjeto *> bufobj;
    fprintf(arq, "+++\n");
    for (TListaX * litem = listaobj->Inicio; litem; litem = litem->ListaDepois)
    {
        unsigned int total = bufobj.size();
        if (total >= MAX_OBJ)
            break;
        TObjeto * obj = litem->Objeto;
    // Checa se objeto j� est� em bufobj
        unsigned int num = obj->NumeroSav;
        if (num >= total || bufobj[num] != obj)
        {
        // Acerta vari�veis
            obj->NumeroSav = total;
            bufobj.push_back(obj);
        // Anota no arquivo
            fprintf(arq, "%s\n", obj->Classe->Nome);
        }
    }
// Anota as vari�veis dos objetos
    unsigned int quantidade = bufobj.size();
    for (unsigned int numobj = 0; numobj < quantidade; numobj++)
    {
        fprintf(arq, "+++\n");
        TClasse * c = bufobj[numobj]->Classe;
            // Procura todas as vari�veis
        for (unsigned int x = 0; x < c->NumVar; x++)
        {
        // Verifica se � "sav" e n�o � "comum"
            if ((c->InstrVar[x][Instr::endProp] & 3) != 2)
                continue;
        // Prepara objeto TVariavel
            TVariavel var;
            var.defvar = c->InstrVar[x];
            int posic = c->IndiceVar[x];
            var.numbit = posic >> 24;
            var.indice = 0;
            if (TVariavel::Tamanho(var.defvar) == 0 ||
                    (posic & 0x400000)) // Vari�vel da classe
                continue;
            // Vari�vel do objeto
            var.endvar = bufobj[numobj]->Vars + (posic & 0x3FFFFF);
        // Anota valor na vari�vel
            posic = (unsigned char)var.defvar[Instr::endVetor];
            switch (var.Tipo())
            {
        // Vari�veis num�ricas
            case varInt:
                do {
                    if (var.indice)
                        fprintf(arq, "%s.%d=%d\n",
                                var.defvar + Instr::endNome,
                                var.indice, var.getInt());
                    else
                        fprintf(arq, "%s=%d\n",
                                var.defvar + Instr::endNome,
                                var.getInt());
                } while (++var.indice < posic);
                break;
        // Ponto flutuante
            case varDouble:
                do {
                    if (var.indice)
                        fprintf(arq, "%s.%d=%.16g\n",
                                var.defvar + Instr::endNome,
                                var.indice, var.getDouble());
                    else
                        fprintf(arq, "%s=%.16g\n",
                                var.defvar + Instr::endNome,
                                var.getDouble());
                } while (++var.indice < posic);
                break;
        // Texto
            case varTxt:
                do {
                    char mens[4096];
                    const char * o = var.getTxt();
                    char * d = mens;
                    for (; *o && d<mens + sizeof(mens) - 2; o++)
                        switch (*o)
                        {
                        case Instr::ex_barra_n: *d++ = '\\'; *d++ = 'n'; break;
                        case Instr::ex_barra_b: *d++ = '\\'; *d++ = 'b'; break;
                        case Instr::ex_barra_c: *d++ = '\\'; *d++ = 'c'; break;
                        case Instr::ex_barra_d: *d++ = '\\'; *d++ = 'd'; break;
                        case '\\': *d++ = '\\';
                        default: *d++ = *o;
                        }
                    *d = 0;
                    if (var.indice)
                        fprintf(arq, "%s.%d=%s\n",
                                var.defvar + Instr::endNome,
                                var.indice, mens);
                    else
                        fprintf(arq, "%s=%s\n",
                                var.defvar + Instr::endNome,
                                mens);
                } while (++var.indice < posic);
                break;
        // Refer�ncia a objetos
            case varObj:
                do {
                    char mens[3] = "";
                    TObjeto * obj = var.getObj();
                    if (obj)
                    {
                        unsigned int num = obj->NumeroSav;
                        if (num<quantidade && bufobj[num] == obj)
                        {
                            mens[0] = (num / 0x40) + 0x21;
                            mens[1] = (num % 0x40) + 0x21;
                            mens[2] = 0;
                        }
                    }
                    if (var.indice)
                        fprintf(arq, "%s.%d=%s\n",
                                var.defvar + Instr::endNome,
                                var.indice, mens);
                    else
                        fprintf(arq, "%s=%s\n",
                                var.defvar + Instr::endNome,
                                mens);
                } while (++var.indice < posic);
                break;
        // Checa outros tipos de vari�veis
            default:
              switch (var.defvar[2])
              {
        // ListaObj
              case Instr::cListaObj:
                do {
                    char mens[512];
                    char * d = mens;
                    TListaObj * lobj = var.end_listaobj + var.indice;
                    TListaX * listax = lobj->Inicio;
                    if (var.indice)
                        sprintf(d, "%s.%d=",
                                var.defvar + Instr::endNome, var.indice);
                    else
                        sprintf(d, "%s=", var.defvar + Instr::endNome);
                    while (*d)
                        d++;
                    if (listax == nullptr)
                        *d++ = '\'';
                    for (; listax; listax = listax->ListaDepois)
                    {
                    // Obt�m o n�mero do objeto
                        unsigned int num = listax->Objeto->NumeroSav;
                        if (num>=quantidade || bufobj[num] != listax->Objeto)
                            continue;
                    // Verifica se tem espa�o em mens
                        if (d >= mens + 150)
                        {
                            *d = 0;
                            fprintf(arq, "%s\n", mens);
                            d=copiastr(mens, ".=");
                        }
                        else
                            *d++ = '\'';
                    // Anota o n�mero do objeto
                        sprintf(d, "%d", num);
                        while (*d)
                            d++;
                    // Anota objetos listaitem
                        TListaItem * listaitem = listax->ListaItem;
                        for (; listaitem; listaitem = listaitem->Depois)
                        {
                        // Verifica se est� marcado como sav
                            if ((listaitem->defvar[Instr::endProp] & 2) == 0)
                                continue;
                        // Verifica se lista pertence a algum objeto
                            if (listaitem->Objeto == nullptr)
                                continue;
                        // Verifica se objeto da listaitem ser� salvo
                            unsigned int nitem = listaitem->Objeto->NumeroSav;
                            if (nitem >= quantidade ||
                                    bufobj[nitem] != listaitem->Objeto)
                                continue;
                        // Verifica se tem espa�o em mens
                            if (d >= mens + 150)
                            {
                                *d = 0;
                                fprintf(arq, "%s\n", mens);
                                d=copiastr(mens, ".=");
                            }
                            else
                                *d++ = '\'';
                        // Anota dados da listaitem
                            if (listaitem->indice)
                                sprintf(d, "%d.%s.%d", nitem,
                                        listaitem->defvar + Instr::endNome,
                                        listaitem->indice);
                            else
                                sprintf(d, "%d.%s", nitem,
                                        listaitem->defvar + Instr::endNome);
                            while (*d) d++;
                        }
                    }
                    *d = 0;
                    fprintf(arq, "%s\n", mens);
                } while (++var.indice < posic);
                break;
        // TextoTxt
              case Instr::cTextoTxt:
                do {
                    char mens[512];
                    char * d = mens;
                    TTextoTxt * txtobj = var.end_textotxt + var.indice;
                    if (var.indice)
                        sprintf(d, "%s.%d=lt",
                                var.defvar + Instr::endNome, var.indice);
                    else
                        sprintf(d, "%s=lt", var.defvar + Instr::endNome);
                    while (*d)
                        d++;
                // Anota texto
                    TTextoBloco * bl = txtobj->Inicio;
                    char * ppos = (bl ? bl->Texto : nullptr);
                    int tampos = (bl ? bl->Bytes : 0);
                    unsigned int pos_atual = 0;
                    unsigned int pos_proc = 0;
                    while (true)
                    {
                    // Anota vari�veis textopos
                        if (pos_atual == pos_proc)
                        {
                            bool textovazio = (pos_atual == 0);
                            pos_proc = txtobj->Bytes + 1;
                            for (TTextoPos * posobj = txtobj->Posic; posobj;
                                    posobj=posobj->Depois)
                            {
                            // Verifica se est� marcado como sav
                                if ((posobj->defvar[Instr::endProp] & 2) == 0)
                                    continue;
                            // Verifica se j� passou pela posi��o
                                if (posobj->PosicTxt < pos_atual)
                                    continue;
                            // Verifica se textopos ser� salvo
                                unsigned int nitem = posobj->Objeto->NumeroSav;
                                if (nitem >= quantidade ||
                                        bufobj[nitem] != posobj->Objeto)
                                    continue;
                            // Acerta lin_proc
                                if (posobj->PosicTxt != pos_atual)
                                {
                                    if (pos_proc > posobj->PosicTxt)
                                        pos_proc = posobj->PosicTxt;
                                    continue;
                                }
                            // Anota posi��o
                                if (textovazio)
                                    d--;
                                else
                                    d=copiastr(d, "\n.=");
                                textovazio = true;
                                if (posobj->indice)
                                    sprintf(d, "p%d.%s.%d\n", nitem,
                                            posobj->defvar + Instr::endNome,
                                            posobj->indice);
                                else
                                    sprintf(d, "p%d.%s\n", nitem,
                                            posobj->defvar + Instr::endNome);
                                fprintf(arq, "%s", mens);
                                d = copiastr(mens, ".=t");
                            }
                        }
                        pos_atual++;
                    // Passa para o pr�ximo bloco se necess�rio
                        if (tampos <= 0)
                        {
                            if (bl == nullptr || bl->Depois == nullptr)
                                break;
                            bl = bl->Depois;
                            ppos = bl->Texto;
                            tampos = bl->Bytes;
                        }
                    // Anota texto se buffer cheio
                        if (d >= mens + 150)
                        {
                            *d = 0;
                            fprintf(arq, "%s\\f\n", mens);
                            d=copiastr(mens, ".=t");
                        }
                    // Anota um byte
                        switch (*ppos)
                        {
                        case Instr::ex_barra_n: *d++ = '\\'; *d++ = 'n'; break;
                        case Instr::ex_barra_b: *d++ = '\\'; *d++ = 'b'; break;
                        case Instr::ex_barra_c: *d++ = '\\'; *d++ = 'c'; break;
                        case Instr::ex_barra_d: *d++ = '\\'; *d++ = 'd'; break;
                        case '\\': *d++ = '\\';
                        default: *d++ = *ppos;
                        }
                        ppos++, tampos--;
                    }
                    *d = 0;
                    if (strcmp(mens, ".=t") != 0)
                        fprintf(arq, "%s\\f\n", mens);
                } while (++var.indice < posic);
                break;
        // TextoVar
              case Instr::cTextoVar:
                do {
                    bool vazio = true;
                    char mens[512];
                    char * d = mens;
                    TTextoVar * txtvar = var.end_textovar + var.indice;
                    if (var.indice)
                        sprintf(d, "%s.%d=",
                                var.defvar + Instr::endNome, var.indice);
                    else
                        sprintf(d, "%s=", var.defvar + Instr::endNome);
                    while (*d)
                        d++;
                    TBlocoVar * bl = txtvar->RBroot;
                    if (bl)
                        bl = bl->RBfirst();
                    for (; bl; bl = TBlocoVar::RBnext(bl))
                    {
                    // Obt�m o conte�do da vari�vel
                        char txttemp[10];
                        const char * var_conteudo = nullptr;
                        if (bl->TipoVar() == TextoVarTipoRef)
                        {
                            // Obt�m o n�mero do objeto
                            TObjeto * obj = bl->getObj();
                            unsigned int num = obj->NumeroSav;
                            if (num>=quantidade || bufobj[num] != obj)
                                continue;
                            sprintf(txttemp, "%d", num);
                            var_conteudo = txttemp;
                        }
                        else
                            var_conteudo = bl->getTxt();
                    // Nome da vari�vel
                        const char * p = bl->NomeVar;
                        assert(*p != 0);
                        while (*p)
                        {
                            switch (*p)
                            {
                            case Instr::ex_barra_n: *d++ = '\\'; *d++ = 'n'; break;
                            case Instr::ex_barra_b: *d++ = '\\'; *d++ = 'b'; break;
                            case Instr::ex_barra_c: *d++ = '\\'; *d++ = 'c'; break;
                            case Instr::ex_barra_d: *d++ = '\\'; *d++ = 'd'; break;
                            case '=': *d++ = '\\'; *d++ = '='; break;
                            case '\\': *d++ = '\\'; *d++ = '\\'; break;
                            default: *d++ = *p;
                            }
                            p++;
                            if (d - mens < 150)
                                continue;
                            *d = 0;
                            fprintf(arq, "%s\\f\n", mens);
                            d = copiastr(mens, ".=");
                        }
                    // Tipo de vari�vel
                        if (bl->TipoVar() != TextoVarTipoTxt || p[-1] == ' ' ||
                                p[-1] == '_' || p[-1] == '@' || p[-1] == '$')
                            p = bl->Tipo();
                        else
                            p = "";
                        for (int cont = 0; cont < 10 && *p; cont++)
                            *d++ = *p++;
                        *d++ = '=';
                    // Conte�do da vari�vel
                        p = var_conteudo;
                        while (*p)
                        {
                            switch (*p)
                            {
                            case Instr::ex_barra_n: *d++ = '\\'; *d++ = 'n'; break;
                            case Instr::ex_barra_b: *d++ = '\\'; *d++ = 'b'; break;
                            case Instr::ex_barra_c: *d++ = '\\'; *d++ = 'c'; break;
                            case Instr::ex_barra_d: *d++ = '\\'; *d++ = 'd'; break;
                            case '\\': *d++ = '\\'; *d++ = '\\'; break;
                            default: *d++ = *p;
                            }
                            p++;
                            if (d - mens < 150)
                                continue;
                            *d = 0;
                            fprintf(arq, "%s\\f\n", mens);
                            d = copiastr(mens, ".=");
                        }
                        *d++ = '\\';
                        *d++ = '0';
                        vazio = false;
                    }
                    *d = 0;
                    if (!vazio)
                        fprintf(arq, "%s\\f\n", mens);
                } while (++var.indice < posic);
                break;
        // TextoObj
              case Instr::cTextoObj:
                do {
                    char mens[512];
                    char * d = mens;
                    TTextoObj * txtobj = var.end_textoobj + var.indice;
                    if (var.indice)
                        sprintf(d, "%s.%d=",
                                var.defvar + Instr::endNome, var.indice);
                    else
                        sprintf(d, "%s=", var.defvar + Instr::endNome);
                    while (*d)
                        d++;
                    TBlocoObj * bl = txtobj->RBroot;
                    if (bl)
                        bl = bl->RBfirst();
                    bool inicio = true;
                    for (; bl; bl=TBlocoObj::RBnext(bl))
                    {
                    // Obt�m o n�mero do objeto
                        unsigned int num = bl->Objeto->NumeroSav;
                        if (num >= quantidade || bufobj[num] != bl->Objeto)
                            continue;
                    // Verifica se tem espa�o em mens
                        if (d >= mens + 150)
                        {
                            *d = 0;
                            fprintf(arq, "%s\n", mens);
                            d=copiastr(mens, ".=");
                            inicio = true;
                        }
                        if (inicio)
                            inicio = false;
                        else
                            *d++ = '\'';
                    // Anota o n�mero do objeto
                        sprintf(d, "%s=%d", bl->NomeVar, num);
                        while (*d)
                            d++;
                    }
                    *d=0;
                    if (strcmp(mens, ".=") != 0)
                        fprintf(arq, "%s\n", mens);
                } while (++var.indice < posic);
                break;
        // DataHora
              case Instr::cDataHora:
                do {
                    char mens[100];
                    var.end_datahora[var.indice].SalvarSav(mens);
                    if (var.indice)
                        fprintf(arq, "%s.%d=%s\n",
                                var.defvar + Instr::endNome,
                                var.indice, mens);
                    else
                        fprintf(arq, "%s=%s\n",
                                var.defvar + Instr::endNome, mens);
                } while (++var.indice < posic);
                break;
              }
            } // switch
        } // for ... vari�veis
    } // for ... objetos
// Fecha arquivo
    fclose(arq);
// Renomeia arquivo
#ifdef __WIN32__
    if (MoveFileEx("intmud-temp.txt", arqnome,
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
        return 1;
#else
    if (rename("intmud-temp.txt", arqnome) >= 0)
        return 1;
#endif
    remove("intmud-temp.txt");
    return 0;
}

//----------------------------------------------------------------------------
void TVarSavDir::NovoDir(const char * nomedir)
{
// Procura objeto com o mesmo nome, retorna se encontrou
    TVarSavDir * y = RBroot;
    while (y)
    {
        int i = strcmp(nomedir, y->Nome);
        if (i == 0)
            return;
        if (i < 0)
            y = y->RBleft;
        else
            y = y->RBright;
    }
// Cria objeto
    new TVarSavDir(nomedir);
}

//----------------------------------------------------------------------------
TVarSavDir::TVarSavDir(const char * nomedir)
{
#ifdef DEBUG
    printf("Novo dir: %s\n", nomedir); fflush(stdout);
#endif
// Aloca mem�ria em Nome
    int tam = strlen(nomedir) + 1;
    Nome = new char[tam];
    memcpy(Nome, nomedir, tam);
// Coloca na RBT
    RBinsert();
// Coloca na lista ligada
    if (Inicio == nullptr)
        Inicio = this;
    if (Fim)
        Fim->Proximo = this;
    Fim = this;
    Proximo = nullptr;
}

//----------------------------------------------------------------------------
TVarSavDir::~TVarSavDir()
{
#ifdef DEBUG
    printf("Fim do dir: %s\n", Nome); fflush(stdout);
#endif
// Desaloca mem�ria em Nome
    if (Nome)
        delete[] Nome;
// Tira da RBT
    RBremove();
// Tira da lista ligada
// Via de regra TVarSavDir s� apaga o primeiro elemento da lista
    assert(Inicio==this);
    Inicio=Proximo;
    if (Fim == this)
        Fim = nullptr;
}

//----------------------------------------------------------------------------
bool TVarSavDir::Checa()
{
    static DIR * dir = nullptr;
    for (int x = 0; x < 10; x++)
    {
    // Abre diret�rio se n�o estiver aberto
        if (dir == nullptr)
        {
            if (Inicio == nullptr)
                return false;
            dir = opendir(Inicio->Nome);
            if (dir == nullptr)
                delete Inicio;
            continue;
        }
        while (true)
        {
    // L� o pr�ximo arquivo
            dirent * sdir = readdir(dir);
            if (sdir == nullptr)
            {
                delete Inicio;
                closedir(dir);
                dir = nullptr;
                break;
            }
    // Checa se � arquivo .sav
            char * pont = sdir->d_name;
            while (*pont)
                pont++;
            pont -= 4;
            if (pont <= sdir->d_name)
                continue;
            if ( pont[0] != '.' || (pont[1] | 0x20) != 's' ||
                 (pont[2] | 0x20) != 'a' || (pont[3] | 0x20) != 'v' )
                continue;
    // Apaga arquivo se expirou
            char arq[2048];
#ifdef __WIN32__
            mprintf(arq, sizeof(arq), "%s\\%s", Inicio->Nome, sdir->d_name);
#else
            mprintf(arq, sizeof(arq), "%s/%s", Inicio->Nome, sdir->d_name);
#endif
#ifdef DEBUG
            printf("%s arq: %s\n", TVarSav::Tempo(arq) == 0 ?
                    "Removendo" : "Checando", arq);
            fflush(stdout);
#endif
            if (TVarSav::Tempo(arq) == 0)
            {
                remove(arq);
                new TVarSavArq(arq);
            }
            break;
        }
    }
    return true;
}

//----------------------------------------------------------------------------
void TVarSavDir::ChecaTudo()
{
    while (Checa());
}

//----------------------------------------------------------------------------
bool TVarSavDir::ChecaPend()
{
    return (Inicio != nullptr);
}

//----------------------------------------------------------------------------
int TVarSavDir::RBcomp(TVarSavDir * x, TVarSavDir * y)
{
    return strcmp(x->Nome, y->Nome);
}

//----------------------------------------------------------------------------
TVarSavArq::TVarSavArq(const char * nome)
{
    Antes = Fim;
    Depois = nullptr;
    (Antes ? Antes->Depois : Inicio) = this;
    Fim = this;
    copiastr(Nome, nome, sizeof(Nome));
}
//----------------------------------------------------------------------------
TVarSavArq::~TVarSavArq()
{
    (Antes ? Antes->Depois : Inicio) = Depois;
    (Depois ? Depois->Antes : Fim) = Antes;
}

//----------------------------------------------------------------------------
TVarSavDir * TVarSavDir::Inicio = nullptr;
TVarSavDir * TVarSavDir::Fim = nullptr;
TVarSavDir * TVarSavDir::RBroot = nullptr;
#define CLASS TVarSavDir    // Nome da classe
#define RBmask 1 // M�scara para bit 0
#include "rbt.cpp.h"
