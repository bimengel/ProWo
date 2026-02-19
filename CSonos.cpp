/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CSonos.cpp
 * Author: josefengel
 * 
 * Created on January 18, 2021, 4:54 PM
 */

#include "ProWo.h"

CSonosEntity::CSonosEntity() {
    m_strID = "";
    m_strName = "";
}

CSonosEntity::~CSonosEntity() {
}

CSonos::CSonos(CIOGroup *pIOGroup) {
    
    LesDatei();
    
    m_pGroupEntity = NULL;
    m_pPlayerEntity = NULL;
    m_pFavoritesEntity = NULL;
    m_pPlaylistEntity = NULL;
    m_iHousehold = 1;
    m_iAnzGroup = 0;
    m_iAnzPlayer = 0;
    m_iAnzPlaylist = 0;
    m_iAnzFavorite = 0;
    m_pCurl = curl_easy_init();
    m_pIOGroup = pIOGroup;
    m_iStep = 0;
    m_iDelay = 0;
    pthread_mutex_init(&m_mutexSonosFifo, NULL); 
}

void CSonos::Control(time_t iUhrzeit)
{
    int iError = 0;

   
    if(iUhrzeit % 86400 == 7200) // um 4 Uhr Nachts iUhrzeit entspricht der Greenwich-Uhrzeit
    {
        m_iStep = 0;
        m_iDelay = 0;
    }
    if(m_iDelay)
    {
        if(iUhrzeit > m_iDelay + 3600) // wenn ein Fehler aufgetreten ist, Initialisierung nach 1 Stunde neustarten
        {
            m_iStep = 0;
            m_iDelay = 0;
        }
    }
    else
    {
        m_strReadBuffer = "";
        switch(m_iStep) {
        case 0: // Refreshtoken aufrufen
            iError = RefreshToken();
            m_SonosAktionEntity.m_iState = -1;
            break;
        case 1: // ReadHouseholds aufrufen
            iError = ReadHouseholds();
            break;
        case 2: // ReadGroups aufrufen 
            iError = ReadGroups();
            break;
        case 3:
            iError = ReadFavorites();
            break;
        case 4:
            iError = ReadPlaylists();
            break;            
        case 5: // Operation ausführen
            iError = ExecuteOperation();
            break;
        default:
            iError = 1;
            break;
        }
        if(iError)
            m_iDelay = iUhrzeit;
    }
}

int CSonos::ExecuteOperation()
{
    int iRet=0, i, iCommand, iState, iVol;
    string str;

    pthread_mutex_lock(&m_mutexSonosFifo);  
    if(m_SonosAktionEntity.m_iState == -1 && !m_SonosFifo.empty())
    {
        m_SonosAktionEntity = m_SonosFifo.front();
        m_SonosFifo.pop();
               
        str = m_strGroupConfig[m_SonosAktionEntity.m_iNrGroup - 1];
        m_strID = "";
        for(i=0; i < m_iAnzGroup; i++)
        {
            if(str.compare(m_pGroupEntity[i].m_strName) == 0)
                m_strID = m_pGroupEntity[i].m_strID;
        }
        if(m_strID.empty())
        {
            // Gruppe nicht gefunden, also alles neu einlesen!
            syslog(LOG_ERR, "Sonos Execution: group not found!");
            return 1;
        }    
    }
    pthread_mutex_unlock(&m_mutexSonosFifo);    
    iCommand = m_SonosAktionEntity.m_iState % 256;
    iState = m_SonosAktionEntity.m_iState / 256;
    switch(iCommand) {
    case -1: // kein Kommando
        break;
    case 0: // Ausschalten
        iRet = Execute(0, 0);
        iCommand = -1;
        break;
    case 1: // Einschalten
        // wird zweimal durchlaufen wenn einschalten mit Lautstärke 
        if(iState) // mit Lautstärke
        {
            iRet = Execute(4, iState);  
            iState = 0;
        }
        else
        {
            iRet = Execute(1, 0);
            iCommand = -1;
        }
        break;
    case 2: // Favoriten 
        iRet = Execute(2, iState);
        iCommand = -1;
        break;
    case 3: // Playlist setzen
        iRet = Execute(3, iState);
        iCommand = -1;
        break;
    case 4: // Lautstärke setzen
        iRet = Execute(4, iState);
        iCommand = -1;
        break;
    case 5: // Lautstärke erhöhen
    case 6: // Lautstärke niedriger    
        if(iCommand == 6)
            iState = -iState;
        iRet = Execute(5, iState);
        iCommand = -1;
        break;
    default:
        iCommand = -1;
        iRet = 1;
        break;
    }
    if(iCommand == -1)
        m_SonosAktionEntity.m_iState = -1;
    else
        m_SonosAktionEntity.m_iState = iState * 256 + iCommand;
    return iRet;
}
int CSonos::GetAnzGroupConfig()
{
    return m_strGroupConfig.size();
}

int CSonos::GetAnzFavoriteConfig()
{
    return m_strFavoriteConfig.size();
}

CSonos::~CSonos() {
 
    if(m_pCurl != NULL)
    {
        curl_easy_cleanup(m_pCurl);   
        m_pCurl = NULL;
    }
    if(m_pGroupEntity != NULL)
    {
        delete [] m_pGroupEntity;
        m_pGroupEntity = NULL;
    }   
    if(m_pPlayerEntity != NULL)
    {
        delete [] m_pPlayerEntity;
        m_pPlayerEntity = NULL;
    }
    if(m_pFavoritesEntity != NULL)
    {
        delete [] m_pFavoritesEntity;
        m_pFavoritesEntity = NULL;
    }
    if(m_pPlaylistEntity != NULL)
    {
        delete [] m_pPlaylistEntity;
        m_pPlaylistEntity = NULL;
    }

    pthread_mutex_destroy(&m_mutexSonosFifo);       
}

size_t CSonos::write_data(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int CSonos::RefreshToken()
{
    string str, strError;
    CJson *pJson = new CJson;
    CJsonNodeValue pJsonNodeValue;    
    int iErr;
    CURLcode res;   
    bool bSuccess;
    int iRet = 0;
    
    while(true)
    {
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L); 
        // Timeout auf 10 Sekunden
        curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);   
        curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, m_curlErrorBuffer);

        // Authentication
        curl_easy_setopt(m_pCurl, CURLOPT_USERNAME, m_strClientId.c_str());
        curl_easy_setopt(m_pCurl, CURLOPT_PASSWORD, m_strClientSecret.c_str()); 
        curl_easy_setopt(m_pCurl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);

        // Url
        curl_easy_setopt(m_pCurl, CURLOPT_URL, "https://api.sonos.com/login/v3/oauth/access");

        // Post fields
        curl_easy_setopt(m_pCurl, CURLOPT_POST, 1L);
        str = "grant_type=refresh_token&refresh_token=" + m_strRefreshToken;
        curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, str.c_str()); 

        // Empfang
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CSonos::write_data);
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&m_strReadBuffer);

        // Perform the request, res will get the return code 
        res = curl_easy_perform(m_pCurl);            
        if(res != CURLE_OK)
        {
            strError = "SONOS RefreshToken (2): " + string(m_curlErrorBuffer);
            iRet = 2;
            break;
        }
        else
        {
            bSuccess = pJson->parse((char *)m_strReadBuffer.c_str(), m_strReadBuffer.length(), &iErr);
            if(bSuccess)
            {
                if(m_pIOGroup->GetTest() == 4)
                    pJson->WriteToFile("Refreshtoken", 14, true);                
                pJson->get((char *)"access_token", &pJsonNodeValue);
                if(pJsonNodeValue.m_iTyp == 4)
                    m_strAuthorization = "Authorization: Bearer " + pJsonNodeValue.m_strValue;
                else 
                {
                    strError = "SONOS RefreshToken(3): access_token not found!";
                    iRet = 3;
                    break;
                }
                pJson->get((char *)"refresh_token", &pJsonNodeValue);
                if(pJsonNodeValue.m_iTyp == 4)
                {
                    m_strRefreshToken = pJsonNodeValue.m_strValue;
                    m_iStep++;
                    break;
                }
                else
                {
                    strError = "SONOS RefreshToken (4): refresh_token not found!";
                    iRet = 4;
                    break;
                }
            }
            else
            {
                strError = "SONOS RefreshToken (5): " + m_strReadBuffer + ", JSON error at pos " + to_string(iErr);
                iRet = 5;
                break;
            }
        }
        break;
    }
    curl_easy_reset(m_pCurl);
    delete pJson;

    if(iRet)
        syslog(LOG_ERR, strError.c_str());
    
    return iRet;
}

void CSonos::SetState(int iNrGroup, int iState)
{
    struct SonosAktionEntity i;
    i.m_iNrGroup = iNrGroup;
    i.m_iState = iState;
    pthread_mutex_lock(&m_mutexSonosFifo);  
    m_SonosFifo.push(i);
    pthread_mutex_unlock(&m_mutexSonosFifo);     
}

void CSonos::LesDatei()
{
   	CReadFile *pReadFile;
    string strKey, str;
    BYTE bAll = 0;
	pReadFile = new CReadFile(); 
    pReadFile->OpenRead (pProgramPath, 13);   

    while(true)
    {
        if(pReadFile->ReadLine())
		{
			strKey = pReadFile->ReadText(':');
            pReadFile->GetChar();
            str = pReadFile->ReadText();
            if(str.empty())
                pReadFile->Error(45);            
			if(strKey.compare("Key") == 0)
            {
                m_strClientId = str;
                bAll += 0x01;
            }
            else if(strKey.compare("Secret") == 0)
            {
                m_strClientSecret = str;
                bAll += 0x02;
            }
            else if(strKey.compare("RefreshToken") == 0)
            {
                m_strRefreshToken = str;
                bAll += 0x04;
            }
            else if(strKey.compare("Household") == 0)
            {
                m_iHousehold = atoi(str.c_str());
                bAll += 0x08;
            }
            else if(strKey.compare("Group") == 0)
                m_strGroupConfig.push_back(str);
            else if(strKey.compare("Favorite") == 0)
                m_strFavoriteConfig.push_back(str);
            else if(strKey.compare("Playlist") == 0)
                m_strPlaylistConfig.push_back(str);
            else
                pReadFile->Error(154);
        }
        else
            break;
    }
    
    if(bAll != 0x0F)
    {
        if(!(bAll & 0x01))
            pReadFile->Error(118);
        if(!(bAll & 0x02))
            pReadFile->Error(119);
        if(!(bAll & 0x04))
            pReadFile->Error(120);
        if(!(bAll & 0x08))
            pReadFile->Error(122);
    }

    pReadFile->Close();
    delete pReadFile;
    
}

int CSonos::ReadHouseholds()
{
    string strError;
    int iRet = 0;
    CURLcode res;   
    struct curl_slist* headers = NULL;
    bool bSuccess;
    CJson *pJson = new CJson;
    CJsonNodeValue pJsonNodeValue; 
    int iErr;
  
    while(true)
    {
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L); 
        // Timeout auf 10 Sekunden
        curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);   
        curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, m_curlErrorBuffer);

        // Url
        curl_easy_setopt(m_pCurl, CURLOPT_URL, "https://api.ws.sonos.com/control/api/v1/households");    

        // Senden
        headers = curl_slist_append(headers, "Content_Type:application/json");
        headers = curl_slist_append(headers, m_strAuthorization.c_str());
        curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers);

        // Empfang
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CSonos::write_data);
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&m_strReadBuffer);    

        // Perform the request, res will get the return code 
        res = curl_easy_perform(m_pCurl);            
        if(res != CURLE_OK)
        {
            strError = "SONOS ReadHouseholds (2): " + string(m_curlErrorBuffer);
            iRet = 2;
        }
        else
        {   
            bSuccess = pJson->parse((char *)m_strReadBuffer.c_str(), m_strReadBuffer.length(), &iErr);
            if(bSuccess)
            {
                if(m_pIOGroup->GetTest() == 4)
                    pJson->WriteToFile("Households", 14, false);  
                pJson->get((char *)"households", &pJsonNodeValue)->get(m_iHousehold-1, &pJsonNodeValue)->get((char *)"id", &pJsonNodeValue);
                if(pJsonNodeValue.m_iTyp == 4)
                {
                    m_strHousehold = pJsonNodeValue.m_strValue;
                    m_iStep++;
                }
                else 
                {
                    strError = "SONOS ReadHouseholds (3): households not found";
                    iRet = 3;
                }
              
            } 
            else
            {
                strError = "SONOS RedHouseholds (4): %s, JSON error at pos " + to_string(iErr);
                iRet = 5;
            }            
        }
        break;
    }
    curl_easy_reset(m_pCurl);
    delete pJson;
    
    if(iRet)
        syslog(LOG_ERR, strError.c_str());
    
    return iRet;
}

int CSonos::ReadGroups()
{
    int i, j, iRet = 0;
    CJson *pJson = new CJson;
    CJsonNode *pJsonNode;
    CJsonNodeValue pJsonNodeValue;     
    CURLcode res;   
    struct curl_slist* headers = NULL;
    string strName, strID;
    string strError;
    int iErr;
    bool bSuccess;
   
    while(true)
    {
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L); 
        // Timeout auf 10 Sekunden
        curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);   
        curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, m_curlErrorBuffer);

        // Url
        m_strUrl = "https://api.ws.sonos.com/control/api/v1/households/" + m_strHousehold + "/groups";
        curl_easy_setopt(m_pCurl, CURLOPT_URL, m_strUrl.c_str());    

        headers = curl_slist_append(headers, "Content_Type:application/json");
        headers = curl_slist_append(headers, "charset=utf-8");
        headers = curl_slist_append(headers, m_strAuthorization.c_str());
        curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers);

        // Empfang
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CSonos::write_data);
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&m_strReadBuffer);    

        // Perform the request, res will get the return code 
        res = curl_easy_perform(m_pCurl);            
        if(res != CURLE_OK)
        {
            strError = "SONSO ReadGroups (3): " + string(m_curlErrorBuffer);
            iRet = 3;
            break;
        }
        else
        {
            bSuccess = pJson->parse((char *)m_strReadBuffer.c_str(), m_strReadBuffer.length(), &iErr);
            if(bSuccess)
            {
                if(m_pIOGroup->GetTest() == 4)
                    pJson->WriteToFile("Groups", 14, false);                  
                m_iStep++;
                pJsonNode = pJson->get((char *)"groups", &pJsonNodeValue);
                j = pJsonNode->size();
                if(m_pGroupEntity != NULL)
                    delete [] m_pGroupEntity;
                m_pGroupEntity = new CSonosEntity[j];
                m_iAnzGroup = j;
                for(i=0; i < j; i++)
                {
                    pJson->get((char *)"groups", &pJsonNodeValue)
                            ->get(i, &pJsonNodeValue)
                            ->get((char *)"name", &pJsonNodeValue);
                    strName = pJsonNodeValue.asString();
                    pJson->get((char *)"groups", &pJsonNodeValue)
                            ->get(i, &pJsonNodeValue)
                            ->get((char *)"id", &pJsonNodeValue);   
                    strID = pJsonNodeValue.asString();                 
                    m_pGroupEntity[i].m_strID = strID;
                    m_pGroupEntity[i].m_strName = strName;
                }
                j = pJson->get((char *)"players",  &pJsonNodeValue)->size();
                if(m_pPlayerEntity != NULL)
                    delete [] m_pPlayerEntity;
                m_pPlayerEntity = new CSonosEntity[j];
                m_iAnzPlayer = j;
                for(i=0; i < j; i++)
                {
                    pJsonNode = pJson->get((char *)"players",  &pJsonNodeValue)
                            ->get(i,  &pJsonNodeValue);
                    pJsonNode->get((char *)"name",  &pJsonNodeValue);
                    strName = pJsonNodeValue.asString();
                    pJsonNode->get((char *)"id",  &pJsonNodeValue);
                    strID = pJsonNodeValue.asString();
                    m_pPlayerEntity[i].m_strID = strID;
                    m_pPlayerEntity[i].m_strName = strName;
                }               
            }
            else
            {
                strError = "SONOS ReadGroups (5): JSON error at pos " + to_string(iErr) + ", " + m_strReadBuffer;
                iRet = 5;
                break;
            }
        }
        break;
    }

    curl_easy_reset(m_pCurl);
    delete pJson;
    
    if(iRet)
        syslog(LOG_ERR, strError.c_str());
    return iRet;
}
int CSonos::ReadFavorites()
{
    int i, j, iRet = 0;
    CURLcode res;   
    struct curl_slist* headers = NULL;
    string strName, strID;
    CJson *pJson = new CJson;
    CJsonNode *pJsonNode;
    CJsonNodeValue pJsonNodeValue;  
    int iErr;
    bool bSuccess;
    string strError;

    while(true)
    {
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L); 
        // Timeout auf 10 Sekunden
        curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);   
        curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, m_curlErrorBuffer);

        // Urlm_strHousehold
        m_strUrl = "https://api.ws.sonos.com/control/api/v1/households/" + m_strHousehold + "/favorites";
        curl_easy_setopt(m_pCurl, CURLOPT_URL, m_strUrl.c_str());    

        headers = curl_slist_append(headers, "Content_Type:application/json");
        headers = curl_slist_append(headers, "charset=utf-8");
        headers = curl_slist_append(headers, m_strAuthorization.c_str());
        curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers);


        // Empfang
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CSonos::write_data);
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&m_strReadBuffer);    

        // Perform the request, res will get the return code 
        res = curl_easy_perform(m_pCurl);            
        if(res != CURLE_OK)
        {
            strError = "ReadFavorites (3) : " +  string(m_curlErrorBuffer);
            iRet = 3;
            break;
        }
        else
        {
            m_iStep++;
            bSuccess = pJson->parse((char *)m_strReadBuffer.c_str(), m_strReadBuffer.length(),  &iErr);
            if(bSuccess)
            {
                if(m_pIOGroup->GetTest() == 4)
                    pJson->WriteToFile("Favorites", 14, false);                 
                pJsonNode = pJson->get((char *)"items",  &pJsonNodeValue);
                j = pJsonNode->size();
                if(m_pFavoritesEntity != NULL)
                {
                    delete [] m_pFavoritesEntity;
                    m_pFavoritesEntity = NULL;
                }
                m_pFavoritesEntity = new CSonosEntity[j];
                m_iAnzFavorite = j;
                for(i=0; i < j; i++)
                {
                    pJsonNode = pJson->get((char *)"items",  &pJsonNodeValue)
                                    ->get(i, &pJsonNodeValue);
                    pJsonNode->get((char *)"name", &pJsonNodeValue);
                    strName = pJsonNodeValue.asString();
                    pJsonNode->get((char *)"id", &pJsonNodeValue);                    
                    strID = pJsonNodeValue.asString();
                    m_pFavoritesEntity[i].m_strID = strID;
                    m_pFavoritesEntity[i].m_strName = strName;
                }                
            }
            else
            {
                strError = "ReadFavorites (5) : JSON error at " + to_string(iErr) + ", " + m_strReadBuffer;
                iRet = 5;
                break;
            }
        }
        break;
    }
    curl_easy_reset(m_pCurl);
    delete pJson;

    if(iRet)
        syslog(LOG_ERR, strError.c_str());
    return iRet;

}
int CSonos::ReadPlaylists()
{
    int i, j, iRet = 0;
    CURLcode res;   
    struct curl_slist* headers = NULL;
    string strName, strID;
    CJson *pJson = new CJson;
    CJsonNode *pJsonNode;
    CJsonNodeValue pJsonNodeValue;  
    int iErr;
    bool bSuccess;
    string strError;

    while(true)
    {
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L); 
        // Timeout auf 10 Sekunden
        curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);   
        curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, m_curlErrorBuffer);

        // Urlm_strHousehold
        m_strUrl = "https://api.ws.sonos.com/control/api/v1/households/" + m_strHousehold + "/playlists";
        curl_easy_setopt(m_pCurl, CURLOPT_URL, m_strUrl.c_str());    

        headers = curl_slist_append(headers, "Content_Type:application/json");
        headers = curl_slist_append(headers, "charset=utf-8");
        headers = curl_slist_append(headers, m_strAuthorization.c_str());
        curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers);

        // Empfang
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CSonos::write_data);
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&m_strReadBuffer);    

        // Perform the request, res will get the return code 
        res = curl_easy_perform(m_pCurl);            
        if(res != CURLE_OK)
        {
            strError = "Playlists (3) : " +  string(m_curlErrorBuffer);
            iRet = 3;
            break;
        }
        else
        {
            m_iStep++;
            bSuccess = pJson->parse((char *)m_strReadBuffer.c_str(), m_strReadBuffer.length(),  &iErr);
            if(bSuccess)
            {
                if(m_pIOGroup->GetTest() == 4)
                    pJson->WriteToFile("Playlists", 14, false);  
                pJsonNode = pJson->get((char *)"playlists",  &pJsonNodeValue);
                j = pJsonNode->size();    
                if(m_pPlaylistEntity != NULL)
                {
                    delete [] m_pPlaylistEntity;
                    m_pPlaylistEntity = NULL;
                }
                m_pPlaylistEntity = new CSonosEntity[j];
                m_iAnzPlaylist = j;
                for(i=0; i < j; i++)
                {
                    pJsonNode = pJson->get((char *)"playlists",  &pJsonNodeValue)
                                    ->get(i, &pJsonNodeValue);
                    pJsonNode->get((char *)"name", &pJsonNodeValue);
                    strName = pJsonNodeValue.asString();
                    pJsonNode->get((char *)"id", &pJsonNodeValue);                    
                    strID = pJsonNodeValue.asString();
                    m_pPlaylistEntity[i].m_strID = strID;
                    m_pPlaylistEntity[i].m_strName = strName;
                }                                     
            }
            else
            {
                strError = "Playlists (5) : JSON error at " + to_string(iErr) + ", " + m_strReadBuffer;
                iRet = 5;
                break;
            }
        }
        break;
    }
    curl_easy_reset(m_pCurl);
    delete pJson;

    if(iRet)
        syslog(LOG_ERR, strError.c_str());
    return iRet;

}

int CSonos::Execute(int iAktion, int iState)
{
    string strError, strData, str;
    int len;
    CJson *pJson = new CJson;
    CJsonNode *pJsonNode;
    CJsonNodeValue JsonNodeValue;  
    CURLcode res;   
    bool bSuccess;
    int iErr, j;
    int iRet = 0;

    m_pCurl = curl_easy_init();
    strData.clear();
    m_strUrl = "https://api.ws.sonos.com/control/api/v1/groups/" + m_strID;
    switch(iAktion) {
        case 0: // Ausschalten
            m_strUrl += "/playback/pause";
            break;
        case 1: // Einschalten
            m_strUrl += "/playback/play";
            break;
        case 2: // Favorite setzen
            m_strUrl += "/favorites";
            str = m_strFavoriteConfig.at(iState-1);
            m_strID = "";
            j = m_iAnzFavorite;
            for(int i=0; i < j; i++)
            {
                if(str.compare(m_pFavoritesEntity[i].m_strName) == 0)
                    m_strID = m_pFavoritesEntity[i].m_strID;
            }
            if(m_strID.empty())
            {
                strError = "Favorite \"" + str +"\" not in the list";
                syslog(LOG_ERR, strError.c_str());
                return 0;
            }
            strData = "{\"favoriteId\":\"" + m_strID + "\",\"action\":\"PLAY_NOW\"," +
                        "\"playOnCompletion\":true}";           
            break;
        case 3: // Playlist setzen
            m_strUrl += "/playlists"; 
            j = m_strPlaylistConfig.size();
            if(iState < 1 || iState > j)
            {
                strError = "Incorrect number for Playlist: " + to_string(iState);
                syslog(LOG_ERR, strError.c_str());
                return 0;
            }
            str = m_strPlaylistConfig.at(iState-1);
            m_strID = "";
            j = m_iAnzPlaylist;
            for(int i=0; i < j; i++)
            {
                if(str.compare(m_pPlaylistEntity[i].m_strName) == 0)
                    m_strID = m_pPlaylistEntity[i].m_strID;
            }
            if(m_strID.empty())
            {
                strError = "Playlist \"" + str +"\" not in the list";
                syslog(LOG_ERR, strError.c_str());
                return 0;
            }
            strData = "{\"playlistId\":\"" + m_strID + "\",\"playOnCompletion\":true," +
                        "\"playModes\":{\"repeat\":true,\"repeatOne\":false,\"shuffle\":true,\"crossfade\":false}," +
                        "\"action\":\"PLAY_NOW\"}";
            break;
        case 4: // iWert = Lautstärke setzen
            m_strUrl += "/groupVolume";
            strData = "{\"volume\":" + to_string(iState) + "}";
            break;    
        case 5:
            m_strUrl += "/groupVolume/relative";
            strData = "{\"volumeDelta\":" + to_string(iState) + "}";
            break;        
        case 100: // Lautsärke lesen
            m_strUrl += "/groupVolume";
            break;
        default:
            strError = "SONOS Paramer, action (" + to_string(iAktion) + "incorrect!";
            syslog(LOG_ERR, strError.c_str());
            return 0;
            break;
    }

    res = CurlPerform(strData);           
    if(res != CURLE_OK)
    {
        strError = "SONOS Verwalt (1) / " + string( m_curlErrorBuffer);
        syslog(LOG_ERR, strError.c_str());  
        m_iStep = 0;
    }
    else
    {   
        bSuccess = pJson->parse((char *)m_strReadBuffer.c_str(), m_strReadBuffer.length(), &iErr);
        if(bSuccess)
        {   
            pJson->get((char *)"fault", &JsonNodeValue);
            if(JsonNodeValue.m_iTyp != 0) // vorhanden also Fehler
            {
                pJson->get((char *)"fault", &JsonNodeValue)->get((char *)"faultstring", &JsonNodeValue);
                strError = "SONOS error (fault): " + JsonNodeValue.asString();
                syslog(LOG_ERR, strError.c_str());
            }
            else
            {
                pJson->get((char *)"errorCode", &JsonNodeValue);
                if(JsonNodeValue.m_iTyp != 0) // vorhanden also Fehler
                {
                    strError = "SONOS error (errorCode): " + JsonNodeValue.asString() +
                        " iAktion = " + to_string(iAktion) + " iState = " + to_string(iState);
                    syslog(LOG_ERR, strError.c_str());
                }
                else 
                    iRet = 0;
            }
        }
        else
        {   
            strError = "SONOS JSON error at pos: " + to_string(iErr) + "(curl), ID: " + m_strID + ", action: " +
                            to_string(iAktion) + " ReadBuffer: " + m_strReadBuffer;
            syslog(LOG_ERR, strError.c_str());
        }
    }
    curl_easy_reset(m_pCurl);
    return iRet;
}

CURLcode CSonos::CurlPerform(string strData)
{
    CURLcode res;   
    struct curl_slist* headers = NULL;

    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L); 
    // Timeout auf 10 Sekunden
    curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);   
    curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, m_curlErrorBuffer);

    // Url
    curl_easy_setopt(m_pCurl, CURLOPT_URL, m_strUrl.c_str());    

    // Header setzen
    headers = curl_slist_append(headers, "Content-Type:application/json;charset=utf-8");
    headers = curl_slist_append(headers, m_strAuthorization.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers);

    // Data (body)
    if(strData.empty())
        curl_easy_setopt(m_pCurl, CURLOPT_HTTPGET, 1L);
    else   
    {
        curl_easy_setopt(m_pCurl, CURLOPT_POST, 1L);
        curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, strData.c_str());
    }

    // write data
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CSonos::write_data);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&m_strReadBuffer);
    m_strReadBuffer = "";

    // Perform the request, res will get the return code 
    res = curl_easy_perform(m_pCurl);  
    return res;  
}

//
//  CBerechneSonos
//
void CBerechneSonos::init(int iNr, CSonos *pSonos)
{
    m_iNrGroup = iNr;
    m_pSonos = pSonos;
}

void CBerechneSonos::SetState(int state)
{
    m_pSonos->SetState(m_iNrGroup, state);
}

