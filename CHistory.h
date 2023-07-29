/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CHistory.h
 * Author: josefengel
 *
 * Created on September 25, 2019, 10:10 AM
 */

#ifndef CHISTORY_H
#define CHISTORY_H

class CHistoryElement
{
public:
    CHistoryElement();
    ~CHistoryElement();
    time_t m_time;
    short m_sTyp;
    union{
        char m_chAusg[MAXAUSGCHANNEL/PORTANZAHLCHANNEL];
        int m_iSwitch;
        CHueProperty m_HueProperty;
    };
};

class CHistory {
public:
    CHistory(int AnzAusg, int iAnzEWAusg, int iAnzHue);
    ~CHistory();
    void Add(char * ptr);
    void Add(int iSwitch);
    void Add(CHueProperty *pHueProperty);
    void SetInvAusgHistoryEnabled(char *pPtr);
    void ControlFiles(char * pcAusgHistory, queue<struct EWAusgEntity> *pEWAusgFifo, CHue *pHue);
    CHistoryElement Control();
    bool InitAddAusg(int iNr);
    bool InitAddEWAusg(int iNr);
    bool InitAddHue(int iNr);
    void SetFilePosAfterDiff(long lPos);
    bool SetDiffTage(int iDiff);
    int GetDiffTage();
    string GetError();
    bool IsFAEnabled(int iChannel);
    bool IsHueEnabled(int iNr);
    
private:
    string m_strError;
    CHistoryElement m_HistoryElement;
    time_t m_iTag;
    int m_iDiffTage;
    int m_iDiffTageExt;     // wird von extern gesetzt
    int m_iAnzAusg;
    int m_iAnzEWAusg;
    int m_iAnzHue;
    int m_iExportPosWrite;
    int m_iExportPosRead;
    long m_lFilePosAfterDiff;
    long m_lReadFilePos;
    CHistoryElement *m_pExportHistoryElement;
    char *m_pAusgEnabled;
    bool *m_pEWAusgEnabled;
    bool *m_pHueEnabled;
    queue<CHistoryElement> m_HistoryWriteFifo;
    queue<CHistoryElement> m_HistoryReadFifo;
    pthread_mutex_t m_mutexHistoryWriteFifo;
    pthread_mutex_t m_mutexHistoryReadFifo;	    
};

#endif /* CHISTORY_H */

