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
}

CSonosEntity::~CSonosEntity() {
}

CSonos::CSonos(CIOGroup *pIOGroup) {
    
    LesDatei();
    
    m_pGroupEntity = NULL;
    m_pPlayerEntity = NULL;
    m_pFavoritesEntity = NULL;
    m_iHousehold = 1;
    m_iAnzGroup = 0;
    m_iAnzPlayer = 0;
    m_iAnzFavorites = 0;
    m_pCurl = curl_easy_init();
    m_pIOGroup = pIOGroup;
    m_iStep = 0;
    m_iDelay = 0;
    m_iSonosACID = -1;

    pthread_mutex_init(&m_mutexSonosFifo, NULL); 

}

int CSonos::GetAnzGroupConfig()
{
    return m_iAnzGroupConfig;
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
    string strReadBuffer;
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
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&strReadBuffer);

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
            bSuccess = pJson->parse((char *)strReadBuffer.c_str(), strReadBuffer.length(), &iErr);

            if(bSuccess)
            {
                if(m_pIOGroup->GetTest() == 4)
                    pJson->WriteToFile("Refreshtoken", 14, true);                
                pJson->get((char *)"access_token", &pJsonNodeValue);
                if(pJsonNodeValue.m_iTyp == 4)
                    m_strToken = pJsonNodeValue.m_strValue;
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
                strError = "SONOS RefreshToken (5): " + strReadBuffer + ", JSON error at pos " + to_string(iErr);
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
    m_iAnzGroupConfig = 0;

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
        }
        else
            break;
        
    }
    m_iAnzGroupConfig = m_strGroupConfig.size();
    
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
    string str, strError;
    int iRet = 0;
    CURLcode res;   
    string strReadBuffer;
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
        str = "Authorization: Bearer " + m_strToken;
        headers = curl_slist_append(headers, str.c_str());
        curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers);

        // Empfang
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CSonos::write_data);
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&strReadBuffer);    

        // Perform the request, res will get the return code 
        res = curl_easy_perform(m_pCurl);            
        if(res != CURLE_OK)
        {
            strError = "SONOS ReadHouseholds (2): " + string(m_curlErrorBuffer);
            iRet = 2;
        }
        else
        {   
            bSuccess = pJson->parse((char *)strReadBuffer.c_str(), strReadBuffer.length(), &iErr);
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
    string strReadBuffer;
    string strName, strID;
    string strBuffer, strError;
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
        strBuffer = "https://api.ws.sonos.com/control/api/v1/households/" + m_strHousehold + "/groups";
        curl_easy_setopt(m_pCurl, CURLOPT_URL, strBuffer.c_str());    

        headers = curl_slist_append(headers, "Content_Type:application/json");
        headers = curl_slist_append(headers, "charset=utf-8");
        strBuffer = "Authorization: Bearer " + m_strToken;
        headers = curl_slist_append(headers, strBuffer.c_str());
        curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers);

        // Empfang
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CSonos::write_data);
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&strReadBuffer);    

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
            bSuccess = pJson->parse((char *)strReadBuffer.c_str(), strReadBuffer.length(), &iErr);
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
                strError = "SONOS ReadGroups (5): JSON error at pos " + to_string(iErr) + ", " + strReadBuffer;
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
    string strReadBuffer;
    string strName, strID;
    CJson *pJson = new CJson;
    CJsonNode *pJsonNode;
    CJsonNodeValue pJsonNodeValue;  
    int iErr;
    bool bSuccess;
    string str, strError;

    while(true)
    {
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L); 
        // Timeout auf 10 Sekunden
        curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);   
        curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, m_curlErrorBuffer);

        // Urlm_strHousehold
        str = "https://api.ws.sonos.com/control/api/v1/households/" + m_strHousehold + "/favorites";
        curl_easy_setopt(m_pCurl, CURLOPT_URL, str.c_str());    

        headers = curl_slist_append(headers, "Content_Type:application/json");
        headers = curl_slist_append(headers, "charset=utf-8");
        str =  "Authorization: Bearer "  + m_strToken;
        headers = curl_slist_append(headers, str.c_str());
        curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers);


        // Empfang
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CSonos::write_data);
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&strReadBuffer);    

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
            bSuccess = pJson->parse((char *)strReadBuffer.c_str(), strReadBuffer.length(),  &iErr);
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
                m_iAnzFavorites = j;
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
                strError = "ReadFavorites (5) : JSON error at " + to_string(iErr) + ", " + strReadBuffer;
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
    string strReadBuffer;
    string strName, strID;
    CJson *pJson = new CJson;
    CJsonNode *pJsonNode;
    CJsonNodeValue pJsonNodeValue;  
    int iErr;
    bool bSuccess;
    string str, strError;

    while(true)
    {
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L); 
        // Timeout auf 10 Sekunden
        curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);   
        curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, m_curlErrorBuffer);

        // Urlm_strHousehold
        str = "https://api.ws.sonos.com/control/api/v1/households/" + m_strHousehold + "/playlists";
        curl_easy_setopt(m_pCurl, CURLOPT_URL, str.c_str());    

        headers = curl_slist_append(headers, "Content_Type:application/json");
        headers = curl_slist_append(headers, "charset=utf-8");
        str =  "Authorization: Bearer "  + m_strToken;
        headers = curl_slist_append(headers, str.c_str());
        curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers);


        // Empfang
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CSonos::write_data);
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&strReadBuffer);    

        // Perform the request, res will get the return code 
        res = curl_easy_perform(m_pCurl);            
        if(res != CURLE_OK)
        {
            strError = "ReadPlaylists (3) : " +  string(m_curlErrorBuffer);
            iRet = 3;
            break;
        }
        else
        {
            m_iStep++;
            bSuccess = pJson->parse((char *)strReadBuffer.c_str(), strReadBuffer.length(),  &iErr);
            if(bSuccess)
            {
                if(m_pIOGroup->GetTest() == 4)
                    pJson->WriteToFile("Playlists", 14, false);  
                pJsonNode = pJson->get((char *)"playlists",  &pJsonNodeValue);
                j = pJsonNode->size();                    
                for(i=0; i < j; i++)
                {
                    pJsonNode = pJson->get((char *)"playlists",  &pJsonNodeValue)
                                    ->get(i, &pJsonNodeValue);
                    pJsonNode->get((char *)"name", &pJsonNodeValue);
                    strName = pJsonNodeValue.asString();
                    if(strName.compare("ProWo") == 0)
                    {   pJsonNode->get((char *)"id", &pJsonNodeValue);  
                        m_iSonosACID = atoi(pJsonNodeValue.asString().c_str());
                        break;
                    }
                }                      
                                               
            }
            else
            {
                strError = "ReadPlaylists (5) : JSON error at " + to_string(iErr) + ", " + strReadBuffer;
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

int CSonos::Execute(string strGroupID, int iState)
{
    string strError, strUrl, strData, strAuthorization;
    CURLcode res;   
    struct curl_slist* headers = NULL;
    int iWert, iAktion, len;
    string strReadBuffer;
    CJson *pJson = new CJson;
    CJsonNode *pJsonNode;
    CJsonNodeValue JsonNodeValue;     
    bool bPost = true;
    bool bSuccess;
    int iErr;
    int iRet = -1;

    strData.clear();
    iWert = iState / 256;
    iAktion = iState % 256;
    switch(iAktion) {
        case 0: // Ausschalten
            strUrl = "https://api.ws.sonos.com/control/api/v1/groups/" + strGroupID + "/playback/pause";
            break;
        case 1: // Einschalten
            strUrl = "https://api.ws.sonos.com/control/api/v1/groups/" + strGroupID + "/playback/play";
            break;
        case 2: // Favorite setzen
            strUrl = "https://api.ws.sonos.com/control/api/v1/groups/" + strGroupID + "/favorites";
            strData = "{\"favoriteId\":\"" + to_string(iWert-1) + "\",\"action\":\"REPLACE\"}";           
            break;
        case 3: // iWert = Lautstärke setzen
            strUrl = "https://api.ws.sonos.com/control/api/v1/groups/" + strGroupID + "/groupVolume";
            strData = "{\"volume\":" + to_string(iWert) + "}";
            break;
        case 4: // Lautstärke erhöhen
            break;
        case 5: // Lautstärke mindern
            break;
        case 6: // Playlist ProWo setzen
            strUrl = "https://api.ws.sonos.com/control/api/v1/groups/" + strGroupID + "/playlists";    
            strData = "{\"playlistId\":\"" + to_string(m_iSonosACID) + "\",\"playOnCompletion\":true," +
                        "\"playModes\":{\"repeat\":true,\"repeatOne\":false,\"shuffle\":false,\"crossfade\":false}," +
                        "\"action\":\"REPLACE\"}";
            break;
        case 100: // Lautsärke lesen
            strUrl = "https://api.ws.sonos.com/control/api/v1/groups/" + strGroupID + "/groupVolume";
            bPost = false;
            break;
        default:
            strError = "SONOS Paramer, action (" + to_string(iAktion) + "incorrect!";
            syslog(LOG_ERR, strError.c_str());
            return -1;
            break;
    }
    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L); 
    // Timeout auf 10 Sekunden
    curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);   
    curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, m_curlErrorBuffer);

    // Url
    curl_easy_setopt(m_pCurl, CURLOPT_URL, strUrl.c_str());    

    // Header setzen
    headers = curl_slist_append(headers, "Content-Type:application/json;charset=utf-8");
    strAuthorization = "Authorization: Bearer " + m_strToken;
    headers = curl_slist_append(headers, strAuthorization.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers);

    // Data (body)
    if(bPost)
        curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, strData.c_str());
    else
        curl_easy_setopt(m_pCurl, CURLOPT_HTTPGET, 1L);

    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CSonos::write_data);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&strReadBuffer);

    // Perform the request, res will get the return code 
    res = curl_easy_perform(m_pCurl);            
    if(res != CURLE_OK)
    {
        strError = "SONOS Verwalt (1) / " + string( m_curlErrorBuffer);
        syslog(LOG_ERR, strError.c_str());  
        m_iStep = 0;
    }
    else
    {   
        bSuccess = pJson->parse((char *)strReadBuffer.c_str(), strReadBuffer.length(), &iErr);
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
                    strError = "SONOS error (errorCode): " + JsonNodeValue.asString();
                    syslog(LOG_ERR, strError.c_str());
                }
                else
                {
                    switch (iAktion) {
                    case 100:
                        pJson->get((char *)"volume", &JsonNodeValue);
                        iRet = JsonNodeValue.asInt();
                        break;
                    default:
                        iRet = 0;
                        break;
                    }
                }
            }
        }
        else
        {   
            strError = "SONOS JSON error at pos " + to_string(iErr) + "(curl),  groupID (" + strGroupID + "), action " +
                            to_string(iAktion) + ": " + strReadBuffer;
            syslog(LOG_ERR, strError.c_str());
        }
    }
    curl_easy_reset(m_pCurl);
    return iRet;
}

void CSonos::Control(time_t iUhrzeit)
{
    int i, iVol, iError = 0;
    struct SonosAktionEntity Entity;
    string strGroup, strID;
   
    if(iUhrzeit % 86400 == 7200) // um 4 Uhr Nachts iUhrzeit entspricht der Greenwich-Uhrzeit
    {
        m_iStep = 0;
        m_iDelay = 0;
    }
    if(m_iDelay)
    {
        if(iUhrzeit > m_iDelay + 300) // wenn ein Fehler aufgetreten ist, Initialisierung nach 5 Minuten neustarten
        {
            m_iStep = 0;
            m_iDelay = 0;
        }
    }
    else
    {
        switch(m_iStep) {
            case 0: // Refreshtoken aufrufen
                iError = RefreshToken();
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
                pthread_mutex_lock(&m_mutexSonosFifo);  
                if(!m_SonosFifo.empty())
                {
                    Entity = m_SonosFifo.front();
                    m_SonosFifo.pop();
                    pthread_mutex_unlock(&m_mutexSonosFifo);                  
                    strGroup = m_strGroupConfig[Entity.m_iNrGroup - 1];
                    strID = "";
                    for(i=0; i < m_iAnzGroup; i++)
                    {
                        if(strGroup.compare(m_pGroupEntity[i].m_strName) == 0)
                            strID = m_pGroupEntity[i].m_strID;
                    }
                    if(strID.empty())
                    {
                        // Gruppe nicht gefunden, also alles neu einlesen!
                        syslog(LOG_ERR, "Sonos Execution: group not found!");
                        iError = 1;
                        break;
                    }    
                    i = Entity.m_iState % 256;
                    if(i == 4 || i == 5)
                    {
                        iVol =  Execute(strID, 100);
                        if(iVol < 0)
                        {
                            syslog(LOG_ERR, "Sonos Execution: volume not defined!");
                            iError = 1;
                            break;
                        }
                        if(i == 4)
                            iVol += Entity.m_iState / 256;
                        else
                            iVol -= Entity.m_iState / 256;
                        if(iVol > 100)
                            iVol = 100;
                        if(iVol < 0)
                            iVol = 0;
                        Entity.m_iState = iVol * 256 + 3;
                    }
                    if(Execute(strID, Entity.m_iState) == -1)
                    {
                        syslog(LOG_ERR, "Sonos Execution error");
                        iError = 1;
                    }
                }
                else
                    pthread_mutex_unlock(&m_mutexSonosFifo);              
                break;
            default:
                break;
        }
        if(iError)
            m_iDelay = iUhrzeit;
    }
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

