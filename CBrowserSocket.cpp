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
#include <iostream>

extern bool bRunProgram;

CBrowserSocket::CBrowserSocket()
{
    m_serverSocket = 0;
}

CBrowserSocket::~CBrowserSocket()
{
    if(m_serverSocket > 0)
        close(m_serverSocket);
}
int CBrowserSocket::Init(CIOGroup *pIOGroup)
{
    int iError = 0;

    m_pIOGroup = pIOGroup;
	
    m_serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if(m_serverSocket == -1)
    {
    	syslog(LOG_ERR, "ProWo, internet socket creation error");
    	iError = 1;
    }

    memset(&m_serverAddr, 0, sizeof(struct sockaddr_in));
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_addr.s_addr = INADDR_ANY;
    m_serverAddr.sin_port = htons(PORTNR);
    iError = bind(m_serverSocket, (struct sockaddr *)&m_serverAddr, sizeof(struct sockaddr_in));
    if(iError == -1)
    {	iError = errno;
        syslog(LOG_ERR, "ProWo, server socket bind error");
        iError = 2;
    }

    if(listen(m_serverSocket, LISTEN_BACKLOG) == -1)
    {	
        syslog(LOG_ERR, "ProWo, server socket listen error");
        iError = 3;
    }
    
    return iError;
}

void CBrowserSocket::Run()
{
    int iNiv1, iNiv2, iNiv3, iNiv4, i;
    string str;

    m_recvdMsg[0] = 0;
    m_clientAddrSize = sizeof(struct sockaddr_in);
    m_acceptSocket = accept(m_serverSocket, (struct sockaddr *)&m_clientAddr,
                             &m_clientAddrSize);
    if(m_acceptSocket == -1)
        return;
    i = recv(m_acceptSocket, m_recvdMsg, MSGSIZE, 0);
    if(i < 0)
    {
        syslog(LOG_ERR, "Internet socket: error while reading\n");
        return;
    }
    if(i == 0)
    {
        syslog(LOG_ERR, "Internet socket closed after read timeout\n");
        return;
    }

    m_iPos = 0;
    iNiv1 = ReadNumber();
    iNiv2 = ReadNumber();
    iNiv3 = ReadNumber();
    iNiv4 = ReadNumber();

    switch(iNiv1) {
    case 0:
        VerwaltHome(iNiv1, iNiv2, iNiv3, iNiv4);
        break;
    case 1:     // Steuerung
        VerwaltSteuerung(iNiv1, iNiv2, iNiv3, iNiv4);
        break;
    case 2:  // Heizung
        if(m_pIOGroup->GetHZAddress() != NULL)
            VerwaltHeizung(iNiv1, iNiv2, iNiv3, iNiv4);
        break;
    case 3: // Zähler
        if(m_pIOGroup->GetAnzZaehler() > 0)
            VerwaltZaehler(iNiv1, iNiv2, iNiv3, iNiv4);
        break;
    case 4: // Alarm
        if(m_pIOGroup->m_pAlarm != NULL)
            VerwaltAlarm(iNiv1, iNiv2, iNiv3, iNiv4);
        break;
    case 5: // Wetterstation
        VerwaltWS(iNiv1, iNiv2, iNiv3, iNiv4);
        break;
    case 6: // Sensoren
        if(m_pIOGroup->GetAnzSensor() > 0)
            VerwaltSensor(iNiv1, iNiv2, iNiv3, iNiv4);
        break;
    case 7: // Clock Alarm
        VerwaltAlarmClock(iNiv1, iNiv2, iNiv3, iNiv4);
        break;
    default:
        str = "{\"type\": \"unknown\"}";
        Send(str.c_str());   
        break;
    }
    close(m_acceptSocket);
}

void CBrowserSocket::VerwaltHome(int iNiv1, int iNiv2, int iNiv3, int iNiv4)
{
    int i, pos, anz;
    class CBrowserEntity menu;
    class CBrowserEntity *pTitel;
    class CBrowserEntity *pMenu;
    bool bFirst;   
    string str;

// Ebene 1 = Menüabfrage - Navigationsbar
    switch(iNiv2){
        case 0: // normale Abfrage
            anz = m_pIOGroup->m_pBrowserMenu->GetAnzahlGroup() - 1;
            if(!iNiv3)
                pos = 1;
            else
            {
                pos = iNiv3 + 4;
                if(pos > anz)
                    pos = 1;
            } 
            break;
        case 1: // message löschen
            pthread_mutex_lock(&ext_mutexNodejs); 
            m_pIOGroup->m_mapWriteMessage.erase(iNiv4);
            pthread_mutex_unlock(&ext_mutexNodejs); 
            break;
        default:
            break;
    }
    anz = m_pIOGroup->m_pBrowserMenu->GetAnzahlGroup() - 1;
    if(!iNiv3)
        pos = 1;
    else
    {
        pos = iNiv3 + 4;
        if(pos > anz)
            pos = 1;
    }
    pTitel = m_pIOGroup->m_pBrowserMenu->SearchGroup(0);
    str = "{\"type\":1,\"pos\":" + to_string(pos) + ",\"anzahl\":" + to_string(anz) + ",\"title\":\"" 
                                    + string(pTitel->m_pText) + "\",";
    Send(str.c_str());
    str = "\"navbar\":[";
    Send(str.c_str());

    bFirst = true;
    for(i=pos; i < pos+4 ; i++)
    {
        pTitel = m_pIOGroup->m_pBrowserMenu->SearchGroup(i);
        if(pTitel == NULL)
            break;
        else
        {   
            if(bFirst) 
                bFirst = false;
            else {
                str = ",";
                Send(str.c_str());
            }
            str = "{\"id\":\"" + to_string(pTitel->m_iNiv1) + "_" + to_string(pTitel->m_iNiv2) 
                            + "_" + to_string(pTitel->m_iNiv3) + "_" + to_string(pTitel->m_iNiv4) +"\",";
            Send(str.c_str());
            str = "\"text\":\"" + string( pTitel->m_pText) + "\",";
            Send(str.c_str());
            str = "\"image\":\"" + string(pTitel->m_pImage) + "\"}";
            Send(str.c_str());
        }
    }
    i = m_pUhr->getUhrmin();
    str = "],\"uhr\":\"" + to_string(i/60) + ":" + strZweiStellen(i%60) + "    "  + to_string(m_pUhr->getTag()) + ". "
                + GetMonthText(m_pUhr->getMonat()) + " " + to_string( m_pUhr->getJahr()) + "\"";
    Send(str.c_str());
    i = m_pUhr->getSA();
    str = ",\"sa\":\"" + to_string(i/60) + ":" + strZweiStellen(i%60) + "\"";
    Send(str.c_str());
    i = m_pUhr->getSU();
    str = ",\"su\":\"" + to_string(i/60) + ":" + strZweiStellen(i%60) + "\"";   
    Send(str.c_str());
    pthread_mutex_lock(&ext_mutexNodejs); 
    pos = m_pIOGroup->GetAnzWriteMessage();
    str = ",\"anzWriteMessage\":\"" + to_string(pos) + "\"";
    Send(str.c_str());
    if(pos)
    {   std::map<int, class CWriteMessage>::iterator it;
        str = ",\"WriteMessage\":[";
        Send(str.c_str());
        bFirst = true;
        str = "";
        for(i=0, it=m_pIOGroup->m_mapWriteMessage.begin(); i < pos; it++,i++)
        {   if(bFirst)
                bFirst = false;
            else
                str = ",";
            str += "{\"Text\":\"" + it->second.m_strText + "\",\"nr\":\"" + to_string(it->first) + "\"}";
            Send(str.c_str());
        }
        str = "]";
        Send(str.c_str());
    }
    pthread_mutex_unlock(&ext_mutexNodejs);    
    // Heizung
    if(m_pIOGroup->GetHZAddress() != NULL)
    {
        CHeizung *pHeizung = m_pIOGroup->GetHZAddress(); 
        CHeizProgramm *pHeizProgramm = pHeizung->GetHPAddress(pHeizung->GetAktHeizProgramm());
        CHeizProgrammTag *pHeizProgrammTag = pHeizProgramm->GetHeizProgrammTag(pHeizung->GetAktHeizProgrammTag()); 
        str = ",\"heizung\":1,\"heizungtext\":\"Heizung:" + pHeizProgramm->GetName() + " - " + pHeizProgrammTag->GetName();
        Send(str.c_str());
        if(pHeizProgramm->m_iDatum != 0)
        {
            int iTag, iMonat, iJahr, iStunde, iMinute;
            m_pUhr->getDatumTag(pHeizProgramm->m_iDatum, &iJahr, &iMonat, &iTag);
            iStunde = pHeizProgramm->m_iUhrzeit / 60;
            iMinute = pHeizProgramm->m_iUhrzeit % 60;
            str = "<br>bis " + to_string(iTag) + "." + strZweiStellen(iMonat) + "." + to_string(iJahr)
                             + " " +  strZweiStellen(iStunde) + "h" + strZweiStellen(iMinute) + "\"";
        } 
        else
            str = "\"";
    }
    else
        str = ",\"heizung\":0";
    Send(str.c_str());
    
    // Alarm
    if(m_pIOGroup->m_pAlarm != NULL)
    {
        int iStatus = m_pIOGroup->m_pAlarm->GetParamInt(1);  
        int iNr = m_pIOGroup->m_pAlarm->GetAlarmNr();  
        if(iStatus == 1)
            iNr = 1;
        str = ",\"alarm\":1,\"alarmstatus\":" + to_string(iStatus);
        Send(str.c_str());
        if(iStatus == 0)
            str = ",\"alarmtext\":\"Alarmanlage: Technikermodus\"";
        else
        {
            str = ",\"alarmtext\":\"Alarmanlage: " + m_pIOGroup->m_pAlarm->GetAlarmeAddr(iNr-1)->GetBez();
            switch(iStatus) {
                case 2:
                    str += "<br>voraktiviert\"";
                    break;
                case 3:
                    str += "<br>aktiviert\"";
                    break;
                case 4:
                    str += "<br>voralarmiert: " + (m_pIOGroup->m_pAlarm->GetSensorAddr() + 
                                m_pIOGroup->m_pAlarm->GetAlarmeAddr(iNr-1)->GetAusloeser()-1)->GetBez() + "\"";
                    break;
                case 5:
                    str += "<br>alarmiert: " + (m_pIOGroup->m_pAlarm->GetSensorAddr() 
                                + m_pIOGroup->m_pAlarm->GetAlarmeAddr(iNr-1)->GetAusloeser()-1)->GetBez() + "\"";
                    break;
                default:
                    str += "\"";
                    break;
            }
        }
    }
    else
        str = ",\"alarm\":0";
    Send(str.c_str());
    
    // GSM
    if(m_pIOGroup->m_pGsm != NULL)
    {
        str = ",\"gsm\":1,\"signal\":" + to_string(m_pIOGroup->m_pGsm->GetSignal()) + ",\"provider\":\"" 
                        + m_pIOGroup->m_pGsm->GetProvider() + "\",\"gsmerror\":\"" + m_pIOGroup->m_pGsm->GetErrorString() + "\"";
    }
    else
        str = ",\"gsm\":0";
    Send(str.c_str());
    
    // History
    if(m_pIOGroup->m_pHistory != NULL)
    {
        str = ",\"history\":1,\"tage\":" + to_string(m_pIOGroup->m_pHistory->GetDiffTage()) + ",\"historyerror\":\"" 
                        + m_pIOGroup->m_pHistory->GetError() + "\"";
    }
    else
        str = ",\"history\":0";
    Send(str.c_str());
    
    // Wetterstation
    if(m_pIOGroup->m_pWStation != NULL)
    {
        CWStation * pWS = m_pIOGroup->m_pWStation;
        str = ",\"ws\":1,\"tempakt\":" + to_string(pWS->GetParam(TEMPAKT)) + ",\"tempmin\":" 
                        + to_string(pWS->GetParam(TEMPMIN)) + ",\"tempmax\":" + to_string(pWS->GetParam(TEMPMAX));
        Send(str.c_str());
        str = ",\"luftdruck\":" + to_string(pWS->GetParam(LUFTDRUCK)) + ",\"feuchte\":"  
                        + to_string(pWS->GetParam(FEUCHTE)) + ",\"helligkeit\":"+ to_string(pWS->GetParam(HELLIGKEIT));
        Send(str.c_str());
        str = ",\"windgeschw\":" + to_string( pWS->GetParam(WINDGESCHW)) + ",\"windgeschwmax\":"
                        + to_string(pWS->GetParam(WINDGESCHWMAX)) + ",\"windricht\":" + to_string(pWS->GetParam(WINDRICHT));
        Send(str.c_str());
        str = ",\"windgeschwmax10\":" + to_string(pWS->GetParam(WINDGESCHWMAX10)) + ",\"windgeschwmax30\":"
                        + to_string(pWS->GetParam(WINDGESCHWMAX30)) +  ",\"windgeschwmax60\":" + to_string(pWS->GetParam(WINDGESCHWMAX60));
        Send(str.c_str());
        str = ",\"nsmengetag\":[";
        for(i=0; i < 3; i++)
        {
            if(i)
                str += ",";
            str += to_string(pWS->GetParam(NSMENGE, 1, i));
        }
        str += "]";
        Send(str.c_str());
        str = ",\"nsmengewoche\":[";
        for(i=0; i < 3; i++)     
        {
            if(i)
                str += ",";
            str += to_string(pWS->GetParam(NSMENGE, 2, i));
        }  
        str += "]";
        Send(str.c_str());
        str = ",\"nsmengemonat\":[";
        for(i=0; i < 3; i++)
        {
            if(i)
                str += ",";
            str += to_string(pWS->GetParam(NSMENGE, 3, i));
        }  
        str += "]";
        Send(str.c_str());
        str = ",\"nsart\":" + to_string(pWS->GetParam(NSART));
    }
    else
        str = ",\"ws\":0";
    str += "}";
    Send(str.c_str());

}
void CBrowserSocket::VerwaltSteuerung(int iNiv1, int iNiv2, int iNiv3, int iNiv4)
{
    char buf[MSGSIZE];
    class CBrowserEntity menu;
    class CBrowserEntity *pTitel;
    class CBrowserEntity *pMenu;
    int i, iState, iLen;
    bool bFirst;
    string str;
  
    ReadBuf(buf, ';');
    if(buf[0] == '1') // gebe das nächste Menü zurück
    {
        // den Status aller Hue Lampen und Gruppen einlesen
        //m_pIOGroup->LesHUE();
        
        if(iNiv2 == 0) // Untermenü
        {
            bFirst = true;
            str = "{\"type\" : 1, \"list\" : [ ";
            Send(str.c_str());            
            for(i=1; i ;i++)
            {
                pTitel = m_pIOGroup->m_pBrowserMenu->SearchTitel(iNiv1, i, 0, 0);
                if(pTitel == NULL)
                    break;
                else
                {   
                    str = "";
                    if(bFirst) 
                        bFirst = false;
                    else 
                        str = ",";
                    str += "{\"id\":\"" + to_string( pTitel->m_iNiv1)
                                        + "_" + to_string( pTitel->m_iNiv2)
                                        + "_" + to_string( pTitel->m_iNiv3)
                                        + "_" + to_string( pTitel->m_iNiv4) +"\",";                                     
                    Send(str.c_str());
                    str = "\"text\":\"" + string(pTitel->m_pText) + "\",";
                    Send(str.c_str());
                    if(pTitel->m_pImage == NULL)
                        str = "\"image\":\"\",";
                    else
                        str = "\"image\":\"" + string(pTitel->m_pImage) + "\",";
                    Send(str.c_str());
                    str = "\"prowotype\":" + to_string(pTitel->m_iTyp) + ",";
                    Send(str.c_str());
                    switch(pTitel->m_iTyp) {
                    case 1: // Untermenü Steuerung
                        str = "\"status\":\"not\"}";                    
                        if(pTitel->m_bSammelSchalter)
                        {
                            int iStatus = 0, iVal;
                            while(pTitel->m_pNextMenu && pTitel->m_pNextMenu->m_iNiv1 == iNiv1
                                && pTitel->m_pNextMenu->m_iNiv2 == i)//&& pTitel->m_pNextMenu->m_iNiv3 == i)
                            {   
                                pTitel = pTitel->m_pNextMenu;                                
                                pthread_mutex_lock(&ext_mutexNodejs);
                                iVal = pTitel->m_pOperState->GetState();
                                pthread_mutex_unlock(&ext_mutexNodejs);                                
                                if(iVal % 256) // gesetzt wenn einer gesetzt
                                {
                                    iStatus = iVal;
                                    break;
                                }
                            }
                            str = "\"status\":\"" + to_string(iStatus) + "\"}";
                        }
                        Send(str.c_str());
                        break;
                    default:
                        break;
                    }
                }
            }
            str = "]}";
            Send(str.c_str());
        }
        // Ebene 3
        else if(iNiv3 == 0) // Schaltebene
        {
            str = "{\"type\":2,\"list\":[";
            Send(str.c_str());
            bFirst = true;
            for(i=1; i ;i++)
            {
                pTitel = m_pIOGroup->m_pBrowserMenu->SearchTitel(iNiv1, iNiv2, i, 0);
                if(pTitel == NULL)
                    break;
                else
                {   str ="";
                    if(bFirst) 
                        bFirst = false;
                    else 
                        str = ",";
                    str += "{\"id\":\"" + to_string( pTitel->m_iNiv1)
                                        + "_" + to_string( pTitel->m_iNiv2)
                                        + "_" + to_string( pTitel->m_iNiv3)
                                        + "_" + to_string( pTitel->m_iNiv4) +"\",";    
                    Send(str.c_str());
                    str =  "\"text\":\""+ string(pTitel->m_pText) + "\",";
                    Send(str.c_str());
                    str = "\"prowotype\":" + to_string( pTitel->m_iTyp) + ",";  
                    Send(str.c_str());
                    if(pTitel->m_pImage == NULL)
                        str = "\"image\":\"\",";
                    else
                        str = "\"image\":\"" + string(pTitel->m_pImage) + "\",";
                    Send(str.c_str()); 
                    str = "\"max\":\"" + to_string(pTitel->m_pOperState->GetMax())  + "\",";
                    Send(str.c_str());                 
                    pthread_mutex_lock(&ext_mutexNodejs);                     
                    str = "\"status\":\"" + to_string(pTitel->m_pOperState->GetState()) + "\"}";                       
                    pthread_mutex_unlock(&ext_mutexNodejs);
                    Send(str.c_str());
                }
            }
            str = "]}";
            Send(str.c_str());            
        }

    }
    else if(buf[0] == '2') 
    { 
        //Daten werden empfangen und sollen abgespeichert werden 
        pTitel = m_pIOGroup->m_pBrowserMenu->SearchTitel(iNiv1, iNiv2, iNiv3, 0);
        if(pTitel != NULL) {
            iLen = ReadBuf(buf, ';');
            iState = atoi(buf);           
            pthread_mutex_lock(&ext_mutexNodejs);
            if(pTitel->m_pOperChange)
                pTitel->m_pOperChange->SetState(iState);
            else 
            {
                for(i=1; ;i++) 
                {
                    pTitel = m_pIOGroup->m_pBrowserMenu->SearchTitel(iNiv1, iNiv2, i, 0);
                    if(pTitel != NULL && pTitel->m_pOperChange != NULL)
                        pTitel->m_pOperChange->SetState(iState);
                    else
                        break;
                }
            }
            m_pIOGroup->SetBerechne();
            pthread_mutex_unlock(&ext_mutexNodejs);
            str = "{\"type\":3}";
            Send(str.c_str());
        }    
    }
    else {
        str += "{\"type\":3,\"id\":\"" + to_string( pTitel->m_iNiv1) + "_" + to_string( pTitel->m_iNiv2)
                        + "_" + to_string( pTitel->m_iNiv3) + "_" + to_string( pTitel->m_iNiv4) +"\",\"error\":\"falsche ID\"}"; 
        Send(str.c_str());
    }
}

//
//  2_x_y_z;type
//
void CBrowserSocket::VerwaltHeizung(int iNiv1, int iNiv2, int iNiv3, int iNiv4)
{
    char buf[MSGSIZE];
    int i, iAnzHeizKoerper, iAktHeizProgramm, iAktHeizProgrammTag, k;
    int iAltHeizProgramm, iAltHeizProgrammTag, iDatum, iUhrzeit, iTemp;
    int iAnzHeizProgramme, iAnzHeizProgrammTage, j, iHeizkoerper;
    CHeizProgramm *pHeizProgramm;
    CHeizProgrammTag *pHeizProgrammTag, *pNeuHeizProgrammTag;
    CHeizKoerper* pHeizKoerper;
    CUhrTemp *pUhrTemp;
    bool bTrue = true;
    string strName, str;
    
    CHeizung *pHeizung = m_pIOGroup->GetHZAddress();    
    ReadBuf(buf, ';');
    while(bTrue) 
    {
        switch(buf[0]) {
            case '1':
                switch(iNiv4) {
                case 0:
                    iAktHeizProgramm = pHeizung->GetAktHeizProgramm();
                    iAktHeizProgrammTag = pHeizung->GetAktHeizProgrammTag();
                    pHeizProgramm = pHeizung->GetHPAddress(iAktHeizProgramm);
                    pHeizProgrammTag = pHeizProgramm->GetHeizProgrammTag(iAktHeizProgrammTag);
                    str = "{\"type\":1,\"HeizProgramm\":\"" + pHeizProgramm->GetName();
                    Send(str.c_str());
                    if(pHeizProgramm->m_iDatum != 0)
                    {
                        int iTag, iMonat, iJahr, iStunde, iMinute;
                        m_pUhr->getDatumTag(pHeizProgramm->m_iDatum, &iJahr, &iMonat, &iTag);
                        iStunde = pHeizProgramm->m_iUhrzeit / 60;
                        iMinute = pHeizProgramm->m_iUhrzeit % 60;
                        str = " - " + to_string(iTag) + "." + strZweiStellen(iMonat) + "." + to_string(iJahr)
                                    + " " + strZweiStellen(iStunde) + "h" + strZweiStellen(iMinute) + "\",";
                    } 
                    else
                    str = "\",";
                    Send(str.c_str());  
                    str = "\"HeizProgrammTag\":\"" + pHeizProgrammTag->GetName() + "\",\"list\":[";               
                    Send(str.c_str());
                    iAnzHeizKoerper = pHeizung->GetAnzHeizKoerper();
                    for(i=0; i < iAnzHeizKoerper ;i++)
                    {
                        str = "";
                        if(i) 
                            str = ",";
                        str += "{\"id\":\"2_" + to_string(iAktHeizProgramm) + "_" + to_string(iAktHeizProgrammTag) 
                                        + "_" + to_string(i+1) + "\",";
                        Send(str.c_str());
                        pHeizKoerper = pHeizung->GetHKAddress(i+1);
                        str = "\"text\":\"" + pHeizKoerper->GetName() + "\",";
                        Send(str.c_str());
                        if(pHeizKoerper->GetSensor() == NULL)
                            str = "\"type\":2,"; // Warmwasser
                        else
                            str = "\"type\":1,"; // Heizkörper
                        Send(str.c_str());
                        str = "\"ist\":" + to_string(((float)pHeizKoerper->GetTemp())/10.0) + 
                                ",\"soll\":" + to_string(((float)pHeizKoerper->GetTempSoll())/10.0) + ",";
                        Send(str.c_str()); 
                        pthread_mutex_lock(&ext_mutexNodejs);
                        if(pHeizKoerper->GetState())
                            str = "\"status\":true}";
                        else
                           str = "\"status\":false}";                        
                        pthread_mutex_unlock(&ext_mutexNodejs);
                        Send(str.c_str());
                    }
                    str = "]}";
                    Send(str.c_str());
                    bTrue = false;
                    break;
                case 1:
                    str = "{\"type\":2,\"Heizprogramme\":[";
                    Send(str.c_str());
                    iAnzHeizProgramme = pHeizung->GetAnzHeizProgramme();
                    for(i=0 ; i < iAnzHeizProgramme; i++)
                    {
                        str = "";
                        if(i) 
                            str = ",";
                        pHeizProgramm = pHeizung->GetHPAddress(i+1);
                        str += "{\"text\": \"" + pHeizProgramm->GetName() + "\",";
                        Send(str.c_str());
                        str = "\"Datum\":" + to_string(pHeizProgramm->m_iDatum) + ",";
                        Send(str.c_str());
                        str = "\"Uhrzeit\":" + to_string(pHeizProgramm->m_iUhrzeit) + ",";
                        Send(str.c_str());
                        str = "\"id\":\"2_" + to_string(i+1) + "_0_0\", \"HeizProgrammTag\":[";
                        Send(str.c_str());
                        iAnzHeizProgrammTage = pHeizProgramm->GetAnzHeizProgrammTag(); 
                        for(j=0; j < iAnzHeizProgrammTage; j++)
                        {
                            str = "";
                            if(j) 
                                str = ",";
                            pHeizProgrammTag = pHeizProgramm->GetHeizProgrammTag(j+1);
                            str += "{\"text\":\"" + pHeizProgrammTag->GetName() + "\",";
                            Send(str.c_str());
                            str = "\"tage\":" + to_string(pHeizProgrammTag->GetGueltigeTage()) + ",";
                            Send(str.c_str());
                            str = "\"id\":\"2_" + to_string(i+1) + "_" + to_string(j+1) + "_0\"}";
                            Send(str.c_str());                    
                        }
                        str = "]}";
                        Send(str.c_str());
                    }
                    str = "]}";
                    Send(str.c_str());
                    break;
                } 
                bTrue = false;
                break;
            case '2':
                j = ReadNumber();
                iAktHeizProgramm = iNiv2;
                iAktHeizProgrammTag = iNiv3;
                pHeizProgramm = pHeizung->GetHPAddress(iAktHeizProgramm);  
                iAnzHeizProgrammTage = pHeizProgramm->GetAnzHeizProgrammTag();
                for(i=0x40; j; j--)
                    i /= 2;
                j = ~i;
                pthread_mutex_lock(&ext_mutexNodejs);
                for(k=0; k < iAnzHeizProgrammTage; k++) {
                    pHeizProgrammTag = pHeizProgramm->GetHeizProgrammTag(k+1);
                    pHeizProgrammTag->SetGueltigkeitsTage(pHeizProgrammTag->GetGueltigeTage() & j);
                }
                pHeizProgrammTag = pHeizProgramm->GetHeizProgrammTag(iAktHeizProgrammTag);
                pHeizProgrammTag->SetGueltigkeitsTage(pHeizProgrammTag->GetGueltigeTage() | i); 
                m_pIOGroup->SetBerechneHeizung();
                pthread_mutex_unlock(&ext_mutexNodejs);
                pHeizung->WriteDatei(pProgramPath);                 
                buf[0] = '1';
                iNiv4 = 1;
                break;
            case '3': // im Heizprogramm einen Tagesblock löschen
                iAktHeizProgramm = iNiv2;
                iAktHeizProgrammTag = iNiv3;
                pHeizProgramm = pHeizung->GetHPAddress(iAktHeizProgramm);  
                pthread_mutex_lock(&ext_mutexNodejs);
                pHeizProgramm->DeleteHeizProgrammTag(iAktHeizProgrammTag);
                m_pIOGroup->SetBerechneHeizung();
                pthread_mutex_unlock(&ext_mutexNodejs);
                pHeizung->WriteDatei(pProgramPath);                 
                buf[0] = '1';
                iNiv4 = 1;
                break;
            case '4': // im HeizProgramm einen Tagesblock einfügen
                iAktHeizProgramm = iNiv2;
                iAltHeizProgramm = ReadNumber();                
                iAltHeizProgramm = ReadNumber();
                pNeuHeizProgrammTag = new CHeizProgrammTag;
                iAltHeizProgrammTag = ReadNumber();
                pHeizProgramm = pHeizung->GetHPAddress(iAltHeizProgramm); 
                pHeizProgrammTag = pHeizProgramm->GetHeizProgrammTag(iAltHeizProgrammTag);
                *pNeuHeizProgrammTag = *pHeizProgrammTag;
                iAnzHeizKoerper = pHeizung->GetAnzHeizKoerper();
                pthread_mutex_lock(&ext_mutexNodejs);
                pNeuHeizProgrammTag->CreateUhrTemp(iAnzHeizKoerper);
                for(i=0; i < iAnzHeizKoerper; i++) {
                    CUhrTemp *pNeu;
                    pUhrTemp = pHeizProgrammTag->GetHKUhrTemp(i+1);
                    pNeu = pNeuHeizProgrammTag->GetHKUhrTemp(i+1);
                    for(j=0 ; j < ANZUHRTEMP; j++)
                        *(pNeu + j) = *(pUhrTemp +j);
                }
                pNeuHeizProgrammTag->SetGueltigkeitsTage(0);
                pHeizProgramm = pHeizung->GetHPAddress(iAktHeizProgramm);  
                pHeizProgramm->AddHeizProgrammTag(pNeuHeizProgrammTag);
                m_pIOGroup->SetBerechneHeizung();
                pthread_mutex_unlock(&ext_mutexNodejs);
                pHeizung->WriteDatei(pProgramPath);                 
                buf[0] = '1';                
                iNiv4 = 1;
                break;
            case '5': // neue Beschreibung
                ReadBuf(buf, ';');
                iAktHeizProgramm = iNiv2;
                iAktHeizProgrammTag = iNiv3;
                strName = buf;
                pHeizProgramm = pHeizung->GetHPAddress(iAktHeizProgramm);
                pthread_mutex_lock(&ext_mutexNodejs);
                if(!iNiv3) 
                    pHeizProgramm->SetName(strName);
                else {
                    pHeizProgrammTag = pHeizProgramm->GetHeizProgrammTag(iAktHeizProgrammTag);
                    pHeizProgrammTag->SetName(strName);
                }
                m_pIOGroup->SetBerechneHeizung();
                pthread_mutex_unlock(&ext_mutexNodejs);
                pHeizung->WriteDatei(pProgramPath); 
                buf[0] = '1';                
                iNiv4 = 1;
                break;
            case '6': // Datum und Uhrzeit für HeizProgramm
                iDatum = ReadNumber();
                iUhrzeit = ReadNumber();
                i = m_pUhr->getDatumTag();
                j = m_pUhr->getUhrmin();
                if(i > iDatum || (i == iDatum && j > iUhrzeit)) {
                    iDatum = 0;
                    iUhrzeit = 0;
                }
                iAktHeizProgramm = iNiv2;
                pthread_mutex_lock(&ext_mutexNodejs);
                pHeizProgramm = pHeizung->GetHPAddress(iAktHeizProgramm);
                pHeizProgramm->m_iDatum = iDatum;
                pHeizProgramm->m_iUhrzeit = iUhrzeit;
                m_pIOGroup->SetBerechneHeizung();
                pthread_mutex_unlock(&ext_mutexNodejs);
                pHeizung->WriteDatei(pProgramPath);                 
                buf[0] = '1';                
                iNiv4 = 1;
                break;   
            case '7': // Uhrzeit und Temperatur eines Heizkörpers
                iAktHeizProgramm = iNiv2;
                iAktHeizProgrammTag = iNiv3;
                iHeizkoerper = iNiv4;
                str = "{\"type\":3,\"id\":\"2_" + to_string(iAktHeizProgramm) + "_" + to_string(iAktHeizProgrammTag)
                                    + "_" + to_string(iHeizkoerper) + "\", \"Heizprogramm\": [";
                Send(str.c_str());
                iAnzHeizProgramme = pHeizung->GetAnzHeizProgramme();
                for(i=0; i < iAnzHeizProgramme; i++) {
                    str = "";
                    if(i)
                        str = ",";
                    str += "{\"Name\":\""  + pHeizung->GetHPAddress(i+1)->GetName() + "\"}";
                    Send(str.c_str());
                }
                str = "],\"HeizprogrammTag\": [";
                Send(str.c_str());
                pHeizProgramm = pHeizung->GetHPAddress(iAktHeizProgramm);  
                iAnzHeizProgrammTage = pHeizProgramm->GetAnzHeizProgrammTag();
                for(i=0; i < iAnzHeizProgrammTage; i++){
                    str = "";
                    if(i)
                        str = ",";
                        str += "{\"Name\":\"" + pHeizProgramm->GetHeizProgrammTag(i+1)->GetName() + "\"}";
                    Send(str.c_str());
                }
                str = "],\"Heizkoerper\":[";
                Send(str.c_str());
                iAnzHeizKoerper = pHeizung->GetAnzHeizKoerper();                    
                for(i=0; i < iAnzHeizKoerper; i++) {
                    str = "";
                    if(i)
                        str = ",";
                    str += "{\"Name\":\"" + pHeizung->GetHKAddress(i+1)->GetName() + "\"}";
                    Send(str.c_str());
                }   
                pHeizProgramm = pHeizung->GetHPAddress(iAktHeizProgramm);
                pHeizProgrammTag = pHeizProgramm->GetHeizProgrammTag(iAktHeizProgrammTag);
                pHeizKoerper = pHeizung->GetHKAddress(iHeizkoerper);
                pUhrTemp = pHeizProgrammTag->GetHKUhrTemp(iHeizkoerper);
                str = "], \"HKType\":" + to_string(pHeizKoerper->GetSensorNr()) + ", \"pkt\":[";
                Send(str.c_str());                
                for(j=0; j < ANZUHRTEMP; j++) {
                    str = "";
                    if(j) 
                        str = ",";
                    str += "{\"time\":" + to_string((pUhrTemp + j)->m_iUhr) + ",\"temp\":" + to_string((pUhrTemp + j)->m_iTemp) + "}";
                    Send(str.c_str());
                }
                str = "]}";
                Send(str.c_str());
                bTrue = false;
                break;
            case '8':
                iAktHeizProgramm = iNiv2;
                iAktHeizProgrammTag = iNiv3;
                iHeizkoerper = iNiv4;  
                pHeizProgramm = pHeizung->GetHPAddress(iAktHeizProgramm);
                pHeizProgrammTag = pHeizProgramm->GetHeizProgrammTag(iAktHeizProgrammTag);
                pUhrTemp = pHeizProgrammTag->GetHKUhrTemp(iHeizkoerper); 
                pthread_mutex_lock(&ext_mutexNodejs);
                for(i=0; i < ANZUHRTEMP; i++) {
                   iUhrzeit = ReadNumber();
                   iTemp = ReadNumber();
                   if(iUhrzeit >= 0 && iUhrzeit < 1440 && iTemp >= 0 && iTemp <= 300) {
                       (pUhrTemp + i)->m_iUhr = iUhrzeit;
                       (pUhrTemp + i)->m_iTemp = iTemp;
                   }
                   else {
                       str = "{\"type\":100,\"id\":\"" + to_string(iNiv1) + "_" + to_string(iNiv2) + "_"
                                        + to_string(iNiv3) + "_" + to_string(iNiv4) + "\",\"error\":\"Time or Temp incorrect\"}";
                       Send(str.c_str());
                       bTrue = false;
                       break;
                   }
                } 
                m_pIOGroup->SetBerechneHeizung();
                pthread_mutex_unlock(&ext_mutexNodejs);
                pHeizung->WriteDatei(pProgramPath); 
                buf[0] = '7';
                break;
        }
    }
}

void CBrowserSocket::VerwaltZaehler(int iNiv1, int iNiv2, int iNiv3, int iNiv4)
{
    char buf[MSGSIZE];
    char ch;
    bool bTrue = true, bFirst;
    int len, i, iAnzZaehler, iOffsetDatum, j, iAnzeigeArt;
    int jahr, monat, tag;
    CZaehler * pZaehler;
    double dblZaehlerStand, dblOffset, dblVerbrauch;
    string str;
    struct tm pt = {0};
    
    ReadBuf(buf, ';');
    while(bTrue) 
    {   ch = buf[0];
        switch(ch) {
            case '1': // Übersicht
                str = "{\"type\":1,\"zaehler\":[";
                Send(str.c_str());                
                iAnzZaehler = m_pIOGroup->GetAnzZaehler();
                for(i=0; i < iAnzZaehler; i++) 
                {
                    str.clear();
                    if(i)
                        str = ",";                   
                    pZaehler = m_pIOGroup->GetZaehler(i+1);
                    iOffsetDatum = pZaehler->GetOffsetDatum();
                    m_pUhr->getUhrzeitStruct(&pt, (time_t)iOffsetDatum);
                    pthread_mutex_lock(&ext_mutexNodejs);
                    dblZaehlerStand = pZaehler->GetDblStand();
                    pthread_mutex_unlock(&ext_mutexNodejs);                    
                    dblOffset = pZaehler->GetDblOffset();
                    iAnzeigeArt = pZaehler->GetAnzeigeArt();
                    str += "{\"id\":\"3_" + to_string(i+1) + "_1_1\",";
                    Send(str.c_str());
                    str = "\"name\":\"" +  pZaehler->GetName() + "\",";
                    Send(str.c_str());
                    str = "\"stand\":\"" + strDblRunden(dblZaehlerStand, 2) + "\",";                    
                    Send(str.c_str());
                    str = "\"offset\":\"" + strDblRunden(dblOffset, 2) + "\",";                    
                    Send(str.c_str());
                    if(iAnzeigeArt)
                        str = "\"anzeige\":\"" + strDblRunden(dblOffset - dblZaehlerStand, 2) + "\",";
                    else
                        str = "\"anzeige\":\"" + strDblRunden(dblZaehlerStand - dblOffset, 2) + "\",";
                    Send(str.c_str());
                    str = "\"anzeigeart\":\"" + to_string(iAnzeigeArt) + "\",";
                    Send(str.c_str());
                    str = "\"datum\":\"" + to_string(pt.tm_year+1900) + "-" + strZweiStellen(pt.tm_mon+1) 
                                    +  "-" + strZweiStellen(pt.tm_mday) + "\",";
                    Send(str.c_str());
                    str =  "\"einheit\":\"" + pZaehler->GetEinheit() + "\"}";
                    Send(str.c_str());
                }
                str = "]}";
                Send(str.c_str());
                bTrue = false;
                break;
            case '2': // Offset und Datum werden übernommen, wenn kein Offset dann wird errechnet
                pZaehler = m_pIOGroup->GetZaehler(iNiv2);
                ReadBuf(buf, ';');
                strptime(buf, "%Y-%m-%dT", &pt);
                iOffsetDatum = mktime(&pt);
                dblOffset = (double)ReadDouble();
                if(!dblOffset)
                    dblOffset = pZaehler->GetOffset(pt.tm_year+1900, pt.tm_mon+1, pt.tm_mday);
                pZaehler->SetOffset(dblOffset, iOffsetDatum);
                buf[0] = '1';
                break;
            case '3':
                //  es wird das 1.x auf einen Zähler geklickt, Anzeige aktuelles Jahr und Monat
                if(iNiv3 == 1 && iNiv4 == 1)
                {
                    iNiv4 = m_pUhr->getMonat()+1;
                    iNiv3 = 2;
                }
            case '4':
                str = "{\"type\":3,\"id\":\"3_" + to_string(iNiv2) + "_" + to_string(iNiv3) 
                            + "_" + to_string(iNiv4) + "\",\"zaehler\":[";
                Send(str.c_str());                
                iAnzZaehler = m_pIOGroup->GetAnzZaehler();
                for(i=0; i < iAnzZaehler; i++) 
                {
                    pZaehler = m_pIOGroup->GetZaehler(i+1);
                    str.clear();
                    if(i)
                        str = ",";
                    str += "\""+ pZaehler->GetName() + "\"";
                    Send(str.c_str()); 
                }
                str = "],\"jahr\":[\"---\"";
                Send(str.c_str());
                pZaehler = m_pIOGroup->GetZaehler(iNiv2); 
                jahr = pZaehler->GetFirstYear();
                for(i = pZaehler->GetAktYear() - jahr + 1; i ; i--)
                {   
                    str = ",\"" + to_string(jahr + i - 1) + "\"";
                    Send(str.c_str());                    
                }
                str = "],\"monat\":[";
                Send(str.c_str());
                for(i=0; i < 13; i++) {
                    str.clear();
                    if(i)
                        str = ",";
                    str += "\"" + GetMonthText(i) + "\"";
                    Send(str.c_str());
                }
                str = "], \"arr\":[";
                Send(str.c_str());
                len = 0;
                bFirst = true;
                iNiv3--;
                iNiv4--;
                jahr = pZaehler->GetAktYear();
                jahr = jahr - (iNiv3-1); 

                if(iNiv3 == 0 && iNiv4 == 0)
                {   // Jahreswerte
                    // jahr = pZaehler->GetAktYear();
                    jahr--;
                    j = jahr - pZaehler->GetFirstYear();
                    for(i=0; i <= j; i++)
                    {
                        str.clear();
                        if(i)
                            str = ",";
                        dblVerbrauch = pZaehler->GetDblWert(jahr - i, pZaehler->GetIntAktTag());
                        str += to_string(dblVerbrauch);
                        Send(str.c_str());
                    }
                }   
                else if(iNiv3 != 0 && iNiv4 == 0)
                {   // Monatswerte
                    for(monat=0; monat < 12; monat++)
                    {   
                        str.clear();
                        if(monat)
                            str = ",";
                        dblVerbrauch = pZaehler->GetDblWert(jahr, monat+1, pZaehler->GetIntAktTag());
                        str += to_string(dblVerbrauch);
                        Send(str.c_str());
                    }
                }
                else
                {   // Tageswerte
                    monat = iNiv4;
                    j = m_pUhr->getAnzahlTage(jahr, monat);
                    for(tag = 0; tag < j; tag++)
                    {
                        str.clear();
                        if(tag)
                            str = ",";
                        dblVerbrauch = pZaehler->GetDblWert(jahr, monat, tag+1, pZaehler->GetIntAktTag());
                        str += to_string(dblVerbrauch);
                        Send(str.c_str());
                    }
                }
                str = "]}";
                Send(str.c_str());
                bTrue = false;
                break;
            default:
                bTrue = false;
                break;
        }
    }
                
}
void CBrowserSocket::Send(const char *ptr, int len)
{	
    if(send(m_acceptSocket, ptr, len, 0) < 0)
        syslog(LOG_ERR, "Internet socket: Fehler beim Senden!!\n");
}

void CBrowserSocket::Send(const char*ptr)
{
    Send(ptr, strlen(ptr));
}

int CBrowserSocket::ReadNumber()
{  
    int j;
    char buf[MSGSIZE];

    for(j=0; m_iPos < MSGSIZE; m_iPos++, j++)
    {
        buf[j] = m_recvdMsg[m_iPos];
        if(!isdigit(buf[j]) && !(buf[j] == '-') && !(buf[j] == '+'))
        {
            buf[j] = 0;
            m_iPos++;
            break;
        }
    }

    return atoi(buf);
}

double CBrowserSocket::ReadDouble()
{  
    int j;
    char buf[MSGSIZE];

    for(j=0; m_iPos < MSGSIZE; m_iPos++, j++)
    {
        buf[j] = m_recvdMsg[m_iPos];
        if(!isdigit(buf[j]) && !(buf[j] == '-') && !(buf[j] == '+') && !(buf[j] == '.'))
        {
            buf[j] = 0;
            m_iPos++;
            break;
        }
    }

    return atof(buf);
}
int CBrowserSocket::ReadBuf(char *buf, char ch)
{
    int j;
	
    for(j=0; m_iPos < MSGSIZE; m_iPos++, j++)
    {
        *(buf + j) = m_recvdMsg[m_iPos];
        if(!*(buf+j) || *(buf + j) == ch)
        {
            *(buf + j) = 0;
            m_iPos++;
            break;
        }
    }

    return j;
}

void CBrowserSocket::VerwaltAlarm(int iNiv1, int iNiv2, int iNiv3, int iNiv4)
{
    char buf[MSGSIZE];
    char ch;
    bool bTrue = true, bWait;
    int i, iVar, iSensor, len;
    CAlarme * pAlarme;
    CAlarmSensor *pSensor;
    string strError, strPwd;
    bool bStateSensor, bStateMust, bStateEnabled;
    string str;
    
    strError.clear();
    ReadBuf(buf, ';');
    while(bTrue)
    {
        ch = buf[0];
        switch(ch) {
            case '1': // Alarm ist in der Navigationsbar angewählt worden
                iVar = m_pIOGroup->m_pAlarm->GetParamInt(1); // Status abfragen
                if(iVar < 2)
                {
                    pthread_mutex_lock(&ext_mutexNodejs); 
                    m_pIOGroup->m_pAlarm->ResetEnabled();
                    pthread_mutex_unlock(&ext_mutexNodejs); 
                }                    
                buf[0] = '9';
                break;
                 
            case '2': // Zustand der verschiedenen Sensoren anzeigen
                iVar = m_pIOGroup->m_pAlarm->GetAnzAlarme();
                if(iNiv2 > 0 && iNiv2 <= iVar )
                {
                    bool bWait = true;
                    pthread_mutex_lock(&ext_mutexNodejs); 
                    m_pIOGroup->SetBerechne();
                    pthread_mutex_unlock(&ext_mutexNodejs);    
                    while(bWait)
                    {
                        delayMicroseconds(100000);
                        pthread_mutex_lock(&ext_mutexNodejs); 
                        if(!m_pIOGroup->GetBerechne())
                            bWait = false;
                        pthread_mutex_unlock(&ext_mutexNodejs);                             
                    }
                    pAlarme = m_pIOGroup->m_pAlarm->GetAlarmeAddr(iNiv2-1);
                    str = "{\"type\":2, \"name\":\"" + pAlarme->GetBez() + "\",";
                    Send(str.c_str());
                    str = "\"id\":\"4_" + to_string(iNiv2) + "_0_0\",\"status\":"
                                    + to_string(pAlarme->GetStateSensorMust()) + ",\"sensoren\":[";
                    Send(str.c_str());                    
                    iVar = pAlarme->GetAnzSensoren();
                    pSensor = m_pIOGroup->m_pAlarm->GetSensorAddr();
                    for(i=0; i < iVar; i++)
                    {
                        iSensor = pAlarme->GetSensorNr(i) - 1;
                        str.clear();
                        if(i)
                            str = ",";
                        str += "{\"name\":\"" + (pSensor+iSensor)->GetBez() + "\",";
                        Send(str.c_str());
                        bStateSensor = (pSensor+iSensor)->GetState();
                        bStateMust = (pSensor+iSensor)->GetMust();
                        bStateEnabled = m_pIOGroup->m_pAlarm->GetEnabled(iSensor + 1); 
                        str = "\"status\":" + to_string(bStateSensor) + ",";
                        Send(str.c_str());
                        str = "\"must\":" + to_string(bStateMust) + ",";
                        Send(str.c_str());
                        str = "\"activated\":" + to_string(bStateEnabled) + ",";
                        Send(str.c_str());
                        str = "\"id\":\"4_" + to_string(iNiv2) + "_" + to_string(i+1) + "_0\"}";
                        Send(str.c_str());
                    }
                    str = "]}";
                    Send(str.c_str());
                }
                bTrue = false;
                break;
                
            case '3': // Alarmanlage aktivieren
                len = ReadBuf(buf, ';');
                if(iNiv2 == 1)
                    strPwd = m_pIOGroup->m_pAlarm->GetTechPwd();
                else
                    strPwd = m_pIOGroup->m_pAlarm->GetPwd();                    
                if(len && strcmp(buf, strPwd.c_str()) == 0)
                {
                    if(iNiv2 > 0 && iNiv2 <= m_pIOGroup->m_pAlarm->GetAnzAlarme())
                    {
                        bool bWait = true;
                        pthread_mutex_lock(&ext_mutexNodejs); 
                        m_pIOGroup->m_pAlarm->SetAlarm(iNiv2);
                        if(m_pIOGroup->m_pHistory != NULL)
                        {
                            if(m_pIOGroup->m_pAlarm->GetAlarmNr() == iNiv2)
                                m_pIOGroup->m_pHistory->SetDiffTage(0);
                        }
                        m_pIOGroup->SetBerechne();
                        pthread_mutex_unlock(&ext_mutexNodejs);    
                        while(bWait)
                        {
                            delayMicroseconds(100000);
                            pthread_mutex_lock(&ext_mutexNodejs); 
                            if(!m_pIOGroup->GetBerechne())
                                bWait = false;
                            pthread_mutex_unlock(&ext_mutexNodejs);                             
                        }
                    }
                }
                else
                    strError = "Falsches Passwort!";
                buf[0] = '9';
                break;
            
            case '4':
                if(m_pIOGroup->m_pHistory)
                {
                    bool bWait = true;
                    pthread_mutex_lock(&ext_mutexNodejs); 
                    m_pIOGroup->m_pHistory->SetDiffTage(iNiv4);
                    m_pIOGroup->SetBerechne();
                    pthread_mutex_unlock(&ext_mutexNodejs);    
                    while(bWait)
                    {
                        delayMicroseconds(100000);
                        pthread_mutex_lock(&ext_mutexNodejs); 
                        if(!m_pIOGroup->GetBerechne())
                            bWait = false;
                        pthread_mutex_unlock(&ext_mutexNodejs);  
                    }
                }
                buf[0] = '9';
                break;

            case '5': // einen Sensor für dieses Einschalten nicht berücksichtigen
                iVar = m_pIOGroup->m_pAlarm->GetParamInt(1); // Status abfragen
                if(iVar < 2 && iNiv2 >= 2 && iNiv2 <= m_pIOGroup->m_pAlarm->GetAnzAlarme())
                {
                    pAlarme = m_pIOGroup->m_pAlarm->GetAlarmeAddr(iNiv2-1);
                    if(iNiv3 > 0 && iNiv3 <= pAlarme->GetAnzSensoren())
                    {
                        iVar = pAlarme->GetSensorNr(iNiv3-1);
                        pthread_mutex_lock(&ext_mutexNodejs);                         
                        m_pIOGroup->m_pAlarm->SetEnabled(iVar, !m_pIOGroup->m_pAlarm->GetEnabled(iVar));
                        pthread_mutex_unlock(&ext_mutexNodejs); 
                        buf[0] = '2';
                    }
                    else
                        bTrue = false;
                }
                else
                    bTrue = false;
                break; 
            case '6': // Knopf  "Zurück" auf der Sensorenliste gedrückt
                buf[0] = '9';
                break;
            case '7': // Knopf "Aktualisieren" auf der Sensorenliste gedrückt
                buf[0] = '2';
                break;
                
            case '9': // Liste der Alarme anzeigen
                bWait = true;
                pthread_mutex_lock(&ext_mutexNodejs); 
                m_pIOGroup->SetBerechne();
                pthread_mutex_unlock(&ext_mutexNodejs);    
                while(bWait)
                {
                    delayMicroseconds(100000);
                    pthread_mutex_lock(&ext_mutexNodejs); 
                    if(!m_pIOGroup->GetBerechne())
                        bWait = false;
                    pthread_mutex_unlock(&ext_mutexNodejs);                             
                }
                iVar = m_pIOGroup->m_pAlarm->GetParamInt(1); // Status abfragen
                str = "{\"type\":1,\"status\":" + to_string(iVar) + ",";
                Send(str.c_str());  
                str = "\"alarmnr\":" + to_string( m_pIOGroup->m_pAlarm->GetAlarmNr()) + ",";
                Send(str.c_str());
                // strError wird bei falschem Passwort gesetzt und angezeigt wenn nicht leer
                str = "\"strError\":\"" + strError + "\",";
                Send(str.c_str());
                str = "\"alarm\":[";
                Send(str.c_str());
                iVar = m_pIOGroup->m_pAlarm->GetAnzAlarme();
                for(i=0; i < iVar; i++)
                {
                    pAlarme = m_pIOGroup->m_pAlarm->GetAlarmeAddr(i);
                    if(pAlarme)
                    {
                        str.clear();
                        if(i)
                            str = ",";
                        str += "{\"id\":\"4_" + to_string(i+1) + "_0_0\",";
                        Send(str.c_str());
                        str = "\"name\":\"" + pAlarme->GetBez() + "\",";
                        Send(str.c_str());
                        str = "\"sensor\":" + to_string((int)pAlarme->GetStateSensorMust()) + "}";
                        Send(str.c_str());
                    }
                    else
                        break;
                }
                if(m_pIOGroup->m_pHistory)
                    str = "],\"history\":1, \"tage\":" + to_string(m_pIOGroup->m_pHistory->GetDiffTage()) + "}";
                else 
                    str = "],\"history\":0}";
                Send(str.c_str());
                bTrue = false;
                break;

            default:
                bTrue = false;
                break;
        }
    }
}

void CBrowserSocket::VerwaltWS(int iNiv1, int iNiv2, int iNiv3, int iNiv4)
{
    char buf[MSGSIZE];
    string str;
    char ch;
    bool bTrue = true, bFirst;
    int i, iAnzValue, j, iAnzeigeArt;
    int jahr, monat, tag;
    CWSValue * pWSValue;
    double dblZaehlerStand, dblOffset, dblVerbrauch;
    struct tm pt;    
    
    ReadBuf(buf, ';');
    while(bTrue)
    {
        switch(buf[0]) {   
            case '3':
                //  es wird das 1.x auf einen Zähler geklickt, Anzeige aktuelles Jahr und Monat
                if(iNiv3 == 1 && iNiv4 == 1)
                {
                    iNiv4 = m_pUhr->getMonat()+1;
                    iNiv3 = 2;
                }
            case '4':
                str = "{\"type\":3,\"id\":\"5_" + to_string(iNiv2) + "_" + to_string(iNiv3) + "_" + to_string(iNiv4) + "\",\"zaehler\":[";
                Send(str.c_str());                
                iAnzValue = ANZWSVALUESTATISTIC;
                for(i=0; i < iAnzValue; i++) 
                {
                    pWSValue = m_pIOGroup->m_pWStation->GetPtrWSValue(i);
                    str.clear();
                    if(i)
                        str = ",";
                    str += "\"" + pWSValue->GetName() + "\"";
                    Send(str.c_str()); 
                }
                str = "],\"jahr\":[\"---\"";
                Send(str.c_str());
                pWSValue = m_pIOGroup->m_pWStation->GetPtrWSValue(iNiv2-1); 
                jahr = pWSValue->GetFirstYear();
                for(i = pWSValue->GetAktYear() - jahr + 1; i ; i--)
                {   
                    str = ",\""+ to_string(jahr + i - 1) + "\"";
                    Send(str.c_str());                    
                }
                str = "],\"monat\":[";
                Send(str.c_str());
                for(i=0; i < 13; i++) {
                    str.clear();
                    if(i)
                        str = ",";
                    str += "\"" + GetMonthText(i) + "\"";
                    Send(str.c_str());
                }
                str = "], \"arr\":[";
                Send(str.c_str());
                iNiv3--;
                iNiv4--;
                jahr = pWSValue->GetAktYear();
                jahr = jahr - (iNiv3-1); 

                if(iNiv3 == 0 && iNiv4 == 0)
                {   // Jahreswerte
                    // jahr = pZaehler->GetAktYear();
                    jahr--;
                    j = jahr - pWSValue->GetFirstYear();
                    for(i=0; i <= j; i++)
                    {
                        str.clear();
                        if(i)
                            str = ","; 
                        dblVerbrauch = pWSValue->GetDblWert(jahr - i, pWSValue->GetValue());
                        str += to_string(dblVerbrauch);
                        Send(str.c_str());
                    }
                }   
                else if(iNiv3 != 0 && iNiv4 == 0)
                {   // Monatswerte
                    for(monat=0; monat < 12; monat++)
                    {   
                        str.clear();
                        if(monat)
                            str = ",";
                        dblVerbrauch = pWSValue->GetDblWert(jahr, monat+1, pWSValue->GetValue());
                        str += to_string(dblVerbrauch);
                        Send(str.c_str());
                    }
                }
                else
                {   // Tageswerte
                    monat = iNiv4;
                    j = m_pUhr->getAnzahlTage(jahr, monat);
                    for(tag = 0; tag < j; tag++)
                    {
                        str.clear();
                        if(tag)
                            str = ",";
                        dblVerbrauch = pWSValue->GetDblWert(jahr, monat, tag+1, pWSValue->GetValue());
                        str += to_string(dblVerbrauch);
                        Send(str.c_str());
                    }
                }
                str = "]}";
                Send(str.c_str());
                bTrue = false;
                break;
            default:
                bTrue = false;
                break;
        }
    }
}

string CBrowserSocket::GetMonthText(int iVal)
{
    string str;
    
    switch(iVal) {
        case 0:
            str = "---";
            break;                            
        case 1:
            str = "Januar";
            break;
        case 2:
            str = "Februar";
            break;
        case 3:
            str = "März";
            break;
        case 4:
            str = "April";
            break;
        case 5:
            str = "Mai";
            break;
        case 6:
            str = "Juni";
            break;
        case 7:
            str = "Juli";
            break;
        case 8:
            str = "August";
            break;
        case 9:
            str = "September";
            break;
        case 10:
            str = "Oktober";
            break;
        case 11:
            str = "November";
            break;
        case 12:
            str = "Dezember";
            break;
        default :
            str = "---";
            break;
    }   
    return str;
}

void CBrowserSocket::VerwaltSensor(int iNiv1, int iNiv2, int iNiv3, int iNiv4)
{
    char buf[MSGSIZE];
    string str;
    char ch;
    bool bTrue = true;
    int i, iAnzSensor, j;
    CSensor *pSensor;
    
    ReadBuf(buf, ';');
    while(bTrue) 
    {   ch = buf[0];
        switch(ch) {   
            case '1': // Einzelnen Sensoren
                str = "{\"type\" : 1, \"sensor\" : [ ";
                Send(str.c_str());  
                iAnzSensor = m_pIOGroup->GetAnzSensor();
                for(i=0; i < iAnzSensor; i++)
                {
                    pSensor = m_pIOGroup->GetSensorAddress(i+1);
                    str.clear();
                    if(i)
                        str = ",";
                    str += "{\"typ\":\"" + to_string(pSensor->GetTyp()) + "\",\"id\":\"6_" + to_string(i+1) + "_0_0\",";
                    Send(str.c_str());
                    str = "\"text\":\"" + pSensor->GetName() + "\",";
                    Send(str.c_str());
                    str = "\"temp\":\"" + strDblRunden((double)pSensor->GetTemp() / 10.0, 1) + "\",";
                    Send(str.c_str());
                    str = "\"humidity\":\"" + strDblRunden((double)pSensor->GetHumidity() / 10.0, 1) + "\",";
                    Send(str.c_str());
                    str = "\"VocSignal\":\"" + to_string(pSensor->GetVocSignal()) + "\"}";
                    Send(str.c_str());
                }
                str = "]}";
                Send(str.c_str());
                bTrue = false;
                break;
            case '2': // Grafik eines Sensors
                iAnzSensor = m_pIOGroup->GetAnzSensor();
                if(iNiv2 > 0 && iNiv2 <= iAnzSensor)
                {
                    pSensor = m_pIOGroup->GetSensorAddress(iNiv2);  
                    str = "{\"type\":2,\"name\":\"" + pSensor->GetName() + "\",";
                    Send(str.c_str()); 
                    j = m_pUhr->getUhrmin();
                    str = "\"uhrmin\":\"" + to_string(j) + "\",\"messwerte\":[ ";
                    Send(str.c_str()); 
                    if(++j >= 1440)
                        j = 0;
                    for(i=0; i < 1440; i++)
                    {
                        str.clear();
                        if(i)
                            str = ",";
                        str += "{\"temp\":\"" + to_string(pSensor->m_sStatTemp[j]) + "\",\"humi\":\""
                                    + to_string( pSensor->m_sStatHumidity[j]) + "\",\"voc\":\""
                                    + to_string( pSensor->m_sStatVocSignal[j]) + "\"}";;
                        Send(str.c_str());
                        if(++j >= 1440)
                            j = 0;
                    }
                    str = "]}";
                    Send(str.c_str());
                    bTrue = false;
                }
                break;
            default:
                bTrue = false;
                break;
        }
    }
}

void CBrowserSocket::VerwaltAlarmClock(int iNiv1, int iNiv2, int iNiv3, int iNiv4)
{
    char buf[MSGSIZE];
    CAlarmClock *pAlarmClock;
    bool bFirst = true, bTrue = true;
    string str1, str2, str3;
    int i, iAnz, iTime, iNrClockAktiv;
    
    ReadBuf(buf, ';');
    while(bTrue)     
    {
        pAlarmClock = m_pIOGroup->GetAlarmClockAddress();
        pthread_mutex_lock(&ext_mutexNodejs);   
        iNrClockAktiv = pAlarmClock->GetState();
        pthread_mutex_unlock(&ext_mutexNodejs);  
        if(iNrClockAktiv)         
        {   
            if(buf[0] == '7') // Schlummern oder Stopp gedrückt
            {
                pthread_mutex_lock(&ext_mutexNodejs);  
                pAlarmClock->SetState(iNiv3); 
                m_pIOGroup->SetBerechne();                     
                pthread_mutex_unlock(&ext_mutexNodejs);  
                buf[0]= '1';   
            }
            else
            {
                // es steht ein Alarm an
                str1 = "{\"type\":6,\"id\":\"7_6_" + to_string(iNrClockAktiv) + "_0\","; 
                Send(str1.c_str());  
                pthread_mutex_lock(&ext_mutexNodejs);              
                str1 = pAlarmClock->GetStringAlarm(iNrClockAktiv-1); 
                pthread_mutex_unlock(&ext_mutexNodejs);                              
                Send(str1.c_str());  
                iTime = m_pUhr->getUhrmin();  
                str2 = ",\"uhrzeit\":\"" + strZweiStellen(iTime / 60) + ":" + strZweiStellen(iTime % 60)  + "\"}";
                Send(str2.c_str());          
                bTrue = false;
            }
        }
        else
        {
            switch(buf[0]) {
                case '1': // Liste der eingegebenen Alarmuhrzeiten
                    str1 = "{\"type\": 1,\"alarm\":[";   
                    Send(str1.c_str()); 
                    pthread_mutex_lock(&ext_mutexNodejs);                
                    iAnz = pAlarmClock->GetAnzAlarm();
                    for(i=0; i < iAnz; i++)
                    {   str1.clear();
                        if(i)
                            str1 = ",";
                        str1 += "{\"id\":\"7_1_" + to_string(i+1) + "_0\",";
                        str2 = pAlarmClock->GetStringAlarm(i) + "}";
                        str1 = str1 + str2;
                        Send(str1.c_str());
                    }
                    pthread_mutex_unlock(&ext_mutexNodejs);                 
                    str1 = "]}";
                    Send(str1.c_str());
                    bTrue = false;
                    break;
                case '2':   // Schalter ein/aus
                    ReadBuf(buf, ';');
                    pthread_mutex_lock(&ext_mutexNodejs);                  
                    if(strcmp(buf, "true") == 0)
                        pAlarmClock->SetActivated(iNiv3-1, true);
                    else
                        pAlarmClock->SetActivated(iNiv3-1, false);
                    pthread_mutex_unlock(&ext_mutexNodejs);                      
                    bTrue = false;
                    str1 = "{\"type\":2}";
                    Send(str1.c_str());                
                    break;
                case '3':   // Anfrage der Daten für die Dialogbox
                    str1 = "{\"type\": 3,\"id\":\"7_2_" + to_string(iNiv3) + "_0\",";            
                    pthread_mutex_lock(&ext_mutexNodejs);               
                    str2 = pAlarmClock->GetStringAlarm(iNiv3-1) + ",\"methodes\":[";
                    str3.clear();
                    iAnz = pAlarmClock->GetAnzMethod();
                    for(i=0; i < iAnz; i++)
                    {
                        if(i)
                            str3 += ",";
                        str3 += "\"" + pAlarmClock->GetStringMethod(i) + "\"";
                    }
                    pthread_mutex_unlock(&ext_mutexNodejs);
                    str3 += "]}";              
                    Send(str1.c_str());
                    Send(str2.c_str());
                    Send(str3.c_str());
                    bTrue = false;
                    break;
                case '4':  // neue Eingaben der Dialogbox
                    {   
                        CAlarmClockEntity alarmclockEntity; 
                        
                        alarmclockEntity.m_bActivated = true;
                        ReadBuf(buf, ';');
                        alarmclockEntity.m_strName = string(buf);
                        i = ReadNumber();
                        alarmclockEntity.m_sRepeat = (short)i;
                        ReadBuf(buf, ';');
                        if(strncmp(buf, "true", 4) == 0)
                            alarmclockEntity.m_bSnooze = true;
                        i = ReadNumber();
                        alarmclockEntity.m_iTimeMin = i;
                        i = ReadNumber();
                        alarmclockEntity.m_iMethod = i;
                        pthread_mutex_lock(&ext_mutexNodejs);  
                        pAlarmClock->SetParam(iNiv3, &alarmclockEntity); 
                        pthread_mutex_unlock(&ext_mutexNodejs);     
                        buf[0] = '1';        
                    }
                    break;
                case '5': // Eintrag löschen
                    pthread_mutex_lock(&ext_mutexNodejs);  
                    pAlarmClock->DeleteParam(iNiv3); 
                    pthread_mutex_unlock(&ext_mutexNodejs);  
                    buf[0]= '1';                
                    break;                 
                default:
                    bTrue = false;
                    break;
            }
        }
    }
    return;
}
