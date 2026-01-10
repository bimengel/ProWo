
#include "ProWo.h"

// Sekunden Timer
CSecTimer::CSecTimer()
{
    m_iDelay = 0;
    m_iStartzeit = 0;
    m_bLastState = false;
}

void CSecTimer::SetLastState()
{
    m_bLastState = (bool)GetState();
}

bool CSecTimer::Increment(int zeit)
{
    bool ret = false;

    if(m_iStartzeit)
    {
        if(m_iStartzeit + m_iDelay <= zeit)
        {
            m_iStartzeit = 0;
            ret = true;
        }
    }
    return ret;
}

time_t CSecTimer::GetState()
{
    time_t aktZeit;

    if(m_iStartzeit)
        aktZeit = m_iStartzeit + m_iDelay - m_pUhr->getUhrzeit(); 
    else
        aktZeit = 0;
    return aktZeit;
}

void CSecTimer::SetState(int iState)
{
    if(iState)
    {
        m_iStartzeit = m_pUhr->getUhrzeit();
        m_iDelay = iState;
    }	
    else
        m_iStartzeit = 0;
}

bool CSecTimer::GetStateE()
{
    bool ret;

    if(m_iStartzeit)
        ret = true;
    else
        ret = false;
    ret = (ret ^ m_bLastState) & ret;

    return ret;
}

bool CSecTimer::GetStateA()
{
    bool ret;

    if(m_iStartzeit)
        ret = true;
    else
        ret = false;
    ret = (ret ^ m_bLastState) & !ret;

    return ret;
}

//
// Basisklasse Operbase für alle Operationen
//
COperBase::COperBase()
{
    m_cType = 0;

}
void COperBase::setType(unsigned char type)
{
    m_cType = type;
}
unsigned char COperBase::getType()
{
    return m_cType;
}


//
// Abgeleitete Klasse von OperBase für Eingänge, 
//
COper::COper()
{
    m_cptr = NULL;
    m_cptrLast = NULL;
    m_coperand = 0;
}
void COper::setOper(char *cptr, char *cptrLast, char operand)
{
    m_cptr = cptr;
    m_cptrLast = cptrLast;
    m_coperand = operand;
}

int COper::resultInt()
{
    int ret=0;

    if(m_cptr != NULL)
    {	ret = *m_cptr & m_coperand;
        if(ret)
           ret = 1;
    }
    return ret;
}

// Eingang, Merker, S-Eingang einschalten
int COperE::resultInt()
{
    int ret=0;
    if(m_cptr != NULL && m_cptrLast != NULL)
    {	
        ret = ((*m_cptr ^ *m_cptrLast) & *m_cptr) & m_coperand;
        if(ret)
            ret = 1;
    }
    return ret;
}

// Eingang, Merker, S-Eingang  ausgeschaltet
int COperA::resultInt()
{
    int ret=0;
    if(m_cptr != NULL && m_cptrLast != NULL)
    {	
        ret = ((*m_cptr ^ *m_cptrLast) & ~(*m_cptr)) & m_coperand;
        if(ret)
            ret = 1;
    }
    return ret;
}

// Eingang, Merker, S-Eingang - Wechsel
int COperW::resultInt()
{
    int ret=0;
    if(m_cptr != NULL && m_cptrLast != NULL)
    {	
        ret = (*m_cptr ^ *m_cptrLast) & m_coperand;
        if(ret)
            ret = 1;
    }
    return ret;
}

int COperEWAusg::resultInt()
{    int ret=0;

    if(m_cptr != NULL)
    {
    	ret = *m_cptr & m_coperand;
        if(ret)
            ret = 1;
    }
    return ret;
}
//
// Integer Merker
//
CInteger::CInteger()
{
    m_iValue = 0;
    m_iLastState = 0;
}
int CInteger::GetState()
{
    return m_iValue;
}
void CInteger::SetState(int state)
{
    m_iValue = state;
}
void CInteger::SetLastState()
{
    m_iLastState = m_iValue;
}
bool CInteger::GetStateE()
{
    bool res = false;

    if(!m_iLastState && m_iValue)
        res = true;
    return res;
}
bool CInteger::GetStateA()
{
    bool res = false;

    if(m_iLastState && !m_iValue)
        res = true;
    return res;
}  
bool CInteger::GetStateW()
{
    bool res = false;

    if(m_iValue != m_iLastState)
        res = true;
    return res;
} 
//
// Operand Integer
//
COperInteger::COperInteger()
{
    m_pInteger = NULL;
}

int COperInteger::resultInt()
{
    return m_pInteger->GetState();
}

void COperInteger::setOper(CInteger *pInteger)
{
    m_pInteger = pInteger;
}

int COperIntegerE::resultInt()
{
    int res = 0;
    if(m_pInteger->GetStateE())
        res = 1;
    return res;
}
int COperIntegerA::resultInt()
{
    int res = 0;

    if(m_pInteger->GetStateA())
        res = 1;
    return res;
}
int COperIntegerW::resultInt()
{
    int res = 0;
    if(m_pInteger->GetStateW())
        res = 1;
    return res;
}
//
// Abgeleitet Klasse für die Timer
//
COperTimer::COperTimer()
{
    m_pSecTimer = NULL;
}

int COperTimer::resultInt()
{
    return m_pSecTimer->GetState();
}

void COperTimer::setOper(CSecTimer *ptr)
{
    m_pSecTimer = ptr;
}

int COperTimerE::resultInt()
{
    int res = 0;
    if(m_pSecTimer->GetStateE())
        res = 1;
    return res;
}
int COperTimerA::resultInt()
{
    int res = 0;
    if(m_pSecTimer->GetStateA())
        res = 1;
    return res;
}

//
// Ablgeleitete Klasse von OperBase für Tag- oder Nachtberechnung
//
COperTagNacht::COperTagNacht()
{
    m_iFct = 0;
}
void COperTagNacht::setOper(int iFct)
{
    m_iFct = iFct;
}
int COperTagNacht::resultInt()
{
    int ret;
    
    switch(m_iFct) {
        case 1:
            ret = m_pUhr->getTagNacht();
            break;
        case 2:
            ret = m_pUhr->getSA();
            break;
        case 3:
            ret = m_pUhr->getSU();
            break;
        default:
            ret = 0;
            break;
    }
    return ret;
}

//
// Ablgeleitete Klasse von OperBase für eine absolute Zahl
//
COperNumber::COperNumber()
{
    m_iNumber = 0;

}

int COperNumber::resultInt()
{
    return m_iNumber;
}

void COperNumber::setOper(int nr)
{
    m_iNumber = nr;
}

//
// Ablgeleitete Klasse von OperBase für ein Heizungsventil
//
COperHK::COperHK()
{
    m_pHeizKoerper = NULL;
}

int COperHK::resultInt()
{
    return m_pHeizKoerper->GetState();
}

void COperHK::setOper(CHeizKoerper *ptr)
{
    m_pHeizKoerper = ptr;
}

//
//  Abgeleitete Klasse von COperBase für einen Temperatursensor
//
COperSTHQ::COperSTHQ()
{
    m_pTempSensor = NULL;
}

void COperSTHQ::setOper(CSensor *ptr)
{
    m_pTempSensor = ptr;
}

int COperSTHQ::resultInt()
{
    return 1;
}

int COperTEMP::resultInt()
{
    return m_pTempSensor->GetTemp();
}     

int COperHUM::resultInt()
{
    return m_pTempSensor->GetParam2();
}  

int COperQUAL::resultInt()
{
    return m_pTempSensor->GetVocSignal();
}
