/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CHistory.cpp
 * Author: josefengel
 * 
 * Created on September 25, 2019, 10:10 AM
 */

#include "ProWo.h"

CHistoryElement::CHistoryElement()
{
    m_sTyp = 0;
    m_time = 0;
}

CHistoryElement::~CHistoryElement()
{

}

//
//
CHistory::CHistory(int AnzAusg, int AnzEWAusg, int iAnzHue) 
{
    int i, j;

    m_iAnzAusg = AnzAusg;
    i = m_iAnzAusg / PORTANZAHLCHANNEL;
    m_pAusgEnabled = new char[i];
    for(j=0; j < i; j++)
        *(m_pAusgEnabled + j) = 0;
    m_iAnzEWAusg = AnzEWAusg;
    if(m_iAnzEWAusg)
    {
        m_pEWAusgEnabled = new bool[AnzEWAusg];
        for(j=0; j < AnzEWAusg; j++)
            *(m_pEWAusgEnabled + j) = false;
    }
    else
        m_pEWAusgEnabled = NULL;
    m_iAnzHue = iAnzHue;
    if(iAnzHue)
    {
        m_pHueEnabled = new bool[iAnzHue];
        for(j=0; j < iAnzHue; j++)
            *(m_pHueEnabled + j) = false;
    }
    else
        m_pHueEnabled = NULL;
    
    pthread_mutex_init(&m_mutexHistoryWriteFifo, NULL);
    pthread_mutex_init(&m_mutexHistoryReadFifo, NULL);    
    m_iTag = 0;
    m_lFilePosAfterDiff = 0l;
    m_iDiffTage = 0;
    m_iDiffTageExt = -1;
    m_strError.clear();
}

CHistory::~CHistory() 
{
    delete [] m_pAusgEnabled;
    if(m_pEWAusgEnabled != NULL)
    {
        delete [] m_pEWAusgEnabled;
        m_pEWAusgEnabled = NULL;
    }
    if(m_pHueEnabled != NULL)
    {
        delete [] m_pHueEnabled;
        m_pHueEnabled = NULL;
    }
    pthread_mutex_destroy(&m_mutexHistoryWriteFifo);
    pthread_mutex_destroy(&m_mutexHistoryReadFifo);    
}

void CHistory::SetFilePosAfterDiff(long lPos)
{
    m_lFilePosAfterDiff = lPos;
}

bool CHistory::InitAddAusg(int iNr)
{
    char ch = 0x01;
    bool bRet;
    
    if(iNr > 0 && iNr <= m_iAnzAusg && iNr <= MAXAUSGCHANNEL)
    {
        bRet = true;
        ch <<= (iNr-1)%PORTANZAHLCHANNEL;
        *(m_pAusgEnabled + ((iNr-1)/PORTANZAHLCHANNEL)) |= ch;
    }
    else
        bRet = false;
    
    return bRet;
}

bool CHistory::InitAddEWAusg(int iNr)
{
    bool bRet;
    
    if(iNr >= 0 && iNr <= m_iAnzEWAusg)
    {
        m_pEWAusgEnabled[iNr-1] = true;
        bRet = true;
    }
    else
        bRet = false;
    
    return bRet;
}

bool CHistory::InitAddHue(int iNr)
{
    bool bRet;
    if(iNr >= 0 && iNr <= m_iAnzHue)
    {
        m_pHueEnabled[iNr-1] = true;
        bRet = true;
    }
    else
        bRet = false;
    return bRet;
}
//
// Hardware Ausgänge
// Typ = 1
void CHistory::Add(char* ptr)
{
    int i;
    CHistoryElement HistoryElement;

    HistoryElement.m_sTyp = 1;
    HistoryElement.m_time = m_pUhr->getUhrzeit();
    for(i=0; i < m_iAnzAusg / PORTANZAHLCHANNEL; i++)
        HistoryElement.m_chAusg[i] = *(ptr + i);
    pthread_mutex_lock(&m_mutexHistoryWriteFifo);
    m_HistoryWriteFifo.push(HistoryElement);
    pthread_mutex_unlock(&m_mutexHistoryWriteFifo);
}

// EasyWave Ausgang
// Typ = 2
void CHistory::Add(int iSwitch)
{
    CHistoryElement HistoryElement;
    
    HistoryElement.m_sTyp = 2;
    HistoryElement.m_time = m_pUhr->getUhrzeit();
    HistoryElement.m_iSwitch = iSwitch;
    pthread_mutex_lock(&m_mutexHistoryWriteFifo);
    m_HistoryWriteFifo.push(HistoryElement);
    pthread_mutex_unlock(&m_mutexHistoryWriteFifo);
}

// Hue Ausgang
// Typ = 3
void CHistory::Add(CHueProperty *pHueProperty)
{
    CHistoryElement HistoryElement;
    
    HistoryElement.m_sTyp = 3;
    HistoryElement.m_time = m_pUhr->getUhrzeit();
    HistoryElement.m_HueProperty = *pHueProperty;
    pthread_mutex_lock(&m_mutexHistoryWriteFifo);
    m_HistoryWriteFifo.push(HistoryElement);
    pthread_mutex_unlock(&m_mutexHistoryWriteFifo);
}

void CHistory::ControlFiles(char * pcAusgHistory, queue<struct EWAusgEntity> *pEWAusgFifo, CHue * pHue)
{
    CReadFile rf;
    int i, iRet, iTag;
    time_t iSec;
    CHistoryElement HistoryElement;    
    string str;
    bool bMust = false;
    
    iSec = m_pUhr->getUhrzeit();
    iTag = iSec / 86400;
    
   
    pthread_mutex_lock(&m_mutexHistoryWriteFifo);
    if(!m_HistoryWriteFifo.empty())
    {
        // fifo in aktuelle Tagesdatei schreiben
        HistoryElement = m_HistoryWriteFifo.front();
        m_HistoryWriteFifo.pop();
        pthread_mutex_unlock(&m_mutexHistoryWriteFifo);
        if(rf.OpenAppendBinary(pProgramPath, 11, iTag))
        {
            rf.WriteLine((char *)&HistoryElement, sizeof(CHistoryElement));
            rf.Close();
        }
        else
            m_strError = rf.GetError();
    }
    else
        pthread_mutex_unlock(&m_mutexHistoryWriteFifo);
    
    pthread_mutex_lock(&ext_mutexNodejs);   
    i = m_iDiffTageExt;
    pthread_mutex_unlock(&ext_mutexNodejs);  
    if(m_iTag != iTag)
    {
        // der Tag hat geändert
        m_lReadFilePos = 0l;
        m_iTag = iTag;        
    }
    if(i != m_iDiffTage)
    {
        // Lichtprogramm ist aktiviert oder ausgeschaltet worden
        m_iDiffTage = i;          
        m_lReadFilePos = 0l;
        if(m_iDiffTage)
        {
            // lichtprogramm ist eingeschaltet worden
            int * pEWAusg = NULL;
            if(m_iAnzEWAusg)
                pEWAusg = new int[m_iAnzEWAusg*2]; // mal zwei da pro channel E und F !!
            CHueProperty * pHueProperty = NULL;
            if(m_iAnzHue)
                pHueProperty = new CHueProperty[m_iAnzHue];
            char * pAusg = NULL;
            if(m_iAnzAusg)
                pAusg = new char[m_iAnzAusg / PORTANZAHLCHANNEL];
            int channel, button;
            time_t time = m_pUhr->getUhrzeit() - m_iDiffTage * 86400;
            for(i = 0; i < m_iAnzEWAusg*2; i++)
                pEWAusg[i] = -1;
            for(i=0; i < m_iAnzAusg / PORTANZAHLCHANNEL; i++)
                pAusg[i] = 0;
            
            if(rf.OpenRead(pProgramPath, 11, iTag - m_iDiffTage) && rf.SetFilePos(m_lReadFilePos))
            {
                while(true)
                {
                    m_lReadFilePos = rf.GetFilePos();
                    if(rf.ReadBinary((char *)&HistoryElement, sizeof(CHistoryElement)))
                    {
                        if(HistoryElement.m_time > time)
                            break;
                        switch(HistoryElement.m_sTyp) {
                            case 1: // Hardware-Ausgänge
                                for(i=0; i < m_iAnzAusg / PORTANZAHLCHANNEL; i++)
                                    pAusg[i] = HistoryElement.m_chAusg[i];                                
                                break;
                            case 2: // EasyWave
                                channel = HistoryElement.m_iSwitch/256 - 1;
                                button = HistoryElement.m_iSwitch%256;
                                if(m_pEWAusgEnabled[channel])
                                {
                                    channel *= 2;
                                    if(button > 1)
                                    {
                                        button -= 2;
                                        channel++;
                                    }
                                    if(button == 0)
                                        pEWAusg[channel] = 1;
                                    else
                                        pEWAusg[channel] = 0;
                                }
                                break;
                            case 3: // Hue
                                if(m_pHueEnabled[HistoryElement.m_HueProperty.m_iNr-1])                                
                                {
                                    pHueProperty[HistoryElement.m_HueProperty.m_iNr-1] =
                                            HistoryElement.m_HueProperty;
                                }
                                break;
                            default:
                                break;
                        }
                    }
                    else
                        break;
                }
                // Ausgänge in die Historyliste eintragen
                pthread_mutex_lock(&ext_mutexNodejs); 
                for(i=0; i < m_iAnzAusg / PORTANZAHLCHANNEL; i++)
                    pcAusgHistory[i] = pAusg[i];                 
                pthread_mutex_unlock(&ext_mutexNodejs);  
                // EasyWave-Ausgänge ein- oder ausschalten 
                for(i=0; i < m_iAnzEWAusg*2; i++)
                {
                    if(pEWAusg[i] != -1)
                    {
                        channel = i / 2 + 1;
                        button = (i % 2) * 2;
                        if(!pEWAusg[i])
                            button++;
                        pthread_mutex_lock(&ext_mutexNodejs);
                        struct EWAusgEntity EWAusg;
                        EWAusg.iSource = 2;
                        EWAusg.iNr = channel * 256 + button;
                        pEWAusgFifo->push(EWAusg);
                        pthread_mutex_unlock(&ext_mutexNodejs);                        
                    }
                }    
                // HUE-Ausgänge ein- und ausschalten
                for(i=0; i < m_iAnzHue; i++)
                {
                   if(m_pHueEnabled[i] && pHueProperty[i].m_iNr) 
                   {
                       pHueProperty[i].m_iSource = 2;
                       pHue->InsertFifo(&pHueProperty[i]);
                   }
                }
                rf.Close();
            }
            else
            {
                m_lReadFilePos = -1l;
                m_strError = "Aufzeichnung von vor " + to_string(m_iDiffTage) + " existiert nicht!";
            }
            
            if(pAusg != NULL)
                delete [] pAusg;
            if(pEWAusg != NULL)
                delete [] pEWAusg;
            if(pHueProperty != NULL)
                delete [] pHueProperty;
        }
        pthread_mutex_lock(&m_mutexHistoryReadFifo);
        m_HistoryReadFifo = {};
        m_HistoryElement.m_time = 0;
        m_HistoryElement.m_sTyp = 0;
        pthread_mutex_unlock(&m_mutexHistoryReadFifo);  
    }
    
    if(m_iDiffTage)
    {
        pthread_mutex_lock(&m_mutexHistoryReadFifo);
        i = m_HistoryReadFifo.size();
        pthread_mutex_unlock(&m_mutexHistoryReadFifo);
        if(i < 3 && m_lReadFilePos >= 0)
        {
            if(rf.OpenRead(pProgramPath, 11, iTag - m_iDiffTage) && rf.SetFilePos(m_lReadFilePos))
            {
                for(; i < 10; i++)
                {
                    if(rf.ReadBinary((char *)&HistoryElement, sizeof(CHistoryElement)))
                    {
                        pthread_mutex_lock(&m_mutexHistoryReadFifo);
                        m_HistoryReadFifo.push(HistoryElement);
                        pthread_mutex_unlock(&m_mutexHistoryReadFifo);                        
                    }
                    else
                    {
                        m_lReadFilePos = -1l;
                        break;
                    }
                }
                if(i == 10)
                    m_lReadFilePos = rf.GetFilePos();
                rf.Close();
            }
            else
            {
                m_lReadFilePos = -1l;
                m_strError = "Aufzeichnung von vor " + to_string(m_iDiffTage) + " existiert nicht!";
            }
        }
    }    
}

CHistoryElement CHistory::Control()
{
    int i;
    time_t time;
    CHistoryElement HistoryElement;
    
    if(m_iDiffTage)
    {
        time = m_pUhr->getUhrzeit();
        pthread_mutex_lock(&m_mutexHistoryReadFifo);
        if(time >= m_HistoryElement.m_time + (time_t)(m_iDiffTage) * 86400)
        {
            HistoryElement = m_HistoryElement;
            if(!m_HistoryReadFifo.empty())
            {
                m_HistoryElement = m_HistoryReadFifo.front();
                m_HistoryReadFifo.pop();
            }
            else
                HistoryElement.m_sTyp = 0;

            switch(HistoryElement.m_sTyp)
            {
                case 1: // Ausgänge
                    for(i=0; i < m_iAnzAusg / PORTANZAHLCHANNEL; i++)
                        HistoryElement.m_chAusg[i] &= m_pAusgEnabled[i];
                    break;
                case 2: // EasyWave Ausgänge
                    if(!m_pEWAusgEnabled[HistoryElement.m_iSwitch/256 - 1])
                        HistoryElement.m_sTyp = 0;
                    break;
                case 3: // HUE Ausgänge
                    if(!m_pHueEnabled[HistoryElement.m_HueProperty.m_iNr-1])
                        HistoryElement.m_sTyp = 0;
                    HistoryElement.m_HueProperty.m_iSource = 2;
                    break;
                default:
                    HistoryElement.m_sTyp = 0;
                    break;
            }
        }
        pthread_mutex_unlock(&m_mutexHistoryReadFifo); 
    }

    return HistoryElement;
}

bool CHistory::SetDiffTage(int iDiff)
{
    CReadFile *pFile;
    string str;

    bool bRet = true;
    if(iDiff >= 0)
    {
        if(m_iDiffTageExt >= 0) // beim Aufstarten = -1
        {
            // die Anzahltage vom Lichtprogramm werden ans Ende der ProWo.history
            // geschrieben. Gehen dadurch nicht bei einem Stromausfall verloren!
            pFile = new CReadFile;
            pFile->OpenWriteExisting(pProgramPath, 10, 0);
            pFile->SetFilePos(m_lFilePosAfterDiff);
            str = "DIFF:" + to_string(iDiff);
            pFile->WriteLine(str.c_str());
            pFile->Close();
            delete pFile;
        }
        m_iDiffTageExt = iDiff;
        m_strError.clear();
    }
    else
    {
        m_strError = "Anzahl Tage < 0 (" + to_string(iDiff) + ")";
        bRet = false;
    }
    return bRet;
}

int CHistory::GetDiffTage()
{
    return m_iDiffTage;
}

void CHistory::SetInvAusgHistoryEnabled(char *pPtr)
{
    int i;
    
    for(i=0; i < m_iAnzAusg / PORTANZAHLCHANNEL; i++ )
        *(pPtr + i) = ~m_pAusgEnabled[i];
}

string CHistory::GetError()
{
    return m_strError;
}

bool CHistory::IsFAEnabled(int iChannel)
{
    bool bRet = true;
    
    if(m_iDiffTage)
        bRet = *(m_pEWAusgEnabled + (iChannel-1));
    
    return bRet;
}

bool CHistory::IsHueEnabled(int iNr)
{
    bool bRet = true;
    
    if(m_iDiffTage)
        bRet = *(m_pHueEnabled + (iNr - 1));
    
    return bRet;
}