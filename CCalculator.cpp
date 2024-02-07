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

CConfigCalculator::CConfigCalculator(CReadFile *pfl)
{
    m_pReadFile = pfl;
}

int CConfigCalculator::GetAnzOper()
{
    return m_iAnzOper;
}

COperBase * CConfigCalculator::GetOper(int idx)
{
    return m_ptrOper[idx];
}
void CConfigCalculator::AddOper(char ch)
{
    COperBase *pOper;
    pOper = new COperBase;
    pOper->setType (ch);
    AddOperToList(pOper);
}

void CConfigCalculator::AddOperToList(COperBase *ptr)
{
    m_ptrOper[m_iAnzOper] = ptr;
    m_iAnzOper++;
    if(m_iAnzOper > MAXOPER)
        m_pReadFile->Error(16);
}

void CConfigCalculator::eval(const char *expr, CIOGroup *pIOGroup, int type)
{
    m_cPtr = expr;
    m_iAnzOper = 0;
    m_iType = type;
    m_pIOGroup = pIOGroup;
    sum();
    COperBase *pOper;
    pOper = new COperBase;
    pOper->setType (0);
    AddOperToList(pOper);
}

// wertet Integerzahl in Dezimaldarstellung aus
void CConfigCalculator::number()
{
    char text[MSGSIZE];
    char nbr[MSGSIZE];
    char event;
    char ew, ch;
    int nr, i, diff, iHelp, j;
    int iStatisticWS = 0;
    
    // Ziel einlesen
    for(i=0; i < MSGSIZE; i++)
    {
        if(isalpha(cur()))
        {
            text[i] = toupper(cur());
            m_cPtr++;
        }
        else
        {   text[i] = 0;
            break;
        }
    }
    if(i >= 20)
        m_pReadFile->Error(25);
    
    nr = 0;
    for(i=0, j=0; i < MSGSIZE; i++)
    {   
        ch = cur();
        if(isdigit(ch) || ch == '-')
        {
            nbr[i] = ch;
            m_cPtr++;
        }
        else if(ch == ',')
        {
            nbr[i] = 0;
            m_cPtr++;
            if(text[0] != 0 || i == 0)
                m_pReadFile->Error(25);
            iHelp = atoi(nbr);
            if(iHelp < 0 || iHelp > 255)
                m_pReadFile->Error(22); 
            if(j)
            {
                j *= 256;
                nr =  j * iHelp + nr;
            }
            else
            {
                nr = iHelp;
                j++;
            }
            i = -1;
        }
        else
        {   
            nbr[i] = 0;
            iHelp = atoi(nbr);
            if(j)
            {    
                if(iHelp < 0 || iHelp > 255)
                    m_pReadFile->Error(22);
                else
                {
                    j *= 256;
                    nr =  j * iHelp + nr;
                }   
            }
            else
                nr = iHelp;
            break;
        }
    }
    
    if(i >= 10)
        m_pReadFile->Error(26);

    if(strncmp(text, "FE", 2) == 0 || strncmp(text, "FA", 2) == 0 && strlen(text) == 2)
    {
        ew = 0;
        if(isalpha(cur()))
        {
            ew = toupper(cur());
            m_cPtr++;
        }
        if(ew < 'A' || ew > 'F')
            m_pReadFile->Error(15);

    }

    // Aktion einlesen
    if(isalpha(cur()))
    {
        event = tolower(cur());
        m_cPtr++;
    }
    else
        event = 0;

    if(text[0] == 0)
    {
        if(event == 'h') // z.B. : 8h21 für die Uhrzeit
        {
            for(i=0; i < 10; i++)
            {
                if(isdigit(cur()))
                {
                    nbr[i] = cur();
                    m_cPtr++;
                }
                else
                {   
                    nbr[i] = 0;
                    break;
                }
            }
            if(i >= 10)
                m_pReadFile->Error(26);

            nr = nr * 60 + atoi(nbr);
        }

        COperNumber *pOper = new COperNumber;
        pOper->setType(1);
        pOper->setOper (nr);
        AddOperToList(pOper);		
    }
    else
    {		
        if(!(event == 'a' || event == 'e' || event == 'w' || event == 0))
            m_pReadFile->Error(35);

        // Easywave Eingänge
        if(strncmp(text, "FE", 2) == 0 && strlen(text) == 2)
        {	
            if(nr <= m_pIOGroup->GetEWBoardAnz() + m_pIOGroup->GetEWUSBEingAnz())
            {   
                COper *pOper;
                char ch;

                switch(ew) {
                case 'A':
                    ch = 0x01;
                    break;
                case 'B':
                    ch = 0x02;
                    break;
                case 'C':
                    ch = 0x04;
                    break;
                case 'D':
                    ch = 0x08;
                    break;
                case 'E':
                    ch = 0x10;
                    break;
                case 'F':
                    ch = 0x20;
                    break;
                default:
                    m_pReadFile->Error(35);
                    break;
                }
                switch(event) {
                case 'e':
                    pOper = (COper *) new COperE;
                    break;
                case 'a':
                    pOper = (COper *) new COperA;
                    break;
                case 'w':
                    pOper = (COper *) new COperW;
                    break;
                default:
                    pOper = new COper;
                    break;
                }
                pOper->setType(1);
                pOper->setOper (m_pIOGroup->GetEWEingAddress(nr), 
                                m_pIOGroup->GetEWEingLastAddress(nr), ch);
                AddOperToList(pOper);
            }
            else
                m_pReadFile->Error(19);

        }
        // Eingänge
        else if(strncmp(text, "E", 1) == 0 && strlen(text) == 1)
        {
            if(nr <= m_pIOGroup->GetEingAnz())
            {   
                char ch = 0x01;
                COper *pOper;

                ch <<= (nr-1)%8;
                switch(event) {
                case 'e':
                    pOper = (COper *) new COperE;
                    break;
                case 'a':
                    pOper = (COper *) new COperA;
                    break;
                case 'w':
                    pOper = (COper *) new COperW;
                    break;
                default:
                    pOper = new COper;
                    break;
                }
                pOper->setType(1);
                pOper->setOper (m_pIOGroup->GetEingAddress(nr), 
                                    m_pIOGroup->GetEingLastAddress(nr), ch);
                AddOperToList(pOper);
            }
            else
                m_pReadFile->Error(20);
        }
        // Ausgänge
        else if(strncmp(text, "A", 1) == 0 && strlen(text) == 1)
        {
            if(nr <= m_pIOGroup->GetAusgAnz())
            {   
                char ch = 0x01;
                COper *pOper;

                ch <<= (nr-1)%8;
                switch(event) {
                case 'e':
                    pOper = (COper *) new COperE;
                    break;
                case 'a':
                    pOper = (COper *) new COperA;
                    break;
                case 'w':
                    pOper = (COper *) new COperW;
                    break;
                default:
                    pOper = new COper;
                    break;
                }
                pOper->setType(1);
                pOper->setOper (m_pIOGroup->GetAusgAddress(nr), 
                                m_pIOGroup->GetAusgLastAddress(nr), ch);
                AddOperToList(pOper);
            }
            else
                m_pReadFile->Error(20);
        }
        // Merker
        else if(strncmp(text, "M", 1) == 0 && strlen(text) == 1)
        {   
            if(nr <= m_pIOGroup->GetMerkAnz())
            {   
                char ch = 0x01;
                COper *pOper;

                ch <<= (nr-1)%8;
                switch(event) {
                case 'e':
                    pOper = (COper *) new COperE;
                    break;
                case 'a':
                    pOper = (COper *) new COperA;
                    break;
                case 'w':
                    pOper = (COper *) new COperW;
                    break;
                default:
                    pOper = new COper;
                    break;
                }
                pOper->setType(1);
                pOper->setOper (m_pIOGroup->GetMerkAddress(nr), 
                                 m_pIOGroup->GetMerkLastAddress(nr), ch);
                AddOperToList(pOper);
            }
            else
                m_pReadFile->Error(27);
        }

        // Software-Eingang
        else if(strncmp(text, "S", 1) == 0 && strlen(text) == 1)
        {
            if(nr <= m_pIOGroup->GetSEingAnz())
            {   
                char ch = 0x01;
                COper *pOper;

                ch <<= (nr-1)%8;
                switch(event) {
                case 'e':
                    pOper = (COper *) new COperE;
                    break;
                case 'a':
                    pOper = (COper *) new COperA;
                    break;
                case 'w':
                    pOper = (COper *) new COperW;
                    break;
                default:
                    pOper = new COper;
                    break;
                }
                pOper->setType(1);
                pOper->setOper (m_pIOGroup->GetSEingAddress(nr), 
                                     m_pIOGroup->GetSEingLastAddress(nr), ch);
                AddOperToList(pOper);
            }
            else
                m_pReadFile->Error(27);
        }
        // Integer Merker
        else if(strncmp(text, "I", 1) == 0 && strlen(text) == 1)  
        {
            if(nr > 0 && nr <= m_pIOGroup->GetIntegerAnz())
            {   
                COperInteger *pOper;
                switch(event) {
                case 'e':
                    pOper = (COperInteger *) new COperIntegerE;
                    break;
                case 'a':
                    pOper = (COperInteger *) new COperIntegerA;
                    break;
                case 'w':
                    pOper = (COperInteger*) new COperIntegerW;
                    break;
                default:
                    pOper = new COperInteger ();
                    break;
                } 
                pOper->setType(1);              
                pOper->setOper(m_pIOGroup->GetIntegerAddress(nr));
                AddOperToList (pOper);
            }
            else
                m_pReadFile->Error(131);              
        }      
        // Tag-Nacht basierend auf Sonnenaufgang und -untergang
        else if(strncmp(text, "TN", 2) == 0 && strlen(text) == 2)
        {
            if(*m_cPtr == ':')
            {
                m_cPtr++;
                for(i=0; i < 11; i++)
                {
                    if(isalpha(cur()))
                    {
                        text[i] = toupper(cur());
                        m_cPtr++;
                    }
                    else
                    {   text[i] = 0;
                        break;
                    }
                }
                if(strncmp(text, "SA", 2) == 0 && strlen(text) == 2)
                    i = 2;
                else if(strncmp(text, "SU", 2) == 0 && strlen(text) == 2)
                    i = 3;
                else
                    m_pReadFile->Error(83);
            }
            else 
                i = 1;
                            
            COperTagNacht *pOper = new COperTagNacht;
            pOper->setType(1);
            pOper->setOper(i);
            AddOperToList(pOper);		
        }
        // Uhrzeit
        else if(strncmp(text, "UHR", 3) == 0 && strlen(text) == 3)
        {
            if(*m_cPtr++ == ':')
            {
                for(i=0; i < 20; i++)
                {
                    if(isalpha(cur()))
                    {
                        text[i] = toupper(cur());
                        m_cPtr++;
                    }
                    else
                    {   text[i] = 0;
                        break;
                    }
                }
                i = 0;
                if(strncmp(text, "UHRZEIT", 7) == 0 && strlen(text) == 7)
                    i = 1;
                else if(strncmp(text, "DATUM", 5)== 0 && strlen(text) == 5)
                    i = 2;
                else if(strlen(text) == 4)
                {
                    if(strncmp(text, "MINW", 4) == 0)
                        i = 3;
                    else if( strncmp(text, "STDW", 4) == 0)
                        i = 4;
                    else if(strncmp(text, "TAGW", 4) == 0)
                        i = 5;
                    else
                        i = 0;
                }
                if(i)
                {
                    COperUhr *pOper = new COperUhr;
                    pOper->setType(1);
                    pOper->setOper(i);
                    AddOperToList(pOper);
                }
                else
                    m_pReadFile->Error(111);
            }
            else
                m_pReadFile->Error(111);                
        }
        // Sekunden Timer
        else if(strncmp(text, "T", 1) == 0 && strlen(text) == 1)
        {	
            if(nr <= m_pIOGroup->GetSecTimerAnz ())
            {   
                COperTimer *pOper;
                switch(event) {
                case 'e':
                    pOper = (COperTimer *) new COperTimerE;
                    break;
                case 'a':
                    pOper = (COperTimer *) new COperTimerA;
                    break;
                default:
                    pOper = new COperTimer ();
                    break;
                }
                pOper->setType(1);
                pOper->setOper (m_pIOGroup->GetSecTimerAddress(nr));
                AddOperToList(pOper);
            }
            else
                m_pReadFile->Error(21);
        }
        // Heizungskörper
        else if(strncmp(text, "HK", 2) == 0 && strlen(text) == 2)
        {
            if(nr <= m_pIOGroup->GetHKAnz())
            {
                COperHK *pOper = new COperHK;
                pOper->setType(1);
                pOper->setOper(m_pIOGroup->GetHKAddress(nr));
                AddOperToList (pOper);
            }
            else
                m_pReadFile->Error(68);
        }
        else if(strncmp(text, "WS", 2) == 0 && strlen(text) == 2)
        {
            if(m_pIOGroup->m_pWStation != NULL)
            {
                if(*m_cPtr++ == ':')
                {
                    for(i=0; i < 25; i++)
                    {
                        if(isalpha(cur()))
                        {
                            text[i] = toupper(cur());
                            m_cPtr++;
                        }
                        else
                        {   text[i] = 0;
                            break;
                        }
                    }
                    if(strncmp(text, "TEMPAKT", 7) == 0 && strlen(text) == 7)
                        i = TEMPAKT;
                    else if(strncmp(text, "TEMPMIN", 7) == 0 && strlen(text) == 7)
                        i = TEMPMIN; 
                    else if(strncmp(text, "TEMPMAX", 7) == 0 && strlen(text) == 7)
                        i = TEMPMAX;                     
                    else if(strncmp(text, "TAUPUNKT", 8)== 0 && strlen(text) == 8)
                        i = TAUPUNKT;
                    else if(strncmp(text, "FEUCHTE", 7) == 0 && strlen(text) == 7)
                        i = FEUCHTE;
                    else if(strncmp(text, "LUFTDRUCK", 9) == 0 && strlen(text) == 9)
                        i = LUFTDRUCK;
                    else if(strncmp(text, "WINDGESCHW", 10) == 0 && strlen(text) == 10)
                        i = WINDGESCHW;
                    else if(strncmp(text, "WINDGESCHWMAX", 13) == 0 && strlen(text) == 13)
                        i = WINDGESCHWMAX;
                    else if(strncmp(text, "WINDGESCHWMAXZEHN", 17) == 0 && strlen(text) == 17)
                        i = WINDGESCHWMAX10;
                    else if(strncmp(text, "WINDGESCHWMAXDREIZIG", 20) == 0 && strlen(text) == 20)
                        i = WINDGESCHWMAX30;
                    else if(strncmp(text, "WINDGESCHWMAXSECHZIG", 20) == 0 && strlen(text) == 20)
                        i = WINDGESCHWMAX60;                    
                    else if(strncmp(text, "WINDRICHT", 9) == 0 && strlen(text) == 9)
                        i = WINDRICHT;
                    else if(strncmp(text, "NSMENGE", 7) == 0 && strlen(text) == 7)
                        i = NSMENGE; 
                    else if(strncmp(text, "NSART", 5) == 0 && strlen(text) == 5)
                        i = NSART;  
                    else if(strncmp(text, "NSINTENS", 8) == 0 && strlen(text) == 8)
                        i = NSINTENS;
                    else if(strncmp(text, "AZIMUT", 7) == 0 && strlen(text) == 7)
                        i = AZIMUT;
                    else if(strncmp(text, "ELEVATION", 9) == 0 && strlen(text) == 9)
                        i = ELEVATION;  
                    else if(strncmp(text, "UVINDEX", 7) == 0 && strlen(text) == 7)
                        i = UVINDEX;
                    else if(strncmp(text, "HELLIGKEIT", 10) == 0 && strlen(text) == 10)
                        i = HELLIGKEIT;
                    else 
                        i = 0;
                    if(i)
                    {
                        COperWStation *pOper = new COperWStation;
                        pOper->setType(1);
                        pOper->setOper(m_pIOGroup->m_pWStation, i);
                        AddOperToList(pOper);
                    }
                    else
                        m_pReadFile->Error(82);
                }
                else
                    m_pReadFile->Error(82);
            }
            else
                m_pReadFile->Error(81);
        }
        else if(strncmp(text, "H", 1) == 0 && strlen(text) == 1) // HUE
        {
            if(nr > 0 && nr <= m_pIOGroup->m_pHue->GetAnz())
            {   
                COperHue *pOper = new COperHue;
                pOper->setType(1);
                pOper->setOper(m_pIOGroup->m_pHue->GetAddress(nr));
                AddOperToList (pOper);
            }
            else
                m_pReadFile->Error(20);            
        }
        else if(strncmp(text, "ALARM", 5) == 0 && strlen(text) == 5)   
        {
            if(m_pIOGroup->m_pAlarm != NULL)
            {
                if(*m_cPtr++ == ':')
                {
                    for(i=0; i < 25; i++)
                    {
                        if(isalpha(cur()))
                        {
                            text[i] = toupper(cur());
                            m_cPtr++;
                        }
                        else
                        {   text[i] = 0;
                            break;
                        }
                    }
                    if(strncmp(text, "STATUS", 6) == 0 && strlen(text) == 6) 
                        i = 1;
                    else if(strncmp(text, "VORAKTIVIERT", 12) == 0 && strlen(text) == 12)
                        i = 2;
                    else if(strncmp(text, "AKTIVIERT", 9) == 0 && strlen(text) == 9)
                        i = 3;                    
                    else if(strncmp(text, "VORALARMIERT", 12) == 0 && strlen(text) == 12)
                        i = 4;
                    else if(strncmp(text, "ALARMIERT", 9) == 0 && strlen(text) == 9)
                        i = 5;
                    else if(strncmp(text, "ABGESCHALTET", 12) == 0 && strlen(text) == 12)
                        i = 6;
                    else if(strncmp(text, "AKTIV", 5) == 0 && strlen(text) == 5)
                        i = 7;
                    else if(strncmp(text, "AUSLOESER", 9) == 0 && strlen(text) == 9)
                        i = 8;
                    else if(strncmp(text, "NR", 2) == 0 && strlen(text) == 2)
                        i = 9;
                    else
                       m_pReadFile->Error(100);   

                    COperAlarm *pOper = new COperAlarm;
                    pOper->setType(1);
                    pOper->SetOper(m_pIOGroup->m_pAlarm, i);
                    AddOperToList(pOper);
                   
                }
                else
                    m_pReadFile->Error(99);
            }
            else
                m_pReadFile->Error(99);                        
        }
        else if(strncmp(text, "ZAEHLER", 7) == 0 && strlen(text) == 7)
        {
            if(nr < 1 || nr > m_pIOGroup->GetAnzZaehler())
                m_pReadFile->Error(65); // unknown counter
            int iInterval, iDiff;
            LesIntervalDiff(&iInterval, &iDiff);
            COperZaehler *pOper = new COperZaehler;
            pOper->setType(1);
            pOper->SetOper(m_pIOGroup->GetZaehler(nr), iInterval, iDiff);
            AddOperToList(pOper);
        }
        else if(strncmp(text, "SMS", 3) == 0 && strlen(text) == 3)
        {
            if(nr < 1 || nr > m_pIOGroup->m_pGsm->GetAnzSMSEmpf())
                m_pReadFile->Error(65); // unknown counter            
            COperSMS *pOper = new COperSMS;
            pOper->setType(1);
            pOper->SetOper(m_pIOGroup->m_pGsm->GetAddressSMSEmpf(nr));
            AddOperToList(pOper);
        }
        else if(strncmp(text, "TS", 2) == 0  && strlen(text) == 2) // Temperatursensor
        {   
            i = m_pIOGroup->GetAnzSensor();
            if(nr < 1 || nr > i)
                m_pReadFile->Error(70);            
            if(*m_cPtr++ == ':')
            {
                for(i=0; i < 25; i++)
                {
                    if(isalpha(cur()))
                    {
                        text[i] = toupper(cur());
                        m_cPtr++;
                    }
                    else
                    {   text[i] = 0;
                        break;
                    }
                }
                if(strncmp(text, "HUM", 3) == 0 && strlen(text) == 3) 
                {     
                    COperHUM *pOper = new COperHUM;
                    pOper->setType(1);
                    pOper->setOper(m_pIOGroup->GetSensorAddress(nr));
                    AddOperToList(pOper);    
                }               
                else if(strncmp(text, "TEMP", 4) == 0 && strlen(text) == 4)
                {
                    COperTEMP *pOper = new COperTEMP;
                    pOper->setType(1);
                    pOper->setOper(m_pIOGroup->GetSensorAddress(nr));
                    AddOperToList(pOper);
                }
                else if(strncmp(text, "QUAL", 4) == 0 && strlen(text) == 4)
                {
                    COperQUAL *pOper = new COperQUAL;
                    pOper->setType(1);
                    pOper->setOper(m_pIOGroup->GetSensorAddress(nr));
                    AddOperToList(pOper);
                }
                else
                    m_pReadFile->Error(107);  
            }
        }
        else if(strncmp(text, "AC", 2) == 0 && strlen(text) == 2)
        {
            if(m_pIOGroup->m_pAlarmClock != NULL)
            {
                if(*m_cPtr++ == ':')
                {
                    for(i=0; i < 25; i++)
                    {
                        if(isalpha(cur()))
                        {
                            text[i] = toupper(cur());
                            m_cPtr++;
                        }
                        else
                        {   text[i] = 0;
                            break;
                        }
                    }
                    if(strncmp(text, "ACTIVE", 6) == 0 && strlen(text) == 6) 
                        i = 1;
                    else if(strncmp(text, "STEP", 4) == 0 && strlen(text) == 4) 
                        i = 2;
                    else if(strncmp(text, "METHOD", 6) == 0 && strlen(text) == 6) 
                        i = 3;
                    else if(strncmp(text, "CHANGE", 6) == 0 && strlen(text) == 6)
                        i = 4;
                    else
                        m_pReadFile->Error(129);                        
                    COperAC *pOper = new COperAC;
                    pOper->setType(1);
                    pOper->setOper(m_pIOGroup->m_pAlarmClock, i);
                    AddOperToList(pOper);
                }
                else
                    m_pReadFile->Error(129);               
            }
            else
                m_pReadFile->Error(129);
        }
        else if(strncmp(text, "TEMPMIN", 7) == 0 && strlen(text) == 7)
        {
            iStatisticWS = TEMPMIN;
            i = 10;
        }
        else if(strncmp(text, "TEMPMAX", 7) == 0 && strlen(text) == 7)
        {
            iStatisticWS = TEMPMAX;
            i = 10;
        }
        else if(strncmp(text, "NSMENGE", 7) == 0 && strlen(text) == 7)
        {
            iStatisticWS = NSMENGE;
            i = 100;
        }
        else if(strncmp(text, "WINDGESCHWMAX", 13) == 0 && strlen(text) == 13)
        {
            iStatisticWS = WINDGESCHWMAX;
            i = 10;
        }
        else
            m_pReadFile->Error(17);
        if(iStatisticWS)
        {
            int iInterval, iDiff;
            LesIntervalDiff(&iInterval, &iDiff);    
            COperWStatistic *pOper = new COperWStatistic;
            pOper->setType(1);
            pOper->setOper(m_pIOGroup->m_pWStation, iStatisticWS, iInterval, iDiff, i);
            AddOperToList(pOper);
        }
    }
}

void CConfigCalculator::LesIntervalDiff(int *iInterval, int *iDiff)
{
    int i;
    char text[MSGSIZE];
    char nbr[MSGSIZE];
    char ch;
    
    if(*m_cPtr++ == ':')
    {
        for(i=0; i < 20; i++)
        {
            if(isalpha(cur()))
            {
                text[i] = toupper(cur());
                m_cPtr++;
            }
            else
            {   text[i] = 0;
                break;
            }
        } 
        *iDiff = 0;
        if(cur() == '#')
        {
            m_cPtr++;
            for(i=0; i < 10; i++)
            {   
                ch = cur();
                if(isdigit(ch))
                {
                    nbr[i] = ch;
                    m_cPtr++;
                }
                else
                {   
                    nbr[i] = 0;
                    break;
                }
            }  
            *iDiff = atoi(nbr);
            if(*iDiff < 0)
                m_pReadFile->Error(110);
        }

        if(strncmp(text, "STAND", 5) == 0 && strlen(text) == 5)
            *iInterval = 0;
        else if(strncmp(text, "TAG", 3) == 0 && strlen(text) == 3)
            *iInterval = 1;
        else if(strncmp(text, "WOCHE", 8) == 0 && strlen(text) == 8)
            *iInterval = 2;
        else if(strncmp(text, "MONAT", 5) == 0 && strlen(text) == 5)
            *iInterval = 3;
        else if(strncmp(text, "JAHR", 4) == 0 && strlen(text) == 4)
            *iInterval = 4;
        else
            m_pReadFile->Error(107);
    }
    else
        m_pReadFile->Error(110);
}
void CConfigCalculator::factor()
{
    switch(cur()) {
    case '(':
        AddOper(cur());
        m_cPtr++;
        sum();
        if(cur() == ')')
        {	AddOper(cur());
                m_cPtr++;
        }
        else
                m_pReadFile->Error(14);
        break;
    case '!': // Not
        AddOper(cur());
        m_cPtr++;
        factor();
        break;
    default:
        number();
        break;
    }
}


void CConfigCalculator::sum()
{
    char iCur;

    prod();
    for(;;)
    {
        iCur = cur();
        if(iCur == '|' || iCur == '+' || iCur == '-') 
        {
            AddOper(iCur);
            m_cPtr++;
            prod(); 
        }
        else
        {
            if(iCur == 0 || iCur == ')')
                break;
            else
                m_pReadFile->Error(86);
        }   
    }
}

void CConfigCalculator::prod()
{
    char iCur;

    factor();
    for(;;)
    {
        iCur = cur();

        if(iCur=='&' || iCur=='^' || iCur=='>' || iCur=='<' || iCur=='=' || iCur=='#' || iCur=='*' || iCur=='/') 
        {
            AddOper(iCur);
            m_cPtr++;
            factor(); 
        }
        else
            break;
    }
}

//
// CCalculator
//
int CCalculator::eval(COperBase **pOper)
{
    m_pOper = pOper;
    m_iType = (*m_pOper)->getType(); 
    return sum();
}

int CCalculator::cur()
{
    return m_iType;
}

// liest nächstes Zeichen aus Eingabe
void CCalculator::next()
{
    m_pOper++;
    m_iType = (*m_pOper)->getType(); 
}

// wertet Integerzahl in Dezimaldarstellung aus
int CCalculator::number()
{
	
    return (*m_pOper)->resultInt();
}

int CCalculator::factor()
{
    int res;
    switch(cur()) {
    case '(':
        next();
        res = sum();
        if(cur() == ')')
            next();
        break;
    case '!': // Not
        next();
        res = factor();
        if(res)
            res = 0;
        else
            res = 1;
        break;
    default:
        res = number();
        next();
        break;
    }
    return res;
}

int CCalculator::sum()
{
    int res = prod();
    bool bOK;
    
    for(bOK = true; bOK; )
    {
        char ch = cur();
        switch(ch) 
        {
            case '|':
                next();
                res |= prod(); 
                break;
            case '+':
                next();
                res += prod();
                break;
            case '-':
                next();
                res -= prod();
                break;
            default:
                bOK = false;
                break;
        }
    }

    return res;
}

int CCalculator::prod()
{   
    bool bOK;
    int res = factor();
    for(bOK=true; bOK;)
    {
        switch(cur()) {
        case '&': 
            next();
            res &= factor(); 
            break;
        case '^':
            next();
            res ^= factor();
            break;
        case '>':
            next();
            res = res > factor();
            break;
        case '<':
            next();				
            res = res < factor();
            break;
        case '=':
            next();				
            res = res == factor();
            break;
        case '#':
            next();
            res = res != factor();	
            break;
        case '*':
            next();
            res *= factor();
            break;
        case '/':
            next();
            res /= factor();
            break;
        default:
            bOK = false;
            break;
        }
    }
    return res;
}

