/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CHueEntity.h
 * Author: josefengel
 *
 * Created on December 6, 2019, 11:13 AM
 */

#ifndef CHUE_H
#define CHUE_H

class CHueProperty {
public:
    CHueProperty();
    int m_iNr;
    int m_iTyp;         // Lampe oder Gruppe
    int m_iID;
    int m_iState;
    int m_iBrightness;
    int m_iSource;      // 1 = Parameter, 2 = Lichtprogramm
};

class CHueEntity {

public:
    CHueEntity();
    ~CHueEntity();
    void init(int iNr, int typ, int id, int iMax, char *pHueFifo);
    int GetState();
    void SetState(int state);
    int GetTyp();
    int GetID();
    int GetMax();
    
private:
    int m_iNr;      // wird gebraucht für die History
    int m_iTyp;     // 1 = LIGHT, 2 = GROUP
    int m_iID;
    int m_iState; // on or off
    int m_iBrightness;
    int m_iMax;
    char *m_pHue; 
};

class CHue {
public:
    CHue(char * pIOGroup);
    ~CHue();
    void SetUser(string str);
    void SetIP(string str);
    int SetEntity(int typ, int address, int iMax);
    int IsDefined();
    int GetAnzEntity();
    CHueEntity * GetAddress(int nr);
    void Control();
    void InsertFifo(CHueProperty * phueProperty);
    
private:
    string m_strUser;
    string m_strIP;
    string m_strHueConnect;
    int m_iAnzahlEntity;
    int m_iAnzHue;
    queue<CHueProperty> m_HueFifo;
    pthread_mutex_t m_mutexHueFifo;	
    CURL *m_pCurl;
    CHueEntity** m_pHueEntity;
    char m_curlErrorBuffer[CURL_ERROR_SIZE];
    char *m_pIOGroup;
};

class COperHue : public COperBase
{
public:
    COperHue();
    virtual int resultInt();
    void setOper(CHueEntity *ptr);
protected:
    CHueEntity * m_pHueEntity;
};

class CBerechneHue : public CBerechneBase
{
public:
    void init(CHueEntity *pHueEntity);
    virtual void SetState(int state);
    virtual int GetState(); 
    virtual int GetMax();   
protected:
    CHueEntity *m_pHueEntity;
};

#endif /* CHUE_H */
