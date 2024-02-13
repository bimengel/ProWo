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

#ifndef _CGSM_H_
#define _CGSM_H_

#define MAXGSM			  500	

class CSMSEmpf
{
public:
    CSMSEmpf();
    int GetState();
    void SetState(int i);
    void SetString(string str);
    bool CompareString(string str);

protected:
    string m_strCommand;
    int m_iState;
};

class CSendSMS
{
public:
	string m_strGSM;
	string m_strSMSText;
};
	
class CUartI2C
{
public:
	CUartI2C();
	~CUartI2C();
	int Init(int baudrate, int bits, int stops, char parity); 
	void SetAddr(int Inh1, int Addr2, int Inh2, int Addr3, int Reg);
	void SendLen(unsigned char *ptrSend, int lenSend);
	int ReadLen(unsigned char *ptr, int iPos);
	void ResetGSM(int state);
	bool ReadState();
	
protected:
	CBoardAddr *m_pBoardAddr;
	int m_iState;
	int m_iError;
	int m_iBaud;
	int m_iBits;
	int m_iStops;
	char m_cParity;
};

class CGsm
{
	
public:
	CGsm();
	~CGsm();
	bool Control(int iZykluszeit, bool bStundenTakt, bool bMinutenTakt);
	int Init(int baudrate, int bits, int stops, char parity, string strPin); 
	void SetAddr(int Inh1, int Addr2, int Inh2, int Addr3, int Reg);
	int InsertSMS(int iNr, int iText);
    void InitSMSEmpf();
	void LesDatei(char *pPath, char *pIOGroup);
	int GetAnzNummern();
	int GetAnzTexte();
	int SearchString(char * cIn, char *cWas, int iLen);	
	int GetSignal();
	string GetRegistered();
	string GetProvider();
	string GetErrorString();
	int GetError(); // 0 oder 1 je ob ein Fehler vorhanden ist oder nicht
    int GetAnzSMSEmpf();
    CSMSEmpf * GetAddressSMSEmpf(int nr);
	
private:
    void Send(int iLen);
    CSMSEmpf *m_pSMSEmpf;
    int m_iAnzSMSEmpf;    
	queue<class CSendSMS> m_SendFifo;
	int m_iState;
	int m_iSubState;
	int m_iEmpf;
	int m_iTimeOut;
    int m_iAnzTimeOut;
	unsigned char m_chSend[MAXGSM];
	unsigned char m_chEmpf[MAXGSM];
	CUartI2C *m_pUartI2C;
	CSendSMS *m_pSendSMS;
	string *m_pNummern;
	CFormatText *m_pSMSTexte;
	int m_iAnzNummern;		// Anzahl der Nummern aus der config-Datei und in m_pNummern;
	int m_iAnzTexte;		// Anzahl der Texte aus der config-Datei und in m_pTexte
	int m_iSignal;			// Signalstärke
	int m_iRegistered;		// Beim Provider angemeldet
	string m_strProvider;  // wenn registriert, Name des Providers
	string m_strError;	 // NULL oder die letzte Fehlermeldung
    string m_strSMSCommand;
    string m_strPin;
    bool m_bFirst;
    
    pthread_mutex_t m_mutexGsmSendFifo;	
};

class CBerechneGSM : public CBerechneBase
{
public:
    CBerechneGSM();
    void init(CGsm *pGsm);	
    virtual void SetState(int state);

private:
    CGsm *m_pGsm;
};

class COperSMS : public COperBase
{
public:
    COperSMS();
    virtual int resultInt();
    void SetOper(CSMSEmpf *pPtr);
    
private:
    CSMSEmpf * m_pSMSEmpf;
};

// GSM - Modul
class COperGsm : public COperBase
{
public:
    COperGsm();
    void SetOper(CGsm *pGsm, int iFct);
	virtual string resultString();
	virtual int resultInt();

protected:
    CGsm *m_pGsm;
	int m_iFct;
};

#endif // _CGSM_H_
