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


#include "ProWo.h"

//
// Basisklasse für die Liste der zu errechnenden Werte (Ausgänge)
//

CBerechneBase::CBerechneBase()
{
    m_pOperBase = NULL;
    m_IfElse = 0;
    m_nr = 0;
}

int CBerechneBase::eval()
{   
    int res;
    CCalculator cc;

    if(m_pOperBase != NULL)
    {
        res = cc.eval(m_pOperBase);
        SetState(res);
    }
    return res;
}

void CBerechneBase::setOper(COperBase **pOperBase)
{
    m_pOperBase = pOperBase;
}

string CBerechneBase::GetString()
{
    return m_pOperBase[0]->resultString();
}

//
//  write : Anzeige von Resultaten
void CBerechneWrite::init(CReadFile *pReadFile, void *pIOGroup)
{
    m_nr = 0;
    ((CIOGroup *)pIOGroup)->SetFormatText(&m_FormatText, pReadFile);
}

void CBerechneWrite::SetState(int iWert)
{
    string str;

    str = m_FormatText.GetString();
    syslog(LOG_INFO, str.c_str());
}

//
// writeMessage
void CBerechneWriteMessage::init(CReadFile *pReadFile, void *pIOGroup, int iAnzeigeArt)
{
    m_nr = pReadFile->GetLine();
    m_pIOGroup = pIOGroup;
    m_iAnzeigeArt = iAnzeigeArt;
    ((CIOGroup *)m_pIOGroup)->SetFormatText(&m_FormatText, pReadFile);
}

void CBerechneWriteMessage::SetState(int iWert)
{
    CWriteMessage writeMessage;
    std::map<int, CWriteMessage>::iterator it;
    CIOGroup * pIOGroup = (CIOGroup *)m_pIOGroup;
    
    writeMessage.m_strText = m_FormatText.GetString();
    it = pIOGroup->m_mapWriteMessage.find(m_nr);
    if(it != pIOGroup->m_mapWriteMessage.end())
    {
        switch(m_iAnzeigeArt) {
            case 1: // Message überschreiben
                it->second = writeMessage;
                break;
            default:
                break;
        }
    }
    else
        pIOGroup->m_mapWriteMessage.insert({m_nr, writeMessage});
}
//
// Integer Merker
//
void CBerechneInteger::init(CInteger *pInteger)
{
    m_pInteger = pInteger;
}
void CBerechneInteger::SetState(int state)
{
    m_pInteger->SetState(state);
}
int CBerechneInteger::GetState()
{
    return m_pInteger->GetState();
}
//
//  Ausgänge
//
void CBerechneAusg::SetState(int state)
{
    if(state % 256)
        *m_cPtr |= m_ch;
    else
        *m_cPtr &= ~m_ch;
}

void CBerechneAusg::init(int nr, char *pPtr)
{
    m_ch = 0x01;
    m_ch <<= (nr-1)%PORTANZAHLCHANNEL;
    m_nr = nr;
    m_cPtr = pPtr;
}

int CBerechneAusg::GetState()
{
    if(*m_cPtr & m_ch)
        return 1;
    else
        return 0;
}

//
// Sekunden Timer
//
void CBerechneSecTimer::SetState(int state)
{
    m_pSecTimer->SetState(state);
}

void CBerechneSecTimer::init(int nr, CSecTimer *pSecTimer)
{
    m_nr = nr;
    m_pSecTimer = pSecTimer;
}

//
// EasyWave Ausgänge
//
void CBerechneEWAusg::init(int nr, char *cPtr, char ch, queue<struct EWAusgEntity> * qPtr)
{
	m_nr = nr;
	m_cPtr = cPtr;
	m_EW = ch;
	m_pFifo = qPtr; 
}

void CBerechneEWAusg::SetState(int state)
{
	char ch;
    struct EWAusgEntity EWAusg;
    
	ch = *m_cPtr;
    state = state % 256;
	// Das Telegramm wird gesendet auch wenn der Zustand bereits richtig
	// gesetzt ist
	switch(m_EW) {
	case 0x20: // CD in der Parameterdatei F
		if(state)
		{
			if (!(ch & 0x20))
				ch |= 0x20;
			// C senden
            EWAusg.iNr = m_nr * 256 + 0x02;
		}
		else
		{
			if(ch & 0x20)
			   ch &= 0xDF;
			// D senden
            EWAusg.iNr = m_nr * 256 + 0x03;
		}
		break;
	case 0x10: // AB in der Parameterdatei E
		if(state)
		{   if(!(ch & 0x10))
			    ch |= 0x10;
            EWAusg.iNr = m_nr * 256 + 0x00;
		}
		else
		{   if(ch & 0x10)
			    ch &= 0xEF;
        EWAusg.iNr = m_nr * 256 + 0x01;
		}
		break;
	case 0x01: // a
		if(state)
		{   if(!(ch & 0x01))
			    ch |= 0x01;
		}
		else
		{   if(ch & 0x01)
			    ch &= 0xFE;
		}
        EWAusg.iNr = m_nr * 256 + 0x00;
		break;		
	case 0x02: // b
		if(state)
		{   if(!(ch & 0x02))
			    ch |= 0x02;
		}
		else
		{   if(ch & 0x02)
			    ch &= 0xFD;
		}
        EWAusg.iNr = m_nr * 256 + 0x01;
		break;		
	case 0x04: // c
		if(state)
		{   if(!(ch & 0x04))
			    ch |= 0x04;
		}
		else
		{   if(ch & 0x04)
			    ch &= 0xFB;
		}
        EWAusg.iNr = m_nr * 256 + 0x02;
		break;		
	case 0x08: // d
		if(state)
		{   if(!(ch & 0x08))
			    ch |= 0x08;
		}
		else
		{   if(ch & 0x08)
			    ch &= 0xF7;
		}
        EWAusg.iNr = m_nr * 256 + 0x03;
		break;	

	default:
        EWAusg.iNr = 0;
		break;
	}
    if(EWAusg.iNr)
    {
        EWAusg.iSource = 1;
        m_pFifo->push(EWAusg);
    }
            
	*m_cPtr = ch;
}

//
// EasyWave Eingänge
//
void CBerechneEWEing::init(int nr, char *cPtr, char ch)
{
	m_nr = nr;
	m_cPtr = cPtr;
	m_EW = ch;
}

void CBerechneEWEing::SetState(int state)
{
    char ch;
    ch = *m_cPtr;
    state = state % 256;
    switch(m_EW) {
    case 0x20: // CD
        if(state)
        {
            if (!(ch & 0x20))
                ch |= 0x20;
        }
        else
        {
            if(ch & 0x20)
               ch &= 0xDF;
        }
        break;
    case 0x10: // AB
        if(state)
        {   if(!(ch & 0x10))
                ch |= 0x10;
        }
        else
        {   if(ch & 0x10)
                ch &= 0xEF;
        }
        break;
    case 0x01: // A
        if(state)
        {   if(!(ch & 0x01))
                ch |= 0x01;
        }
        else
        {   if(ch & 0x01)
                ch &= 0xFE;
        }
        break;		
    case 0x02: // B
        if(state)
        {   if(!(ch & 0x02))
                    ch |= 0x02;
        }
        else
        {   if(ch & 0x02)
                    ch &= 0xFD;
        }
        break;		
    case 0x04: // C
        if(state)
        {   if(!(ch & 0x04))
                    ch |= 0x04;
        }
        else
        {   if(ch & 0x04)
                    ch &= 0xFB;
        }
        break;		
    case 0x08: // D
        if(state)
        {   if(!(ch & 0x08))
                    ch |= 0x08;
        }
        else
        {   if(ch & 0x08)
                    ch &= 0xF7;
        }
        break;		
    default:
        break;
    }

    *m_cPtr = ch;
}


