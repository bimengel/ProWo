/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CMusic.cpp
 * Author: josefengel
 * 
 * Created on January 15, 2020, 10:32 PM
 */

#include "ProWo.h"

string CMusic::GetFileName(int nr)
{
    if(nr < 1 || nr > m_iAnzahl)
        nr = 1;
    return m_pFileName[nr-1];
}

int CMusic::GetVolume(int nr)
{
    if(nr < 1 || nr > m_iAnzahl)
        nr = 1;
    return m_piVolume[nr-1];
}

void CMusic::LesDatei(char *pProgrammPath)
{
   	string strDatei;
    int iVolume;
    int iAnzahl;
	CReadFile *pReadFile;
    struct mpd_pair* pPair;
        
	pReadFile = new CReadFile();

	pReadFile->OpenRead (pProgramPath, 8);
	for(iAnzahl = 0;;)
	{	
		if(pReadFile->ReadLine())
            iAnzahl++;
        else
            break;
    }
    pReadFile->Close();
    
    m_pFileName = new string[iAnzahl];
    m_piVolume = new int[iAnzahl];
    m_iAnzahl = iAnzahl;
    
    pReadFile->OpenRead (pProgramPath, 8);
	for(iAnzahl = 0;;iAnzahl++)
	{	
		if(pReadFile->ReadLine())
		{
            strDatei = pReadFile->ReadText(';');
            if(strDatei.empty())
                pReadFile->Error(102);
            pReadFile->ReadSeparator();
            iVolume = pReadFile->ReadNumber();
            if(iVolume < 1 || iVolume > 100)
                pReadFile->Error(103);
            m_pFileName[iAnzahl] = strDatei;
            m_piVolume[iAnzahl] = iVolume;
            if(ConnectMpd())
            {  
                if(mpd_send_list_files(m_pConnmpd, NULL))
                {
                    while(true)
                    {
                        pPair = mpd_recv_pair(m_pConnmpd);
                        if(pPair == NULL)
                            pReadFile->Error(104);
                        mpd_return_pair(m_pConnmpd, pPair);  
                        if(strncmp("file", pPair->name, 4) == 0)
                        {
                            if(strncmp(pPair->value, strDatei.c_str(), strDatei.length()) == 0)
                                break;
                        }
                     
                    }
                }
                else
                    pReadFile->Error(104);
                mpd_connection_free(m_pConnmpd);
                m_pConnmpd = NULL;        
            }
        }
        else
            break;
    }
    pReadFile->Close();

}

bool CMusic::ConnectMpd()
{
    bool bRet = true;
    m_pConnmpd = mpd_connection_new("localhost", 6600, 30000);
    if (mpd_connection_get_error(m_pConnmpd) != MPD_ERROR_SUCCESS)
	{
        m_pConnmpd = NULL;
        syslog(LOG_ERR, "Cannot connect to Music Player Daemon!");
        bRet =false;
    }    
    return bRet;
}
CMusic::CMusic() {

    m_iStartPlay = 0;
    if(!ConnectMpd())
        return;
    if(!mpd_run_update(m_pConnmpd, NULL))
        syslog(LOG_ERR, "Cannot update Music Player Daemon!");
    if(!mpd_run_consume(m_pConnmpd, true))
        syslog(LOG_ERR, "Cannot set consume to true!");
    mpd_connection_free(m_pConnmpd);
    m_pConnmpd = NULL;
    m_iAnzahl = 0; 
    pthread_mutex_init(&m_mutexMusicFifo, NULL);       
}

CMusic::CMusic(const CMusic& orig) {
}

CMusic::~CMusic() 
{
    pthread_mutex_destroy(&m_mutexMusicFifo);       
}

int CMusic::GetAnzahl()
{
    return m_iAnzahl;
}
void CMusic::InsertFifo(CMusicProperty * musicProperty)
{
    pthread_mutex_lock(&m_mutexMusicFifo);       
    m_MusicFifo.push(*musicProperty);
    pthread_mutex_unlock(&m_mutexMusicFifo);       
}

void CMusic::Control()
{
    CMusicProperty MusicProperty;

    pthread_mutex_lock(&m_mutexMusicFifo); 
 
    if(!m_MusicFifo.empty())
    {
        MusicProperty = m_MusicFifo.front();    
        pthread_mutex_unlock(&m_mutexMusicFifo); 
        if(!ConnectMpd())
            return;
        switch(m_iStartPlay) {
        case 0:
            if(MusicProperty.m_iVolume)
            {
                if(!mpd_send_add_id(m_pConnmpd, MusicProperty.m_strName.c_str()) && !mpd_response_finish(m_pConnmpd))
                    syslog(LOG_ERR, "Can not add a song!");
            }
            else
            {
                if(!mpd_run_stop(m_pConnmpd))
                    syslog(LOG_ERR, "Cannot stop Media Player!");
            }
            m_iStartPlay = 1;
            break;
        case 1:
            if(MusicProperty.m_iVolume)
            {
                if(!mpd_run_repeat(m_pConnmpd, MusicProperty.m_bRepeat))
                    syslog(LOG_ERR, "Cannot set Media Player volume!");                 
                if(!mpd_run_consume(m_pConnmpd, !MusicProperty.m_bRepeat))
                    syslog(LOG_ERR, "Cannot set Media Player single-Mode");
                if(!mpd_run_set_volume(m_pConnmpd, MusicProperty.m_iVolume))
                    syslog(LOG_ERR, "Cannot not Media Player volume!");
                m_iStartPlay = 2;
            }
            else
            {
                if(!mpd_run_clear(m_pConnmpd))
                    syslog(LOG_ERR, "Cannot clear the playlist");   
                m_iStartPlay = 3;
            }
            break;
        case 2:
            if(!mpd_send_play(m_pConnmpd) && !mpd_response_finish(m_pConnmpd))
                syslog(LOG_ERR, "Cannot start play from the Media Player!");
            m_iStartPlay = 3;
            break;
        default:
            break;
        }
        if(m_iStartPlay == 3)
        {
            pthread_mutex_lock(&m_mutexMusicFifo);  
            m_MusicFifo.pop(); 
            pthread_mutex_unlock(&m_mutexMusicFifo);  

            m_iStartPlay = 0;
        }
        mpd_connection_free(m_pConnmpd);          
        m_pConnmpd = NULL;
    } 
    else
        pthread_mutex_unlock(&m_mutexMusicFifo);         
}


//
// Music
void CBerechneMusic::init(CMusic* pMusic)
{
    m_pMusic = pMusic;
}

void CBerechneMusic::SetState(int state)
{
    CMusicProperty MusicProperty;
    int iAnz;
    
    if(state == 0)
        MusicProperty.m_iVolume = 0;
    else
    {
        if(state > 1000)
        {
            MusicProperty.m_bRepeat = true;
            state -= 1000;
        }
        else 
            MusicProperty.m_bRepeat = false;
        iAnz = m_pMusic->GetAnzahl();
        if(state < 1 || state > iAnz)
            state = 1;
        MusicProperty.m_iVolume = m_pMusic->GetVolume(state);
        MusicProperty.m_strName = m_pMusic->GetFileName(state);
    }
    
    m_pMusic->InsertFifo(&MusicProperty);

}


