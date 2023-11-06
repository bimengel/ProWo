/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CHue.c
 * Author: josefengel
 * 
 * Created on December 6, 2019, 11:13 AM
 */

#include "ProWo.h"

/*
int CIOGroup::LesHUE()
{
    CURLcode res;
    char text[100];
    bool bRet = true;
    int len, i;
    string str;
    
    struct timespec start, finish; 
    long lDelay;
    clock_gettime(CLOCK_REALTIME, &start);     
    str = "https://" + m_strHueIP + "/api/" + m_strHueUser;
    len = snprintf(text, 100, "%s", str.c_str());
    for(i=0; i < m_iAnzHue; i++)
    {
        if(m_pCurl) {
            if(m_pHue[i]->GetTyp() == 1)
                snprintf(&text[len], 100-len, "/lights/%d", m_pHue[i]->GetID());
            else
                snprintf(&text[len], 100-len, "/groups/%d", m_pHue[i]->GetID());
            curl_easy_setopt(m_pCurl, CURLOPT_URL, text);
            curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)m_pHue[i]);
            // Perform the request, res will get the return code 
            res = curl_easy_perform(m_pCurl);
            // Check for errors  
            if(res != CURLE_OK)
            {
                sprintf(text, "curl_easy_perform() failed: %s\n",  curl_easy_strerror(res));
                syslog(LOG_ERR, text);
                bRet = false;
            }
        } 
    }

    clock_gettime(CLOCK_REALTIME, &finish);
    lDelay = (finish.tv_nsec - start.tv_nsec)/1000; // microsekunden 

    return 1;
}


size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    bool success;
    float bri;
    Json::Reader reader;
    Json::Value value;
    CHueEntity * pHue;
   
    pHue = (CHueEntity *)userp;
    success = reader.parse((char *)buffer, value);
    if(success)
    {
        if(pHue->GetTyp() == 1)   // lights
            success = value["state"]["on"].asBool();
        else                    // groups
            success = value["state"]["all_on"].asBool();
        
        pthread_mutex_lock(&ext_mutexNodejs);
        pHue->ActualizeState(success);
        pthread_mutex_unlock(&ext_mutexNodejs);        
    }

    return size * nmemb;
}
     //
    // HUE-Protokolle versenden
    //
    char text[100];
    int state;
    long lSize;
    str = "https://" + m_strHueIP + "/api/" + m_strHueUser;
    len = snprintf(buffer, 100, "%s", str.c_str());
   
    for(i=0; i < m_iAnzHue; i++)
    {
        if(m_pHue[i]->IsChanged())
        {
            state = m_pHue[i]->GetState();
            m_pHue[i]->ActualizeState(state);
            if(m_pHue[i]->GetTyp() == 1)
            {
                snprintf(&buffer[len], 100-len, "/lights/%d/state", m_pHue[i]->GetID());
                if(state)
                    lSize = snprintf(text, 100, "{\"on\":true}");
                else
                    lSize = snprintf(text, 100, "{\"on\":false}");  
            }
            else
            {
                snprintf(&buffer[len], 100-len, "/groups/%d/action", m_pHue[i]->GetID());
                if(state)
                    lSize = snprintf(text, 100, "{\"on\":true}");
                else
                    lSize = snprintf(text, 100, "{\"on\":false}");  
            }
            curl_easy_setopt(m_pCurl, CURLOPT_URL, buffer);
            curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(m_pCurl, CURLOPT_READFUNCTION, read_data);
            curl_easy_setopt(m_pCurl, CURLOPT_READDATA, (void *)text);
            curl_easy_setopt(m_pCurl, CURLOPT_INFILESIZE, lSize);
            // Perform the request, res will get the return code 
            res = curl_easy_perform(m_pCurl);
            break;
        }
    }
*/

size_t read_data(void *ptr, size_t size, size_t nmemb, void *userdata)
{

    size_t retcode;
    
    /* copy as much data as possible into the 'ptr' buffer, but no more than
       'size' * 'nmemb' bytes! */
    retcode = snprintf((char *)ptr, size * nmemb, (char *)userdata );

    return retcode;
}

void CHue::Control()
{
    CHueProperty HueProperty;
    CURLcode res;
    bool bSend = true;
    string strBuffer;
    char text[MSGSIZE];
    long lSize;

    pthread_mutex_lock(&m_mutexHueFifo);  
    if(!m_HueFifo.empty())
    {
        int i = 0;
        
        HueProperty = m_HueFifo.front();
        m_HueFifo.pop();
        pthread_mutex_unlock(&m_mutexHueFifo);  
        if(((CIOGroup *)m_pIOGroup)->m_pHistory != NULL)
        {
            if(HueProperty.m_iSource == 1 && ((CIOGroup *)m_pIOGroup)->m_pHistory->GetDiffTage() &&
                    ((CIOGroup *)m_pIOGroup)->m_pHistory->IsHueEnabled(HueProperty.m_iNr))
                bSend = false;
            else
                ((CIOGroup *)m_pIOGroup)->m_pHistory->Add(&HueProperty);
        }
        if(bSend)
        {    
            strBuffer = m_strHueConnect;
            if(HueProperty.m_iTyp == 1)
                strBuffer += "/lights/"  + to_string(HueProperty.m_iID) + "/state";
            else
                strBuffer += "/groups/" + to_string(HueProperty.m_iID) + "/action";
            if(HueProperty.m_iState)
            {
                lSize = snprintf(text, MSGSIZE, "{\"on\":true");
                if(HueProperty.m_iBrightness != 0)
                    lSize += snprintf(text+lSize, MSGSIZE-lSize, ",\"bri\":%d}", HueProperty.m_iBrightness);
                else
                    lSize += snprintf(text+lSize, MSGSIZE-lSize, "}");
            }
            else 
                lSize = snprintf(text, MSGSIZE, "{\"on\":false}");
            curl_easy_setopt(m_pCurl, CURLOPT_URL, strBuffer.c_str());
            curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(m_pCurl, CURLOPT_READFUNCTION, read_data);
            curl_easy_setopt(m_pCurl, CURLOPT_READDATA, (void *)text);
            curl_easy_setopt(m_pCurl, CURLOPT_INFILESIZE, lSize);
            // Perform the request, res will get the return code 
            res = curl_easy_perform(m_pCurl);            
            if(res != CURLE_OK)
                syslog(LOG_ERR, m_curlErrorBuffer);
        }
    }
    else
        pthread_mutex_unlock(&m_mutexHueFifo);  
}

CHue::CHue(char * pIOGroup) 
{
    m_strUser.clear();
    m_strIP.clear();
    m_strHueConnect.clear();
    m_iAnzahl = 0;
    m_pIOGroup = pIOGroup;
    m_pHueEntity = new CHueEntity *[((CIOGroup *)pIOGroup)->m_iMaxAnzHueEntity];
    pthread_mutex_init(&m_mutexHueFifo, NULL);      

    m_pCurl = curl_easy_init();
    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L); 
    // Timeout auf 10 Sekunden
    curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);   
    curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, m_curlErrorBuffer);
}

int CHue::SetEntity(int typ, int ID)
{
    int error = 0;
    CHueEntity * pHueEntity;
    if(m_iAnzahl < ((CIOGroup *)m_pIOGroup)->m_iMaxAnzHueEntity) 
    {
        pHueEntity = new CHueEntity;
        pHueEntity->init(m_iAnzahl+1, typ, ID, (char*)(this));
        m_pHueEntity[m_iAnzahl] = pHueEntity;
        m_iAnzahl++;
    }
    else
        error = 89;
    return error;
}

CHue::~CHue()
{
    int i;
    pthread_mutex_destroy(&m_mutexHueFifo);   
    if(m_pHueEntity)
    {
        for(i=0; i < ((CIOGroup *)m_pIOGroup)->m_iMaxAnzHueEntity; i++)
            delete m_pHueEntity[i];
        delete [] m_pHueEntity;
        m_pHueEntity = NULL;
    }
    curl_easy_cleanup(m_pCurl);    
}

void CHue::SetUser(string str)
{
    m_strUser = str;
    m_strHueConnect = "https://" + m_strIP + "/api/" + m_strUser;
}
void CHue::SetIP(string str)
{
    m_strIP = str;
    m_strHueConnect = "https://" + m_strIP + "/api/" + m_strUser;
}

int CHue::IsDefined()
{
    int error = 0;
    
    if(m_strUser.empty())
        error = 91;
    if(m_strIP.empty())
        error = 90;
    return error;
}

int CHue::GetAnz()
{
    return m_iAnzahl;
}

void CHue::InsertFifo(CHueProperty * pHueProperty)
{
    pthread_mutex_lock(&m_mutexHueFifo);  
    m_HueFifo.push(*pHueProperty);
    pthread_mutex_unlock(&m_mutexHueFifo); 
}

//
//  CHueProperty
//
CHueProperty::CHueProperty()
{
    m_iNr = 0;
    m_iTyp = 0;
    m_iID = 0;
    m_iState = 0;
    m_iBrightness = 0;
    m_iSource = 0; 
}

//
//  Hue Entity
//
CHueEntity * CHue::GetAddress(int nr)
{
    return m_pHueEntity[nr-1];
}

CHueEntity::CHueEntity()
{
    m_iTyp = 0;
    m_iID = 0;
    m_iState = 0;
    m_pHue = NULL;
    m_iNr = 0;
}
CHueEntity::~CHueEntity()
{

}
void CHueEntity::init(int iNr, int typ, int id, char *pHue)
{
    m_iNr = iNr;
    m_iTyp = typ;
    m_iID = id;
    m_pHue = pHue;
}

void CHueEntity::SetState(int state)
{
    if(!(state % 256))
        state = 0;
    m_iState = state;
    CHueProperty HueProperty;
    HueProperty.m_iNr = m_iNr;
    HueProperty.m_iTyp = m_iTyp;
    HueProperty.m_iID = m_iID;
    HueProperty.m_iState = m_iState % 256;
    HueProperty.m_iBrightness  = m_iState / 256;
    HueProperty.m_iSource = 1;
    ((CHue *)m_pHue)->InsertFifo(&HueProperty);
}

int CHueEntity::GetState()
{
    return m_iState; 
}

int CHueEntity::GetTyp()
{
    return m_iTyp;
}

int CHueEntity::GetID()
{
    return m_iID;
}

COperHue::COperHue()
{
    m_pHueEntity = NULL;
}

int COperHue::resultInt()
{
    return m_pHueEntity->GetState();
}
void COperHue::setOper(CHueEntity* ptr)
{
    m_pHueEntity = ptr;
}

void CBerechneHue::SetState(int state)
{
    m_pHueEntity->SetState(state);
}

void CBerechneHue::init(CHueEntity *pHue)
{
    m_pHueEntity = pHue;
}
int CBerechneHue::GetState()
{
    return m_pHueEntity->GetState();
}

