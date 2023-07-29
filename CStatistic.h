/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CStatistic.h
 * Author: josefengel
 *
 * Created on April 26, 2020, 12:23 PM
 */

#ifndef CSTATISTIC_H
#define CSTATISTIC_H

#define VALTYPTOT   0
#define VALTYPMIN   1
#define VALTYPMAX   2

struct _monthDaten
{
    int val[31];
    int average;
    int total;
    int max;
    int min;
};

struct _yearDaten
{
    struct _monthDaten mon[12];
    int year;
};

class CStatistic {
public:
    CStatistic();
    virtual ~CStatistic();
    int GetAktYear();
    int GetFirstYear();
    virtual string WriteSpecificLine() { string str; str.clear(); return str; };
    virtual void ReadSpecificLine(CReadFile *pReadFile) {};
    string GetName();
    string GetEinheit();
    void SetName(string str);
    void SaveValue(int iVal);  
    int GetAverageMonDay(int mon);
    int GetAverageMonTotal(int mon);    
    double GetWertTag(int iJahr, int iMonat, int iTag, int iTagWert);
    double GetWertMonat(int iJahr, int iMonat, int iTagWert);
    double GetWertJahr(int iJahr, int iTagWert); 
    int GetIntWert(int iJahr, int iMonat, int iTag, int iTagWert);
    int GetIntWert(int iJahr, int iMonat, int iTagWert);
    int GetIntWert(int iJahr, int iTagWert);
    double GetDblWert(int iJahr, int iMonat, int iTag, int iTagWert);
    double GetDblWert(int iJahr, int iMonat, int iTagWert);
    double GetDblWert(int iJahr, int iTagWert);
    string GetStringWert(int iJahr, int iMonat, int iTag, int iTagWert);
    string GetStringWert(int iJahr, int iMonat, int iTagWert);
    string GetStringWert(int iJahr, int iTagWert);
    int GetResultInt(int iInterval, int iDiff, int iWert);
    void SetFactor(int iFactor);
    int GetFactor();
    
protected:
    void SchreibDatei();
    void LesDatei(int nr);
    bool copyFile(const char *SRC, const char* DEST);    
    struct _yearDaten *AppendYear(struct _yearDaten *pAlt, int Year);
    struct _yearDaten *m_pDaten;// Daten der letzten Jahre 
    int m_iFileTyp;             // Filetyp wie in CReadFile definiert
    int m_iError;
    int m_iFirstYear;			// erst eingelesen Jahr
    int m_iAktYear;             // aktuelles Jahr in m_pDaten
    int m_iNummer;              //   
    int m_iValTyp;              // 0= total, 1= min, 2= max
    int m_iFactor;              // Faktor zwischen Zählerständen und Werten    
    string m_strName;           // Name des Zählers
    string m_strEinheit;        // Einheit des Zählers
};

#endif /* CSTATISTIC_H */

