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
  
void CModBus::Control()
{
    int ret, len, i;
    unsigned char *ptr;
    char ptrLog[100];
    
    ret = m_pRS485->Control();

    switch(ret) {
    case IDLE:
        for(i = 0; i < m_iAnzModBusClient; i++)
        {
            ret = m_pModBusClient[i]->GetStatus();
            if(ret == 1) // warten auf Senden
            {   
                delayMicroseconds(m_iWaitBetweenProtocol);  // 50 msek warten, Modbus-Pause
                m_iaktAktiv = i;
                ptr = m_pModBusClient[m_iaktAktiv]->GetSendPtr();
                len = m_pModBusClient[m_iaktAktiv]->GetSendLen();
                ret = CalcCRC(ptr, len);
                *(ptr + len) = ret %256;
                *(ptr + len +1) = ret /256;
                m_pRS485->SendLen(m_pModBusClient[m_iaktAktiv]->GetSendPtr(), 
                                  m_pModBusClient[m_iaktAktiv]->GetEmpfPtr(), 
                                  m_pModBusClient[m_iaktAktiv]->GetSendLen(),
                                  m_pModBusClient[m_iaktAktiv]->GetEmpfLen());
                m_pModBusClient[m_iaktAktiv]->SetError(NOERROR);
                m_pModBusClient[m_iaktAktiv]->SetStatus(2); // wird gesendet
                m_iWait = 0;
                break;
            }
        }
        break;

    case RECEIVED:
        if(m_iaktAktiv >= 0 && m_iaktAktiv < m_iAnzModBusClient)
        {
            // Daten sind gesendet worden und entweder es erfolgte ein Empfang
            // oder m_pModBusClient m_iError wurde auf ERRTIMEOUT
            ret = m_pModBusClient[m_iaktAktiv]->GetError();
            if(ret == NOERROR)
            {
                ret = m_pRS485->GetError ();
                // RS485 Fehler werden in CRS485 in die Logdatei geschrieben
                // müssen aber noch in den aktuellen Client übernommen werden.
                if(!ret)
                {   
                    ptr = m_pModBusClient[m_iaktAktiv]->GetEmpfPtr();
                    len = m_pModBusClient[m_iaktAktiv]->GetEmpfLen();
                    len += 5; // Kopf + CRC
                    i = CalcCRC (ptr, len);
                    if(i)
                    {
                        m_pModBusClient[m_iaktAktiv]->SetError(ERRCRC);
                        syslog(LOG_ERR, "ModBus: CRC Error");
                    }

                    // gibt es einen ModBus-Fehler ?
                    if (*(ptr +1) & 0x80)
                    {
                        m_pModBusClient[m_iaktAktiv]->SetError(*(ptr + 2));
                        syslog(LOG_ERR, "ModBus RTU Error 0x%x", *(ptr + 2));

                    }
                }
                else
                    m_pModBusClient[m_iaktAktiv]->SetError(ret);		
            }
            m_pModBusClient[m_iaktAktiv]->SetStatus (3);
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
                m_pModBusClient[m_iaktAktiv]->SetError(ERRTIMEOUT);
        }
        break;
    }

}

CModBus::CModBus(int iAnz)
{
    int i;

    m_pRS485 = NULL;
    m_pModBusClient = new CModBusClient *[iAnz];

    for(i=0; i < iAnz; i++)
        m_pModBusClient[i] = NULL;
    m_iAnzModBusClient = 0;
    m_iaktAktiv = -1;
    m_iWaitBetweenProtocol = 0;
    m_iMaxAnzModBusClient = iAnz;
    pthread_mutex_init(&m_mutexModBus, NULL);
	
}

CModBus::~CModBus()
{
    int i;

    if(m_pModBusClient)
    {
        for(i=0; i < m_iMaxAnzModBusClient; i++)
            delete m_pModBusClient[i];
        delete [] m_pModBusClient;
        m_pModBusClient = NULL;
    }
    pthread_mutex_destroy(&m_mutexModBus);
	
}
int CModBus::GetAnzClient()
{
    return m_iAnzModBusClient;
}

CModBusClient * CModBus::AppendModBus(int Address, int iWaitBetweenProtocol)
{
    CModBusClient *pRet;

    if(m_iAnzModBusClient < m_iMaxAnzModBusClient )
    {
        m_pModBusClient[m_iAnzModBusClient] = new CModBusClient;
        pRet = m_pModBusClient[m_iAnzModBusClient];
        m_iAnzModBusClient++;
        m_iWaitBetweenProtocol = iWaitBetweenProtocol;
        pRet->SetAddress(Address, &m_mutexModBus);
    }
    else
        pRet = NULL;

    return pRet;
}

void CModBus::SetCRS485(CRS485 * pRS485)
{
    m_pRS485 = pRS485;
}

unsigned int CModBus::CalcCRC(unsigned char* ptr, int length)
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
//  cModBusClient
//
void CModBusClient::SetStatus(int state)
{
    pthread_mutex_lock(m_pmutexModBus);
    m_iStatus = state;
    pthread_mutex_unlock(m_pmutexModBus);
}

int CModBusClient::GetStatus()
{   int i;
	
    pthread_mutex_lock(m_pmutexModBus);
    i = m_iStatus;
    pthread_mutex_unlock(m_pmutexModBus);
    return i;
}

void CModBusClient::SetError(int err)
{
    m_iError = err;
}

int CModBusClient::GetError()
{	
    return m_iError;
}

void CModBusClient::StartSend()
{
    SetStatus(1);
}

unsigned char *CModBusClient::GetSendPtr()
{
    return m_chSend;
}

unsigned char *CModBusClient::GetEmpfPtr()
{
    return m_chEmpf;
}


int CModBusClient::SetAddress(int address, pthread_mutex_t *pmutex)
{
    int ret;

    if(address > 0 && address < 255)
    {
        m_iAddress = address;
        ret = address;
        m_chSend[0] =  (char)address;
        m_pmutexModBus = pmutex;
    }
    else
        ret = -1;

    return ret;
};

void CModBusClient::SetAddress(int address)
{
    m_iAddress = address;
    m_chSend[0] = (char)m_iAddress;
}

int CModBusClient::GetAddress()
{
	return m_iAddress;
}

CModBusClient::CModBusClient()
{
    m_iAddress = 0;
    m_iNewAddress = 0;
    m_iStatus = 0;
    m_iSendLen = 0;
    m_iEmpfLen = 0;
    m_iError = 0;
    m_pmutexModBus = NULL;

};

void CModBusClient::SetSendLen(int len)
{
    m_iSendLen = len;
}

int CModBusClient::GetSendLen()
{
    return m_iSendLen;
}

void CModBusClient::SetEmpfLen(int len)
{
    m_iEmpfLen = len;
}

int CModBusClient::GetEmpfLen()
{
    return m_iEmpfLen;
}

void CModBusClient::SetNewAddress(int newAddress)
{
    m_iNewAddress = newAddress;
}

int CModBusClient::GetNewAddress()
{
    return m_iNewAddress;
}
