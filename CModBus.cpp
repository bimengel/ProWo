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

/* MODBUS RTU
 * 
 * Senden
 *	  1 byte : Adresse
 *	  1 byte : Funktion
 *   Funktion 0x03 + 0x04
 *    2 bytes : Startadresse
 *    2 bytes : Anzahl Register
 *    2 bytes : CRC
 * 
 * Antwort
 *    1 byte : Adresse
 *    1 byte : Funktion wenn bit 0x80 gesetzt dann ein Fehler
 *    1 byte : Anzahl bytes n oder Fehler
 *    n bytes : Daten
 *    2 bytes : CRC
 * 
 */
  
void CModBusRTU::Control()
{
    int ret, len, i;
    unsigned char *ptr;
    char ptrLog[100];
    
    ret = m_pRS485->Control();

    switch(ret) {
    case IDLE:
        for(i = 0; i < m_iAnzModBusClient; i++)
        {
            ret = m_pModBusRTUClient[i]->GetStatus();
            if(ret == 1) // warten auf Senden
            {   
                delayMicroseconds(m_iWaitBetweenProtocol);  // 50 msek warten, Modbus-Pause
                m_iaktAktiv = i;
                ptr = m_pModBusRTUClient[m_iaktAktiv]->GetSendPtr();
                len = m_pModBusRTUClient[m_iaktAktiv]->GetSendLen();
                ret = CalcCRC(ptr, len);
                *(ptr + len) = ret %256;
                *(ptr + len +1) = ret /256;
                m_pRS485->SendLen(m_pModBusRTUClient[m_iaktAktiv]->GetSendPtr(), 
                                  m_pModBusRTUClient[m_iaktAktiv]->GetEmpfPtr(), 
                                  m_pModBusRTUClient[m_iaktAktiv]->GetSendLen(),
                                  m_pModBusRTUClient[m_iaktAktiv]->GetEmpfLen());
                m_pModBusRTUClient[m_iaktAktiv]->SetError(NOERROR);
                m_pModBusRTUClient[m_iaktAktiv]->SetStatus(2); // wird gesendet
                m_iWait = 0;
                break;
            }
        }
        break;

    case RECEIVED:
        if(m_iaktAktiv >= 0 && m_iaktAktiv < m_iAnzModBusClient)
        {
            // Daten sind gesendet worden und entweder es erfolgte ein Empfang
            // oder m_pModBusRTUClient m_iError wurde auf ERRTIMEOUT
            ret = m_pModBusRTUClient[m_iaktAktiv]->GetError();
            if(ret == NOERROR)
            {
                ret = m_pRS485->GetError ();
                // RS485 Fehler werden in CRS485 in die Logdatei geschrieben
                // müssen aber noch in den aktuellen Client übernommen werden.
                if(!ret)
                {   
                    ptr = m_pModBusRTUClient[m_iaktAktiv]->GetEmpfPtr();
                    len = m_pModBusRTUClient[m_iaktAktiv]->GetEmpfLen();
                    len += 5; // Kopf + CRC
                    i = CalcCRC (ptr, len);
                    if(i)
                    {
                        m_pModBusRTUClient[m_iaktAktiv]->SetError(ERRCRC);
                        syslog(LOG_ERR, "ModBus: CRC Error");
                    }

                    // gibt es einen ModBus-Fehler ?
                    if (*(ptr +1) & 0x80)
                    {
                        m_pModBusRTUClient[m_iaktAktiv]->SetError(*(ptr + 2));
                        syslog(LOG_ERR, "ModBus RTU Error 0x%x", *(ptr + 2));

                    }
                }
                else
                    m_pModBusRTUClient[m_iaktAktiv]->SetError(ret);		
            }
            m_pModBusRTUClient[m_iaktAktiv]->SetStatus (3);
            m_pRS485->SetState(IDLE);
        }
        else
            m_iaktAktiv = 0;
        break;

    default:
        if(m_iWait++ > 10000)
        {
            m_pRS485->Reset(); // Zustand zurücksetzen
            m_iWait = 0;
            if(m_iaktAktiv >= 0 && m_iaktAktiv < m_iAnzModBusClient)
                m_pModBusRTUClient[m_iaktAktiv]->SetError(ERRTIMEOUT);
        }
        break;
    }

}

//
// CModBus
//
CModBus::CModBus()
{
    m_iMaxAnzModBusClient = 0;
    m_iAnzModBusClient = 0;
}

int CModBus::GetAnzClient()
{
    return m_iAnzModBusClient;
}

//
// CModBusRTU
//
CModBusRTU::CModBusRTU(int iAnz)
{
    int i;

    m_pRS485 = NULL;
    m_pModBusRTUClient = new CModBusRTUClient *[iAnz];

    for(i=0; i < iAnz; i++)
        m_pModBusRTUClient[i] = NULL;
    m_iaktAktiv = -1;
    m_iWaitBetweenProtocol = 0;
    m_iMaxAnzModBusClient = iAnz;
    pthread_mutex_init(&m_mutexModBus, NULL);
	
}

CModBusRTU::~CModBusRTU()
{
    int i;

    if(m_pModBusRTUClient)
    {
        for(i=0; i < m_iMaxAnzModBusClient; i++)
            delete m_pModBusRTUClient[i];
        delete [] m_pModBusRTUClient;
        m_pModBusRTUClient = NULL;
    }
    pthread_mutex_destroy(&m_mutexModBus);
	
}

CModBusRTUClient * CModBusRTU::AppendModBus(int Address, int iWaitBetweenProtocol)
{
    CModBusRTUClient *pRet;

    if(m_iAnzModBusClient < m_iMaxAnzModBusClient )
    {
        m_pModBusRTUClient[m_iAnzModBusClient] = new CModBusRTUClient;
        pRet = m_pModBusRTUClient[m_iAnzModBusClient];
        m_iAnzModBusClient++;
        m_iWaitBetweenProtocol = iWaitBetweenProtocol;
        pRet->SetAddress(Address, &m_mutexModBus);
    }
    else
        pRet = NULL;

    return pRet;
}

void CModBusRTU::SetCRS485(CRS485 * pRS485)
{
    m_pRS485 = pRS485;
}

unsigned int CModBusRTU::CalcCRC(unsigned char* ptr, int length)
{
    int carry;
    int i, j;
    unsigned int CRC16 = 0XFFFF;
    unsigned char b;

    for(j=0; j < length; j++)
    {	
        b = *(ptr + j);
        CRC16 ^= b & 0xFF;
        for(i=0; i<8; i++)
        {
            carry = CRC16 & 0x0001;
            CRC16 >>= 1;
            if(carry)
                CRC16 ^=0xA001;
        }
    }
    return CRC16;
}

//
//  CModBusRTUClient
//
void CModBusRTUClient::SetStatus(int state)
{
    pthread_mutex_lock(m_pmutexModBusRTU);
    m_iStatus = state;
    pthread_mutex_unlock(m_pmutexModBusRTU);
}

int CModBusRTUClient::GetStatus()
{   int i;
	
    pthread_mutex_lock(m_pmutexModBusRTU);
    i = m_iStatus;
    pthread_mutex_unlock(m_pmutexModBusRTU);
    return i;
}

void CModBusRTUClient::SetError(int err)
{
    m_iError = err;
}

int CModBusRTUClient::GetError()
{	
    return m_iError;
}

void CModBusRTUClient::StartSend()
{
    SetStatus(1);
}

unsigned char *CModBusRTUClient::GetSendPtr()
{
    return m_chSend;
}

unsigned char *CModBusRTUClient::GetEmpfPtr()
{
    return m_chEmpf;
}


int CModBusRTUClient::SetAddress(int address, pthread_mutex_t *pmutex)
{
    int ret;

    if(address > 0 && address < 255)
    {
        m_iAddress = address;
        ret = address;
        m_chSend[0] =  (char)address;
        m_pmutexModBusRTU = pmutex;
    }
    else
        ret = -1;

    return ret;
};

void CModBusRTUClient::SetAddress(int address)
{
    m_iAddress = address;
    m_chSend[0] = (char)m_iAddress;
}

int CModBusRTUClient::GetAddress()
{
	return m_iAddress;
}

CModBusRTUClient::CModBusRTUClient()
{
    m_iAddress = 0;
    m_iNewAddress = 0;
    m_iStatus = 0;
    m_iSendLen = 0;
    m_iEmpfLen = 0;
    m_iError = 0;
    m_pmutexModBusRTU = NULL;

};

void CModBusRTUClient::SetSendLen(int len)
{
    m_iSendLen = len;
}

int CModBusRTUClient::GetSendLen()
{
    return m_iSendLen;
}

void CModBusRTUClient::SetEmpfLen(int len)
{
    m_iEmpfLen = len;
}

int CModBusRTUClient::GetEmpfLen()
{
    return m_iEmpfLen;
}

void CModBusRTUClient::SetNewAddress(int newAddress)
{
    m_iNewAddress = newAddress;
}

int CModBusRTUClient::GetNewAddress()
{
    return m_iNewAddress;
}
