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

#ifndef _COPER_H_
#define _COPER_H_

#define MAXOPER 128

class CInteger
{
public:
    CInteger();
    int GetState();
    void SetLastState();
    void SetState(int state);
    bool GetStateE();
    bool GetStateA();    
private:
    int m_iValue;
    int m_bLastState;
};

class CSecTimer
{
public:
    CSecTimer();
    bool Increment(int zeit);
    time_t GetState();
    bool GetStateE();
    bool GetStateA();
    void SetState(int iState);
    void SetLastState();
private:
    time_t m_iStartzeit;
    time_t m_iDelay;
    bool m_bLastState;
};

class COperBase
{
public:
    COperBase();
    virtual int resultInt() {return 1;}
    virtual string resultString() { return "";}
    void setType(unsigned char type);
    unsigned char getType();

private:
    unsigned char m_cType;
};

class COper: public COperBase
{
    public:
    COper();
    virtual int resultInt();
    void setOper(char *cptr, char *cptrLast, char oper);
protected:
    char *m_cptr;
    char *m_cptrLast;
    char m_coperand;
};

// Einschalten
class COperE : public COper
{
public:
    virtual int resultInt();
};

// Ausschalten
class COperA:public COper
{
public:
    virtual int resultInt();
};

// Wechsel, Ein- oder Ausschalten
class COperW:public COper
{
public:
    virtual int resultInt();
};

// Timer
class COperTimer:public COperBase
{
public:
    COperTimer();
    virtual int resultInt();
    void setOper(CSecTimer *ptr);
protected:
    CSecTimer *m_pSecTimer;
};

// Timer Einschalten
class COperTimerE:public COperTimer
{
    virtual int resultInt();
};
// Timer Ausschalten
class COperTimerA:public COperTimer
{
    virtual int resultInt();
};

class COperTagNacht : public COperBase
{
public:
    COperTagNacht();
    void setOper(int iFct);
    virtual int resultInt();
protected:
    int m_iFct;
};


class COperNumber : public COperBase
{
public:
    COperNumber();
    virtual int resultInt();
    void setOper(int nr);

protected:
    int m_iNumber;
};

//
//  Integer Merker
//
class COperInteger : public COperBase
{
public:
    COperInteger();
    virtual int resultInt();
    void setOper(CInteger *pInteger);

protected:
    CInteger *m_pInteger;
    bool m_bLastState;
};
// Integer einschalten
class COperIntegerE:public COperInteger
{
    virtual int resultInt();
};
// Integer ausschalten
class COperIntegerA:public COperInteger
{
    virtual int resultInt();
};
//
//  Heizkörper
//
class COperHK : public COperBase
{
public:
    COperHK();
    virtual int resultInt();
    void setOper(CHeizKoerper *ptr);
protected:
    CHeizKoerper *m_pHeizKoerper;
};

class COperSTHQ : public COperBase
{
public:
    COperSTHQ();
    virtual int resultInt();
    void setOper(CSensor *ptr);
    
protected:
    CSensor *m_pTempSensor;
};

//
// Temperatur
//
class COperTEMP : public COperSTHQ
{
    virtual int resultInt();
};

class COperHUM : public COperSTHQ
{
    virtual int resultInt();
};

class COperQUAL : public COperSTHQ
{
    virtual int resultInt();
};

#endif // _COPER_H_