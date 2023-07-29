//
//  TF1 : Thermo und Feuchtigkeitssensor
//
// Diese Funktion wird auch gebraucht wenn die Adresse eines TQS3  umzustellen.
// Aus diesem Grunde darf die Funktion nicht nur einmal pro Minute
// aufgerufen werden, sondern öfter. Dies erfolgt über die Variable m_iChangeSensorModBusAddress
// die gesetzt wird wenn eine Änderung vorgenommen werden soll. Diese Funktion
// gibt 1 zurück wenn der Vorgang beendet ist oder abgebrochen wird.
//

#include "ProWo.h"

int CTH1 :: LesenStarten()
{
    unsigned char *ptr;
    int status;
    int ret = 0;
    char ptrLog[100];
    int32_t voc_index = 0;
    
    status = m_pModBusClient->GetStatus();  
    if(status == 3) // Empfang ist erfolgt
    {   
        status = m_pModBusClient->GetError ();
        ptr = m_pModBusClient->GetEmpfPtr();
        if(!status)
        {
            switch(m_iFunction) {
            case 1: // Temperatur und Feuchtigkeit lesen
                if(!*(ptr+3) && !*(ptr+4))
                {
                    m_iVocRawSignal = ptr[9]*256 + ptr[10];
                    if(m_iVocRawSignal)
                        VocAlgorithm_process(&m_VocAlgorithmParams, (int32_t)m_iVocRawSignal, (int32_t *)&voc_index);
                    pthread_mutex_lock(&ext_mutexNodejs);
                    m_iTemp = ptr[5]*256 + ptr[6];
                    m_iHumidity = ptr[7]*256 + ptr[8];
                    m_iVocSignal = (int)voc_index;
                    PlaceInStat();
                    pthread_mutex_unlock(&ext_mutexNodejs);
                }
                else
                {   
                    sprintf(ptrLog, "TH1 - Nummer: %d  Address: %d incorrect temperature", m_iNummer, m_pModBusClient->GetAddress());				    ;
                    syslog(LOG_ERR, ptrLog);
                }
                break;

            case 2: // Identification
                ret = m_pModBusClient->GetEmpfLen();
                *(ptr+ret+3) = 0;
                sprintf(ptrLog, "TH1 Modbus-Address = %d, ID = %s", m_pModBusClient->GetAddress() , ptr+5);
                syslog(LOG_INFO, ptrLog);
                ret = m_pModBusClient->GetAddress();
                SetFunction(1, 0, 0);
                break;

            case 3: // Adresse geändert
                ret = m_pModBusClient->GetNewAddress();
                ptr = m_pModBusClient->GetEmpfPtr();
                if(*(ptr+4) == (unsigned char)ret)
                {
                    sprintf(ptrLog, "Adresse geändert");
                    syslog(LOG_INFO, ptrLog);
                    m_pModBusClient->SetAddress(ret);
                }
                else
                {
                    sprintf(ptrLog, "Address not changed");
                    syslog(LOG_ERR, ptrLog);
                }
                SetFunction(1, 0, 0);
                break;
            default:
                break;
            }
        }
        else
        {	
            int adr = m_pModBusClient->GetAddress();
            if(m_iFunction == 2)
            {
                if(adr > 255)
                {
                    adr = 1;
                    ret = 256;
                }    
                else
                    adr++;
                m_pModBusClient->SetAddress(adr);		
            }
            else
            {
                sprintf(ptrLog, "TH1 - Nummer %d, Adresse = %d,  error %d", m_iNummer, adr, status);
                syslog(LOG_ERR, ptrLog);
                ret = -1;
            }
        }
        // Status wird auf 1 gesetzt, die Anfrage wird mit Senden neu gestartet
        m_pModBusClient->StartSend();
    }
    if(status == 0)
        m_pModBusClient->StartSend();
    return(ret);
}

CTH1::CTH1(int nr) : CSensorModBus( nr)
{
    VocAlgorithm_init(&m_VocAlgorithmParams);
    m_iTyp = 2;
}

CTH1::~CTH1() {
}

void CTH1 :: SetFunction(int func, int iLED, int iI2CTakt)
{
    unsigned char *pCh = m_pModBusClient->GetSendPtr ();
    m_iFunction = func;

    switch(func) {
    case 1:
        // Temperaturanfrage
        pCh[1] = 0x04; // function :reading of input register
        // !! folgendes ist nicht MODBUS-Konform denn hier müsste das Startregister stehen!!!
        // Ich brauch es um die Abfragerate der Sensoren und ob die LED eingeschaltet werden oder nicht
        pCh[2] = 0x00;
        pCh[3] = iI2CTakt * 2 + iLED;
        pCh[4] = 0x00; // register count
        pCh[5] = 0x03;
        m_pModBusClient->SetSendLen(6);
        m_pModBusClient->SetEmpfLen(8);
        break;
    case 3: 
        // Adresse ändern
        pCh[1] = 0x06;
        pCh[2] = 0x00;
        pCh[3] = 0x01;
        pCh[4] = 0x00;
        pCh[5] = m_pModBusClient->GetNewAddress();   // neue Adresse
        m_pModBusClient->SetSendLen(6);
        m_pModBusClient->SetEmpfLen(3);
        break;
    case 2:
    default:
        // Identifikationsanfrage
        pCh[1] = 0x11; 
        pCh[2] = 0x00; 
        pCh[3] = 0x00; 
        pCh[4] = 0x00;
        pCh[5] = 0x00;        
        m_pModBusClient->SetEmpfLen(14);
        m_pModBusClient->SetSendLen(6);
        break;
    }
}

