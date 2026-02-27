/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ProWo
 * Copyright (C) root 2014 <root@raspberrypi>
 * 
ProWo is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * ProWo is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CBERECHNE_H_
#define _CBERECHNE_H_

struct EWAusgEntity
{
    int iNr;    // channel*256 + button
    int iSource;   // 1 = Parameter, 2 = History, 3 = Handsteuerung (nicht getestet)
};
//
//  CBerechneBase wird eingesetzt:
//      - GetState() : Browsermenu zum Lesen des Zustandes 
//      - SetState() : zum Ändern des Zustandes z.B Inkrement S0-Zähler
//      - GesString() : Gibt im Stringformat den Wert des ersten COperBase zurück
//      - eval: zum Berechnen von der in m_pOperBase gespeicherten Gleichung
class CBerechneBase
{
public:
    CBerechneBase();
    int eval();
    void setOper(COperBase **pOperBase);
    void setIfElse(int IfElse) {m_IfElse = IfElse; };
    int getIfElse() {return m_IfElse; };
    virtual int GetState(){ return 0;};
    virtual void SetState(int state){}; 
    virtual int GetMax() { return 0;};
    int GetTyp();
    string GetString();
	
protected:
    int m_nr;       // wird gebraucht in SetState bei Easywave, writeMessage
    int m_iTyp;     // gebraucht bei SetState, ist 1 bei HUE und 2 bei Somfy
    int m_IfElse; // 0=immer 1=wenn true, 2=wenn false
    COperBase **m_pOperBase;
};

class CFormatText
{
public:
    CFormatText();
    ~CFormatText();  
    string GetString();
    void SetString(string str);
    void Init(int iAnz);
    void SetOper(int iIdx, CBerechneBase *pBerechne, char ch);
protected:
    string m_strText;
    CBerechneBase **m_pBerechneBase;
    char *m_pOperTyp;
};

class CBerechneWrite:public CBerechneBase
{
public:
    void init(CReadFile *pReadFile, void *pIOGroup);
    virtual void SetState(int iWert);
protected:
    CFormatText m_FormatText;
};

class CBerechneWriteMessage:public CBerechneBase
{
public:
    void init(CReadFile *pReadFile, void *pIOGroup, int iAnzeigeArt);
    virtual void SetState(int iWert);
protected:
    CFormatText m_FormatText;
    void * m_pIOGroup; 
    int m_iAnzeigeArt;
};

class CBerechneAusg : public CBerechneBase
{
public:
    void init(int nr, char *cPtr);
    virtual void SetState(int state);
    virtual int GetState();
protected:
    char *m_cPtr;
    char m_ch;
};

class CBerechneSecTimer : public CBerechneBase
{
public:
    void init(int nr, CSecTimer *pSecTimer);	
    virtual void SetState(int state);

protected:
    CSecTimer *m_pSecTimer;
};

class CBerechneEWAusg : public CBerechneBase
{
public:
    void init(int nr, char *ptr, char ch, queue<struct EWAusgEntity> * qPtr);
    virtual void SetState(int state);
	
protected:
    char *m_cPtr;
    char m_EW;
    queue<struct EWAusgEntity> * m_pFifo;
};

class CBerechneEWEing : public CBerechneBase
{
public:
    void init(int nr, char *ptr, char ch);
    virtual void SetState(int state);
	
protected:
    char *m_cPtr;
    char m_EW;
};

class CBerechneInteger : public CBerechneBase
{
public:
    void init(CInteger *pInteger);
    virtual void SetState(int state);
    virtual int GetState();
protected:
    CInteger *m_pInteger;
};


#endif // _CBERECHNE_H_
