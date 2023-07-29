/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ProWo
 * Copyright (C) root 2014 <root@raspberrypi>
 * 
ProWo is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * ProWo is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CALCULATOR_H_
#define _CALCULATOR_H_
#include <string.h>

class CConfigCalculator
{
public:
    CConfigCalculator(CReadFile  *pfl);
    void eval(const char *expr, CIOGroup *pIOGroup, int type);
    int GetAnzOper();
    COperBase * GetOper(int i);
protected:
	
private:
    void factor();
    void sum();
    void prod();
    char cur() {return *m_cPtr;}
    void number();
    void AddOper(char ch);
    void AddOperToList(COperBase *pOper);
    void LesIntervalDiff(int *iInterval, int *iDiff);
    CReadFile *m_pReadFile;
    const char *m_cPtr;
    COperBase *m_ptrOper[MAXOPER];
    int m_iAnzOper;
    int m_iType; // 1=Ausgang, 2=Merker, 3=Timer 
    CIOGroup *m_pIOGroup;
};

class CCalculator
{
public:
    int eval(COperBase **pOper);
	
private:
    int factor();
    int sum();
    int prod();
    int cur(); 
    void next();
    int number();
    COperBase **m_pOper;
    int m_iType;    // 0 oder 1 wenn ein Wert, sonst z.B. Vergleichzeichen
};

#endif // _CALCULATOR_H_
