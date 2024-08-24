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

#ifndef _CModBusRTU_H_
#define _CModBusRTU_H_

#define ANZSENDCHAR	256
#define ANZRECEIVECHAR  256

// Errors
#define ERRCRC		10

class CModBusRTUClient
{

public:
    CModBusRTUClient();
    int SetAddress(int address, pthread_mutex_t *pmutext);
    void SetAddress(int address);
    void SetNewAddress(int newAddress);
    int GetNewAddress();
    int GetAddress();
    int GetStatus();
    void SetError(int err);
    int GetError();
    void SetStatus(int state);
    unsigned char *GetSendPtr();
    unsigned char *GetEmpfPtr();
    int GetEmpfLen();
    void SetEmpfLen(int len);
    void SetSendLen(int len);
    int GetSendLen();
    void StartSend();
	
protected:
    int m_iAddress;
    int m_iNewAddress;
    unsigned char m_chSend[ANZSENDCHAR];
    unsigned char m_chEmpf[ANZRECEIVECHAR];
    int m_iSendLen;
    int m_iEmpfLen;
    int m_iStatus;
    int m_iError;
    pthread_mutex_t *m_pmutexModBusRTU;
	
private:

};

class CModBus
{
public:
    CModBus();
    int GetAnzClient();
    virtual void Control() { return; };

protected:
    int m_iMaxAnzModBusClient; 
    int m_iAnzModBusClient;      
};

class CModBusRTU : public CModBus
{
public:
    CModBusRTU(int iMaxAnzModBusClient);
    ~CModBusRTU();
    CModBusRTUClient * AppendModBus(int Address, int iWaitBetweenProtocol);
    void SetCRS485(CRS485 * pCRS485);
    virtual void Control();
	
protected:
    unsigned int CalcCRC(unsigned char* ptr, int length);
    CModBusRTUClient **m_pModBusRTUClient;
    CRS485 * m_pRS485;
    pthread_mutex_t m_mutexModBus;
    int m_iaktAktiv;
    int m_iWait;
    int m_iWaitBetweenProtocol;

private:

};
#endif // _CModBusRTU_H_
