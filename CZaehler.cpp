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

#include "ProWo.h"

//
//  CZaehlerVirt
//
CZaehlerVirt::CZaehlerVirt(int nr):CZaehler(nr)
{
    m_iFactor = 1;
    m_pZaehler1 = NULL;
    m_pZaehler2 = NULL;
    m_iArt = 0;
    m_iTyp = 4;
}

void CZaehlerVirt::init(CZaehler *pZaehler1, CZaehler *pZaehler2, int iType)
{
    m_pZaehler1 = pZaehler1;
    m_pZaehler2 = pZaehler2;
    m_iArt = iType;
    m_iFactor = 100;
}

void CZaehlerVirt::LesenStarten()
{
    double dblStand1;
    double dblStand2;
    
    if(m_pZaehler1 != NULL && m_pZaehler2 != NULL)
    {
        pthread_mutex_lock(&ext_mutexNodejs);
        dblStand1 = m_pZaehler1->GetDblStand();
        dblStand2 = m_pZaehler2->GetDblStand();
        switch(m_iArt) {
        case 1: // Summe
            dblStand1 += dblStand2;
            break;
        case 2: // Differenz
            dblStand1 -= dblStand2;
            break;
        default:
            dblStand1 = 0;
            break;
        }
        m_iStandGanzzahl = (int)(dblStand1);
        m_iStandKommazahl = (int)(dblStand1 * (double)m_iFactor) % m_iFactor;
        pthread_mutex_unlock(&ext_mutexNodejs);
    }
}				
			
//
//  CZaehlerS0
//
CZaehlerS0::CZaehlerS0(int nr):CZaehler (nr)
{
    m_iFactor = 1;
    m_iTyp = 2;
    m_iLastState = 0;
}

void CZaehlerS0::Increment(int iVal)
{
    if(iVal && iVal != m_iLastState)
    {
        m_iStandKommazahl++;
        if(m_iStandKommazahl >= m_iFactor)
        {
            m_iStandGanzzahl++;
            m_iStandKommazahl -= m_iFactor;
        } 
          
    } 
    m_iLastState = iVal;
}

CZaehlerZT::CZaehlerZT(int nr) : CZaehler(nr)
{
    m_iFactor = 1;
    m_iTyp = 3;
    m_iLastTime = m_pUhr->getUhrmin();
}

void CZaehlerZT::Increment(int iVal)
{
    int iDiff;
    int iTime = m_pUhr->getUhrmin();
    if(iVal)
    {
        iDiff = iTime - m_iLastTime;
        if(iDiff < 0)
            iDiff += 1440;
        m_iStandKommazahl += iDiff;
        if(m_iStandKommazahl >= m_iFactor)
        {
            m_iStandGanzzahl += m_iStandKommazahl / m_iFactor;
            m_iStandKommazahl %= m_iFactor;
        }
    }
    m_iLastTime = iTime;
}
//
// CZaehlerABB Stromzähler ABB
//
CZaehlerABB::CZaehlerABB(int nr):CZaehler (nr)
{
    m_pModBusClient = NULL;
    m_iFactor = 100;
    m_iTyp = 1;
}

void CZaehlerABB::LesenStarten()
{
    unsigned char *ptr;
    int ret;
    char ptrLog[100];
    int iStand;

    ret = m_pModBusClient->GetStatus();
    if(ret == 3) // Empfang ist erfolgt
    {   
        ret = m_pModBusClient->GetError(); 
        if(!ret)
        {
            ptr = m_pModBusClient->GetEmpfPtr();
            iStand = ((ptr[7]*256 + ptr[8])*256 + ptr[9])*256 + ptr[10];
            pthread_mutex_lock(&ext_mutexNodejs);             
            m_iStandGanzzahl = iStand / m_iFactor;
            m_iStandKommazahl = iStand % m_iFactor;
            pthread_mutex_unlock(&ext_mutexNodejs);             
        }
        else
        {
            sprintf(ptrLog, "Zählernummer %d, error %d", m_iNummer, ret);
            syslog(LOG_ERR, ptrLog);
        }
    }

    // Status wird auf 1 gesetzt, die Anfrage wird mit Senden neu gestartet
    m_pModBusClient->StartSend();
}

void CZaehlerABB :: SetModBusClient(CModBusClient * pClient)
{
    m_pModBusClient = pClient;
    unsigned char *pCh = pClient->GetSendPtr ();
    pCh[1] = 0x03;
    pCh[2] = 0x50;
    pCh[3] = 0x08;
    pCh[4] = 0x00;
    pCh[5] = 0x04;
    pClient->SetSendLen(6);
    pClient->SetEmpfLen(8);
}

//
//  CZaehler
//

int CZaehler::GetIntStand()
{
    return m_iStandGanzzahl * m_iFactor + m_iStandKommazahl;
}

double CZaehler::GetDblStand()
{
    return (double)GetIntStand() / (double)m_iFactor;
}

int CZaehler::GetIntOffset()
{
    return m_iOffset;
}

double CZaehler::GetDblOffset()
{
    return ((double)m_iOffset) / ((double)m_iFactor);
}

int CZaehler::GetAnzeigeArt()
{
    return m_iAnzeigeArt;
}

void CZaehler::SetAnzeigeArt(int iArt)
{
    m_iAnzeigeArt = iArt;
}

double CZaehler::GetOffset(int jahr, int monat, int tag)
{
    int year, anzTage;
    double ret = 0;
    struct tm t;
    bool ok = true;
    
    m_pUhr->getUhrzeitStruct(&t);
    if(jahr >= m_iFirstYear && jahr <= m_iAktYear)
    {
        year = jahr - m_iFirstYear;
        anzTage = m_pUhr->getAnzahlTage(jahr, monat);
        if(monat >= 1 && monat <= 12)
        {
            if(jahr == m_iAktYear)
            {
                if(monat > t.tm_mon + 1)
                    ok = false;
                if(monat == t.tm_mon+1)
                {
                    if(tag > t.tm_mday)
                        ok = false;
                }
            }
            if(!(tag >= 1 && tag <= anzTage))
                ok = false;
            if(ok)
            {
                for(; jahr <= m_iAktYear; jahr++)
                {
                    year = jahr - m_iFirstYear;
                    for(;monat <= 12; monat++)
                    {
                        for(; tag <= 31; tag++)
                        {
                            ret += m_pDaten[year].mon[monat-1].val[tag-1];                            
                        }
                        tag = 1;
                    }
                    monat = 1;
                }
                ret = (double)(m_iStandGanzzahl * m_iFactor + m_iStandKommazahl) - ret;
                ret = ret / (double)m_iFactor;
            }   
        }
    }
    return ret;
}

void CZaehler::SetOffset(double offset, int datumTag)
{
    m_iOffset = offset * m_iFactor;
    m_iOffsetDatum = datumTag;
    SchreibDatei ();
}

int CZaehler::GetOffsetDatum()
{
    return m_iOffsetDatum;
}

bool CZaehler::IsTyp(int typ)
{
    if(m_iTyp == typ)
        return true;
    else
        return false;
}
int CZaehler::GetIntAktTag()
{
    return (m_iStandGanzzahl * m_iFactor + m_iStandKommazahl) - m_iLetzteStand;
}
void CZaehler::SaveDayValue()
{
    SaveValue((m_iStandGanzzahl * m_iFactor + m_iStandKommazahl) - m_iLetzteStand);
    m_iLetzteStand = m_iStandGanzzahl * m_iFactor + m_iStandKommazahl;
}

CZaehler::CZaehler(int nr)
{
    m_iOffset = 0;
    m_iOffsetDatum = 0;
    m_iStandGanzzahl = 0;
    m_iStandKommazahl = 0;
    m_iLetzteStand = 0;
    m_iTyp = 0;
    m_iAnzeigeArt = 0;
    m_iFileTyp = 5;
    m_iValTyp = VALTYPTOT; 
    LesDatei(nr);
}

CZaehler::~CZaehler()
{
    SchreibDatei();
}

string CZaehler::GetresultString(int iInterval, int iDiff)
{
    int iRet;
    char buf[MSGSIZE];
    string strRet;
    
    iRet = GetresultInt(iInterval, iDiff);
    snprintf(buf, MSGSIZE, "%0.2f", (double)iRet / (double)m_iFactor);
    strRet = buf;
    return strRet;
}

int CZaehler::GetresultInt(int iInterval, int iDiff)
{   
    int iRet;
    switch(iInterval) {
        case 0: // Stand des Zählers
            iRet = GetIntStand();
            break;
        case 1: // Tag
        case 2: // Woche
        case 3: // Monat
        case 4: // Jahr
            iRet = GetResultInt(iInterval, iDiff, GetIntAktTag());
            break;
        case 5: // Wert 
            if(m_iAnzeigeArt)
                iRet = GetIntOffset() - GetIntStand();
            else
                iRet = GetIntStand() - GetIntOffset();
                break;
            break;
        default:
            iRet = 0;
            break;
    }
    return iRet;
}

string CZaehler::WriteSpecificLine()
{
    string strRet;
    
    strRet = "2000;" + to_string(m_iStandGanzzahl) + ";" + to_string(m_iStandKommazahl) + ";"
            + to_string(m_iLetzteStand) + ";" + to_string( m_iOffset) + ";" + to_string(m_iOffsetDatum) + ";";
    return strRet;
}


void CZaehler::ReadSpecificLine(CReadFile *pReadFile)
{
  
    m_iStandGanzzahl = pReadFile->ReadZahl ();
    if(!pReadFile->ReadSeparator ())
        pReadFile->Error (62);                    
    m_iStandKommazahl = pReadFile->ReadZahl ();
    if(!pReadFile->ReadSeparator ())
        pReadFile->Error (62);                    
    m_iLetzteStand = pReadFile->ReadZahl ();
    if(!pReadFile->ReadSeparator ())
        pReadFile->Error (62);                    
    m_iOffset = pReadFile->ReadZahl ();
    if(!pReadFile->ReadSeparator())
         pReadFile->Error (62); 
    m_iOffsetDatum = pReadFile->ReadZahl ();   
    if(!pReadFile->ReadSeparator ())
        pReadFile->Error (62);
}

//
//  Zeitzähler eines anstehenden Eingangs zB
//
CBerechneZTZaehler::CBerechneZTZaehler()
{
    m_pZaehler = NULL;
}

void CBerechneZTZaehler::init(CZaehler *pZaehler)
{
    m_pZaehler = pZaehler;
}

void CBerechneZTZaehler::SetState(int state)
{
    if(m_pZaehler != NULL)
        m_pZaehler->Increment(state);
}

//
//  S0-Zaehler
//
CBerechneS0Zaehler::CBerechneS0Zaehler()
{
    m_pZaehler = NULL;
}

void CBerechneS0Zaehler::init(CZaehler *pZaehler)
{
    m_pZaehler = pZaehler;
}

void CBerechneS0Zaehler::SetState(int state)
{
    if(m_pZaehler != NULL)
        m_pZaehler->Increment(state);
}

COperZaehler::COperZaehler()
{
    m_iInterval = 0;
    m_iDiff = 0;
    m_pZaehler = NULL;
}
string COperZaehler::resultString()
{
    return m_pZaehler->GetresultString(m_iInterval, m_iDiff);
}

int COperZaehler::resultInt()
{
    return m_pZaehler->GetresultInt(m_iInterval, m_iDiff);
}

void COperZaehler::SetOper(CZaehler *pZaehler, int iInterval, int iDiff)
{
    m_pZaehler = pZaehler;
    m_iInterval = iInterval;
    m_iDiff = iDiff;
}
