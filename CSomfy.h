#ifndef CSOMFY_H
#define CSOMFY_H

// Diese Klasse wird im Fifo-Register abgelegt
class CSomfyProperty {

public:
    CSomfyProperty();
    int m_iState;
    int m_iVal;
    int m_iNr;
    int m_iTyp;         // zB Licht oder Rollo
    int m_iSource;      // 1 = Parameter, 2 = Lichtprogramm    
private:
};

class CSomfyEntity {
public:
    CSomfyEntity();
    ~CSomfyEntity();
    void init(int iNr, int typ, int iMax, char *pSomfy, string strUrl);
    int GetState();
    void SetState(int iVal);
    int GetMax();
    int GetTyp();
    int GetID();
    string GetstrUrl();
    
private:
    int m_iNr;      // wird gebraucht für die History
    int m_iTyp;     // 
    int m_iID;
    int m_iState; // on or off
    int m_iVal;
    int m_iMax;
    string m_strUrl;
    char *m_pSomfy;
};

class CSomfy {
public:
    CSomfy(char *pIOGroup);
    ~CSomfy();
    void SetPin(string strPin);
    void SetPort(int iPort);
    void SetToken(string strToken);
    void Control();
    int SetEntity(int typ, int iMax, string m_strUrl);
    int GetAnzEntity();
    int IsDefined();
    CSomfyEntity * GetAddress(int nr); 
    void SetState(int nr, int state);
    void InsertFifo(CSomfyProperty * pSomfyProperty);
private:
    static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
    char m_curlErrorBuffer[CURL_ERROR_SIZE];     
    string m_strPin;
    int m_iPort;
    string m_strToken;
    string m_strSomfyConnect;
    queue<CSomfyProperty> m_SomfyFifo;
    pthread_mutex_t m_mutexSomfyFifo;	
    CURL *m_pCurl;
    char *m_pIOGroup;
    int m_iAnzahlEntity;
    CSomfyEntity ** m_pSomfyEntity;
};

class CBerechneSomfy : public CBerechneBase
{
public:
    CBerechneSomfy();
    void init(int nr, CSomfy *pSomfy);
    virtual void SetState(int state);
    virtual int GetState(); 
    virtual int GetMax();
       
protected:
    CSomfy *m_pSomfy;
};

class COperSomfy : public COperBase
{
public:
    COperSomfy();
    virtual int resultInt();
    void setOper(CSomfyEntity *ptr);
protected:
    CSomfyEntity * m_pSomfyEntity;
};

#endif // CSOMFY_H