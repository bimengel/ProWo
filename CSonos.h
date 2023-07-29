/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CSonos.h
 * Author: josefengel
 *
 * Created on January 18, 2021, 4:54 PM
 */

#ifndef CSONOS_H
#define CSONOS_H

class CIOGroup;

struct SonosAktionEntity
{
    int m_iNrGroup;
    int m_iState;
};

class CSonosEntity {
public:
    CSonosEntity();
    virtual ~CSonosEntity();    
    string m_strName;
    string m_strID;
};

class CSonos {
public:
    CSonos(CIOGroup *pIOGroup);
    virtual ~CSonos();
    int GetAnzGroupConfig();
    void SetState(int iNrGroup, int iState);
    void Control(time_t iUhrzeit);
private:
    int RefreshToken();
    int ReadHouseholds();
    static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
    int ReadGroups();
    int ReadFavorites();
    int ReadPlaylists();
    void LesDatei();    
    int Execute(string strGroupID, int iState);
    
private:
    int m_iAnzGroupConfig;
    vector<string> m_strGroupConfig;
    int m_iAnzGroup;
    int m_iAnzPlayer;
    int m_iAnzFavorites;
    int m_iStep; // 1: RefreshToken erhalten, 2: Households erhalten, 3: Gruppen gelesen
    int m_iDelay; 
    CURL * m_pCurl;
    char m_curlErrorBuffer[CURL_ERROR_SIZE];    
    string m_strClientId;
    string m_strClientSecret;
    string m_strRefreshToken;
    int m_iHousehold;
    string m_strToken;
    string m_strHousehold;
    CSonosEntity *m_pGroupEntity;
    CSonosEntity *m_pPlayerEntity;
    CSonosEntity *m_pFavoritesEntity;
    string m_strGroup;
    CIOGroup *m_pIOGroup;
    pthread_mutex_t m_mutexSonosFifo;    
    queue<struct SonosAktionEntity> m_SonosFifo;
    int m_iSonosACID;
};

class CBerechneSonos : public CBerechneBase 
{
public:
    void init(int iNr, CSonos *pSonos);
    virtual void SetState(int state);
private:
    int m_iNrGroup;
    CSonos* m_pSonos;
};

#endif /* CSONOS_H */

