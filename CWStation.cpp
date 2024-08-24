/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "ProWo.h"

CWStation::CWStation(int nr)
{
    int i;

    for(i=0; i < 60; i++)
        m_usWG60[i] = 300;    
    m_iNr = nr;
    m_bFirst = true;
    m_iPosWG60 = 0;
    m_usWGMAX10 = 300;
    m_usWGMAX30 = 300;
    m_usWGMAX60 = 300;
    m_pWSValueStatistic[0] = new CWSValueNSMenge(1);
    m_pWSValueStatistic[1] = new CWSValueMinTemp(2);
    m_pWSValueStatistic[2] = new CWSValueMaxTemp(3);
    m_pWSValueStatistic[3] = new CWSValueMaxWindGeschw(4);
}
CWStation::~CWStation()
{
    int i;
    for(i=0; i < ANZWSVALUESTATISTIC; i++)
        delete m_pWSValueStatistic[i];
}
void CWStation::SetModBusClient(CModBusRTUClient *pClient)
{
    m_pModBusRTUClient = pClient;
    SetFunction(2);
}
CWSValue * CWStation::GetPtrWSValue(int iIdx)
{
    if(iIdx >= 0 && iIdx < ANZWSVALUESTATISTIC)
        return m_pWSValueStatistic[iIdx];
    else
        return NULL;
}
void CWStation::SetFunction(int iFunc)
{
    unsigned char *pCh = m_pModBusRTUClient->GetSendPtr ();
    switch (iFunc) {
        case 1:     // Messwerte Status
            pCh[1] = 4;  // Funktion
            pCh[2] = 0;  // Adresse 
            pCh[3] = 0;
            pCh[4] = 0;    // Anzahl Register
            pCh[5] = ANZMESSWERTESTATUS;
            m_pModBusRTUClient->SetSendLen(6);
            m_pModBusRTUClient->SetEmpfLen(ANZMESSWERTESTATUS * 2);
            break;
        case 2:  // Messwerte - Service
            pCh[1] = 4;
            pCh[2] = 0;
            pCh[3] = 139;
            pCh[4] = 0;
            pCh[5] = ANZMESSWERTESERVICE;
            m_pModBusRTUClient->SetSendLen(6);
            m_pModBusRTUClient->SetEmpfLen(ANZMESSWERTESERVICE * 2);
            break;
        default:
            break;
    }
}

void CWStation::LesenStarten()
{
    unsigned char *ptr;
    int status, i, j, anz;
    bool bTrue = true;
    unsigned short max;
    int ret = 0;
    char ptrLog[MSGSIZE];

    status = m_pModBusRTUClient->GetStatus();  
    if(status == 3) // Empfang ist erfolgt
    {   
        status = m_pModBusRTUClient->GetError ();
        ptr = m_pModBusRTUClient->GetEmpfPtr();
        unsigned char *pCh = m_pModBusRTUClient->GetSendPtr ();        
        if(!status)
        {   
            if(pCh[3] == 0)  // Messwerte-Status
            {   
                pthread_mutex_lock(&ext_mutexNodejs);
                for(i=0; i < ANZMESSWERTESTATUS; i++)
                    m_ssMesswerteStatus[i] = (signed short)(*(ptr + 3 + i*2) * 256 + *(ptr + 4 + i*2));

                for(i=0; i < ANZWSVALUESTATISTIC; i++)
                    m_pWSValueStatistic[i]->Control(m_ssMesswerteStatus);
                // Windgeschwindigkeit der letzten 60 Minuten
                m_usWG60[m_iPosWG60++] = (unsigned short)m_ssMesswerteStatus[IDXWINDGESCHW];
                if(m_iPosWG60 >= 60)
                    m_iPosWG60 = 0;
                
                anz = 10;
                i = 0;
                j = m_iPosWG60-1;
                max = 0;
                while(bTrue)
                {
                    for( ; i < anz; i++, j--)
                    {
                        if(j < 0)
                            j = 59;
                        if(max < m_usWG60[j])
                            max = m_usWG60[j];
                    }
                    switch(i) {
                        case 10:
                            m_usWGMAX10 = max;
                            anz = 30;
                            break;
                        case 30:
                            m_usWGMAX30 = max;
                            anz = 60;
                            break;
                        case 60:
                            m_usWGMAX60 = max;
                            bTrue = false;
                            break;
                        default:
                            bTrue = false;
                            break;
                    }
                }

                pthread_mutex_unlock(&ext_mutexNodejs);
            }
            else if(pCh[3] == 139) // Messwerteservice
            {
                for(i = 0; i < ANZMESSWERTESERVICE; i++)
                    m_ssMesswerteService[i] = *(ptr + 3 + i*2) * 256 + *(ptr + 4 + i*2);  
                SetFunction(1);
            }
        }
        else
        {	
            snprintf(ptrLog, MSGSIZE, "WS10 - Nummer %d, error %d", m_iNr, status);
            syslog(LOG_ERR, ptrLog);
            ret = -1;
        }
    }
    m_pModBusRTUClient->StartSend();
}

void CWStation::TagWechsel()
{
    int i;
    
    pthread_mutex_lock(&ext_mutexNodejs);    
    m_pWSValueStatistic[0]->SaveDayValue(GetParam(NSMENGEABS));
    m_pWSValueStatistic[1]->SaveDayValue(GetParam(TEMPAKT));
    m_pWSValueStatistic[2]->SaveDayValue(GetParam(TEMPAKT));
    m_pWSValueStatistic[3]->SaveDayValue(GetParam(WINDGESCHW));
   pthread_mutex_unlock(&ext_mutexNodejs);
    SetFunction(2); // 1x pro Tag die Servicedaten einlesen
}

int CWStation::GetParam(int iParam)
{
    int ret = 0, i, j;
    
    switch(iParam) {
    case TEMPAKT: // TEMP aktuelle Temperatur
        ret = m_ssMesswerteStatus[IDXTEMP];
        break;
    case TEMPMIN: // minimale Temperatur 
        ret = m_pWSValueStatistic[1]->GetValue();
        break;
    case TEMPMAX: // maximale Temperatur
        ret = m_pWSValueStatistic[2]->GetValue();
        break;
    case TAUPUNKT:
        ret = m_ssMesswerteStatus[IDXTAUPUNKT];
        break;
    case FEUCHTE: // Feuchte
        ret = m_ssMesswerteStatus[IDXFEUCHTE];
        break;
    case LUFTDRUCK: // Luftdruck
        ret = m_ssMesswerteStatus[IDXLUFTDRUCK];
        break;
    case WINDGESCHW: // aktuelle Windgeschwindigkeit
        ret = m_ssMesswerteStatus[IDXWINDGESCHW];
        break; 
    case WINDGESCHWMAX: // maximale Windgeschwindigkeit des aktuellen Tages
        ret = m_pWSValueStatistic[3]->GetValue();
        break;
    case WINDGESCHWMAX10:
        ret = m_usWGMAX10;
        break;
    case WINDGESCHWMAX30:
        ret = m_usWGMAX30;
        break;
    case WINDGESCHWMAX60:
        ret = m_usWGMAX60;
        break;
    case WINDRICHT: // Windrichtung
        ret = m_ssMesswerteStatus[IDXWINDRICHT];
        break; 
    case NSMENGE: // Niederschlagsmenge
        ret = m_pWSValueStatistic[0]->GetValue();
        break; 
    case NSMENGEABS:
        ret = m_ssMesswerteStatus[IDXNSMENGE];
        break;
    case NSART: // Niederschlagsart
        ret = m_ssMesswerteStatus[IDXNSART];
        break; 
    case NSINTENS: // Niederschlagsintensität
        ret = m_ssMesswerteStatus[IDXNSINTENS];
        break; 
    case AZIMUT: // Sonnenstand Azimut
        ret = m_ssMesswerteStatus[IDXAZIMUT];
        break;
    case ELEVATION: // Sonnenstand Elevation
        ret = m_ssMesswerteStatus[IDXELEVATION];
        break;
    case UVINDEX: // UV-Index
        ret = m_ssMesswerteStatus[IDXUVINDEX];
        break; 
    case HELLIGKEIT: // Helligkeit
        ret = m_ssMesswerteStatus[IDXHELLIGKEIT];
        break;
    default:
        break;
    }
    return ret;
}

int CWStation::GetParam(int iParam, int iInterval, int iDiff)
{
    int i;
    
    switch(iParam) {
        case NSMENGE:
            i = 0;
            break;
        case TEMPMIN:
            i = 1;
            break;
        case TEMPMAX:
            i = 2;
            break;
        case WINDGESCHWMAX:
            i = 3;
            break;
        default:
            return 0;
            break;
    }
    return m_pWSValueStatistic[i]->GetResultInt(iInterval, iDiff, m_pWSValueStatistic[i]->GetValue());
}
//
//  Abgeleitete Klasse von COperBase für die Wetterstation
COperWStation::COperWStation()
{
    m_pWStation = NULL;
    m_iFct = 0;
}

int COperWStation::resultInt()
{
    return m_pWStation->GetParam(m_iFct);
}
void COperWStation::setOper(CWStation *ptr, int fct)
{
    m_pWStation = ptr;
    m_iFct = fct;
}

COperWStatistic::COperWStatistic()
{
    m_pWStation = NULL;
    m_iParam = 0;
    m_iInterval = 0;
    m_iDiff = 0;
    m_iDiv = 1;
}

int COperWStatistic::resultInt()
{
    return m_pWStation->GetParam(m_iParam, m_iInterval, m_iDiff);

}
string COperWStatistic::resultString()
{
    string str;
    char buf[MSGSIZE];
    
    switch(m_iDiv) {
        case 10:
            snprintf(buf, MSGSIZE, "%0.1f", (float)resultInt() / 10.0);
            break;
        case 100:
            snprintf(buf, MSGSIZE, "%0.2f", (float)resultInt() / 100.0);  
            break;
        default:
        case 1:
            snprintf(buf, MSGSIZE, "%d", resultInt());
            break;
    }
    str = buf;
    return str;
    
}
void COperWStatistic::setOper(CWStation *pWStation, int iParam, int iInterval, int iDiff, int iDiv)
{
    m_pWStation = pWStation;
    m_iParam = iParam;
    m_iInterval = iInterval;
    m_iDiff = iDiff;
    m_iDiv = iDiv;
}
