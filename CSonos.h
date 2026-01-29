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
    int GetAnzFavoriteConfig();
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
    int ExecuteOperation();  
    int Execute(int iAktion, int iState);
    CURLcode CurlPerform(string strData);   
    
private:
    vector<string> m_strGroupConfig;
    vector<string> m_strFavoriteConfig;
    vector<string> m_strPlaylistConfig;
    int m_iAnzGroup;
    int m_iAnzPlayer;
    int m_iAnzFavorite;
    int m_iAnzPlaylist;
    int m_iStep; 
    int m_iDelay; 
    CURL * m_pCurl;
    char m_curlErrorBuffer[CURL_ERROR_SIZE];    
    string m_strClientId;
    string m_strClientSecret;
    string m_strRefreshToken;
    int m_iHousehold;
    string m_strReadBuffer;
    string m_strUrl;
    string m_strAuthorization;
    string m_strHousehold;
    string m_strID;
    CSonosEntity *m_pGroupEntity;
    CSonosEntity *m_pPlayerEntity;
    CSonosEntity *m_pFavoritesEntity;
    CSonosEntity *m_pPlaylistEntity;
    CIOGroup *m_pIOGroup;
    pthread_mutex_t m_mutexSonosFifo;    
    struct SonosAktionEntity m_SonosAktionEntity;    
    queue<struct SonosAktionEntity> m_SonosFifo;
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

