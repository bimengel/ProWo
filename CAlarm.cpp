/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CAlarm.cpp
 * Author: josefengel
 * 
 * Created on January 9, 2020, 5:02 PM
 */

#include "ProWo.h"

CAlarmSensor::CAlarmSensor()
{
    m_pBerechne  = NULL;
    m_bState = true;
    m_bLastState = true;
    m_bMust = true;
}
bool CAlarmSensor::ChangeState()
{
    bool bRet = false;
    
    if(m_bState != m_bLastState)
        bRet = true;
    return bRet;
}

void CAlarmSensor::SetSensorBez(string strSensor, string strBez, int iMust)
{
    m_strSensor = strSensor;
    m_strBez = strBez;
    if(iMust == 1)
        m_bMust = false;
}

string CAlarmSensor::GetBez()
{
    return m_strBez;
}

string CAlarmSensor::GetSensor()
{
    return m_strSensor;
}

void CAlarmSensor::SetBerechne(CBerechneBase *pBerechne)
{
    m_pBerechne = pBerechne;
}


void CAlarmSensor::Berechne()
{
    if(m_pBerechne != NULL)
    {
        m_bLastState = m_bState;
        m_bState = (bool) m_pBerechne->eval();
    }
}

bool CAlarmSensor::GetState()
{
    return m_bState;
}
bool CAlarmSensor::GetLastState()
{
    return m_bLastState;
}
bool CAlarmSensor::GetMust()
{
    return m_bMust;
}

CAlarmSensor * CAlarm::GetSensorAddr()
{
    return m_pAlarmSensor;
}
//
//  CAlarm
//
CAlarm::CAlarm(int iAnzSensor, int iAnzAlarme) {
    int i;
    
    m_iAnzSensoren = iAnzSensor;
    m_iAnzAlarme = iAnzAlarme;
    m_pAlarmSensor = new CAlarmSensor[iAnzSensor];
    m_pAlarme = new CAlarme[iAnzAlarme];
    m_pbEnabled  = new bool[iAnzSensor];
    ResetEnabled();
    m_iState = 1;   // Sabotage wird überwacht
    m_iLastState = 1;
    m_iAlarm = 1;
    m_iDelayStart = 0;
    m_iAlarmStartStop = 0;  // Signal von Aussen, beim Starten Nummer des Alarms
                            // oder wenn verschieden von 0 zum stoppen des Alarms
    m_strPwd = "alarmpwdjen2020";
    m_strTechPwd = "alarmpwdjen2020";
    pthread_mutex_init(&m_mutexAlarmFifo, NULL);       
}

CAlarm::~CAlarm() {
    
    pthread_mutex_destroy(&m_mutexAlarmFifo);   
    if(m_pAlarmSensor)
        delete [] m_pAlarmSensor;
    if(m_pAlarme)
        delete [] m_pAlarme;
}

void CAlarm::ResetEnabled()
{
    int i;
    
    for(i=0; i < m_iAnzSensoren; i++)
        m_pbEnabled[i] = true;   
}
void CAlarm::SetSensorBez(int iSensor, string strSensor, string strBez, int iMust)
{
    if(iSensor >= 0 && iSensor < m_iAnzSensoren)
        m_pAlarmSensor[iSensor].SetSensorBez(strSensor, strBez, iMust);
}

void CAlarm::SetAlarmeBez(int iIdx, int iAnz, string strBez)
{
    if(iIdx >= 0 && iIdx < m_iAnzAlarme)
        m_pAlarme[iIdx].SetBez(iAnz, strBez);
    
}

bool CAlarm::SetAlarmeSensor(int iAlarm, int iIdx, int iNr)
{
    bool bRet = true;
    if(iAlarm >= 0 && iAlarm < m_iAnzAlarme && iNr > 0 && iNr <= m_iAnzSensoren)
        m_pAlarme[iAlarm].SetSensorNr(iIdx, iNr);
    else
        bRet = false;
    return bRet;
}
int CAlarm::GetAnzAlarme()
{
    return m_iAnzAlarme;
}
void CAlarm::SetPwd(string strPwd)
{
    m_strPwd = strPwd;
}
string CAlarm::GetPwd()
{
    return m_strPwd;
}
void CAlarm::SetTechPwd(string strPwd)
{
    m_strTechPwd = strPwd;
}
string CAlarm::GetTechPwd()
{
    return m_strTechPwd;
}
void CAlarm::SetDelay(int iAlarm, int iDelayAktiv, int iDelayAlarm)
{
    if(iAlarm >= 0 && iAlarm < m_iAnzAlarme)
        m_pAlarme[iAlarm].SetDelay(iDelayAktiv, iDelayAlarm);
}

void CAlarm::SetBerechneSensor(int iIdx, CBerechneBase *pBerechne)
{
    if(iIdx >= 0 && iIdx < m_iAnzSensoren)
        m_pAlarmSensor[iIdx].SetBerechne(pBerechne);
}
void CAlarm::SetAlarm(int iAlarmNr)
{
    m_iAlarmStartStop = iAlarmNr;
}
void CAlarm::BerechneAlarme()
{
    int i, j, iIdx, iAusloeser;
    bool bState, bStateMust;
    CAlarme *pAlarme;
    
    // Zustand der Sensoren berechnen
    if(m_iAnzSensoren)
    {
        for(i=0; i < m_iAnzSensoren; i++)
            m_pAlarmSensor[i].Berechne();
    }
    else
        return;
    if(m_iAnzAlarme)
    {
        for(i=0; i < m_iAnzAlarme; i++)
        {
            bState = true;
            bStateMust = true;
            iAusloeser = 0;
            pAlarme = GetAlarmeAddr(i);
            for(j = pAlarme->GetAnzSensoren(); j; j--)
            {
                iIdx = pAlarme->GetSensorNr(j-1)-1;
                if(*(m_pbEnabled + iIdx) && !m_pAlarmSensor[iIdx].GetState())
                {
                    if(bState)
                    {
                        bState = false;
                        iAusloeser = iIdx+1;
                    }
                    if(m_pAlarmSensor[iIdx].GetMust())
                        bStateMust = false;
                }
            }
            pAlarme->SetStateSensor(bState, bStateMust, iAusloeser);
        }
    }
    else
        return;   
}

void CAlarm::Berechne(int iTime)
{
    int i, j;
    CAlarme *pAlarme;
    string str;
    
    BerechneAlarme();
    
    switch(m_iState) {
    case 0: // Technikermodus
        if(m_iAlarmStartStop == 1)
            SetState(1, "Sabotage eingeschaltet!");
        else
        {
            if(m_iAlarmStartStop > 1 && m_iAlarmStartStop <= m_iAnzAlarme)
            {
                m_iAlarm = m_iAlarmStartStop;
                m_iDelayStart = iTime;
                str = "Alarmanlage voraktiviert: " + m_pAlarme[m_iAlarm-1].GetBez();
                SetState(2, str);
            }
        }
        m_iAlarmStartStop = 0;
        break;
    case 1: // Sabotagemodus ist eingeschaltet
        if(!m_pAlarme[0].GetStateSensor()) // Kontrolle der Sabotage-Sensoren
            SetState(5, "Sabotagealarm! ");
        else
        {
            if(m_iAlarmStartStop) // von Aussen ist ein Alarm gesetzt worden
            {
                if(m_iAlarmStartStop == 1)
                    SetState(0, "Technikermodus eingeschaltet");
                if(m_iAlarmStartStop > 1 && m_iAlarmStartStop <= m_iAnzAlarme)
                {
                    m_iAlarm = m_iAlarmStartStop;
                    m_iDelayStart = iTime;
                    str = "Alarmanlage voraktiviert: " + m_pAlarme[m_iAlarm-1].GetBez();
                    SetState(2, str);
                }  
                m_iAlarmStartStop = 0;
            }
        }
        break;
    case 3: // Alarmanlage ist scharf, testen ob alle Sensoren OK sind
        pAlarme = GetAlarmeAddr(m_iAlarm-1);
        if(!pAlarme->GetStateSensor())
        {
            i = pAlarme->GetAusloeser() - 1;
            m_iDelayStart = iTime;
            if(i >= 0 && i < m_iAnzSensoren)
                str = "Alarm: " + m_pAlarmSensor[i].GetBez();
            else
                str = "Alarm: Ursache nicht bekannt!";
            SetState(4, str);
        }
        break;
    case 4:
    case 5:
        str = "";
        for(i=0; i < m_iAnzSensoren; i++)
        {
            if(m_pAlarmSensor[i].ChangeState())
            {
                if(m_pAlarmSensor[i].GetState())
                    str += m_pAlarmSensor[i].GetBez() + ": ";
                else
                    str += m_pAlarmSensor[i].GetBez() + ": ";
                if(m_pAlarmSensor[i].GetMust())
                {   // es ist ein Fenster oder Tür
                    if(m_pAlarmSensor[i].GetState()) 
                        str += " geschlossen; ";
                    else
                        str += " geöffnet; ";
                }        
                else
                {
                    if(m_pAlarmSensor[i].GetState()) 
                        str = "";
                    else
                        str += " aktiviert; ";
                }
            }
        }    
        if(str.length() > 0)
        {
            str = "Sensorwechsel: " + str;
            SetState(-1, str);
        }    
        break;
    default:
        break;
    }

    if(m_iAlarmStartStop) 
    {
        if(m_iAlarmStartStop == 1)
            SetState(0, "Technikermodus eingeschaltet!");
        else
            SetState(1, "Alarm ausgeschaltet");
        m_iAlarmStartStop = 0;
    } 
}
bool CAlarm::BerechneTimer(int iTime)
{
    bool bRet = false;
    string str;
    
    if(m_iAlarm)
    {
        switch(m_iState) {
            case 2: // DelayStart warten
                if(m_iDelayStart + m_pAlarme[m_iAlarm-1].GetDelayAktiv() < iTime)
                {
                    str = "Alarmanlage aktiv: " + m_pAlarme[m_iAlarm-1].GetBez();
                    SetState(3, str);
                    bRet = true;
                }
                break;
            case 4: // DelayAlarm warten
                if(m_iDelayStart + m_pAlarme[m_iAlarm-1].GetDelayAlarm() < iTime)
                {
                    SetState(5, "Alarm ausgegeben!");
                    bRet = true;
                }
                break;          
            default:
                break;
        }
    }
    return bRet;
}

int CAlarm::GetParamInt(int fct)
{
    int iRet = 0;
    
    if(m_iState != m_iLastState)
    {
        switch(fct) {
        case 2: // VORAKTIVIERT
        if(m_iState == 2)
            iRet = 1;
        break; 
        case 3: // AKTIVIERT
            if(m_iState == 3)
                iRet = 1;
            break;
        case 4: // VORALAMIERT
            if(m_iState == 4)
                iRet = 1;
            break;
        case 5: // ALARMIERT
            if(m_iState == 5)
                iRet = 1;
            break;
        case 6: // ALARMABGESCHALTET
            if(m_iState == 1 || m_iState == 0)
                iRet = 1;
            break;
        default:
            break;
        }
        if(iRet)
            m_iLastState = m_iState;
    }
    switch(fct) {
        case 1: // Status
            iRet = m_iState;
            break;
        case 9: // Nummer des aktiverten Alarms
            iRet = m_iAlarm;
            break;
        default:
            break;
    }

    return iRet;
}

string CAlarm::GetParamString(int fct)
{
    string str;
    int i;
    
    str.clear();
    switch(fct){
        case 7:
            str = m_pAlarme[m_iAlarm-1].GetBez();
            break;
        case 8: // Bezeichnung des Auslösers des Alarms
            i = m_pAlarme[m_iAlarm - 1].GetAusloeser();
            if(i)
                str = m_pAlarmSensor[i-1].GetBez();
            break;
        default:
            str = to_string(GetParamInt(fct));
            break;
    }
    return str;
}

int CAlarm::GetAlarmNr()
{
    return m_iAlarm;
}

void CAlarm::SetState(int iState, string str)
{
    int i, iAnz, iIdx;
    CAlarme *pAlarme;
    
    // iState = -1 wird genutzt um eine Zeichenkette in die Log-Datei zu schreiben
    if(iState >= 0)
    {
        m_iLastState = m_iState;
        m_iState = iState;
    }   
    switch(iState) {
        case 2: // Voraktivierung des Alarm (alle deaktivierten Sensoren aufführen!
            pAlarme = &m_pAlarme[m_iAlarm-1];
            iAnz = pAlarme->GetAnzSensoren();
            for(i=0; i < iAnz; i++)
            {
                iIdx = pAlarme->GetSensorNr(i) - 1;
                if(!m_pbEnabled[iIdx])
                    str = str + "; " + m_pAlarmSensor[iIdx].GetBez();
            }            
            break;
        case 5: // es steht ein Alarm an
            pAlarme = &m_pAlarme[m_iAlarm-1];
            iIdx = pAlarme->GetAusloeser()-1;
            if(iIdx >= 0 && iIdx < m_iAnzSensoren)
                str += m_pAlarmSensor[iIdx].GetBez() + "  ";
            else
                str += "Ausloeser nicht bekannt!  ";
            break;
        default:
            break;
    }
    str = m_pUhr->GetUhrzeitStr() + " " + str + "\n";
    pthread_mutex_lock(&m_mutexAlarmFifo);    
    m_LogFifo.push(str);
    pthread_mutex_unlock(&m_mutexAlarmFifo); 
}

void CAlarm::Control()
{
    string str;
    CReadFile file;
    
    pthread_mutex_lock(&m_mutexAlarmFifo);     
    if(!m_LogFifo.empty())
    {
        str = m_LogFifo.front();
        m_LogFifo.pop();
        pthread_mutex_unlock(&m_mutexAlarmFifo); 
        file.OpenAppend(pProgramPath, 9, 0);
        file.WriteLine((char *)str.c_str());
        file.Close();
    }
    else
        pthread_mutex_unlock(&m_mutexAlarmFifo); 
}
bool CAlarm::GetEnabled(int iNr)
{
    bool bRet = false;
    
    if(iNr > 0 && iNr <= m_iAnzSensoren)
        bRet = m_pbEnabled[iNr-1];
    return bRet;
}
void CAlarm::SetEnabled(int iNr, bool bValue)
{
    if(iNr > 0 && iNr <= m_iAnzSensoren)
        m_pbEnabled[iNr-1] = bValue;
}

//
//      CAlarme
//
CAlarme::CAlarme() {
    
    m_piSensor = NULL;
    m_strBez ="";
    m_iAnzSensoren = 0;
    m_pBerechne = NULL;
    m_bStateSensor = true;
    m_bStateSensorMust = true;
    m_iDelayAktiv = 120;
    m_iDelayAlarm = 60;  
    m_iAusloeser = 0;
}

CAlarme * CAlarm::GetAlarmeAddr(int iIdx)
{
    CAlarme * pAlarme = NULL;
    
    if(iIdx >= 0 && iIdx < m_iAnzAlarme)
        pAlarme = &m_pAlarme[iIdx];
    
    return pAlarme;
}
void CAlarme::SetBez(int iAnz, string strBez)
{
    m_strBez = strBez;
    m_piSensor = new int[iAnz];
    m_iAnzSensoren = iAnz;
}

string CAlarme::GetBez()
{
    return m_strBez;
}

void CAlarme::SetSensorNr(int iIdx, int iNr)
{
    if(iIdx >= 0 && iIdx < m_iAnzSensoren)
        m_piSensor[iIdx] = iNr;
}

int CAlarme::GetAnzSensoren()
{
    return m_iAnzSensoren;
}

int CAlarme::GetSensorNr(int iIdx)
{
    int ret = 0;
    
    if(iIdx >= 0 && iIdx < m_iAnzSensoren)
        ret = m_piSensor[iIdx];
    
    return ret;
}

void CAlarme::SetStateSensor(bool bState, bool bMust, int iAusloeser)
{
    if(m_bStateSensor && !bState) // Status ändert
        m_iAusloeser = iAusloeser;
    m_bStateSensor = bState;
    m_bStateSensorMust = bMust;
}
bool CAlarme::GetStateSensor()
{
    return m_bStateSensor;
}
bool CAlarme::GetStateSensorMust()
{
    return m_bStateSensorMust;
}
void CAlarme::SetDelay(int iDelayAktiv, int iDelayAlarm)
{
    m_iDelayAktiv = iDelayAktiv;
    m_iDelayAlarm = iDelayAlarm;
}
int CAlarme::GetDelayAktiv()
{
    return m_iDelayAktiv;
}

int CAlarme::GetDelayAlarm()
{
    return m_iDelayAlarm;
}

int CAlarme::GetAusloeser()
{
    return m_iAusloeser;
}

//
//   Operand
//
COperAlarm::COperAlarm()
{
    m_pAlarm = NULL;
    m_iFct = 0;
}

void COperAlarm::SetOper(CAlarm* pAlarm, int fct)
{
    m_pAlarm = pAlarm;
    m_iFct = fct;
}

int COperAlarm::resultInt()
{
    return m_pAlarm->GetParamInt(m_iFct);
}

string COperAlarm::resultString()
{
    return m_pAlarm->GetParamString(m_iFct);
}

//
// Zieloperand
//
void CBerechneAlarm::init(CAlarm* pAlarm, CHistory *pHistory)
{
    m_pAlarm = pAlarm;
    m_pHistory = pHistory;
}

void CBerechneAlarm::SetState(int state)
{
    int iAktAlarm;
    
    iAktAlarm = m_pAlarm->GetAlarmNr();
    if(iAktAlarm > 1)
    {
        m_pAlarm->SetAlarm(iAktAlarm);        
        m_pHistory->SetDiffTage(0);
    }
}