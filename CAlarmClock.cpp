/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CAlarmClock.cpp
 * Author: josefengel
 * 
 * Created on January 16, 2022, 11:31 AM
 */

#include "ProWo.h"

CAlarmClockEntity::CAlarmClockEntity()
{
    m_bActivated = false;
    m_strName.clear();
    m_bSnooze = false;
    m_sRepeat = 0;
    m_iTimeMin = 0;
    m_iMethod = 0;
}

CAlarmClock::CAlarmClock() {

    m_iNrClockAktiv = 0;
    m_iLastTimeMin = 0;
    m_iStep = 0;
    m_iSnooze = 0;
    m_iChange = 0;
    LesDatei();
}

void CAlarmClock::SetParam(int iNr, CAlarmClockEntity *palarmclockEntity)
{
    bool bEnd = true;
    int iTime = palarmclockEntity->m_iTimeMin;
    CAlarmClockEntity *pnewalarmclockEntity = new CAlarmClockEntity;
    vector<CAlarmClockEntity *>::iterator it;

    DeleteParam(iNr);
    *pnewalarmclockEntity = *palarmclockEntity;  
    it = m_vectAlarmClockEntity.begin();
    while(it != m_vectAlarmClockEntity.end())
    {
        if(iTime > (*(*it)).m_iTimeMin )
            it++;
        else
        {
            m_vectAlarmClockEntity.insert(it, pnewalarmclockEntity);
            bEnd = false;
            break;
        }
    }
    if(bEnd)
         m_vectAlarmClockEntity.push_back(pnewalarmclockEntity);
}

void CAlarmClock::DeleteParam(int iNr)
{
    if(iNr > 0 && iNr <= m_vectAlarmClockEntity.size()) // iNr beinhaltet die alte Uhrzeit 
    {
        delete m_vectAlarmClockEntity[iNr-1];
        m_vectAlarmClockEntity[iNr-1] = NULL;
        m_vectAlarmClockEntity.erase(m_vectAlarmClockEntity.begin() + iNr - 1);
    }
}
void CAlarmClock::SetActivated(int iIdx, bool bActivated)
{
    if(iIdx >= 0 && iIdx < m_vectAlarmClockEntity.size())
        m_vectAlarmClockEntity.at(iIdx)->m_bActivated = bActivated;   
}

int CAlarmClock::GetAnzAlarm()
{
    return m_vectAlarmClockEntity.size();
}

int CAlarmClock::GetAnzMethod()
{
    return m_vectMethod.size();
}
CAlarmClock::~CAlarmClock() 
{
    int i, iAnz;

    SchreibDatei();
    iAnz = m_vectAlarmClockEntity.size();
    for(i=0; i < iAnz; i++)
        delete m_vectAlarmClockEntity[i];
}

void CAlarmClock::SetState(int iState)
{
    switch(iState) {
        case 0: // Wecker ausschalten
            m_iNrClockAktiv = 0;
            m_iStep = 0;
            m_iChange = 1;
            break;
        case 1: // Schlummern einschalten
            m_iSnooze = 9 - m_iStep; // 9 Minuten - bereits abgelaufene Zeit
            if(m_iSnooze < 1)
                m_iSnooze = 1;        
            m_iStep = 0;
            m_iChange = 1;
            break;
        default:
            break;
    }
}

void CAlarmClock::LesDatei()
{
    CReadFile *pReadFile;
    CAlarmClockEntity *pAlarmClockEntity;
    int i, iTimeMin, iTyp = 0, iMethod;
    string str;
    short sRepeat;
    bool bSnooze, bActivated, bSeparatur;

    pReadFile = new CReadFile;
    // JEN 01.09.23
    // es wird nichts eingelesen wenn die Datei ProWo.alarmclock nicht existiert
    if(pReadFile->OpenRead (pProgramPath, 15, 0, 1))
    {
		while(pReadFile->ReadLine())
		{
            str = pReadFile->ReadText('\n');
            if(str.compare("METHOD") == 0)
            {
                iTyp = 1;
                continue;
            }
            if(str.compare("ALARM") == 0)
            {
                iTyp = 2;
                continue;
            }
            pReadFile->ResetLine();
            switch(iTyp) {
            case 1: // METHOD
                str = pReadFile->ReadText();
                m_vectMethod.push_back(str);
                bSeparatur = true;
                break;
            case 2: // ALARM
                iTimeMin = pReadFile->ReadNumber(); // Uhrzeit in min
                if(iTimeMin < 0 || iTimeMin >= 1440)  {
                    pReadFile->Error(74);
                }
                bSeparatur = pReadFile->ReadSeparator();
                if(!bSeparatur)
                    break;
                i = pReadFile->ReadNumber();
                if(i)
                    bActivated = true;
                else
                    bActivated = false;
                bSeparatur = pReadFile->ReadSeparator();
                if(!bSeparatur)
                    break;                
                str= pReadFile->ReadText(';');
                bSeparatur = pReadFile->ReadSeparator();
                if(!bSeparatur)
                    break;            
                i = pReadFile->ReadNumber();
                if(i)
                    bSnooze = true;
                else
                    bSnooze = false;
                bSeparatur = pReadFile->ReadSeparator();
                if(!bSeparatur)
                    break;                
                sRepeat = pReadFile->ReadNumber();
                bSeparatur = pReadFile->ReadSeparator();
                iMethod = pReadFile->ReadNumber();
                pAlarmClockEntity = new CAlarmClockEntity();
                pAlarmClockEntity->m_bActivated = bActivated;
                pAlarmClockEntity->m_bSnooze = bSnooze;
                pAlarmClockEntity->m_sRepeat = sRepeat;
                pAlarmClockEntity->m_strName = str;
                pAlarmClockEntity->m_iTimeMin = iTimeMin;
                pAlarmClockEntity->m_iMethod = iMethod;
                m_vectAlarmClockEntity.push_back(pAlarmClockEntity);
                break;
            default:
                break;
            }
            if(!bSeparatur)
                pReadFile->Error(62);
        }
        syslog(LOG_INFO, "ProWo.alarmclock has been read");        
        pReadFile->Close();
    }
    delete pReadFile;
    
}

void CAlarmClock::SchreibDatei()
{
    CReadFile *pReadFile;
    int i, iAnz;
    string str;

    pReadFile = new CReadFile;
    if(pReadFile->OpenWrite(pProgramPath, 15))
    {
        iAnz = m_vectMethod.size();
        str = "METHOD\n";
        pReadFile->WriteLine(str.c_str());
        for(i=0; i < iAnz; i++)
        {
            str = m_vectMethod[i] + "\n";
            pReadFile->WriteLine(str.c_str());
        }
        iAnz = m_vectAlarmClockEntity.size();
        str = "ALARM\n";
        pReadFile->WriteLine(str.c_str());        
        for(i=0; i < iAnz; i++)  
        {
            str = to_string(m_vectAlarmClockEntity[i]->m_iTimeMin) + ";" + to_string(m_vectAlarmClockEntity[i]->m_bActivated) 
                    + ";\"" + m_vectAlarmClockEntity[i]->m_strName + "\";"  + to_string(m_vectAlarmClockEntity[i]->m_bSnooze) 
                    + ";" + to_string(m_vectAlarmClockEntity[i]->m_sRepeat) + ";" + to_string(m_vectAlarmClockEntity[i]->m_iMethod) + ";\n";
            pReadFile->WriteLine(str.c_str());
        }
        pReadFile->Close();
        syslog(LOG_INFO, "ProWo.alarmclock has been writen");            
    }
    delete pReadFile;

}

void CAlarmClock::Control(int iTimeMin, int iTag)
{
    int i, iWochenTag, iAnz;
    
    iTag = 0x1 << iTag;
    if(m_iLastTimeMin != iTimeMin)
    {
        if(m_iSnooze)
        {
            m_iSnooze--;
            if(m_iSnooze <= 0)
            {
                m_iChange = 1;
                m_iSnooze = 0;
                m_iStep = 1;
            }
        }
        else 
        {
            if(!m_iNrClockAktiv) // kein Wecker ist aktiv
            {
                iAnz = m_vectAlarmClockEntity.size();
                for(i=0; i < iAnz; i++)              
                {
                    if(m_vectAlarmClockEntity[i]->m_bActivated)
                    {
                        if(!m_vectAlarmClockEntity[i]->m_sRepeat || (m_vectAlarmClockEntity[i]->m_sRepeat & iTag))
                        {
                            if(m_vectAlarmClockEntity[i]->m_iTimeMin == iTimeMin)
                            {
                                m_iNrClockAktiv = i + 1;
                                m_iStep = 1;
                                m_iSnooze = 0;
                                m_iChange = 1;
                            }
                        }
                    }
                }
            }
            else
            {
                if(m_iStep > 300)
                {
                    m_iNrClockAktiv = 0;
                    m_iStep = 0;
                }
                else
                    m_iStep++;
                m_iChange = 1;
            }
        }
        m_iLastTimeMin = iTimeMin;
    }
}

string CAlarmClock::GetStringAlarm(int iIdx)
{
    string strRet, str1, str2;
    int iUhrMin;

    strRet.clear();
    if(iIdx >= 0 && iIdx < m_vectAlarmClockEntity.size())
    {
        if(m_vectAlarmClockEntity[iIdx]->m_bActivated)
            str1 = "true";
        else
            str1 = "false";
        if(m_vectAlarmClockEntity[iIdx]->m_bSnooze)
            str2 = "true";
        else
            str2 = "false";
        strRet = "\"activated\":\"" + str1
                                + "\",\"repeat\":" + to_string(m_vectAlarmClockEntity[iIdx]->m_sRepeat)
                                + ",\"time\":" + to_string(m_vectAlarmClockEntity[iIdx]->m_iTimeMin) 
                                + ",\"name\":\"" + m_vectAlarmClockEntity[iIdx]->m_strName
                                + "\",\"uhr\":\"" + strZweiStellen( m_vectAlarmClockEntity[iIdx]->m_iTimeMin / 60) 
                                + ":" + strZweiStellen(m_vectAlarmClockEntity[iIdx]->m_iTimeMin % 60) 
                                + "\",\"snooze\":\"" + str2 + "\",\"method\":"+ to_string(m_vectAlarmClockEntity[iIdx]->m_iMethod); 
    }
    else
    {   
        iUhrMin = m_pUhr->getUhrmin();
        strRet = "\"activated\":\"false\",\"repeat\":0,\"time\":" + to_string(iUhrMin) + ",\"name\":\"\","
                                + "\"uhr\":\"" + strZweiStellen( iUhrMin / 60) + ":" + strZweiStellen(iUhrMin % 60) 
                                + "\",\"snooze\":\"false\",\"method\":1";
    }                        
    return strRet;
} 

string CAlarmClock::GetStringMethod(int iIdx)
{
    string strRet;

    strRet.clear();
    if(iIdx >= 0 && iIdx < m_vectMethod.size())
        strRet = m_vectMethod[iIdx];

    return strRet;
}

int CAlarmClock::GetAktivMethod()
{
    int iRet = 0;
    if(m_iNrClockAktiv)
        iRet = m_vectAlarmClockEntity[m_iNrClockAktiv-1]->m_iMethod;

    return iRet;
}

int CAlarmClock::GetStep()
{
    int iRet = 0;
    if(m_iNrClockAktiv)
        iRet = m_iStep;

    return iRet;
}
int CAlarmClock::GetState()
{
    return m_iNrClockAktiv;
}

int CAlarmClock::GetChange()
{
    int iRet;

    iRet = m_iChange;
    m_iChange = 0;

    return iRet;
}
//
//  Abgeleitete Klasse von COperBase für den Wecker (AC - alarmclock)
//
COperAC::COperAC()
{
    m_pAlarmClock = NULL;
    m_iFct = 0;
}

int COperAC::resultInt()
{
    int ret;
    
    switch(m_iFct) {
        case 1: // ACTIVE
            ret = m_pAlarmClock->GetState();
            break;
        case 2: // STEP
            ret = m_pAlarmClock->GetStep();
            break;
        case 3: // METHOD
            ret = m_pAlarmClock->GetAktivMethod();        
            break;
        case 4: // CHANGE
            ret = m_pAlarmClock->GetChange();
            break;
        default:
            ret = 0;
            break;
    }
    return ret;
}
void COperAC::setOper(CAlarmClock *ptr, int iFct)
{
    m_pAlarmClock = ptr;
    m_iFct = iFct;
}

void CBerechneAC::init(CAlarmClock *pAlarmClock)
{
    m_pAlarmClock = pAlarmClock;
}
    
void CBerechneAC::SetState(int state)
{
    m_pAlarmClock->SetState(state);
}
