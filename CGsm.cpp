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

#define RHR		0x00*8 // Read
#define THR		0x00*8 // Write
#define IER		0x01*8 // R/W
#define FCR		0x02*8 // W
#define DLL		0x00*8
#define DLH		0x01*8
#define EFR		0x02*8
#define	IIR		0x02*8 // R
#define LCR		0x03*8 // R/W
#define MCR		0x04*8 // R/W
#define LSR		0x05*8 // R
#define MSR		0x06*8 // R
#define TCR		0x06*8
#define TLR		0x07*8 
#define IODIR   0x0A*8
#define IOSTATE 0x0B*8

#define GSMNOTDEFINED		0
#define GSMWAITSTATE		1
#define GSMRESETDELAY		2
#define GSMRESET			3
#define GSMWAITANSWER	    4
#define GSMIDLE				5
#define GSMERROR			6
#define GSMWAITREGISTER     7

#define GSMSENDAT				10
#define GSMSOFTRESET			11
#define GSMREVISION             12
#define GSMCTRLCHARFRAMING		13
#define GSMBAUDRATE				14	
#define GSMRTSCTS				15 	
#define GSMECHOOFF				16
#define GSMPIN                  17
#define GSMPINERF               18
#define GSMSIGNAL	   			19
#define GSMREGISTERED			20
#define GSMPROVIDER				21
#define GSMSMSTEXTFORMAT		22
#define GSMSETCHARACTERSET		23
#define GSMDELETEALLSMS			24	
#define GSMEXTENDEDERRORCODE    25

#define GSMSENDGSMNR            30	
#define GSMSENDSMSTEXT          31
#define GSMGETSMS               32

//
//  GSM - Modem "GSM2 click"
//
//  
void CGsm::InitSMSEmpf()
{
    int i = 0;
    while(i < m_iAnzSMSEmpf)
    {
        m_pSMSEmpf[i].SetState(0);
        i++;
    }
}

bool CGsm::Control(int iZykluszeit, bool bStundenTakt, bool bMinutenTakt)
{
	int iSend = 0, iPos, i, iEmpf, iLen;
    char buf[MSGSIZE];
    bool bTrue = true;
    bool bRet = false;
    
	switch(m_iState) {
	case GSMNOTDEFINED:
		switch(m_iSubState) {
		case GSMNOTDEFINED:
			// Resetausgang auf 1 setzen, der RESET-Eingang des GSM-Moduls
			// wird auf low gesetzt
			m_pUartI2C->ResetGSM(1);
			m_iSubState = GSMWAITSTATE;
			m_iSignal = 0;
			m_iRegistered = 0;
            m_strProvider.clear();
            m_strError.clear();
            m_strSMSCommand.clear();
			m_iTimeOut = 0;
            m_iAnzTimeOut = 0;
			break;
		case GSMWAITSTATE:
			// warten bis der Ausgang STAT des GSM-Moduls auf high geht
			if(m_pUartI2C->ReadState())
			{
				m_iTimeOut++;
				// 3 Sekunden warten
				if(m_iTimeOut > 3000/iZykluszeit)
				{   // Reset wieder deaktivieren
					m_pUartI2C->ResetGSM(0);
					m_iSubState = GSMRESETDELAY;
					m_iTimeOut = 0;
				}
			}
			break;
		case GSMRESETDELAY:
			m_iTimeOut++;
			// Bootzeit des GSM-Moduls abwarten (30 Sekunden)
			if(m_iTimeOut > 30000/iZykluszeit)
			{
				m_iSubState = GSMRESET;
				m_iTimeOut = 0;
			}
			break;
		case GSMRESET:
			// AT senden und auf OK warten
			iSend = sprintf((char *)m_chSend, "AT");
			m_iSubState = GSMSENDAT;
			m_iState = GSMWAITANSWER;
			break;
		default:
			m_iSubState = GSMNOTDEFINED;
			break;
		}
		break;
	case  GSMWAITANSWER:
		iPos = m_iEmpf;
		m_iEmpf = m_pUartI2C->ReadLen (m_chEmpf, m_iEmpf);
		if(SearchString((char *)m_chEmpf, (char *)"OK", m_iEmpf) || m_chEmpf[0] == '>')
		{
			switch(m_iSubState) {
			case GSMSENDAT:
				// Soft Reset
                // Get Factory settings
				iSend = sprintf((char *)m_chSend, "AT&F");
				m_iSubState = GSMSOFTRESET;
				break;
			case GSMSOFTRESET:
                // Version lesen
                iSend = sprintf((char *)m_chSend, "AT+GMR");
                m_iSubState = GSMREVISION;
                break;
            case GSMREVISION:
				// flow control RTS/CTS
				iSend = sprintf((char *)m_chSend, "AT+IFC=2,2");
				m_iSubState = GSMRTSCTS;
				break;
			case GSMRTSCTS:
				// echo off
				iSend = sprintf((char *)m_chSend, "ATE0");
				m_iSubState = GSMECHOOFF;
				break;
            case GSMECHOOFF:
                // Extended error code aktivieren
                iSend = sprintf((char *)m_chSend, "AT+CMEE=2");
                m_iSubState = GSMEXTENDEDERRORCODE;
				break; 
            case GSMEXTENDEDERRORCODE:
                // muss der Pincode übertragen werden
                iSend = sprintf((char *)m_chSend, "AT+CPIN?");
                m_iSubState = GSMPIN;
                break;
			case GSMPIN:
                iPos = SearchString((char *)m_chEmpf, (char *)"+CPIN: READY", m_iEmpf);
                if(iPos)
                {   // GSM Signal level (0-30)
                    iSend = sprintf((char *)m_chSend, "AT+CSQ");
                    m_iSubState = GSMSIGNAL;
                }
                else
                {
                    iSend = sprintf((char *)m_chSend , "AT+CPIN=%s", m_strPin.c_str());
                    m_iSubState = GSMPINERF;
                }
                break;
            case GSMPINERF:
                // GSM Signal level (0-30)
                iSend = sprintf((char *)m_chSend, "AT+CSQ");
                m_iSubState = GSMSIGNAL;               
                break;
			case GSMSIGNAL:
				// Signalstärke abspeichern
				iPos = SearchString((char *)m_chEmpf, (char *)"+CSQ:", m_iEmpf);
				if(iPos)
				{
					m_iSignal = atoi((char *)(&m_chEmpf[iPos]));
                    m_iState = GSMWAITREGISTER;
					m_iSubState = GSMREGISTERED;
                    iSend = sprintf((char *)m_chSend, (char *)"AT+CREG?");
				}
				else
				{
					m_iState = GSMNOTDEFINED;
					m_iSubState = GSMNOTDEFINED;
				}
				break;
			case GSMREGISTERED:
				// Registrierung abspeichern, 0=not, 1=home network
				// 2=not registered but searching, 3=denied
				// 4=unknown, 5=roaming
				iPos = SearchString((char *)m_chEmpf, (char *)"+CREG:", m_iEmpf);
				if(iPos)
				{
					iPos += SearchString((char *)(&m_chEmpf[iPos]), (char *)",", 
					                     m_iEmpf - iPos);
					m_iRegistered = atoi((char *)(&m_chEmpf[iPos]));
				}
				if(!iPos)
				{
					m_iState = GSMNOTDEFINED;
					m_iSubState = GSMNOTDEFINED;
				}
				else
				{
                    m_strError.clear();
                    switch(m_iRegistered) {
                        case 0:
                            m_strError = "GSM: No Provider";
                            m_iState = GSMERROR;
                            break;
                        case 1: // registered home network
                        case 5: // registered roaming
        					// provider name anfragen
                            m_iTimeOut = 0;
            				iSend = sprintf((char *)m_chSend, "AT+COPS?");
                			m_iSubState = GSMPROVIDER;
                            break;
                        case 2: // searching
                            m_iTimeOut = 0;
                            if(m_iAnzTimeOut++ > 10)
                            {
                                m_strError = "GSM waiting registration time out (3min)";
                                m_iState = GSMERROR;
                            }
                            else
                                m_iState = GSMWAITREGISTER;
                            break;
                        case 3: // registration denied
                            m_strError = "GSM: registration denied";
                            m_iState = GSMERROR;
                            break;
                        default:
                            m_strError = "GSM: don't find provider, unknown error";
                            m_iState = GSMERROR;
                            break;                        
					}
                    if(!m_strError.empty())
                        syslog(LOG_ERR, m_strError.c_str());
				}
				break;
			case GSMPROVIDER:
				iPos = SearchString((char *)m_chEmpf, (char *)"COPS:", m_iEmpf);
				if(iPos)
				{   int iLen;
					iPos += SearchString((char *)(&m_chEmpf[iPos]), (char *)"\"", 
					                     m_iEmpf - iPos);
					iLen = SearchString((char *)(&m_chEmpf[iPos]), (char *)"\"", 
					                    m_iEmpf - iPos);
					if(iLen)
                    {
                        m_chEmpf[iPos+iLen-1] = 0;
						m_strProvider = string((const char *)&m_chEmpf[iPos]);
                    }
					// SMS in text format
					iSend = sprintf((char *)m_chSend, "AT+CMGF=1");
					m_iSubState = GSMSMSTEXTFORMAT;
				}
				else
				{
					m_iState = GSMNOTDEFINED;
					m_iSubState = GSMNOTDEFINED;
				}
				break;
			case GSMSMSTEXTFORMAT:
				iSend = sprintf((char *)m_chSend, "AT+CSCS=\"GSM\"");
				m_iSubState = GSMSETCHARACTERSET;
				break;
			case GSMSETCHARACTERSET:
				// Alle SMS löschen
				iSend = sprintf((char *)m_chSend, "AT+CMGD=1,4");
				m_iSubState = GSMDELETEALLSMS;
				break;
			case GSMDELETEALLSMS:
				m_iState = GSMIDLE;
				break;
				// Senden der SMS
			case GSMSENDGSMNR:
				iSend = sprintf((char *)m_chSend, "%s", 
					                m_pSendSMS->m_strSMSText.c_str());
				m_chSend[iSend++] = 0x1A;
				m_iSubState = GSMSENDSMSTEXT;
				break;
			case GSMSENDSMSTEXT:
				m_pSendSMS = NULL;
                pthread_mutex_lock(&m_mutexGsmSendFifo);                 
				m_SendFifo.pop();
                pthread_mutex_unlock(&m_mutexGsmSendFifo); 
				m_iState = GSMIDLE;
				break;	
            case GSMGETSMS:
                if(m_iEmpf > 2)
                {    
                    iLen = sprintf((char *)m_chSend, "AT+CMGD=1,4"); // alle SMS löschen
                    iEmpf = m_iEmpf;
                    Send(iLen);
                    iPos = 0;
                    while(bTrue)
                    {
                        iPos += SearchString((char *)&m_chEmpf[iPos], (char *)"REC UNREAD", iEmpf);                
                        if(iPos)
                        {
                            for(i=0; iPos < iEmpf && i < 3; iPos++)
                            {
                                if(m_chEmpf[iPos] == ',')
                                    i++;
                            }
                            for(i=0; iPos < iEmpf && i < 2; iPos++)
                            {
                                if(m_chEmpf[iPos] == '"')
                                    i++;
                            }
                            if(iPos < iEmpf)
                            {
                                iLen = SearchString((char *)&m_chEmpf[iPos], (char *)"OK", iEmpf -iPos) - 2;
                                i = SearchString((char *)&m_chEmpf[iPos], (char *)"+CMGL", iEmpf - iPos) - 5;
                                if(i > 0 && i < iLen)
                                    iLen = i;
                                else
                                    bTrue = false;
                                if(iLen > 0)
                                {
                                    m_chEmpf[iPos+iLen] = 0;
                                    m_strSMSCommand = string((const char *)&m_chEmpf[iPos]);
                                    if(m_pSMSEmpf != NULL)
                                    {
                                        for(i=0; i < GetAnzSMSEmpf(); i++)
                                        {
                                            if(m_pSMSEmpf[i].CompareString(m_strSMSCommand))
                                            {
                                                m_pSMSEmpf[i].SetState(1);
                                                bRet = true;                                            
                                                break;
                                            }
                                        }
                                    }
                                } 
                                iPos += iLen;
                            }
                        }  
                        else
                            bTrue = false;
                    }
                    m_iSubState = GSMDELETEALLSMS;
                }
                else
                    m_iState = GSMIDLE;
                break;
			default:
				break;
			}
			break;
		}
   
		else if(SearchString((char *)m_chEmpf, (char *)"RRRRRRRRRR", m_iEmpf)
                || SearchString((char *)m_chEmpf, (char *)"ERROR", m_iEmpf)
                || SearchString((char *)m_chEmpf, (char *)"NORMAL POWER DOWN", m_iEmpf))
		{
            if(m_bFirst)
            {
                m_bFirst = false;
                m_iState = GSMNOTDEFINED;
                m_iSubState = GSMNOTDEFINED;
            }
            else
            {
                m_chEmpf[m_iEmpf] = 0;
                sprintf((char *)m_chSend, "GSM - ERROR:  %s", (char *)m_chEmpf);
                syslog(LOG_ERR, (char *)m_chSend);
                m_strError = string((char *)m_chEmpf, m_iEmpf);
                if(m_iSubState == GSMSENDGSMNR || m_iSubState == GSMSENDSMSTEXT)
                {
                    // falsche Handynummer oder Text, aus der Fifo-queue entfernen
                    pthread_mutex_lock(&m_mutexGsmSendFifo);                   
                    m_SendFifo.pop();
                    pthread_mutex_unlock(&m_mutexGsmSendFifo); 
                }
                m_iState = GSMERROR;
                m_iSubState = GSMNOTDEFINED;
            }
		}
		else if(m_iSubState == GSMSENDAT)
		{
			if((m_iTimeOut / 30)*30 == m_iTimeOut)
				iSend = sprintf((char *)m_chSend, "AT");
		}
				
		m_iTimeOut++;
		// die Timeoutzeit beträgt 300 sekunden 
		if(m_iTimeOut > 300000/iZykluszeit)
		{
			m_iState = GSMNOTDEFINED;
			m_iSubState = GSMNOTDEFINED;
			iSend = 0;
			syslog(LOG_ERR, "GSM Timeout error");
		}
		break;
	case GSMIDLE:
        if(bMinutenTakt)
		{
            if(bStundenTakt)
            {   // einmal pro Stunde wird das Signal, die Registrierung und der 
                // Provider eingelesen
                m_iState = GSMWAITANSWER;
                iSend = sprintf((char *)m_chSend, "AT+CSQ");
                m_iSubState = GSMSIGNAL;
            }
    		else
            {
                iSend = sprintf((char *)m_chSend, "AT+CMGL=\"ALL\"");
                m_iState = GSMWAITANSWER;
                m_iSubState = GSMGETSMS;
            }
        }
        else
		{
			// GSM ist registriert in einem GSM Netzwerk und wartet auf Senden
            pthread_mutex_lock(&m_mutexGsmSendFifo); 
			if(!m_SendFifo.empty())
			{
				m_pSendSMS = &m_SendFifo.front();
				m_iSubState = GSMSENDGSMNR;
				iSend = sprintf((char *)m_chSend, "AT+CMGS=\"%s\"", 
		            					m_pSendSMS->m_strGSM.c_str());
			}
            pthread_mutex_unlock(&m_mutexGsmSendFifo);             
		}
		break;
	case GSMERROR: // warten bis zur nächsten vollen Stunde
		if(bStundenTakt)
		{
			m_iState = GSMNOTDEFINED;
			m_iSubState = GSMNOTDEFINED;
		}
		break;
    case GSMWAITREGISTER:
        if(m_iTimeOut++ > 30000 / iZykluszeit)
        {
            iSend = sprintf((char *)m_chSend, "AT+CREG?");
            m_iState = GSMWAITANSWER;
        }
        break;
	default:
		m_iState = GSMNOTDEFINED;
		m_iSubState = GSMNOTDEFINED;
		break;
	}
	
	if(iSend)
        Send(iSend);
    
    return bRet;
}

//
//  SMS ist UTF-16
//
void CGsm::Send(int iLen)
{
    int i, j;
    
    for(i=0, j=0; i < iLen; i++,j++)
    {
        switch(m_chSend[i]) {
            case 0xC3:
                i++;
                switch(m_chSend[i]) {
                    case 0x84: //Ä
                        m_chSend[j] = 0xC4;
                        break;
                    case 0x9C: // Ü
                        m_chSend[j] = 0xDC;
                        break;
                    case 0x96: // Ö
                        m_chSend[j] = 0xD6;
                        break;
                    case 0xA4: // ä
                        m_chSend[j] = 0xE4;
                        break;
                    case 0xBC: // ü
                        m_chSend[j] = 0xFC;
                        break;
                    case 0xB6: // ö
                        m_chSend[j] = 0xF6;
                        break;
                    default:
                        m_chSend[j++] = 0xC3;
                        m_chSend[j] = m_chSend[i];
                        break;
                }
                break;
            case 0xC2:
                i++;
                switch(m_chSend[i]) {
                    case 0xB0:
                        m_chSend[j] = 0xA4; // °
                        break; 
                    default:
                        m_chSend[j++] = 0xC2;
                        m_chSend[j] = m_chSend[i];
                        break;                        
                }
                break;
            default:
                m_chSend[j] = m_chSend[i];
                break;
        }
    }
    m_chSend[j++] = 0x0D;
    m_chSend[j++] = 0x0A;
    m_iEmpf = 0;
    m_pUartI2C->SendLen(m_chSend, j);
    m_iState = GSMWAITANSWER;
    if(j > 4)
        m_iTimeOut = 0;  
}

int CGsm::InsertSMS(int iNr, int iText)
{
	CSendSMS sms;
    int iSize;
	sms.m_strGSM = m_pNummern[iNr-1];
	sms.m_strSMSText = m_pSMSTexte[iText-1].GetString();
    pthread_mutex_lock(&m_mutexGsmSendFifo);     
	m_SendFifo.push(sms);
    iSize = m_SendFifo.size();    
    pthread_mutex_unlock(&m_mutexGsmSendFifo);  
	return iSize;
}

CGsm::CGsm()
{
	m_pUartI2C = new CUartI2C;
	m_iState = GSMNOTDEFINED;
	m_iSubState = GSMNOTDEFINED;
	m_pSendSMS = NULL;	
	m_pNummern = NULL;
	m_pSMSTexte = NULL;
    m_pSMSEmpf = NULL;
	m_iSignal = 0;
	m_iRegistered = 0;
    m_strError.clear();
    m_strProvider.clear();
    m_strSMSCommand.clear();
    m_strPin.clear();
    m_bFirst = true;
    pthread_mutex_init(&m_mutexGsmSendFifo, NULL);  
}

CGsm::~CGsm()
{
    pthread_mutex_destroy(&m_mutexGsmSendFifo); 
   
	if(m_pUartI2C != NULL) {
		delete m_pUartI2C;
        m_pUartI2C = NULL;
    }
	if(m_pNummern != NULL) {
		delete [] m_pNummern;
        m_pNummern = NULL;
    }
	if(m_pSMSTexte != NULL) {
		delete [] m_pSMSTexte;
        m_pSMSTexte = NULL;
    }
    if(m_pSMSEmpf != NULL)
    {
        delete [] m_pSMSEmpf;
        m_pSMSEmpf = NULL;
    }
}

void CGsm::LesDatei(char *pProgramPath, char * pIO)
{
	string str;
	int iCase = 0, i, j, iParam, iPos, iAnz, iPosParam;
    char buf[MSGSIZE];
	CReadFile *pReadFile;
    CConfigCalculator *pcc;
	COperBase *pOperBase;
    CIOGroup * pIOGroup;
    char ch;
    
    pIOGroup = (CIOGroup *)pIO;
	pReadFile = new CReadFile;
	pReadFile->OpenRead (pProgramPath, 4);
	m_iAnzNummern = 0;
	m_iAnzTexte = 0;
    m_iAnzSMSEmpf = 0;
	
	for(;;)
	{	
		if(pReadFile->ReadLine())
		{
			str = pReadFile->ReadAlpha ();
			if(str.compare("Nummern") == 0)
			{
				iCase = 1;
				continue;
			}
			else if(str.compare("Texte") == 0)
			{
				iCase = 2;
				continue;
			}
            else if(str.compare("Kommandos") == 0)
            {
                iCase = 3;
                continue;
            }
			switch(iCase) {
			case 1:
				m_iAnzNummern++;
				break;
			case 2:
				m_iAnzTexte++;
				break;
            case 3:
                m_iAnzSMSEmpf++;
                break;
			default:
				pReadFile->Error(43);
				break; 
			}
		}
		else
			break;
	}
	pReadFile->Close();
	if(m_iAnzNummern)
		m_pNummern = new string[m_iAnzNummern];
	else
		pReadFile->Error(44);

	if(m_iAnzTexte)
		m_pSMSTexte = new CSMSText[m_iAnzTexte];
	else
		pReadFile->Error(45);
	
    if(m_iAnzSMSEmpf)
        m_pSMSEmpf = new CSMSEmpf[m_iAnzSMSEmpf];
    
	iCase = 0;
	pReadFile->OpenRead (pProgramPath, 4);
	for(;;)
	{	
		if(pReadFile->ReadLine())
		{
			str = pReadFile->ReadAlpha ();
			if(str.compare("Nummern") == 0)
			{
				iCase = 1;
				i = 0;
				continue;
			}
			else if(str.compare("Texte") == 0)
			{
				iCase = 2;
				i = 0;
				continue;
			}
            else if(str.compare("Kommandos") == 0)
            {
                iCase = 3;
                i = 0;
                continue;
            }
			pReadFile->ResetLine ();
			switch(iCase) {
			case 1:
				str = pReadFile->ReadGsmNr();
				m_pNummern[i] = str;
				i++;
				break;
			case 2:
				str = pReadFile->ReadText(';');
				m_pSMSTexte[i].SetString(str);
                for(iPos = 0, iParam = 0; iPos != string::npos ;)
                {
                    iPos = str.find("#", iPos+1);
                    if(iPos != string::npos)
                        iParam++;
                }  
                if(iParam)
                {
                    m_pSMSTexte[i].Init(iParam);
                    pReadFile->ReadSeparator();                    
                    
                    for(iPos=0, iPosParam=0 ; iPos < iParam; iPos++)
                    {
                        iPosParam = str.find("#", iPosParam+1);
                        ch = str[iPosParam+1];
                        if(ch != 's' && ch != 'd')
                            pReadFile->Error(107);
                        pcc = new CConfigCalculator(pReadFile);
                        pReadFile->ReadBuf(buf, ';');
                        if(!strlen(buf))
                            pReadFile->Error(106);
                        pcc->eval(buf, pIOGroup, 1);
                        iAnz = pcc->GetAnzOper ();
                        if(iAnz > 2)
                            pReadFile->Error(107);
                        pOperBase = new COperBase;
                        pOperBase = pcc->GetOper(0);
                        m_pSMSTexte[i].SetOper(iPos, pOperBase, ch);
                        delete pcc;
                        pcc = NULL; 
                    }
                }
				i++;
				break;
            case 3:
                str = pReadFile->ReadText(';');
                m_pSMSEmpf[i].SetString(str);
                i++;
                break;
			default:
				pReadFile->Error(43);
				break; 
			}
		}
		else
			break;
	}
	pReadFile->Close();
	
	delete pReadFile;
}

int CGsm::GetAnzNummern()
{
    return m_iAnzNummern;
}

int CGsm::GetAnzTexte()
{
    return m_iAnzTexte;
}

int CGsm::Init(int baudrate, int bits, int stops, char parity, string strPin)
{
    m_strPin = strPin;
    return m_pUartI2C->Init(baudrate, bits, stops, parity);
}

void CGsm::SetAddr(int Inh1, int Addr2, int Inh2, int Addr3, int Reg)
{
    m_pUartI2C->SetAddr(Inh1, Addr2, Inh2, Addr3, Reg);
}

int CGsm::SearchString(char * cIn, char *cWas, int iLen)
{
	int iRet = 0;
	int i, j;
	int lenWas = strlen(cWas);
	int len = iLen - lenWas;
	
	for(i=0; i <= len ; i++)
	{
		for(j=0; j < lenWas; j++)
		{
			if(*(cIn + i + j) != *(cWas + j))
				break;
		}
		if(j == lenWas)
		{
			iRet = i + j;
			break;
		}
	}

	return iRet;
}
int CGsm::GetSignal()
{
	return m_iSignal;
}
string CGsm::GetRegistered()
{
	string str;
	
	switch(m_iRegistered) {
	case 0:
		str = "not registered";
		break;
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	case 5:
		break;
	default:
		break;
	}

	return str;
}
string CGsm::GetProvider()
{
	return m_strProvider;
}

string CGsm::GetError()
{
    int len;
    
	static string noError = "no error";
	if(!m_strError.empty())
	{
        len = m_strError.length();
        m_strError.replace(0, len, '"', ' ');
        m_strError.replace(0, len, '\'', ' ');
        return m_strError;
    }
    else
		return noError;
}

int CGsm::GetAnzSMSEmpf()
{
    return m_iAnzSMSEmpf;
}

CSMSEmpf * CGsm::GetAddressSMSEmpf(int nr)
{
    if(m_pSMSEmpf != NULL)
        return &m_pSMSEmpf[nr-1];
    else
        return NULL;
}
	
//
//  UART - Schnittstelle am I2C-Bus mit dem Breakout-board SC16IS750
//
int CUartI2C::ReadLen(unsigned char *ptr, int iPos)
{
	int ch;
	
	m_pBoardAddr->setI2C_gpio();
	for(; iPos < MAXGSM-1; )
	{   
		ch = BSC_ReadReg (1, m_pBoardAddr->Addr3, LSR) & 0x9F; 
		if(!ch)
			break;
		else // Zeichen empfangen (!! mit oder ohne Fehler !!)
		{	*(ptr + iPos) = (unsigned char)BSC_ReadReg (1, m_pBoardAddr->Addr3, RHR);
			if(*(ptr+iPos) != 0x0D && *(ptr+iPos) != 0x0A)
				iPos++;
		}
	}
	*(ptr + iPos) = 0;
	return iPos;
}

void CUartI2C::SendLen(unsigned char *ptr, int len)
{
	int i;
	
	m_pBoardAddr->setI2C_gpio();
	//BSC_WriteString(len, m_pBoardAddr->Addr3, ptr, THR);
	for(i=0; i < len; i++)
		BSC_WriteReg (1, m_pBoardAddr->Addr3, *(ptr+i), THR);
}

CUartI2C::CUartI2C()
{
	m_iState = NOTDEFINED;
	m_iError = NOERROR;
	m_pBoardAddr = new CBoardAddr;
}

CUartI2C::~CUartI2C()
{
	if(m_pBoardAddr != NULL)
		delete m_pBoardAddr;
}

void CUartI2C::SetAddr(int Inh1, int Addr2, int Inh2, int Addr3, int Reg)
{
    m_pBoardAddr->SetAddr(Inh1, Addr2, Inh2, Addr3, Reg);
}

int CUartI2C::Init(int baudrate, int bits, int stops, char parity)
{
    int ret, div, iRet = 1;
    
    m_pBoardAddr->setI2C_gpio();

    // LCR[7] = 1, Divisor latch enable
    BSC_WriteReg(1, m_pBoardAddr->Addr3, 0x80, LCR);
    ret = BSC_ReadReg (1, m_pBoardAddr->Addr3, LCR);
   
    // Crystal 14.7456 MHz --> 921600
    // Crystal 3.6864 MHz --> 230400
    // Divisor
    if(baudrate == 2400 || baudrate == 4800 || baudrate == 9600 
                    || baudrate == 19200 || baudrate == 38400 || baudrate == 57600
                    || baudrate == 115200)
    {   div = 230400 / baudrate;
        BSC_WriteReg(1, m_pBoardAddr->Addr3, div%256, DLL);
        BSC_WriteReg(1, m_pBoardAddr->Addr3, div/256, DLH);
        ret = BSC_ReadReg(1, m_pBoardAddr->Addr3, DLH) * 256;
        ret += BSC_ReadReg(1, m_pBoardAddr->Addr3, DLL);
        if(ret != div)
        {
            syslog(LOG_ERR, "GSM-board address not response!");
            return 0;
        }             
        m_iBaud = baudrate;
    }
    else
    {   syslog(LOG_ERR, "Unknown baudrate for GSM-communication");
        return 0;
    }

    // LCR Line Control Register
    // EFR frei schalten mit 0xBF
    BSC_WriteReg(1, m_pBoardAddr->Addr3, 0xBF, LCR);
    
    // EFR : Enhanced Feature Register
    // EFR, Auto CRS und RTS und enable enhanced functions
    // Auto CTS und RTS aktiviert 
    BSC_WriteReg(1, m_pBoardAddr->Addr3, 0xD0, EFR);
    
    // IO0-6 als Ausgang IO7 als Eingang für STAT
    BSC_WriteReg(1, m_pBoardAddr->Addr3, 0x7F, IODIR);
    BSC_WriteReg(1, m_pBoardAddr->Addr3, 0x00, IOSTATE);

    ret = 0;
    m_cParity = parity;
    switch(parity) {
    case 'N':
            break;
    case 'E':
            ret = 0x18;
            break;
    case 'O':
            ret = 0x08;
            break;
    default:
            iRet = 0;
            syslog(LOG_ERR, "Unknown parity for GSM-communication");
            break;
    }
    if(ret && stops == 2)
    {   
            iRet = 0;
            syslog(LOG_ERR, "2 stopbits not possible with parity");
    }
    m_iStops = stops;
    if(stops == 2)
            ret += 0x04;
    m_iBits = bits;
    switch(bits) {
    case 8:
            ret += 0x03;
            break;
    case 7:
            ret += 0x02;
            break;
    default:
            iRet = 0;
            syslog(LOG_ERR, "Only 7 or 8 data bits are possible");
            break;
    }
    // 
    BSC_WriteReg(1, m_pBoardAddr->Addr3, ret, LCR);

    // FCR  7:6 = 11 RX FIFO Trigger 60 Characters
    //		5:4 = 11 TX FIFO Trigger 56 Spaces
    //		2 = 1 Reset TX FIFO
    //		1 = 1 Reset RX FIFO
    //		0 = 1 Enable FIFO
    BSC_WriteReg(1, m_pBoardAddr->Addr3, 0xF7, FCR);

    // MCR Modem Control Register
    // Enable TCR and TLR
    BSC_WriteReg(1, m_pBoardAddr->Addr3, 0x04, MCR);

    // TCR Transmission Control Register
    // RX FIFO trigger to halt transmission 60=0x3C/4= F
    // RX FIFO trigger level to resume 32=0X20/4 = 8
    BSC_WriteReg(1, m_pBoardAddr->Addr3, 0x8F, TCR);
    
    // TLR Trigger Level Register
    // transmit and receive FIFO trigger levels 
    // IF TLR are null, FCR are used
    BSC_WriteReg(1, m_pBoardAddr->Addr3, 0x00, TLR);

    // IER Enable no Sleep mode, no interrupt
    BSC_WriteReg(1, m_pBoardAddr->Addr3, 0x00, IER);

    // LSR Line Status Register
    BSC_ReadReg (1, m_pBoardAddr->Addr3, LSR);

    return iRet;
}

void CUartI2C::ResetGSM(int state)
{
	int ret;
	
	m_pBoardAddr->setI2C_gpio();
	ret = BSC_ReadReg(1, m_pBoardAddr->Addr3, IOSTATE);
	if(state)
		ret = 0x01;
	else
		ret = 0x00;
	
	BSC_WriteReg(1, m_pBoardAddr->Addr3, ret, IOSTATE);
 }

bool CUartI2C::ReadState()
{
	bool bRet = false;
	int ret;
	
	m_pBoardAddr->setI2C_gpio();
	ret = BSC_ReadReg(1, m_pBoardAddr->Addr3, IOSTATE);
	if(ret & 0x80)
		bRet = true;
	return bRet;
}

CSMSText::CSMSText()
{
    m_strText.clear();
    m_pOper = NULL;
    m_pOperTyp = NULL;
}

CSMSText::~CSMSText()
{
    if(m_pOper != NULL)
    {
        delete [] (COperBase *)m_pOper;
        m_pOper= NULL;
    }
    if(m_pOperTyp != NULL)
    {
        delete [] m_pOperTyp;
        m_pOperTyp = NULL;
    }
}

string CSMSText::GetString()
{
    string str, strHelp;
    int iParam, iPos, iStart, iIdx;
    
    str.clear();
    for(iPos = 0, iParam = 0, iIdx = 0; iPos != string::npos ; iIdx++)
    {
        iStart = iPos;
        iPos = m_strText.find("#", iPos+1);
        if(iPos == string::npos)
        {    
            str += m_strText.substr(iStart, m_strText.length() - iStart);
            break;
        }
        str += m_strText.substr(iStart, iPos - iStart);
        switch(m_pOperTyp[iIdx]) {
            case 'd':
                strHelp = to_string(m_pOper[iIdx]->resultInt());
                break;
            case 's':
                strHelp = m_pOper[iIdx]->resultString();
                break;
            default:
                strHelp.clear();
                break;
        }
        if(strHelp.empty())
            strHelp = "#";
        str = str + strHelp;
        iPos += 2;
    }    
    
    return str;
}

void CSMSText::SetString(string str)
{
    m_strText = str;
}

void CSMSText::Init(int iAnz)
{
    m_pOper = new COperBase* [iAnz];
    m_pOperTyp = new char[iAnz];
}

void CSMSText::SetOper(int iIdx, COperBase *pOper, char ch)
{
    m_pOper[iIdx] = pOper;
    m_pOperTyp[iIdx] = ch;
}

//
//  CBerechneGSM
//
CBerechneGSM::CBerechneGSM()
{
    m_pGsm = NULL;
}

void CBerechneGSM::init(CGsm *pGsm)
{
    m_pGsm = pGsm;
}

void CBerechneGSM::SetState(int state)
{
    int iNrTel, iNrText;
    iNrTel = state / 1000;
    iNrText = state % 1000;
    if(state && m_pGsm != NULL)
        m_pGsm->InsertSMS (iNrTel, iNrText);
}

//
//  CSMSEmpf
//
CSMSEmpf::CSMSEmpf()
{
    m_iState = 0;
    m_strCommand.clear();
}

int CSMSEmpf::GetState()
{
    return m_iState;
}
void CSMSEmpf::SetState(int i)
{
    m_iState = i;
}
void CSMSEmpf::SetString(string str)
{
    m_strCommand = str;
}
bool CSMSEmpf::CompareString(string str)
{
    bool bRet = true;
    if(m_strCommand.compare(str))
        bRet = false;
    return bRet;
}
//
COperSMS::COperSMS()
{
    m_pSMSEmpf = NULL;
} 

int COperSMS::resultInt()
{
    int iRet;
    
    if(m_pSMSEmpf != NULL)
        iRet = m_pSMSEmpf->GetState();
    else
        iRet = 0;
    return iRet;
}  
 
void COperSMS::SetOper(CSMSEmpf *pPtr)
{   
    m_pSMSEmpf = pPtr;
}