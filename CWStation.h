/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CWStation.h
 * Author: josefengel
 *
 * Created on February 22, 2019, 7:00 PM
 */

#ifndef CWSTATION_H
#define CWSTATION_H

#define IDXTEMP            19
#define IDXTAUPUNKT        23
#define IDXFEUCHTE         29
#define IDXLUFTDRUCK       39
#define IDXWINDGESCHW      49
#define IDXWINDRICHT       54
#define IDXNSMENGE         59
#define IDXNSMENGEDIFF     60
#define IDXNSART           61
#define IDXNSINTENS        62
#define IDXAZIMUT          70
#define IDXELEVATION       71
#define IDXUVINDEX         74
#define IDXHELLIGKEIT      75
#define IDXDAEMMERUNG      76

#define TEMPAKT             1
#define TEMPMIN             2
#define TEMPMAX             3
#define TAUPUNKT            4
#define FEUCHTE             5
#define LUFTDRUCK           6
#define WINDGESCHW          7
#define WINDGESCHWMAX       8
#define WINDGESCHWMAX10     9
#define WINDGESCHWMAX30     10
#define WINDGESCHWMAX60     11
#define WINDRICHT           12
#define NSMENGE             13
#define NSMENGEABS          14
#define NSART               15
#define NSINTENS            16
#define AZIMUT              17
#define ELEVATION           18
#define UVINDEX             19
#define HELLIGKEIT          20

#define ANZMESSWERTESTATUS  77
#define ANZMESSWERTESERVICE 10
#define ANZWSVALUESTATISTIC 4

class CWStation
{
public:
    CWStation(int nr);
    ~CWStation();  
    void SetModBusClient(CModBusClient *pClient);
    void LesenStarten();
    void SetFunction(int iFunc);
    int GetParam(int iParam);
    int GetParam(int iParam, int iInterval, int iDiff);
    void TagWechsel();
    CWSValue * GetPtrWSValue(int iIdx);
    
protected:
    bool m_bFirst;
    int m_iNr;
    CModBusClient *m_pModBusClient; 
    signed short m_ssMesswerteStatus[ANZMESSWERTESTATUS];
    signed short m_ssMesswerteService[ANZMESSWERTESERVICE];
    unsigned short m_usWG60[60]; 
    unsigned short m_usWGMAX10;
    unsigned short m_usWGMAX30;
    unsigned short m_usWGMAX60;
    int m_iPosWG60;
    CWSValue *m_pWSValueStatistic[ANZWSVALUESTATISTIC];
};

class COperWStation : public COperBase
{
public:
    COperWStation();
    virtual int resultInt();
    void setOper(CWStation *ptr, int fct);
protected:
    CWStation * m_pWStation;
    int m_iFct;
};

class COperWStatistic : public COperBase
{
public:
    COperWStatistic();
    virtual int resultInt();
    virtual string resultString();
    void setOper(CWStation *pWStation, int iParam, int iInterval, int iDiff, int iDiv);
protected:
    CWStation *m_pWStation;
    int m_iParam;
    int m_iInterval;
    int m_iDiff;
    int m_iDiv;
};
#endif /* CWSTATION_H */

