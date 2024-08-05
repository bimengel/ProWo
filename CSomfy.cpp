#include "ProWo.h"

size_t read_Somfydata(void *ptr, size_t size, size_t nmemb, void *userdata)
{

    size_t retcode;
    
    /* copy as much data as possible into the 'ptr' buffer, but no more than
       'size' * 'nmemb' bytes! */
    retcode = snprintf((char *)ptr, size * nmemb, (char *)userdata );

    return retcode;
}

CSomfy::CSomfy(char * pIOGroup)
{
    m_strPin = "";
    m_strToken = "";
    m_iPort = 0;
    m_strSomfyConnect = "";
    m_curlErrorBuffer[0] = 0;    
    m_pCurl = NULL;
    m_iAnzahlEntity = 0;
    m_pIOGroup = pIOGroup; 
    m_pSomfyEntity = new CSomfyEntity *[((CIOGroup *)pIOGroup)->m_iMaxAnzSomfyEntity];
    pthread_mutex_init(&m_mutexSomfyFifo, NULL);
    m_pCurl = curl_easy_init();      
}

CSomfy::~CSomfy()
{ 
    int i;

    if(m_pSomfyEntity)
    {
        for(i=0; i < ((CIOGroup *)m_pIOGroup)->m_iMaxAnzSomfyEntity; i++)
            delete m_pSomfyEntity[i];
        delete [] m_pSomfyEntity;
        m_pSomfyEntity = NULL;
    }   
    pthread_mutex_destroy(&m_mutexSomfyFifo);
    curl_easy_cleanup(m_pCurl);     
}

void CSomfy::SetPin(string strPin)
{
    m_strPin = strPin;
}

void CSomfy::SetPort(int iPort)
{
    m_iPort = iPort;
}

void CSomfy::SetToken(string strToken)
{
    m_strToken = "Authorization: Bearer " + strToken;
    if(m_iPort && !m_strPin.empty())
        m_strSomfyConnect = "https://gateway-" + m_strPin + ".local:" + to_string(m_iPort) 
                            + "/enduser-mobile-web/1/enduserAPI";
}

void CSomfy::SetState(int iNr, int iVal)
{
    CSomfyProperty SomfyProperty;
    GetAddress(iNr)->SetState(iVal);
    SomfyProperty.m_iNr = iNr;
    SomfyProperty.m_iState = iVal % 256;
    SomfyProperty.m_iBrightness = iVal / 256;
    SomfyProperty.m_iSource = 1;
    SomfyProperty.m_iTyp = GetAddress(iNr)->GetTyp();
    pthread_mutex_lock(&m_mutexSomfyFifo);  
    m_SomfyFifo.push(SomfyProperty);
    pthread_mutex_unlock(&m_mutexSomfyFifo); 
}

int CSomfy::GetAnzEntity()
{
    return m_iAnzahlEntity;
}

int CSomfy::SetEntity(int typ, string strUrl)
{
    int error = 0;
    CSomfyEntity * pSomfyEntity;

    if(m_iAnzahlEntity < ((CIOGroup *)m_pIOGroup)->m_iMaxAnzSomfyEntity) 
    {
        pSomfyEntity = new CSomfyEntity;
        pSomfyEntity->init(m_iAnzahlEntity+1, typ, (char*)(this), strUrl);
        m_pSomfyEntity[m_iAnzahlEntity] = pSomfyEntity;
        m_iAnzahlEntity++;
    }
    else
        error = 149;
    return error;
}

int CSomfy::IsDefined()
{
    if(m_strSomfyConnect.empty())
        return 150;
    else
        return 0;
}

void CSomfy::InsertFifo(CSomfyProperty * pSomfyProperty)
{
    pthread_mutex_lock(&m_mutexSomfyFifo);  
    m_SomfyFifo.push(*pSomfyProperty);
    pthread_mutex_unlock(&m_mutexSomfyFifo);     
}
void CSomfy::Control()
{
    CSomfyProperty SomfyProperty;
    string strBuffer;
    string strReadBuffer;
    string str; 
    string strError;  
    string strUrl;
    string strJson;
    CURLcode res;
    bool bSend = true;    
    struct curl_slist* slist = NULL;
    bool bSuccess;
    CJson *pJson = new CJson;
    int iRet, iErr, iTyp, iAktion, iBrightness;
    CSomfyEntity *pSomfyEntity;

    pthread_mutex_lock(&m_mutexSomfyFifo);  
    if(!m_SomfyFifo.empty())
    {
        SomfyProperty = m_SomfyFifo.front();
        m_SomfyFifo.pop();        
        pthread_mutex_unlock(&m_mutexSomfyFifo);  
        iTyp = SomfyProperty.m_iTyp;
        iAktion = SomfyProperty.m_iState;
        iBrightness = SomfyProperty.m_iBrightness;
        pSomfyEntity = GetAddress(SomfyProperty.m_iNr);
        if(((CIOGroup *)m_pIOGroup)->m_pHistory != NULL)
        {
            if(SomfyProperty.m_iSource == 1 && ((CIOGroup *)m_pIOGroup)->m_pHistory->GetDiffTage() &&
                    ((CIOGroup *)m_pIOGroup)->m_pHistory->IsSomfyEnabled(SomfyProperty.m_iNr))
                bSend = false;
            else
                ((CIOGroup *)m_pIOGroup)->m_pHistory->Add(&SomfyProperty);            
        } 
        strBuffer = m_strSomfyConnect + "/exec/apply";
        strJson = "{\"actions\":[{\"commands\":[{\"name\":";      
        switch(iTyp)
        {
            case 1: // LED Licht
                if(iAktion)
                {  
                    strJson += "\"on\"}";
                    if(iBrightness)
                        strJson += ",{\"name\":\"setIntensity\",\"parameters\":[" + to_string(iBrightness) + "]}";
                }
                else
                    strJson += "\"off\"}";
                break;
            case 2: // Markise
                if(iAktion) 
                    strJson += "\"down\"}";
                else
                    strJson += "\"up\"}";
                break;           
            default:
                syslog(LOG_ERR, "Somfy: undefined typ");
                return;   
        }
        strJson += "],\"deviceURL\":\"" + pSomfyEntity->GetstrUrl() + "\"}]}";
        // URL
        curl_easy_setopt(m_pCurl, CURLOPT_URL, strBuffer.c_str()); 

        // Timeout auf 10 Sekunden
        curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);   
        curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, m_curlErrorBuffer); 
        // URL verification
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L); 
        // Senden
        slist = curl_slist_append(slist, "Content-Type:application/json");
        slist = curl_slist_append(slist, m_strToken.c_str());
        curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, slist);

        // POST
        curl_easy_setopt(m_pCurl, CURLOPT_POST, 1L);
        curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, strJson.c_str()); 
        // Empfang
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CSomfy::write_data);
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&strReadBuffer);    

        // Perform the request, res will get the return code 
        res = curl_easy_perform(m_pCurl);   

        // free the list
        curl_slist_free_all(slist);         
        if(res != CURLE_OK)
        {
            strError = "Somfy - send error: " + string(m_curlErrorBuffer);
            iRet = 2;
        }     
    }
    else
        pthread_mutex_unlock(&m_mutexSomfyFifo); 
    curl_easy_reset(m_pCurl);
    delete pJson;        
}

size_t CSomfy::write_data(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

CSomfyProperty::CSomfyProperty()
{
    m_iState = 0;
    m_iNr = 0;
    m_iTyp = 0;
    m_iSource = 0;
    m_iBrightness = 100;
}

CSomfyEntity::CSomfyEntity()
{
    m_iNr = 0;
    m_iTyp = 0;
    m_iID = 0;
    m_iState = 0;
    m_iBrightness = 0;
    m_pSomfy = NULL;
}

//
//  CSomfyEntity
CSomfyEntity::~CSomfyEntity()
{

}

CSomfyEntity * CSomfy::GetAddress(int nr)
{
    return m_pSomfyEntity[nr-1];
}

void CSomfyEntity::init(int iNr, int typ, char *pSomfy, string strUrl)
{
    int pos;
    m_iNr = iNr;
    m_iTyp = typ;
    m_iState = 0;
    m_pSomfy = pSomfy;
    m_iBrightness = 100;
/*  m_strUrl = "";
    for(pos = 0; pos < strUrl.length(); pos++)    
    {
        if(strUrl.at(pos) == ':')
            m_strUrl += "%3A";
        else if(strUrl.at(pos) == '/')
            m_strUrl += "%2F";
        else
            m_strUrl += strUrl.at(pos);
    }*/
   m_strUrl = strUrl;
}
int CSomfyEntity::GetState()
{
    return m_iBrightness * 256 + m_iState;
}
void CSomfyEntity::SetState(int iVal)
{
    m_iState = iVal % 256;
    m_iBrightness = iVal / 256;  
}    

int CSomfyEntity::GetTyp()
{
    return m_iTyp;
}
int CSomfyEntity::GetID()
{
    return m_iID;
}
string CSomfyEntity::GetstrUrl()
{
    return m_strUrl;
}
//
//  CBerechneSomfy
//
CBerechneSomfy::CBerechneSomfy()
{
    m_pSomfy = NULL;
}
void CBerechneSomfy::init(int nr, CSomfy *pSomfy)
{
    m_pSomfy = pSomfy;
    m_nr = nr;
}
void CBerechneSomfy::SetState(int iVal)
{
    m_pSomfy->SetState(m_nr, iVal);
}
int CBerechneSomfy::GetState()
{
    return m_pSomfy->GetAddress(m_nr)->GetState();
}