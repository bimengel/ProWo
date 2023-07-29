/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CMusic.h
 * Author: josefengel
 *
 * Created on January 15, 2020, 10:32 PM
 */

#ifndef CMUSIC_H
#define CMUSIC_H

class CMusicProperty {
public:
    int m_iVolume;
    string m_strName;
    bool m_bRepeat;
};

class CMusic {
public:
    CMusic();
    CMusic(const CMusic& orig);
    virtual ~CMusic();
    void Control();
    int GetAnzahl();
    void LesDatei(char *pProgrammPath);
    string GetFileName(int nr);
    int GetVolume(int nr);
    void InsertFifo(CMusicProperty *musicProperty);
    
private:
    queue<CMusicProperty> m_MusicFifo; 
    int m_iStartPlay;
    bool ConnectMpd();
    struct mpd_connection *m_pConnmpd;
    int m_iAnzahl;
    string * m_pFileName;
    int * m_piVolume;
    pthread_mutex_t m_mutexMusicFifo;	 
};

class CBerechneMusic : public CBerechneBase
{
public:
    void init(CMusic *pMusic);
    virtual void SetState(int state);
private:
    CMusic * m_pMusic;
};

#endif /* CMUSIC_H */

