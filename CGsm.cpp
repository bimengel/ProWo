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

/*
    Das Prinzip ist wenn ein Fehler entsteht, dann wird 
    m_iError = 1 gesetzt und nach einer Minute wird das AT-Kommando getestet, ist dies auch noch
    nicht erfolgreich 
    m_iError=2 Fehler wird von dem Modul gesendet, AT nach einer Stunde
    m_iError=3 Hardware Reset
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
#define GSMERROR            6
#define GSMWAITREGISTER     7

#define GSMSENDAT				10
#define GSMSOFTRESET			11
#define GSMDISABLEURC           12
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
#define GSMMESSAGEINDICATION    24
#define GSMDELETEALLSMS			25	
#define GSMEXTENDEDERRORCODE    26

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
	int iSend = 0, iPos, i, iLen;
    char buf[MSGSIZE];
    bool bRet = false, bRepeat = false;
    string strHelp;

	switch(m_iState) {
	case GSMNOTDEFINED:
		switch(m_iSubState) {
		case GSMNOTDEFINED:
			// Resetausgang auf 1 setzen, der RESET-Eingang des GSM-Moduls
			// wird auf low gesetzt
			m_pUartI2C->ResetGSM(0);
			m_iSubState = GSMWAITSTATE;
			m_iSignal = 0;
			m_iRegistered = 0;
            m_strProvider.clear();
            m_strError.clear();
            m_strSMSCommand.clear();
			m_iTimeOut = 0;
            m_iAnzTimeOut = 0;
            m_iAnzTimeOutCREG = 0;
            m_bFirst = true;
			break;
		case GSMWAITSTATE:
            m_iTimeOut++;
			// warten bis der Ausgang STAT des GSM-Moduls auf high geht
			if(m_pUartI2C->ReadState())
			{
				// 3 Sekunden warten
				if(m_iTimeOut > 3000/iZykluszeit)
				{   // Reset wieder deaktivieren
 					m_pUartI2C->ResetGSM(1);
					m_iSubState = GSMRESETDELAY;
					m_iTimeOut = 0;
				}
			}
            else
            {   // nach 3 Minuten
                if(m_iTimeOut > 180000/iZykluszeit)
                {
                    m_strError = "GSM Error: hardware reset not successful";
                    m_iError = 3;
                    m_iState = GSMERROR;
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
            m_iEmpf = 0;
            m_pUartI2C->ReadLen (m_chEmpf, m_iEmpf, m_bTest);
			iSend = sprintf((char *)m_chSend, "AT+CFUN=1");
			m_iSubState = GSMSENDAT;
			m_iState = GSMWAITANSWER;
			break;
		default:
            m_strError = "GSM error: sub state not defined!";
            syslog(LOG_ERR, m_strError.c_str());
			m_iState = GSMERROR;
			break;
		}
		break;     
	case  GSMWAITANSWER:      
        m_iEmpf = m_pUartI2C->ReadLen (m_chEmpf, m_iEmpf, m_bTest);
        if(m_iEmpf)
        {
            if(SearchString((char *)m_chEmpf, (char *)"ERROR", m_iEmpf))       
            {
                m_strError = string((char *)m_chEmpf, m_iEmpf);
                if(m_iSubState == GSMSENDSMSTEXT || m_iSubState == GSMSENDGSMNR)
                {
                    // falsche Handynummer oder Text, aus der Fifo-queue entfernen
                    pthread_mutex_lock(&m_mutexGsmSendFifo);                   
                    m_SendFifo.pop();
                    pthread_mutex_unlock(&m_mutexGsmSendFifo); 
                }
                if(SearchString((char *)m_chEmpf, (char *)"+CMS", m_iEmpf)) // Fehler bei SMS senden oder empfangen
                {
                    if(SearchString((char *)m_chEmpf, (char *)"Short message transfer rejected", m_iEmpf))             
                        m_strError += ". Karte bitte neu aufladen!";
                    m_iState = GSMIDLE;
                }
                else
                {   
                    m_iState = GSMERROR;
                    m_iError = 2;
                }
                syslog(LOG_ERR, m_strError.c_str());                
                m_iSubState = GSMNOTDEFINED;
            }             
            if(SearchString((char *)m_chEmpf, (char *)"OK", m_iEmpf) || m_chEmpf[0] == '>')
            {
                m_iAnzTimeOut = 0;
                switch(m_iSubState) {
                case GSMSENDAT:
                    // Get Factory settings
                    iSend = sprintf((char *)m_chSend, "AT&F");
                    m_iSubState = GSMSOFTRESET;
                    break;
                case GSMSOFTRESET:
                    // URC disable (Unsolicitated Result)
                    iSend = sprintf((char *)m_chSend, "AT+QIURC=0");
                    m_iSubState = GSMDISABLEURC;
                    break;
                case GSMDISABLEURC:
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
                    iSend = sprintf((char *)m_chSend , "AT+CPIN?");
                    m_iSubState = GSMPIN;
                    break;
                case GSMPIN:
                    iPos = SearchString((char *)m_chEmpf, (char *)"+CPIN: READY", m_iEmpf);
                    if(iPos)
                    {   // GSM Signal level (0-30)
                        iSend = sprintf((char *)m_chSend, "AT+CSQ");
                        m_iSubState = GSMSIGNAL;
                        break;
                    }
                    else
                    {
                        iPos = SearchString((char *)m_chEmpf, (char *)"+CPIN: SIM", m_iEmpf);
                        if(iPos)
                        {
                            iSend = sprintf((char *)m_chSend , "AT+CPIN=%s", m_strPin.c_str());
                            m_iSubState = GSMPINERF;
                        }
                        else
                            bRepeat = true;
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
                    }
                    else
                        bRepeat = true;
                    break;
                case GSMREGISTERED:
                    // Registrierung abspeichern, 0=not, 1=home network
                    // 2=not registered but searching, 3=denied
                    // 4=unknown, 5=roaming
                    iPos = SearchString((char *)m_chEmpf, (char *)"+CREG:", m_iEmpf);
                    if(iPos)
                    {
                        iPos += SearchString((char *)(&m_chEmpf[iPos]), (char *)",", m_iEmpf - iPos);
                        m_iRegistered = atoi((char *)(&m_chEmpf[iPos]));
                        strHelp.clear();
                        switch(m_iRegistered) {
                            case 0:
                                strHelp = "GSM: No Provider";
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
                                if(m_iAnzTimeOutCREG++ > 10)
                                    strHelp = "GSM waiting registration time out (3min)";
                                else
                                    m_iState = GSMWAITREGISTER;
                                break;
                            case 3: // registration denied
                                strHelp = "GSM: registration denied";
                                break;
                            default:
                                strHelp = "GSM: don't find provider, unknown error";
                                break;                        
                        }
                        if(!strHelp.empty())
                        {
                            m_strError = strHelp;
                            syslog(LOG_ERR, m_strError.c_str());
                            m_iState = GSMERROR;
                            m_iError = 1;
                        }
                    }
                    else
                        bRepeat = true;               
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
                            m_iRepeat = 0;
                        }
                        // SMS in text format
                        iSend = sprintf((char *)m_chSend, "AT+CMGF=1");
                        m_iSubState = GSMSMSTEXTFORMAT;
                    }
                    else
                        bRepeat = true;   
                    break;
                case GSMSMSTEXTFORMAT:
                    iSend = sprintf((char *)m_chSend, "AT+CSCS=\"8859-1\""); // war GSM
                    m_iSubState = GSMSETCHARACTERSET;
                    break;
                case GSMSETCHARACTERSET:
                    iSend = sprintf((char *)m_chSend, "AT+CNMI=2,0");                
                    m_iSubState = GSMMESSAGEINDICATION;
                    break;
                case GSMMESSAGEINDICATION:
                    if(m_bFirst)
                    {   
                        m_bFirst = false;
                        iSend = sprintf((char *)m_chSend, "AT+CMGD=1,3");
                        m_iSubState = GSMDELETEALLSMS;
                    }
                    else
                    {
                        m_iState = GSMIDLE;
                        m_strError.clear();
                        m_iError = 0;                    
                    }
                    break;
                case GSMDELETEALLSMS:
                    // dieser Punkt wird immer durchlaufen nach einem Fehler oder Neustart
                    m_iState = GSMIDLE;
                    m_strError.clear();
                    m_iError = 0;
                    break;
                case GSMSENDGSMNR:
                    iSend = sprintf((char *)m_chSend, "%s", 
                                        m_pSendSMS->m_strSMSText.c_str());               
                    m_chSend[iSend++] = 0x1A;
                    m_iSubState = GSMSENDSMSTEXT;
                    break;
                case GSMSENDSMSTEXT: // SMS ist versendet worden
                    m_pSendSMS = NULL;
                    pthread_mutex_lock(&m_mutexGsmSendFifo);                 
                    m_SendFifo.pop();
                    pthread_mutex_unlock(&m_mutexGsmSendFifo); 
                    m_iState = GSMIDLE;
                    break;	
                case GSMGETSMS:
                    // die Anfrage nach SMS ist gemacht worden
                    if(GetSMS())
                    {   // SMS ist empfangen worden, Kommando oder auch nicht
                        // SMS löschen senden
                        iSend = sprintf((char *)m_chSend, "AT+CMGD=1,3");
                        m_iSubState = GSMDELETEALLSMS;                        
                    }
                    else
                        m_iState = GSMIDLE;
                    break;
                default:
                    break;
                }
                if(bRepeat)
                {
                    if(m_iRepeat++ > 3)
                    {
                        m_iState = GSMERROR;
                        m_iSubState = GSMERROR;
                        m_iError = 1;
                        m_strError = "GSM Error: to much repeat";
                    }
                    else
                        iSend = m_iLastSend;
                }
                else
                    m_iRepeat = 0;
                m_iEmpf = 0;
                break;
            } 
            if(SearchString((char *)m_chEmpf, (char *)"NORMAL POWER DOWN", m_iEmpf))
            {
                m_iSubState = GSMRESET;
                m_iState = GSMNOTDEFINED;               
            }
            // es wird auf Antwort gewartet aber keine empfangen 
            // die Timeoutzeit beträgt 3 Minuten
            if(m_iTimeOut++ > 180000/iZykluszeit)
            {
                m_iAnzTimeOut++;
                m_iState = GSMERROR;
                m_iSubState = GSMERROR;
                m_iError = 1;
                m_strError = "GSM Error: timeout no answer";
                syslog(LOG_ERR, m_strError.c_str());
            }
        }            
        else
        {   if(m_iTimeOut++ > 180000/iZykluszeit) // 3 Minuten
            {
                m_iAnzTimeOut++;
                m_iState = GSMERROR;
                m_iSubState = GSMERROR;
                m_iError = 1;
                m_strError = "GSM Error: timeout OK missed";
                syslog(LOG_ERR, m_strError.c_str());
            }
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
            {   // einmal pro Minute wird kontrolliert ob ein SMS empfangen wurde
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
                m_iState = GSMWAITANSWER;
				iSend = sprintf((char *)m_chSend, "AT+CMGS=\"%s\",", 
		            					m_pSendSMS->m_strGSM.c_str());
                if(m_pSendSMS->m_strGSM.at(0) == '+')
                    iSend += sprintf((char *)&m_chSend[iSend], "145");
                else
                    iSend += sprintf((char *)&m_chSend[iSend], "129"); 
                iSend += sprintf((char *)&m_chSend[iSend], "\r");                
			}
            pthread_mutex_unlock(&m_mutexGsmSendFifo);             
		}
		break;
	case GSMERROR:  
        if(bMinutenTakt)                 
        {   
            switch(m_iError) {
            case 1: // leichter Fehler mit AT-Kommando starten
                if(m_iAnzTimeOut > 2)
                {   
                    m_iState = GSMNOTDEFINED;
                    m_iSubState = GSMNOTDEFINED; 
                }
                else
                {
                    m_iState = GSMNOTDEFINED;
                    m_iSubState = GSMRESET; 
                }                
                break;
            case 2: // Verbindung ist ok, wird aber ein Fehler gemeldet
                    // CME ERROR, bei CMS Fehler wird m_iState nicht auf GSMERROR gesetzt
                if(bStundenTakt)
                {
                    m_iState = GSMNOTDEFINED;
                    m_iSubState = GSMRESET;                
                }
                break;
            case 3: // Hardware Reset
                if(bStundenTakt)
                {
                    m_iState = GSMNOTDEFINED;
                    m_iSubState = GSMNOTDEFINED;                    
                }            
            default:
                m_iState = GSMNOTDEFINED;
                m_iSubState = GSMNOTDEFINED;
                break;
            }
            break;
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
        m_strError = "GSM error state not defined!";
        syslog(LOG_ERR, m_strError.c_str());
        m_iState = GSMERROR;
        m_iError = 1;
		break;
	}	
	if(iSend)
    {
        Send(iSend);
        m_iLastSend = iSend;
    }
    
    return bRet;
}

//
// gibt true zurück wenn eine SMS empfangen wurde
// ob ein Kommando oder nicht
//
bool CGsm::GetSMS()
{
    int i, iLen, iPos, iPosNeu;
    bool bTrue = true;
    bool bRet = false;              
    string str;

    if(m_iEmpf > 2)
    {    
        iPos = 0;
        while(bTrue)
        {
            iPosNeu = SearchString((char *)&m_chEmpf[iPos], (char *)"REC UNREAD", m_iEmpf - iPos);                
            if(iPosNeu)
            {   
                iPos += iPosNeu;
                bRet = true;
                for(i=0; iPos < m_iEmpf && i < 3; iPos++)
                {
                    if(m_chEmpf[iPos] == ',')
                        i++;
                }
                for(i=0; iPos < m_iEmpf && i < 2; iPos++)
                {
                    if(m_chEmpf[iPos] == '"')
                        i++;
                }
                if(iPos < m_iEmpf)
                {
                    iLen = SearchString((char *)&m_chEmpf[iPos], (char *)"OK", m_iEmpf -iPos) - 2;
                    i = SearchString((char *)&m_chEmpf[iPos], (char *)"+CMGL", m_iEmpf - iPos) - 5;
                    if(i > 0 && i < iLen)
                        iLen = i;
                    else
                        bTrue = false;
                    if(iLen > 0)
                    {
                        m_chEmpf[iPos+iLen] = 0;
                        m_strSMSCommand = string((const char *)&m_chEmpf[iPos]);
                        m_bIsLastSMS = true;
                        if(m_pSMSEmpf != NULL)
                        {
                            for(i=0; i < GetAnzSMSEmpf(); i++)
                            {
                                if(m_pSMSEmpf[i].CompareString(m_strSMSCommand))
                                {
                                    m_pSMSEmpf[i].SetState(1);                                          
                                    break;
                                }
                            }
                        }
                        str = "ProWo Received SMS: " + m_strSMSCommand;
                        syslog(LOG_INFO, str.c_str());
                    } 
                    iPos += iLen;
                }
            }  
            else
                bTrue = false;
        }
    }
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
    if(m_chSend[j-1] != 0x0D && m_chSend[j-1] != 0x1A) // nach dem Senden der SMS-Nummer und des Textes
    {
        m_chSend[j++] = 0x0D;
        m_chSend[j++] = 0x0A;
    }
    m_iEmpf = 0;
    m_pUartI2C->SendLen(m_chSend, j, m_bTest);
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
    m_iError = 0;
    m_iRepeat = 0;
    m_bFirst = true;
    m_bTest = false;
    m_bIsLastSMS = false;
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
	int iCase = 0, i;
	CReadFile *pReadFile;
    CIOGroup * pIOGroup;
    char ch;
    
    pIOGroup = (CIOGroup *)pIO;
	pReadFile = new CReadFile;
	pReadFile->OpenRead (pProgramPath, 4);
	m_iAnzNummern = 0;
	m_iAnzTexte = 0;
    m_iAnzSMSEmpf = 0;
	
    if(pIOGroup->GetTest() == 6)
        m_bTest = true;
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
		m_pSMSTexte = new CFormatText[m_iAnzTexte];
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
                pIOGroup->SetFormatText(&m_pSMSTexte[i], pReadFile);
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

int CGsm::GetError()
{
    int iRet;

    if(m_strError.empty())
        iRet = 0;
    else
        iRet = 1;
    return iRet;
}
string CGsm::GetErrorString()
{
    int len, i;
    string noError = "no error";
	
    if(!m_strError.empty())
	{
        noError = m_strError;
        len = noError.length();
        for(i=0; i < len; i++)
        {
            if(noError[i] == '"')
                noError[i] = ' ';
            if(noError[i] == 134) // Backslash 
                noError[i] = ' ';
        }
    }
	return noError;
}
string CGsm::GetLastSMS()
{
    return m_strSMSCommand;
}

int CGsm::IsLastSMS()
{
    int i = (int) m_bIsLastSMS;
    m_bIsLastSMS = false;
    return i;
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
int CUartI2C::ReadLen(unsigned char *ptr, int iPos, bool bTest)
{
	int ch, iPosOrg;

    iPosOrg = iPos;
    string str;
	m_pBoardAddr->setI2C_gpio();
	for(; iPos < MAXGSM-1; )
	{   
		ch = BSC_ReadReg (1, m_pBoardAddr->Addr3, LSR) & 0x9F; 
		if(!ch)
			break;
		else // Zeichen empfangen (!! mit oder ohne Fehler !!)
		{	if(ch & 0x8E)
            // 20250722 Es steht ein Fehler an (Holzheim)
            {   str = "GSM Empfang Fehler (LSR Register): " + to_string(ch);
                syslog(LOG_ERR, str.c_str());
            }
            ch = BSC_ReadReg (1, m_pBoardAddr->Addr3, RHR);
			if(ch != 0x0D && ch != 0x0A)
            {
                if(!isascii(ch) || ch <= 0x20) // Wert befindet sich zwischen 32 und 127
                    ch = ' ';
                *(ptr + iPos) = ch;
                iPos++;
            }
		} 
	}
	*(ptr + iPos) = 0;
    if(iPos != iPosOrg)
    {
        if(bTest)
        {
            str = "GSM Empfang: " + string((char *)ptr, iPos);
            syslog(LOG_INFO, str.c_str());
        }
    }
	return iPos;
}

void CUartI2C::SendLen(unsigned char *ptr, int len, bool bTest)
{
	int i;
	string str;
    
    if(bTest)
    {
        str = "Senden: " + string((char *)ptr, len);
        syslog(LOG_INFO, str.c_str());
    }
	m_pBoardAddr->setI2C_gpio();
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

CFormatText::CFormatText()
{
    m_strText.clear();
    m_pBerechneBase = NULL;
    m_pOperTyp = NULL;
}

CFormatText::~CFormatText()
{
    if(m_pBerechneBase != NULL)
    {
        delete [] (CBerechneBase *)m_pBerechneBase;
        m_pBerechneBase = NULL;
    }
    if(m_pOperTyp != NULL)
    {
        delete [] m_pOperTyp;
        m_pOperTyp = NULL;
    }
}

string CFormatText::GetString()
{
    string str, strHelp;
    int iParam, iPos, iStart, iIdx;
    
    str.clear();
    for(iPos = 0, iParam = 0, iIdx = 0; iPos != string::npos ; iIdx++)
    {
        iStart = iPos;
        iPos = m_strText.find("#", iPos);
        if(iPos == string::npos)
        {    
            str += m_strText.substr(iStart, m_strText.length() - iStart);
            break;
        }
        str += m_strText.substr(iStart, iPos - iStart);
        switch(m_pOperTyp[iIdx]) {
            case 'd':
                strHelp = to_string(m_pBerechneBase[iIdx]->eval());
                break;
            case 's':
                strHelp = m_pBerechneBase[iIdx]->GetString();
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

void CFormatText::SetString(string str)
{
    m_strText = str;
}

void CFormatText::Init(int iAnz)
{
    m_pBerechneBase = new CBerechneBase * [iAnz];
    m_pOperTyp = new char[iAnz];
}

void CFormatText::SetOper(int iIdx, CBerechneBase *pBerechne, char ch)
{
    m_pBerechneBase[iIdx] = pBerechne;
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

// GSM
COperGsm::COperGsm()
{
    m_pGsm = NULL;
    m_iFct = 0;
}
void COperGsm::SetOper(CGsm *pGsm, int iFct)
{
    m_pGsm = pGsm;
    m_iFct = iFct;
}
string COperGsm::resultString()
{
    string str;

    switch(m_iFct) {
        case 1: // Error
            str = m_pGsm->GetErrorString();
            break;
        case 2: // letzte SMS
            str = m_pGsm->GetLastSMS();
            break;        
        default:
            str = "";
            break;
    }
    return str;
}
int COperGsm::resultInt()
{
    int iRet;

    switch(m_iFct) {
        case 1:
            iRet = m_pGsm->GetError();
            break;
        case 2:
            iRet = m_pGsm->IsLastSMS();
            break;
        default:
            iRet = 0;
            break;
    }
    return iRet;
}