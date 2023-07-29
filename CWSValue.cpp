/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CWSValue.cpp
 * Author: josefengel
 * 
 * Created on April 27, 2020, 12:45 PM
 */

#include "ProWo.h"

CWSValue::CWSValue() 
{
    m_iFileTyp = 12;

}

CWSValue::~CWSValue() 
{

}

string CWSValue::WriteSpecificLine()
{
    string str;
    str.clear();
    return str;
}

void CWSValue::ReadSpecificLine(CReadFile *pReadFile)
{
    return;
}

double CWSValue::GetValueDbl()
{
    int iVal;
    iVal = GetValue();
    return (double)iVal / (double)m_iFactor;
}

//
// tägliche Niederschlagsmenge
//
CWSValueNSMenge::CWSValueNSMenge(int nr)
{
    m_iNSMengeAbsTagAnfang = 0;
    m_iValTyp = VALTYPTOT;
    m_iFactor = 100;
    m_iVal = 0;
    LesDatei(nr);
}

CWSValueNSMenge::~CWSValueNSMenge()
{
    SchreibDatei();
}

int CWSValueNSMenge::GetValue()
{
    int iVal;
    
    iVal = m_iVal - m_iNSMengeAbsTagAnfang;
    if(iVal < 0)
        iVal += 65536;
    return iVal;
}

void CWSValueNSMenge::SaveDayValue(int iAkt)
{
    int iVal;
    
    iVal = GetValue();
    m_iNSMengeAbsTagAnfang = m_iVal;
    SaveValue(iVal);
}

string CWSValueNSMenge::WriteSpecificLine()
{
    string strRet;
    
    strRet = "2000;" + to_string(m_iVal) + ";" + to_string(m_iNSMengeAbsTagAnfang) + ";";
   
    return strRet;
}

void CWSValueNSMenge::ReadSpecificLine(CReadFile *pReadFile)
{
    m_iVal = pReadFile->ReadZahl ();
    if(!pReadFile->ReadSeparator ())
        pReadFile->Error (62);
    m_iNSMengeAbsTagAnfang = pReadFile->ReadZahl ();
    if(!pReadFile->ReadSeparator ())
        pReadFile->Error (62);    
}

void CWSValueNSMenge::Control(short * sPtr) 
{
    m_iVal = (unsigned int)*(sPtr + IDXNSMENGE);
};

//
// maximale Tagestemperatur
//
CWSValueMaxTemp::CWSValueMaxTemp(int nr)
{
    m_iVal = INT_MIN;
    m_iValTyp = VALTYPMAX;
    m_iFactor = 10;
    LesDatei(nr);
}

CWSValueMaxTemp::~CWSValueMaxTemp()
{
    SchreibDatei();
}

string CWSValueMaxTemp::WriteSpecificLine()
{
    string strRet;
    
    strRet = "2000;" + to_string(m_iVal) + ";";
    
    return strRet;
}
void CWSValueMaxTemp::SaveDayValue(int iAkt)
{
    int iVal;
    
    iVal = m_iVal;
    m_iVal = iAkt;
    SaveValue(iVal);
}

void CWSValueMaxTemp::ReadSpecificLine(CReadFile *pReadFile)
{
    m_iVal = pReadFile->ReadZahl ();
    if(!pReadFile->ReadSeparator ())
        pReadFile->Error (62);
}

void CWSValueMaxTemp::Control(short * sPtr)
{
    if(*(sPtr + IDXTEMP) > m_iVal)
        m_iVal = *(sPtr + IDXTEMP);   
}

//
// minimale Temperatur pro Tag
//
CWSValueMinTemp::CWSValueMinTemp(int nr)
{
    m_iVal = INT_MAX;
    m_iValTyp = VALTYPMIN;
    m_iFactor = 10;
    LesDatei(nr);
}

CWSValueMinTemp::~CWSValueMinTemp()
{
    SchreibDatei();
}

void CWSValueMinTemp::SaveDayValue(int iAkt)
{
    int iVal;
    
    iVal = m_iVal;
    m_iVal = iAkt;
    SaveValue(iVal);
}

string CWSValueMinTemp::WriteSpecificLine()
{
    string strRet;
    
    strRet = "2000;" + to_string(m_iVal) + ";";
    
    return strRet;
}

void CWSValueMinTemp::ReadSpecificLine(CReadFile *pReadFile)
{
    m_iVal = pReadFile->ReadZahl ();
    if(!pReadFile->ReadSeparator ())
        pReadFile->Error (62);
}

void CWSValueMinTemp::Control(short * sPtr)
{
    if(*(sPtr + IDXTEMP) < m_iVal)
        m_iVal = *(sPtr + IDXTEMP);   
}

//
// maximale Windgeschwindigkeit pro Tag
//
CWSValueMaxWindGeschw::CWSValueMaxWindGeschw(int nr)
{
    m_iVal = INT_MIN;
    m_iValTyp = VALTYPMAX;
    m_iFactor = 10;
    LesDatei(nr);
}

CWSValueMaxWindGeschw::~CWSValueMaxWindGeschw()
{
    SchreibDatei();
}

void CWSValueMaxWindGeschw::SaveDayValue(int iAkt)
{
    int iVal;
    
    iVal = m_iVal;
    m_iVal = iAkt;
    SaveValue(iVal);
}

string CWSValueMaxWindGeschw::WriteSpecificLine()
{
    string strRet;
    
    strRet = "2000;" + to_string(m_iVal) + ";";
    
    return strRet;
}

void CWSValueMaxWindGeschw::ReadSpecificLine(CReadFile *pReadFile)
{
    m_iVal = pReadFile->ReadZahl ();
    if(!pReadFile->ReadSeparator ())
        pReadFile->Error (62);
}

void CWSValueMaxWindGeschw::Control(short * sPtr)
{
    if((unsigned short)*(sPtr + IDXWINDGESCHW) > m_iVal)
        m_iVal = (unsigned short)*(sPtr + IDXWINDGESCHW);
}