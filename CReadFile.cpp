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

#include "ProWo.h"

long CReadFile::GetFilePos()
{
    return ftell(m_fp);
}

bool CReadFile::SetFilePos(long lPos)
{
    bool bRet = false;
    
    if(m_fp != NULL)
    {
        if(fseek(m_fp, lPos, SEEK_SET))
        {
            m_strError = "file (" + m_strFileName + ")  seek to position (" + to_string(lPos) + "not possible";
            syslog(LOG_ERR, m_strError.c_str());
        }  
        else
            bRet = true;
    }
    return bRet;
}
char CReadFile::GetChar()
{
    if(m_iPos < m_iLen)
        return m_cBuf[m_iPos++];
    else
        return ' ';
}

char CReadFile::GetChar(int iIdx)
{
    if(iIdx >= 0 && iIdx < m_iLen)
        return m_cBuf[iIdx];
    else
        return ' ';
}

int CReadFile::ReadBuf(char *buf, int len)
{
    int i;
    for(i=0; i < len && m_iPos < m_iLen; i++, m_iPos++)
        *(buf + i) = m_cBuf[m_iPos];
    *(buf+i) = 0;
    return i;
}

int CReadFile::ReadBuf(char *buf, char ch)
{
    int i;
    for(i=0; m_iPos < m_iLen && m_cBuf[m_iPos] != ch; i++, m_iPos++)
        *(buf+i) = m_cBuf[m_iPos];
    *(buf+i) = 0;
    if(i==0 && m_iPos >= m_iLen) 
        i = -1; // JEN 25.01.18 Ende erreicht ohne ch gefunden zu haben !!
    else
        m_iPos++;
    return i;
}

bool CReadFile::ReadEqual ()
{
    bool ret = false;

    if(m_iPos < m_iLen)
    {
        if(m_cBuf[m_iPos++] == '=')
            ret = true;
    }
    return ret;
}

bool CReadFile::ReadSeparator ()
{
    bool ret = false;

    if(m_iPos < m_iLen)
    {
        if(m_cBuf[m_iPos++] == ';')
            ret = true;
    }
    return ret;
}

bool CReadFile::IsDigit()
{
    bool bRet = false;
    if(isdigit(m_cBuf[m_iPos]))
        bRet = true;
    return bRet;
}

int CReadFile::ReadZahl()
{
    string str;

    str.clear();
    for(; m_iPos < m_iLen ; )
    {
        if(isdigit(m_cBuf[m_iPos]) || m_cBuf[m_iPos] == '-')
        {
            str += m_cBuf[m_iPos];
            m_iPos++;
        }
        else
            break;
    }

    return atoi(str.c_str());

}

int CReadFile::ReadNumber()
{
    string str;

    str.clear();
    for(; m_iPos < m_iLen ; )
    {
        if(isdigit(m_cBuf[m_iPos]))
        {
            str += m_cBuf[m_iPos];
            m_iPos++;
        }
        else
            break;
    }

    return atoi(str.c_str());
	
}
void CReadFile::ResetLine()
{
    m_iPos = 0;
}

string CReadFile::ReadAlpha()
{
    string str;

    str.clear();
    for(; m_iPos < m_iLen ; )
    {
        if(isalpha(m_cBuf[m_iPos]))
        {
            str += m_cBuf[m_iPos];
            m_iPos++;
        }
        else
            break;
    }

    return str;
}

string CReadFile::ReadText()
{
    string str;
    char ch, chUmlaut;

    str.clear();
    for(; m_iPos < m_iLen ; )
    {
        ch = m_cBuf[m_iPos];
        if(ch == 0xC3)
        {
            m_iPos++;
            chUmlaut = m_cBuf[m_iPos];
            // ÄÜÖäüö
            if(chUmlaut == 0x84 || chUmlaut == 0x9C || chUmlaut == 0x96 
                    || chUmlaut == 0xA4 || chUmlaut == 0xBC || chUmlaut == 0xB6)
            {
                m_iPos++;
                str = str + ch + chUmlaut;
            }
        }
        else if(isprint(ch) || isblank(ch))
        {
            str += ch;
            m_iPos++;
        }
        else
            break;
    }

    return str;
}

string CReadFile::ReadText(char chTrennzeichen)
{
    string str;
    char ch;

    str.clear();
    for(; m_iPos < m_iLen; )
    {
        ch = m_cBuf[m_iPos];
        if(ch != chTrennzeichen && ch != 13 && ch != 10)
        {
            str += ch;
            m_iPos++;
        }
        else
            break;
    }

    return str;
}

string CReadFile::ReadGsmNr()
{
    string str;
    bool bFirst = true;

    str.clear();
    for(; m_iPos < m_iLen ; )
    {
        if(isdigit(m_cBuf[m_iPos]) || (bFirst && m_cBuf[m_iPos] == '+'))
        {
            str += m_cBuf[m_iPos];
            m_iPos++;
        }
        else
            break;
    }

    return str;
}
	
int CReadFile::OpenRead(char *pProgramPath, int type, int nr, int iTest)
{
    string strName;

    m_iType = type;

    strName = GetFileName(pProgramPath, type, nr);
    m_fp = fopen(strName.c_str(), "r");

    // JEN 01.09.23 folgende Tests entfernt
    // type != 5 (Zählerdaten) 
    // type != 12 (Wetterdaten)
    // type != 15 (Wecker)
    // type != 11 (Daten der History)
    if(!iTest && m_fp == NULL)
    {   
        m_strError= "Can't open file for reading " + strName;
        syslog(LOG_ERR, m_strError.c_str());
        Error(1);
    }

    // gibt 0 zurück wenn die Datei nicht existiert. Wird gebraucht wenn ein
    // Zaehler generiert wird
    if(m_fp == NULL)
        return 0;
    else
        return 1;
}

int CReadFile::OpenWrite(char *pProgramPath, int type, int nr)
{
    string strName;

    strName = GetFileName(pProgramPath, type, nr);
    ControlPath(strName);
    m_fp = fopen(strName.c_str(), "w");
    if(m_fp == NULL)
    {   
        m_strError = "Can't open file for writing " + strName;
        syslog(LOG_ERR, m_strError.c_str());
        Error(1);
    }

    return 1;
}
int CReadFile::OpenAppend(char *pProgramPath, int type, int nr)
{
    string strName;
    
    strName = GetFileName(pProgramPath, type, nr);
    ControlPath(strName);
    m_fp = fopen(strName.c_str(), "a");
    if(m_fp == NULL)
    {   
        m_strError = "Can't open file for writing " + strName;
        syslog(LOG_ERR, m_strError.c_str());
        Error(1);
    }

    return 1;    
}

int CReadFile::OpenWriteExisting(char *pProgramPath, int type, int nr)
{
    string strName;
    
    strName = GetFileName(pProgramPath, type, nr);
    m_fp = fopen(strName.c_str(), "r+");
    if(m_fp == NULL)
    {   
        m_strError = "Can't open file for writing " + strName;
        syslog(LOG_ERR, m_strError.c_str());
        Error(1);
    }

    return 1;    
}
int CReadFile::WriteLine(const char *pBuf)
{
    return WriteLine(pBuf, strlen(pBuf));
}
int CReadFile::WriteLine(const char *pBuf, int len)
{
    int ret;
    
    ret = fwrite(pBuf, 1, len, m_fp);
    if(ret == len)
        return 1;
    else
    {   
        m_strError = "Can't write to file " + m_strFileName;
        syslog(LOG_ERR, m_strError.c_str());
        return 0;
    }
}


string CReadFile::GetFileName(char *pProgramPath, int typ, int nr)
{
    // typ = 1, Konfiguration
    //      = 2, Parametrierung
    //      = 3, Menu
    //      = 4, GSM Konfiguration
    //      = 5, Zaehlerdaten je nach Nummer, nr = Zählernummer
    //      = 6, Heizung Konfiguration 
    //      = 7, Alarm
    //      = 8, Musik
    //      = 9, Logdatei der Alarmanlage
    //      = 10, History ProWo.history
    //      = 11, History Write- und Read-Datei nr = Datum im time_t Format
    //      = 12, Wetterstation Werte
    //      = 13, Sonos
    //      = 14, Sonosparamter (wenn test = 4)
    //      = 15, Alarm Clock (Wecker)
    //      = 16 Standardverzeichnis für copyFile Zähler und Datei lesen 
    //      = 17 Standardverzeichnis für copyFile Wetterstation und Datei lesen
    
    string strName;

    strName.clear();
    switch(typ) {
    case 1:
        strName = "Standard/ProWo.config";
        break;
    case 2:
        strName = "Standard/ProWo.parameter";
        break;
    case 3:
        strName = "Standard/ProWo.menu";
        break;
    case 4:
        strName = "Standard/ProWo.gsmconf";
        break;
    case 5: // Zaehlerdaten entsprechend der Indexnummer
        {   
            struct tm pt;
            m_pUhr->getUhrzeitStruct(&pt);
            strName =  "Data/" + to_string(pt.tm_year+1900) + "/Zaehler" + to_string(nr) + "/Zaehler" + to_string(nr) + ".dat";
        }
        break;
    case 6:
        strName = "Standard/ProWo.heizung";
        break;
    case 7:
        strName = "Standard/ProWo.alarm";
        break;
    case 8:
        strName = "Standard/ProWo.musik";
        break;
    case 9: // Logdatei der Alarmanlage
        {   
            struct tm pt;
            m_pUhr->getUhrzeitStruct(&pt);
            strName =  "Data/" + to_string(pt.tm_year+1900) + "/Alarm/" + to_string(pt.tm_mon+1) + ".log";
        }
        break;
    case 10: 
        strName = "Standard/ProWo.history";
        break;
    case 11:
        {
            int monat, jahr;
            monat = nr / 31;
            jahr = monat / 12;
            strName = "Data/History/" + to_string(jahr) + "/" + to_string(monat) + "/" + to_string(nr) + ".bin";
        }
        break;
    case 12: // Wetterdaten entsprechend der Indexnummer
        {   
            struct tm pt;
            m_pUhr->getUhrzeitStruct(&pt);
            strName =  "Data/" + to_string(pt.tm_year+1900) + "/WSDaten" + to_string(nr) + "/WSDaten" + to_string(nr) + ".dat";
        }
        break;    
    case 13: // Sonos
        strName = "Standard/ProWo.sonos";
        break;
    case 14: // Sonosparamter
        strName = "Data/Sonos.log";
        break;
    case 15: // Alarm Clock
        strName = "Standard/ProWo.alarmclock";
        break;
    case 16:
        {   
            struct tm pt;
            m_pUhr->getUhrzeitStruct(&pt);
            strName =  "Standard/Zaehler" + to_string(nr) + ".dat";
        }
        break;
    case 17:
        {   
            struct tm pt;
            m_pUhr->getUhrzeitStruct(&pt);
            strName =  "Standard/WSDaten" + to_string(nr) + ".dat";
        }
        break;    
    default:
        break;
    }

    strName = string(pProgramPath) + strName;
    m_strFileName = strName;
    return strName;
}
int CReadFile::ReadLine()
{
    int ok=1;
    bool bFirst = true;
    bool bEnableSpace = false;
    int i;
    
    while(ok)
    {	
        m_iZeile++;
        bEnableSpace = false;
        for(m_iPos=0; m_iPos < 256; m_iPos++)
        {	
            m_cBuf[m_iPos] = fgetc(m_fp);
            i = m_iPos;
            if(m_cBuf[m_iPos] == 0xff) 
                break;
            if(m_cBuf[m_iPos] == '\n')
                    break;
            if(m_cBuf[m_iPos] == '"')
            {
                if(bEnableSpace)
                    bEnableSpace = false;
                else
                    bEnableSpace = true;
                m_iPos--;
                continue;
            }
            // Leerzeichen und Tabulator entfernen
            // Leerzeichen werden erst entfernt wenn ein Komma eingelesen wurde
            if(m_iType == 3 ) // PHP Menu
            {	   
                if(bFirst)
                {   
                    if(m_cBuf[m_iPos] == 0x20)
                        continue;
                    if(m_cBuf[m_iPos] == '\t')
                    {
                        m_cBuf[m_iPos] = 0x20;
                        continue;
                    }
                    if(m_cBuf[m_iPos] == ',')
                        bFirst = false;
                }
                else
                {   
                    if((m_cBuf[m_iPos] == 0x20 && !bEnableSpace) || m_cBuf[m_iPos] == '\t') 
                        m_iPos--;
                }
            }
            else if(m_iType == 4 ) // gsmconfig
            {	   

            }
            else // Config und Parameter
            {   if((m_cBuf[m_iPos] == 0x20 && !bEnableSpace) || m_cBuf[m_iPos] == '\t') 
                            m_iPos--;
            }
        }
        // es ist eine Zeile eingelesen
        if(m_cBuf[0] == '\'') // handelt sich um eine Kommentarzeile
            continue;
        if(m_cBuf[m_iPos] == 0xff)
        {   
            m_cBuf[m_iPos] = '\n';
            break;			// Ende der Datei
        }
        if(m_iPos == 0)
            continue;		// es handelt sich um eine Leerzeile
        ok = 0;
        m_cBuf[m_iPos] = 0;
    }
    for(i=0; i < m_iPos && m_cBuf[i] != '\''; i++);
    m_iLen = i;
    m_iPos = 0;
    return m_iLen;
}

bool CReadFile::ReadBinary(char *pBuf, int len)
{
    int i;
    bool bRet = true;
    
    i = fread(pBuf, 1 , len, m_fp);
    if(i != len)
        bRet = false;
    return bRet;
}
void CReadFile::Close()
{
    if(m_fp != NULL)
        fclose(m_fp);
    m_fp = NULL;
}

CReadFile::CReadFile()
{
    m_iType = 0;
    m_fp = NULL;
    m_iZeile = 0;
    m_iPos = 0;
    m_strError.clear();
}

	
//
//  ruft throw auf mit einer berechneten Nummer bestehend aus
//
//  1. byte : 0 nicht definiert, 1=Config, 2=Param
//  2. byte : Zeilennummer
//  3. byte : Position in der Zeile
//  4. byte : Fehlerbezeichnung

void CReadFile::Error(int error)
{
    int ret;
    ret = ((m_iType*4096 + m_iZeile)*256 + m_iPos+1)*256 + error;
    throw(ret);
}

int CReadFile::CountSpace()
{
    int i;

    for(i=0; m_iPos < 256 && m_cBuf[i] == 0x20; i++, m_iPos++);

    return i;
}

void CReadFile::ControlPath(string str)
{
    int iPos, result;
    struct stat status;
    
    iPos = str.find_last_of("/");
    str = str.substr(0, iPos);
    result = stat(str.c_str(), &status);
    if(result != 0 || !S_ISDIR(status.st_mode))
    {
        ControlPath(str);
        mkdir(str.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
}

bool CReadFile::OpenAppendBinary(char *pProgramPath, int type, int nr)
{
    string strName;
    bool bRet = true;
    
    strName = GetFileName(pProgramPath, type, nr);
    ControlPath(strName);
    m_fp = fopen(strName.c_str(), "ab");
    if(m_fp == NULL)
    {
        m_strError = "Can't open file for writing " +  strName;
        syslog(LOG_ERR, m_strError.c_str());
        bRet = false;
    }  

    return bRet;    
}

string CReadFile::GetError()
{
    return m_strError;
}