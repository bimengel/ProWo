/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ProWo
 * Copyright (C) root 2015 <root@ProWo2>
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
// CSensor
//
CSensor::CSensor(int nr)
{
    int i;
    
    m_iNummer = nr;
    m_iTemp = 0;
    m_iKorrTemp = 0;
    m_iParam2 = 0;
    m_iKorrParam2 = 0;
    m_iVocSignal = 0;
    for(i=0; i < ANZSTATSENSOR; i++)
    {
        m_sStatParam2[i] = 0;
        m_sStatTemp[i] = 0;
        m_sStatVocSignal[i] = 0;
    }
}

CSensor::~CSensor()
{

}

// Wert der Sensor-Temperatur abfragen
int CSensor::GetTemp()
{
    return m_iTemp + m_iKorrTemp;
}

// Wert der Sensor-Feuchtigkeit abfragen
int CSensor::GetParam2()
{
    return m_iParam2 + m_iKorrParam2;
}

// Wert der Sensor-VocSignal abfragen
int CSensor::GetVocSignal()
{
    return m_iVocSignal;
}
int CSensor::GetTyp()
{
    return m_iTyp;
}

void CSensor::SetName(string strName)
{
    m_strName = strName;
}

void CSensor::SetKorrTemp(int iKorrTemp)
{
    m_iKorrTemp = iKorrTemp;
}

void CSensor::SetKorrParam2(int iKorrParam2)
{
    m_iKorrParam2 = iKorrParam2;
}
string CSensor::GetName()
{
    return m_strName;
}

//
// CSensorModBus 
//
CSensorModBus :: CSensorModBus(int nr) : CSensor (nr)
{
    m_iNummer = nr;
    m_pModBusRTUClient = NULL;
    m_iFunction = 0;
}

void CSensorModBus :: SetModBusClient(CModBusRTUClient *pClient)
{
    m_pModBusRTUClient = pClient;
}

//
//  TQS3 + 4 von der Firma Papouch
//
//
// Diese Funktion wird auch gebraucht wenn die Adresse eines TQS3  umzustellen.
// Aus diesem Grunde darf die Funktion nicht nur einmal pro Minute
// aufgerufen werden, sondern öfter. Dies erfolgt über die Variable m_iChangeSensorModBusAddress
// die gesetzt wird wenn eine Änderung vorgenommen werden soll. Diese Funktion
// gibt 1 zurück wenn der Vorgang beendet ist oder abgebrochen wird.
//
int CTQS3 :: LesenStarten()
{
    unsigned char *ptr;
    int status;
    int ret = 0;
    char ptrLog[100];

    status = m_pModBusRTUClient->GetStatus();  
    if(status == 3) // Empfang ist erfolgt
    {   
        status = m_pModBusRTUClient->GetError ();
        ptr = m_pModBusRTUClient->GetEmpfPtr();
        if(!status)
        {
            switch(m_iFunction) {
            case 1: // Temperatur lesen
                if(!*(ptr+3) && !*(ptr+4))
                {
                    pthread_mutex_lock(&ext_mutexNodejs);
                    m_iTemp = ptr[5]*256 + ptr[6];
                    PlaceInStat();
                    pthread_mutex_unlock(&ext_mutexNodejs);
                }
                else
                {   
                    sprintf(ptrLog, "TQS3 - Nummer %d  incorrect temperature", m_iNummer);				    ;
                    syslog(LOG_ERR, ptrLog);
                }
                break;

            case 2: // Identification
                ret = m_pModBusRTUClient->GetEmpfLen();
                *(ptr+ret+3) = 0;
                sprintf(ptrLog, "Adresse = %d, ID = %s", m_pModBusRTUClient->GetAddress(), ptr+5);
                syslog(LOG_ERR, ptrLog);
                ret = m_pModBusRTUClient->GetAddress();
                SetFunction(1);
                break;

            case 3: // Permission for configuration changes 
                sprintf(ptrLog, "Permission for configuration changes");
                syslog(LOG_ERR, ptrLog);
                SetFunction(4);
                break;
            case 4: // Adresse ändern
                sprintf(ptrLog, "TQS3 address changed");
                syslog(LOG_ERR, ptrLog);
                m_pModBusRTUClient->SetAddress(m_pModBusRTUClient->GetNewAddress());
                SetFunction(5);
                break;
            case 5: // Adresse ändern
                sprintf(ptrLog, "TQS3 address changed");
                syslog(LOG_ERR, ptrLog);
                ret = m_pModBusRTUClient->GetNewAddress();
                SetFunction(1);
                break;
            }
        }
        else
        {	
            int adr = m_pModBusRTUClient->GetAddress();
            if(m_iFunction == 2)
            {
                if(adr > 255)
                {
                    adr = 1;
                    ret = 256;
                }    
                else
                    adr++;
                m_pModBusRTUClient->SetAddress(adr);		
            }
            else
            {
                sprintf(ptrLog, "TQS3 - Nummer %d, Adresse = %d,  error %d", m_iNummer, adr, status);
                syslog(LOG_ERR, ptrLog);
                ret = -1;
            }
        }
        // Status wird auf 1 gesetzt, die Anfrage wird mit Senden neu gestartet
        m_pModBusRTUClient->StartSend();
    }
    if(status == 0)
        m_pModBusRTUClient->StartSend();
    return(ret);
}

CTQS3::CTQS3(int nr) : CSensorModBus( nr)
{
    m_iTyp = 1;
}

CTQS3::~CTQS3() {
}

void CTQS3 :: SetFunction(int func)
{
    unsigned char *pCh = m_pModBusRTUClient->GetSendPtr ();
    m_iFunction = func;

    switch(func) {
    case 1:
        // Temperaturanfrage
        pCh[1] = 0x04; // function :reading of input register
        pCh[2] = 0x00; // start address
        pCh[3] = 0x00;
        pCh[4] = 0x00; // register count
        pCh[5] = 0x02;
        m_pModBusRTUClient->SetSendLen(6);
        m_pModBusRTUClient->SetEmpfLen(4);
        break;
    case 3: 
        // Permission for configuration changes
        pCh[1] = 0x06; // function  write holding register
        pCh[2] = 0x00; 
        pCh[3] = 0x00; 
        pCh[4] = 0x00;
        pCh[5] = 0xFF;
        m_pModBusRTUClient->SetSendLen(6);
        m_pModBusRTUClient->SetEmpfLen(3);
        break;
    case 4:
        // Adresse ändern
        pCh[1] = 0x06;
        pCh[2] = 0x00;
        pCh[3] = 0x01;
        pCh[4] = 0x00;
        pCh[5] = m_pModBusRTUClient->GetNewAddress();   // neue Adresse
        m_pModBusRTUClient->SetSendLen(6);
        m_pModBusRTUClient->SetEmpfLen(3);
        break;
    case 5: 
        // Permission for configuration changes
        pCh[1] = 0x06; // function  write holding register
        pCh[2] = 0x00; 
        pCh[3] = 0x00; 
        pCh[4] = 0x00;
        pCh[5] = 0x00;
        m_pModBusRTUClient->SetSendLen(6);
        m_pModBusRTUClient->SetEmpfLen(3);
        break;
    case 2:
    default:
        // Identifikationsanfrage
        pCh[1] = 0x11; 
        m_pModBusRTUClient->SetEmpfLen(19);
        m_pModBusRTUClient->SetSendLen(2);
        break;
    }
}
void CSensor::PlaceInStat()
{
    int idx;
    idx = m_pUhr->getUhrmin();
    if(idx < ANZSTATSENSOR)
    {
        m_sStatParam2[idx] = (short)m_iParam2;
        m_sStatTemp[idx] = (short)m_iTemp;
        m_sStatVocSignal[idx] = (short)m_iVocSignal;
    }
}
//
// Ultrasonic Level Sensor
//
CUltrasonicLevelSensor::CUltrasonicLevelSensor(int nr) : CSensorModBus( nr)
{
    m_iTyp = 3;
}

CUltrasonicLevelSensor::~CUltrasonicLevelSensor() {
}

void CUltrasonicLevelSensor :: SetFunction(int func)
{
    unsigned char *pCh = m_pModBusRTUClient->GetSendPtr ();
    m_iFunction = func;

    switch(func) {
    case 1: // Distanzabfrage berechneter Wert aus mehreren Messungen (500ms)
        pCh[1] = 0x03; // function :reading of input register
        pCh[2] = 0x01; // Register address 0x0100
        pCh[3] = 0x00;
        pCh[4] = 0x00; // register count
        pCh[5] = 0x01;
        m_pModBusRTUClient->SetSendLen(6);
        m_pModBusRTUClient->SetEmpfLen(2);
        break;
    case 2: // Distanzabfrage sofort Wert (Antwortzeit 100ms)
        pCh[1] = 0x03; // function :reading of input register
        pCh[2] = 0x01; // Register address 0x0101
        pCh[3] = 0x01;
        pCh[4] = 0x00; // register count
        pCh[5] = 0x01;
        m_pModBusRTUClient->SetSendLen(6);
        m_pModBusRTUClient->SetEmpfLen(2);  
        break;  
    case 3: // Temperaturabfrage
        pCh[1] = 0x03; // function :reading of input register
        pCh[2] = 0x01; // Register address 0x0102
        pCh[3] = 0x02;
        pCh[4] = 0x00; // register count
        pCh[5] = 0x01;
        m_pModBusRTUClient->SetSendLen(6);
        m_pModBusRTUClient->SetEmpfLen(2);
        break;
    case 4: // Adresse ändern
        pCh[1] = 0x06;
        pCh[2] = 0x01;
        pCh[3] = 0x02;
        pCh[4] = 0x00;
        pCh[5] = m_pModBusRTUClient->GetNewAddress();   // neue Adresse
        m_pModBusRTUClient->SetSendLen(6);
        m_pModBusRTUClient->SetEmpfLen(3);
        break;
    default:
        break;
    }
}
int CUltrasonicLevelSensor :: LesenStarten()
{
    unsigned char *ptr;
    int status;
    int ret = 0;
    char ptrLog[100];

    status = m_pModBusRTUClient->GetStatus();  
    if(status == 3) // Empfang ist erfolgt
    {   
        status = m_pModBusRTUClient->GetError ();
        ptr = m_pModBusRTUClient->GetEmpfPtr();
        if(!status)
        {
            switch(m_iFunction) {
            case 1: // Distanz lesen berechneter Wert
            case 2: // Wert direkt nach Messung
                if(*(ptr+1) == 0x03 && *(ptr+2) == 0x02)
                {
                    pthread_mutex_lock(&ext_mutexNodejs);
                    m_iParam2 = ptr[3]*256 + ptr[4];
                    PlaceInStat();
                    pthread_mutex_unlock(&ext_mutexNodejs);                   
                }
                else
                {   
                    sprintf(ptrLog, "UltrasonicLevelSensor - Nummer %d  incorrect distance value", m_iNummer);				    ;
                    syslog(LOG_ERR, ptrLog);
                }
                break;
            case 3:
                if(*(ptr+1) == 0x03 && *(ptr+2) == 0x02)
                {
                    pthread_mutex_lock(&ext_mutexNodejs);
                    m_iTemp = ptr[3]*256 + ptr[4];
                    PlaceInStat();
                    pthread_mutex_unlock(&ext_mutexNodejs);
                    syslog(LOG_INFO, (to_string(m_iTemp).c_str()));                    
                }
                else
                {   
                    sprintf(ptrLog, "UltrasonicLevelSensor - Nummer %d  incorrect temperatur value", m_iNummer);				    ;
                    syslog(LOG_ERR, ptrLog);
                }
                break;                
            case 4: // Adresse ändern
                sprintf(ptrLog, "UltrasonicLevelSensor address changed");
                syslog(LOG_ERR, ptrLog);
                m_pModBusRTUClient->SetAddress(m_pModBusRTUClient->GetNewAddress());
                SetFunction(1);
                break;
            }
        }
        else
        {	
            int adr = m_pModBusRTUClient->GetAddress();
            sprintf(ptrLog, "UltrasonicLevelSensor - Nummer %d, Adresse = %d,  error %d", m_iNummer, adr, status);
            syslog(LOG_ERR, ptrLog);
            ret = -1;
        }
        // Status wird auf 1 gesetzt, die Anfrage wird mit Senden neu gestartet
        m_pModBusRTUClient->StartSend();
    }
    if(status == 0)
        m_pModBusRTUClient->StartSend();
    return(ret);
}
