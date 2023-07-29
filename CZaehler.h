/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ProWo
 * Copyright (C) root 2014 <root@ProWo2>
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

#ifndef _CZAEHLER_H_
#define _CZAEHLER_H_


class CZaehler : public CStatistic
{
public:
    CZaehler(int nr);
    ~CZaehler();

    virtual void LesenStarten() { };
    virtual void Increment(int iVal) { };
    virtual string WriteSpecificLine();
    virtual void ReadSpecificLine(CReadFile *pReadFile);    
    
    int GetAnzeigeArt();
    void SetAnzeigeArt(int iArt);
    void SetOffset(double offset, int datumTag); 
    int GetIntStand();          // Zählerstand ohne Berücksichtigung von m_iFactor
    double GetDblStand();       // Zählerstand / m_iFactor
    int GetIntAktTag();         // Wert des heutigen Tages
    double GetOffset();
    double GetOffset(int iJahr, int iMonat, int iTag);
    int GetOffsetDatum();
    bool IsTyp(int typ);
    string GetresultString(int fct, int iDiff);
    int GetresultInt(int fct, int iDiff);
    void SaveDayValue();
    
protected:
    int m_iTyp; // 0 : nicht definert
                // 1 : ABB Zaehler
                // 2 : S0Zähler
                // 3 : ZTZähler
                // 4 : Differenz- und Additionszähler (virtuell)
    int m_iAnzeigeArt;  // 0 = Stand - Offset
                        // 1 = Offset - Stand (z.B. für Tank Ölheizung)
    int m_iOffset;              // zB Stand beim letzten Zaehler lesen
    int m_iOffsetDatum;         // Datum des Offset
    int m_iStandGanzzahl;		// aktueller Zählerstand (Teil Ganzzahl)
    int m_iStandKommazahl;      // Stand hinter dem Komma
    int m_iLetzteStand;			// Zählerstand für die Tagesberechnung

};

class CZaehlerABB : public CZaehler
{
public:
    CZaehlerABB(int nr);
    virtual void LesenStarten();
    void SetModBusClient(CModBusClient * pModBusClient);

protected:
    CModBusClient * m_pModBusClient;
	
};

//
//  Zähler über Impulse
//  Der Zählvorgang muss in der Parametrierung definiert sein
//  z.B. ZAELERx = 1 wenn die Flanke eines Eingangs erfolgte
//
class CZaehlerS0 : public CZaehler
{
public:
    CZaehlerS0(int nr);
    virtual void Increment(int iVal);
protected:
    int m_iLastState;
};
//
//  Zählt die Dauer eines anstehenden Signals
//
class CZaehlerZT : public CZaehler
{
public:
    CZaehlerZT(int nr);
    virtual void Increment(int iVal);
protected:
    int m_iLastTime;
};

//
//  Virtueller Zähler
//
class CZaehlerVirt : public CZaehler
{
public:
    CZaehlerVirt(int nr);
    void init(CZaehler *pZaehler1, CZaehler *pZaehler2, int iType);
    virtual void LesenStarten();

protected:
    CZaehler *m_pZaehler1;
    CZaehler *m_pZaehler2;
    int m_iArt;
};


//
//  Zeitzähler eines anstehenden Signals
//
class CBerechneZTZaehler : public CBerechneBase
{
public:
    CBerechneZTZaehler();
    void init(CZaehler *pZaehler);	
    virtual void SetState(int state); // Inkrementiert den Zähler

private:
    CZaehler *m_pZaehler;
};

//
// S0 Zähler, erfasst die Impulse an einem Eingang
//
class CBerechneS0Zaehler:public CBerechneBase
{
public:
    CBerechneS0Zaehler();
    void init(CZaehler *pZaehler);	
    virtual void SetState(int state); // Inkrementiert den Zähler

private:
    CZaehler *m_pZaehler;
};

class COperZaehler : public COperBase
{
public:
    COperZaehler();
    virtual int resultInt();
    virtual string resultString();
    void SetOper(CZaehler *pZaehler, int iInterval, int iDiff);
    
protected:
    CZaehler *m_pZaehler;
    int m_iInterval;
    int m_iDiff;
};
#endif // _CZAEHLER_H_
