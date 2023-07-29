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
    int iSource;   // 1 = Parameter, 2 = History
};
//
//  CBerechneBase wird eingesetzt:
//      - GetState() : Browsermenu zum Lesen des Zustandes 
//      - SetState() : zum Ändern des Zustandes z.B Inkrement S0-Zähler
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
    virtual char * GetContent() {return NULL;};
	
protected:
    int m_nr;
    int m_IfElse; // 0=immer 1=wenn true, 2=wenn false
    COperBase **m_pOperBase;
};

class CBerechneWrite:public CBerechneBase
{
public:
    CBerechneWrite();
    void init(string str);
    virtual void SetState(int iWert);
protected:
    string m_strText;
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
protected:
    CInteger *m_pInteger;
};


#endif // _CBERECHNE_H_
