/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CWSValue.h
 * Author: josefengel
 *
 * Created on April 27, 2020, 12:45 PM
 */

#ifndef CWSVALUE_H
#define CWSVALUE_H

class CWSValue : public CStatistic
{
public:
    CWSValue();
    ~CWSValue();
    virtual string WriteSpecificLine();
    virtual void ReadSpecificLine(CReadFile *pReadFile);
    virtual void Control(short * iPtr) {};
    virtual void SaveDayValue(int iAkt) {};
    virtual int GetValue() { return m_iVal; };
    double GetValueDbl();
    
protected:
    int m_iVal;
};

class CWSValueNSMenge : public CWSValue
{
public:
    CWSValueNSMenge(int nr);
    ~CWSValueNSMenge();
    virtual string WriteSpecificLine();
    virtual void ReadSpecificLine(CReadFile *pReadFile); 
    virtual void Control(short * iPtr);
    virtual void SaveDayValue(int iAkt);
    virtual int GetValue();
    
private:
    int m_iNSMengeAbsTagAnfang;
};

class CWSValueMaxTemp : public CWSValue
{
public:
    CWSValueMaxTemp(int nr);
    ~CWSValueMaxTemp();
    virtual string WriteSpecificLine();
    virtual void ReadSpecificLine(CReadFile *pReadFile); 
    virtual void Control(short * sPtr);
    virtual void SaveDayValue(int iAkt);
    
private:

};

class CWSValueMinTemp : public CWSValue
{
public:
    CWSValueMinTemp(int nr);
    ~CWSValueMinTemp();
    virtual string WriteSpecificLine();
    virtual void ReadSpecificLine(CReadFile *pReadFile); 
    virtual void Control(short * sPtr);
    virtual void SaveDayValue(int iAkt);
   
private:

};

class CWSValueMaxWindGeschw : public CWSValue
{
public:
    CWSValueMaxWindGeschw(int nr);
    ~CWSValueMaxWindGeschw();
    virtual string WriteSpecificLine();
    virtual void ReadSpecificLine(CReadFile *pReadFile); 
    virtual void Control(short * sPtr);
    virtual void SaveDayValue(int iAkt);
    
private:

};
#endif /* CWSVALUE_H */

