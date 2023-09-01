#include "ProWo.h"

// MCP 23017 Register
#define IODIR	0x00 // IO Direction Register
#define IPOL	0x01 // Input Polarity Port Register
#define GPINTEN 0x02 // Interrupt on Change Pin's
#define DEFVAL  0x03 // Default Value Register
#define INTCON  0x04 // Interrupt on Change Control Register
#define IOCON	0x05 // IO Expander Configuration Register
#define GPPU    0x06 // GPIO Pull-up Resistor Register (100 k) 
#define INTF	0x07 // Interrupt Flag Register
#define INTCAP	0x08 // Interrupt captured value for port register
#define GPIO	0x09 // General purpose I/O port register (value on the port)
#define OLAT	0x0A // Output Latch Register

//RTRM08 Register
#define RX_MODE			0
#define PWD_RST			0
#define PWD				1 // Module Transceiver is switched off
#define RX_RQ			2 // Receive request mode
#define RX_PL			3 // Receive polling mode
#define LN_RQ			4 // Learn Request mode
#define LN_PL			5 // Learn polling mode
#define DELLN			6 // Transceiver switched off, deletes a learned telegram

#define RX_STATUS		1
#define RX_TEL_CHANGED	(1 << 7) // receive a EasyWave telegram
#define RX_TEL_VALID	(1 << 6) // receive a knowed EasyWave telegram

#define RX_CHANNEL		2
#define RX_BUTTON		3
#define RX_USERDATA0	4
#define RX_USERDATA1	5
#define RX_ACK			6

#define TX_STATUS		0x10
#define TX_TRANSMIT		(1 << 7)

#define TX_CHANNEL		0x11
#define TX_BUTTON		0x12
#define TX_ACK			0x13

#define RX_LN_CH0_7		0x20
#define RX_LN_CH8_15	0x21
#define RX_LN_CH16_23	0x22
#define RX_LN_CH24_31	0x23

// 
// bOK, nur auf true beim Ersten Aufruf
//

void CIOGroup::Control(bool bOK)
{
    int i, j;
    bool bWriteHistory = false;
    bool bSekundenTakt = false;
    bool bStundenTakt = false;
    bool bMinutenTakt = false;
    bool bBerechne = false;
    string str;

    CHistoryElement HistoryElement;
    struct EWAusgEntity EWAusg;
    
    int len, res;

    // Uhr aktualisieren
    m_pUhr->aktUhr();    

    j = check_gpio(18);
    if(bOK || j)
    {   // beim Aufstarten und wenn sich KEIN INT ansteht
        // (INT steht an wenn sich ein Eingang, HW oder Funk, geändert hat)
        // m_iControl wird bei jedem Durchlauf wo ein INT anstand um eine Einheit erhöht
        // und wenn dies 20x hintereinander erfolgt wird "INT steht immer an" 
        // in der dritten Zeile des Displays angezeigt
        if(m_iControl >= 20)
        {   // INT stand mehr als 20x an und "INT steht immer an" wird in der
            // dritten Zeile gelöscht
            str.clear();
            LCD_AnzeigeZeile3(str.c_str());            
        }
        m_iControl = 0;

        // Temperatursensoradresse (TQS3 + TF1) ändern 
        if(m_iChangeSensorModBusAddress)
        {   
            i = m_pSensor[m_iChangeSensorModBusAddress - 1]->LesenStarten();    
            if(i)
            {
                m_iChangeSensorModBusAddress = 0;
                if(i > 0)
                {
                    if(i == 256)
                        str = "Sensor not found";
                    else
                        str = "Adresse: " + to_string(i);
                    LCD_AnzeigeZeile3(str.c_str());                
                }
            }
            else
                return;
        }  
        
        if(m_pUhr->SekundenTakt())
        {	
            j = m_pUhr->getUhrzeit();
            bSekundenTakt = true;
            // Increment gibt true zurück wenn ein der Timer abgelaufen ist
            for(i=0; i < m_SecTimerAnz; i++)
                    bBerechne += m_pSecTimer[i].Increment(j);
            
            // Timer der Alarmanalge berechnen
            if(m_pAlarm != NULL)
                bBerechne += m_pAlarm->BerechneTimer(j);
            
            // Die Dauer die ein Funksignal ansteht beträgt maximal etwa 35 Sekunden
            // Sollte das Abschaltsignal nicht empfangen worden sein, werden die 
            // Signale A - D nach einer EWEINGDELAY (45 s) Dauer gelöscht.
            for(i=0; i < m_EWAnzBoard * EWANZAHLCHANNEL; i++)
            {
                if(m_pEWEingDelay[i] > 0)
                {
                    m_pEWEingDelay[i]--;
                    if(m_pEWEingDelay[i] <= 0)
                    {
                        str = "Funkeingangsnummer " + to_string(i+1) + " zurückgesetzt : " + to_string(m_pEWEing[i] & 0xF0);
                        syslog(LOG_INFO, str.c_str());
                        m_pEWEing[i] &= 0xF0;
                        bBerechne = true;
                    }
                }
            }
            if(m_iTest == 1)
            {   // Sensoren einlesen
                for(i=0; i < m_iAnzSensor; i++)
                    m_pSensor[i]->LesenStarten();
            }
            else if(m_iTest == 2)
            {   // Zähler einlesen
                for(i=0; i < m_iAnzZaehler; i++)
                    m_pZaehler[i]->LesenStarten();
            }
            else if(m_iTest == 3) // Wetterstation einlesen
            {
                if(m_pWStation != NULL)
                    m_pWStation->LesenStarten();
            }
            else if(m_iTest == 5) // Ausgänge hintereinander ansteuern
            {
                
            }
        }

        // die Berechnung erfolgt einmal pro Minute
        if(m_pUhr->MinutenTakt() || bOK)
        {
            bBerechne = true;
            bMinutenTakt = true;

            if(m_iTest == 0 || m_iTest == 4) 
            {
                // Wetterstation einlesen
                if(m_pWStation != NULL)
                    m_pWStation->LesenStarten();
                // Sensoren einlesen
                for(i=0; i < m_iAnzSensor; i++)
                    m_pSensor[i]->LesenStarten();
                // Zähler einlesen
                for(i=0; i < m_iAnzZaehler; i++)
                    m_pZaehler[i]->LesenStarten();
            }
        }

        // Einmal pro Stunde wird getestet ob das GSM mit einem Provider verbunden ist
        if(m_pUhr->StundenTakt ())
            bStundenTakt = true;

        // Tagestakt
        if(m_pUhr->TagesTakt())
        {
            // beim Tageswechsel, die Zählerstände speichern
            for(i=0; i < m_iAnzZaehler; i++)
                m_pZaehler[i]->SaveDayValue ();

            if(m_pWStation != NULL)
                m_pWStation->TagWechsel();
        }
        
        // Für die Schnittstelle UART - GSMBoard kann für die AT-Kommandos
        // ist ein flow control definert
        // Aus diesem Grunde braucht auch nur die Kontrolle  alle 500 msec 
        // gestartet zu werden
        // muss in diesem Thread untergebracht sein, da auf den I2C-Bus zugegriffen wird!
        if(m_pGsm != NULL)
        {
            m_gsmtimeslice++;
            if(m_gsmtimeslice >= GSMZYKLUSZEIT || bStundenTakt || bMinutenTakt)
            {
                m_gsmtimeslice = 0;
                if(m_pGsm->Control(GSMZYKLUSZEIT, bStundenTakt, bMinutenTakt))
                    m_bBerechne = true;
            }
        }
    }
    else
    {
        Les_IOGroup();
        bBerechne = true;
        if(m_iControl == 20)
        {
            str = "INT immer aktiv!";
            LCD_AnzeigeZeile3(str.c_str());
        }    
        else
            m_iControl++;
      
    }
    if(bOK)	// Beim Aufstarten werden alle Eingänge eingelesen
        LesAlleEing ();
    if(m_iTest == 10)
    {
        usleep(500000);
        LesAlleEing();
    }

    pthread_mutex_lock(&ext_mutexNodejs);   
    if(m_pHistory != NULL)
    {
        HistoryElement = m_pHistory->Control();
        // die Kontrolle ob in ProWo.history (enabled) ist bereits erfolgt
        switch(HistoryElement.m_sTyp)
        {
            case 1: // Ausgänge
                for(i=0; i < m_AusgAnz / PORTANZAHLCHANNEL; i++)
                    m_pcAusgHistory[i] = HistoryElement.m_chAusg[i];
                bBerechne = true;
                break;
            case 2: // EasyWave Ausgänge
                EWAusg.iNr = HistoryElement.m_iSwitch;
                EWAusg.iSource = 2;
                m_EWAusgFifo.push(EWAusg);
                break;
            case 3: // HUE Ausgänge
                HistoryElement.m_HueProperty.m_iSource = 2;
                m_pHue->InsertFifo(&HistoryElement.m_HueProperty);
                break;
            default:
                break;
        }
            
    }
 
    // Status der Heizungsventile berechnen
    if(m_pHeizung != NULL)
    {
        if(bMinutenTakt || m_bBerechneHeizung)
            m_pHeizung->Control();
        m_bBerechneHeizung = false;
    }
    if(bBerechne)
        m_bBerechne = true;
    if((m_bBerechne || bOK) && !m_bHandAusg)
    {
        // Hier werden alle Berechungen bezüglich des Zustandes der Merker,
        // Ausgänge, Timer und EasyWaveaus- und eingänge berechnet
        BerechneParameter(m_pUhr->getUhrzeit());
    }
    bBerechne = m_bBerechne;
    m_bBerechne = false;
    pthread_mutex_unlock(&ext_mutexNodejs);
    
    if(bOK) // Programm startet
    {	// Ausgänge anstellen
        for(i=0; i<m_AusgAnz / PORTANZAHLCHANNEL;i++)	
        {
            Write_Ausg (&m_pAusgBoardAddr[i], m_pcAusg[i]);
            m_pcAusgLast[i] = m_pcAusg[i];
        }
        bWriteHistory = true;
    }

    if(bBerechne)
    {   
        // Ausgänge anstellen, nur wenn auch eine Änderung
        // Für alle Ausgänge den aktuellen Wert als letzten setzen
        if(m_pHistory && m_pHistory->GetDiffTage())        
        {
            for(i=0; i < m_AusgAnz / PORTANZAHLCHANNEL; i++)
            {
                m_pcAusg[i] &= m_pcInvAusgHistoryEnabled[i];
                m_pcAusg[i] |= m_pcAusgHistory[i];            
            }
        }
        if(m_iTest == 5)
        {
            if(bSekundenTakt || !m_iAusgTest)
            {   if(m_iAusgTest > m_AusgAnz)
                    m_iAusgTest = 1;
                else
                    m_iAusgTest++;
            }
            for(i=0; i < m_AusgAnz / PORTANZAHLCHANNEL; i++)
                m_pcAusg[i] = 0;
            SetAusgStatus(m_iAusgTest, true);
        }  
        for(i=0; i < m_AusgAnz / PORTANZAHLCHANNEL; i++)
        {
            if(m_pcAusgLast[i] != m_pcAusg[i])
            {
                Write_Ausg (&m_pAusgBoardAddr[i], m_pcAusg[i]);
                m_pcAusgLast[i] = m_pcAusg[i];
                bWriteHistory = true;
            }
        }
        
        // alle SMS Eingänge auf 0 setzen !!
        if(m_pGsm != NULL)
            m_pGsm->InitSMSEmpf(); 
    }
    
    if(bWriteHistory && m_pHistory != NULL)
        m_pHistory->Add(m_pcAusg);

    //
    //  EasyWave-Protokolle versenden
    //
    if(!m_iLastEWButton && !m_EWAusgFifo.empty())
    {	int channel, button;
        struct EWAusgEntity EWAusg;
        bool bSend = true;
        
        EWAusg = m_EWAusgFifo.front();
        channel = EWAusg.iNr / 256;
        button = EWAusg.iNr % 256;
        if(m_pHistory != NULL && m_pHistory->GetDiffTage())
        {
            // Lichtprogramm ist aktiviert
            // Parameter (iSource == 1) nur senden wenn nicht vom Lichtprogramm gesteuert
            if(EWAusg.iSource == 1 && m_pHistory->IsFAEnabled(channel))
                bSend = false;
        }            
        if(bSend)
        {
            if(EW_TransmitFinish(channel))
            {
                EW_Transmit(channel, button);		
                m_EWAusgFifo.pop();
                if(m_pHistory != NULL)
                    m_pHistory->Add(EWAusg.iNr);
            }
        }
        else
            m_EWAusgFifo.pop();
    }    
}

//
// Parametrierung berechnen
//
void CIOGroup::BerechneParameter(int iTime)
{
    int res, i;
    static bool bFirst = true;
    
    if(bFirst)
        bFirst = false;
    else
    {
        if(m_pAlarm != NULL)
        {
            // gibt != von 0 zurück wenn die Alarmanlage abgeschaltet wurde
            m_pAlarm->Berechne(iTime);
        }
    }
    // Parameterliste berechnen
    BerechneZeile(0, true, true);

    // Für alle Softwareeingänge den aktuellen Wert als letzten setzen
    res = m_SEingAnz / PORTANZAHLCHANNEL;
    for(i=0; i < res; i++)
        m_pcSEingLast[i] = m_pcSEing[i];

    // Für alle EWEAxx, EWEBxx, EWECxx und EWEDxx Eingänge Status auf 0 setzen
    // Der Test auf den Wert entspricht dann einer Flanke
    res = GetEWAnz();
    for(i=0; i < res; i++)
        m_pEWEingLast[i] = m_pEWEing[i];

    // Für alle Eingänge den aktuellen Wert als letzten setzen
    for(i=0; i < m_EingAnz / PORTANZAHLCHANNEL; i++)
    {
        m_pcEingLast[i] = m_pcEing[i];
        m_pEingInterrupt[i] = 0;
    }

    // Für alle Merker den aktuellen Wert als letzten setzen
    for(i=0; i < m_MerkAnz / PORTANZAHLCHANNEL; i++)
        m_pcMerkLast[i] = m_pcMerk[i];

    // für alle Integer Merker den aktuellen Wert als letzten setzen
    for(i=0; i < m_IntegerAnz; i++)
        m_pInteger[i].SetLastState ();

    // für alle Timer den aktuellen Wert als letzten setzen
    for(i=0; i < m_SecTimerAnz; i++)
        m_pSecTimer[i].SetLastState ();  
}
//
//	Berechnet die Parameterliste
//	bei jedem IF ruft sie sich selber

int CIOGroup::BerechneZeile(int iLine, bool bState, bool bEval)
{
    bool bElse = false;
    bool bTemp;

    for(; iLine < m_iBerechneAnz; iLine++)
    {
        switch(m_pBerechne[iLine]->getIfElse ()) {
        case 0: // normale Zeile
            if(bState)
            {	if((bEval && !bElse) || (!bEval && bElse))
                  m_pBerechne[iLine]->eval();
            }
            break;
        case 1:	// IF Zeile
            bTemp = (bool) m_pBerechne[iLine]->eval();
            iLine = BerechneZeile(++iLine, bState && ((bEval && !bElse) || (!bEval && bElse)), bTemp);
            break;
        case 2:	// Else Zeile
            bElse = true;
            break;
        case 3:
        default:
            return iLine;
            break;
        }
    }
    return iLine;
}
int CIOGroup::GetTest()
{
    return m_iTest;
}

void CIOGroup::Les_IOGroup()
{
    int i, gpio1, gpio2, interrupt, ret;
    
    for(i=0; i < m_EingAnz / PORTANZAHLCHANNEL; i++)
    {
        (&m_pEingBoardAddr[i])->setI2C_gpio();
        interrupt = BSC_ReadReg(1, m_pEingBoardAddr[i].Addr3, 
                          m_pEingBoardAddr[i].Reg + INTF);
        // JEN 24.09.19 m_pEingInterrupt
        // könnte gebraucht werden wenn die Impulse nicht korrekt erfasst werden
        // wird im Programm noch nicht verwertet !!! und könnte entfernt werden
        m_pEingInterrupt[i] = interrupt;
        // im INTCAP Register werden die Eingänge zum Zeitpunkt des Interrupts
        // gespeichert. Der Interrupt erfolgt wenn die Eingänge sich von
        // dem Inhalt der DEFVAL-Registers unterscheiden
        gpio1 = BSC_ReadReg(1, m_pEingBoardAddr[i].Addr3,
              m_pEingBoardAddr[i].Reg + INTCAP);
        // neue Eingangszustände für kommende Änderungen definieren
        BSC_WriteReg(1, m_pEingBoardAddr[i].Addr3, gpio1,
              m_pEingBoardAddr[i].Reg + DEFVAL);
        // wird gebraucht um den Interrupt zu löschen
        gpio2 = BSC_ReadReg(1, m_pEingBoardAddr[i].Addr3,
                  m_pEingBoardAddr[i].Reg + GPIO);   
        // Übernahme des neuen Zustandes der Eingänge
        m_pcEing[i] = gpio1;
    }
    

    for(i=0; i < m_EWAnzBoard; i++)
    {
        if(EW_TransmitFinish((i+1)*EWANZAHLCHANNEL))
            ret += EW_Received(i);
    }
}

int CIOGroup::Write_Ausg(CBoardAddr *addr, int value)
{
    addr->setI2C_gpio();
    BSC_WriteReg(1, addr->Addr3, value, addr->Reg + 0x0A);
    return 1;	
}

void CIOGroup::InitGroup()
{
    int i, j, ok, fd, result;
    m_iZykluszeit = ZYKLUSZEIT;

    try
    {
        // der Mainboard-Type muss bekannt sein damit die Uhrzeit gelesen werden kann;
        PreReadConfig(pProgramPath);
        m_pUhr = new CUhr;

        
        // ProWo.config einlesen
        ReadConfig(pProgramPath);
        syslog(LOG_INFO, "ProWo.config has been read");
        
        // Wenn die Datei ProWo.heizung existiert diese einlesen
        m_pReadFile = new CReadFile(); 
        fd = m_pReadFile->OpenRead(pProgramPath, 6, 0, 1);
        m_pReadFile->Close();
        delete m_pReadFile;  
        m_pReadFile = NULL;
        if(fd > 0)
        {   // ProWo.heizung existiert und wird eingelesen
            m_pHeizung = new CHeizung;
            LesHeizung(pProgramPath);
            syslog(LOG_INFO, "ProWo.heizung has been read");
        }

        // Wenn es die Datei ProWo.musik gibt, diese einlesen
        m_pReadFile = new CReadFile(); 
        fd = m_pReadFile->OpenRead(pProgramPath, 8, 0, 1);
        m_pReadFile->Close();
        delete m_pReadFile;  
        m_pReadFile = NULL;
        if(fd > 0)
        {   // ProWo.musik ist vorhanden
            m_pMusic = new CMusic();
            m_pMusic->LesDatei(pProgramPath);
            syslog(LOG_INFO, "ProWo.musik has been read");
        }

        // wenn Softwareeingänge definiert sind, das Menu einlesen
        // war früher in PHP geschrieben, seit 2018 in jQuery, Name ist geblieben
        if(m_SEingAnz)
        {
            LesBrowserMenu (pProgramPath);
            syslog(LOG_INFO, "ProWo.menu has been read");
        }	

        // Alarm einlesen wenn es die Datei ProWo.alarm gibt
        m_pReadFile = new CReadFile();         
        fd = m_pReadFile->OpenRead(pProgramPath, 7, 0, 1);
        m_pReadFile->Close();
        delete m_pReadFile;  
        m_pReadFile = NULL;
        if(fd > 0)
        {   // ProWo.alarm existiert und wird eingelesen
            LesAlarm(pProgramPath);
            syslog(LOG_INFO, "ProWo.alarm has been read");
        }

        // GSM - Konfiguration einlesen
        if(m_pGsm != NULL)
        {
            m_pGsm->LesDatei(pProgramPath, (char *)this);
            syslog(LOG_INFO, "ProWo.gsmconf has been read");
        }

        // History einlesen wenn es die Datei ProWo.history gibt
        m_pReadFile = new CReadFile();         
        fd = m_pReadFile->OpenRead(pProgramPath, 10, 0, 1);
        m_pReadFile->Close();
        delete m_pReadFile;  
        m_pReadFile = NULL;
        if(fd > 0)
        {   // ProWo.history existiert und wird eingelesen
            LesHistory(pProgramPath);
            syslog(LOG_INFO, "ProWo.history has been read");
        }        
        
        // Sonos einlesen wenn es die Datei ProWo.sonos gibt
        m_pReadFile = new CReadFile();         
        fd = m_pReadFile->OpenRead(pProgramPath, 13, 0, 1);
        m_pReadFile->Close();
        delete m_pReadFile;  
        m_pReadFile = NULL;
        if(fd > 0)
        {   // ProWo.sonos existiert und wird eingelesen
            m_pSonos = new CSonos(this);
            syslog(LOG_INFO, "ProWo.sonos has been read");
        }
        
        // Parametrierung einlesen
        LesParam(pProgramPath);
        syslog(LOG_INFO, "ProWo.parameter has been read");
        
    }

    catch(int er)
    {	unsigned int type, zeile, pos, error, e;
        string str1, str2, str3;
        CKeyBoard kb;
        int button;
        
        e = (unsigned int)er; 
        error = e %256;
        e /= 256;
        pos = e%256;
        e /= 256;
        zeile = e%4096;
        type = e/4096;

        switch(type) {
        case 1 : //Config
            str1 = "Config: ";
            break;
        case 2 : // Parametrierung
            str1 = "Parameter: ";
            break;
        case 3: // Menukonfiguration
            str1 = "Menu: ";
            break;
        case 4: // GSM Konfiguration
            str1 = "GSM: ";
            break;
        case 5: // PV Daten
            str1 = "Zaehlerx: ";
            break;
        case 6: // Heizung
            str1 = "Heizung: ";
            break;
        case 7: // Alarm
            str1 = "Alarm: ";
            break;
        case 8: // Music
            str1 = "Musik: ";
            break;
        case 10: // History
            str1 = "History: ";
            break;
        default:
            zeile = 0;
            str1 = "Unknown Error!!";
            break;
        }

        if(zeile)
        {
            str1 += to_string(error);
            str2 = "Line:" + to_string(zeile) + ",Pos:" + to_string(pos);
        }
        else
            str2.clear();
        switch(error) {
        case 1:
            str3 = "Can't open file";
            break;
        case 2:
            str3 = "  error";
            break;
        case 3:
            str3 = " false RS485 nbr";
            break;
        case 4:
            str3 = " no '=' defined";
            break;
        case 5:
            str3 = " number to high";
            break;
        case 6:
            str3 = " not implemented";
            break;
        case 7:
            str3 = " unknown board";
            break;
        case 8:
            str3 = " address exist";
            break;
        case 9:
            str3 = "addr. no response";
            break;
        case 10:
            str3 = "merker number";
            break;
        case 11:
            str3 = "timer number";
            break;
        case 12:
            str3 = "EW output number";
            break;
        case 13:
            str3 = "unknown address";
            break;
        case 14:
            str3 = "')' failure";
            break;
        case 15:
            str3 = "F button letter";
            break;
        case 16:
            str3 = "line too long";
            break;
        case 17:
            str3 = "unknown operand";
            break;
        case 18:
            str3 = "unknown FxxTT";
            break;
        case 19:
            str3 = "unknown F nbre";
            break;
        case 20:
            str3 = "input number";
            break;
        case 21:
            str3 = "timer number";
            break;
        case 22:
            str3 = "number > 255";
            break;
        case 23: // keine Handynummer
            str3 = "no GSM number";
            break;
        case 24:
            str3 = "IF statement '('";
            break;
        case 25:
            str3 = "text too long";
            break;
        case 26:
            str3 = "number too long";
            break;
        case 27:
            str3 = "merker number";
            break;
        case 28:
            str3 = "IF...ENDIF inc.";
            break;
        case 29:
            str3 = "ELSE without IF";
            break;
        case 30:
            str3 = "ENDIF without IF";
            break;
        case 31:
            str3 = "number bits";
            break;
        case 32:
            str3 = "number stop bits";
            break;
        case 33:
            str3 = "parity false";
            break;
        case 34:
            str3 = "baud rate";
            break;
        case 35:
            str3 = "action incorrect";
            break;
        case 36:
            str3 = "SEing nbr ";
            break;
        case 37:
            str3 = "incorrect addr.";
            break;
        case 38:
            str3 = "MODBUS nof def.";
            break;
        case 39:
            str3 = "Anzahl Zaehler";
            break;
        case 40:
            str3 = "RS485_1 not def.";
            break;
        case 41:
            str3 = "RS485_2 not def.";
            break;
        case 42:
            str3 = "Modbus is def.";
            break;
        case 43: // gsmconf
            str3 = "type not defined";
            break;
        case 44:
            str3 = "no GSM-Nr def.";
            break;
        case 45:
            str3 = "no text defined";
            break;
        case 46:
            str3 = "GSM-Nr not def";
            break;
        case 47:
            str3 = "SMS-Text not def";
            break;
        case 48:
            str3 = "Niv 1 not defined";
            break;
        case 49:
            str3 = "Niv > 5";
            break;
        case 50:
            str3 = "Niv 2 before 1";
            break;
        case 51:
            str3 = "Niv 3 before 2";
            break;
        case 52:
            str3 = "Niv 4 before 3";
            break;
        case 53:
            str3 = "undefined state";
            break;
        case 54:
            str3 = "undef. output";
            break;
        case 55:
            str3 = "undef. merker";
            break;
        case 56:
            str3 = "undef. change";
            break;
        case 57:
            str3 = "undef. SEing";
            break;
        case 58:
            str3 = "unknown ";
            break;
        case 59:
            str3 = "GSM param-syslog";
            break;
        case 60:
            str3 = "year not correct";
            break;
        case 61:
            str3 = "incorrect month";
            break;
        case 62:
            str3 = "incor. separator";
            break;
        case 63:
            str3 = "inc line";
            break;
        case 64:
            str3 = "incorrect number";
            break;
        case 65: // Param Zaehlernummer falsch
            str3 = "unknown counter";
            break;
        case 66:
            str3 = "Sensor number";
            break;
        case 67:
            str3 = "GSM not defined";
            break;
        case 68:
            str3 = "Anz Heizkörper";
            break;
        case 69:
            str3 = "new definition";
            break;
        case 70:
            str3 = "Sensor not def";
            break;
        case 71:
            str3 = "Anz. Heizkörper";
            break;
        case 72: 
            str3 = "'h' keine Uhrz.";
            break;
        case 73:
            str3 = "'-' not def";
            break;
        case 74:
            str3 = "time error";
            break;
        case 75:
            str3 = "Temperatur > 30";
            break;
        case 76:
            str3 = "Anz Blocks zu >";
            break;
        case 77: // Formatfehler in ProWo.heizung
            str3 = "File format!";
            break;
        case 78:
            str3 = "syntax error!";
            break;
        case 79:
            str3 = "unkn. test mode";
            break;
        case 80:
            str3 = "unkn. Mainboard";
            break;
        case 81:
            str3 = "WS not defined";
            break;
        case 82:
            str3 = "wrong WS param";
            break;
        case 83:
            str3 = "wrong TN param";
            break;
        case 84:
            str3 = "'(' failure";
            break;
        case 85:
            str3 = "wrong factor!";
            break;
        case 86:
            str3 = "unknown operator";
            break;
        case 87:
            str3 = "wrong counter";
            break;
        case 88:
            str3 = "HUE lights to >";
            break;
        case 89:
            str3 = "HUE > ANZHUE";
            break;
        case 90:
            str3 = "no HUE-bridge IP";
            break;
        case 91:
            str3 = "no HUE-User!";
            break;
        case 92:
            str3 = "not after ALARM";
            break;
        case 93:
            str3 = "DelayAktiv < 10s";
            break;
        case 94:
            str3 = "DelayAlarm < 10s";
            break;
        case 95:
            str3 = "no Sensor def.!";
            break;
        case 96:
            str3 = "only 1 or 2!";
            break;
        case 97:
            str3 = "Sabotage  first!";
            break;
        case 98:
            str3 = "Sabotage not def";
            break;
        case 99:
            str3 = "ALARM not def.!";
            break;
        case 100:
            str3 = "wrong ALARM par.";
            break;
        case 101:
            str3 = "MUSIK not def.!";
            break;
        case 102:
            str3 = "not a file name";
            break;
        case 103:
            str3 = "1 < volume < 100";
            break;
        case 104:
            str3 = "song don't exist";
            break;
        case 105:
            str3 = "no PIN code";
            break;
        case 106:
            str3 = "Param not def.!";
            break;
        case 107:
            str3 = "wrong param!";
            break;
        case 108:
            str3 = "wrong # type";
            break;
        case 109:
            str3 = "wrong count par.";
            break;
        case 110:
            str3 = "ZAEHLER not def!";
            break;
        case 111:
            str3 = "wrong UHR param.";
            break;
        case 112:
            str3 = "to much output";
            break;
        case 113:
            str3 = "wrong param";
            break;
        case 114:
            str3 = "HUE not defined";
            break;
        case 115:
            str3 = "noth. after Diff";
            break;
        case 116:
            str3 = "wrong sensor no!";
            break;
        case 117:
            str3 = "wrong brightness";
            break;
        case 118:
            str3 = "Key not defined";
            break;
        case 119:
            str3 = "Secret not def.";
            break;
        case 120:
            str3 = "RefreshToken ??";
            break;
        case 121:
            str3 = "Sonos not def.";
            break;
        case 122: 
            str3 = "Sonos houshold?";
            break;
        case 123: 
            str3 = "Sonos no Group!";
            break;
        case 124:
            str3 = "Def after sensor";
            break;
        case 125:
            str3 = "File format err.";
            break;
        case 126:
            str3 = "multi NbrClock  ";
            break;
        case 127:
            str3 = "bad nbre clock! ";
            break;
        case 128:
            str3 = "NbrClock not def";
            break;
        case 129:
            str3 = "Alarmclock undef";
            break;
        case 130:
            str3 = "Alarmclock twice";
            break;
        case 131:
            str3 = "Integer number";
            break;
        case 132:
            str3 = "ProWo.heizung !!";
            break;
        default:
            str3 = "error not def.!";
            break;
        }
        LCD_AnzeigeZeile1(str1.c_str());
        LCD_AnzeigeZeile2(str2.c_str());
        LCD_AnzeigeZeile3(str3.c_str());
        str1 += ", " + str2 + ", " + str3;
        syslog(LOG_ERR, str1.c_str());

        for(;;)
        {	
            delayMicroseconds (2000);

            button = kb.getch();
            if(button == BUTTONUPDOWN || !bRunProgram)
                throw(1);
            if(button == BUTTONOKCANCEL)
                system("reboot");
        }
    }
}

//
// # in der ersten Kolonne bedeutet Kommentarzeile
// 1. Zeichen:
//		Axx : Ausgang mit der Nummer
//		Mxx : Merker mit Nummer
//		Txx,yyy : Timer mit Nummer und Dauer in Sekunden
//		EWXX : Ausgang EasyWave mit Nummer
// gefolgt von =
//	Zeichenkette mit der Logik
//		Exx : Eingang mit der Nummer
//		Mxx : Merker mit der Nummer
//		Txx	: Timer mit der Nummer
//		TN : Test auf Tag oder Nacht je nach Kalendertag
//	Die Lgik erfolgt nach dem Prinzip . vor - (Es können Klammern benutzt werden)
//  Operatoren:
//		& : UND
//		| : ODER
//		! : NICHT (vor einem Operanten
//
void CIOGroup::LesParam(char *pProgramPath)
{
    int ok, nbre, type, handyNummer;
    int anz, i, j, idxBerechne;
    int anzIfElse;
    char cEW;
    char buf[256];
    string str, strEW;
    CConfigCalculator *pcc;
    COperBase **pOperBase;
    CBerechneAusg *pAusg;
    CBerechneSecTimer *pTimer;
    CBerechneEWAusg *pEWAusg;
    CBerechneEWEing *pEWEing;
    CBerechneGSM *pGsm;
    CBerechneBase *pIfElse;
    CBerechneS0Zaehler *pS0Zaehler;
    CBerechneZTZaehler *pZTZaehler;
    CBerechneWrite *pWrite;
    CBerechneHue *pHue;
    CBerechneMusic *pMusic;
    CBerechneAlarm *pAlarm;
    CBerechneSonos *pSonos;   
    CBerechneAC *pAC;
    CBerechneInteger *pInteger;

    bool bS0Zaehler;

    m_pReadFile = new CReadFile;
    m_pReadFile->OpenRead (pProgramPath, 2);
    for(i=0; ; i++)
    {
        if(!m_pReadFile->ReadLine ())
            break;
    }

    m_pReadFile->Close ();
    delete m_pReadFile;
    m_pReadFile = NULL;

    m_iBerechneAnz = i;
    m_pBerechne = new CBerechneBase *[m_iBerechneAnz];

    m_pReadFile = new CReadFile;
    m_pReadFile->OpenRead (pProgramPath, 2);

    anzIfElse = 0;

    for(idxBerechne=0; ; idxBerechne++)
    {
        if(m_pReadFile->ReadLine ())
        {
            bS0Zaehler = false;
            str = m_pReadFile->ReadAlpha ();
            nbre = m_pReadFile->ReadNumber();
            if(str.compare("S") == 0 && nbre == 0) // S0 bei S0ZAEHLER
            {
                str = m_pReadFile->ReadAlpha ();
                nbre = m_pReadFile->ReadNumber(); 
                bS0Zaehler = true;
            }

            if(str.compare("SONOS") == 0)
            {
                if(m_pSonos == NULL)
                    m_pReadFile->Error(121);
                if(nbre < 1 || nbre > m_pSonos->GetAnzGroupConfig())
                    m_pReadFile->Error(123);
                type = 14;
            }
            else if(str.compare("AC") == 0)
            {
                if(m_pAlarmClock == NULL)
                    m_pReadFile->Error(129);
                type = 15;
            }
            else if(str.compare("ALARM") == 0)
            {
                if(m_pAlarm == NULL)
                    m_pReadFile->Error(99);
                type = 13;
            }
            else if(str.compare("MUSIK") == 0)
            {
                if(m_pMusic == NULL)
                    m_pReadFile->Error(101);
                type = 12;
            }
            else if(str.compare("H") == 0)
            {
                if(m_pHue == NULL || nbre < 1 || nbre > m_pHue->GetAnz())
                    m_pReadFile->Error(5);
                type = 11;   
            }
            else if(str.compare("ZTZAEHLER") == 0)
            {
                if(nbre < 1 || nbre > m_iAnzZaehler)
                    m_pReadFile->Error(64);
                type = 10;
            }
            else if(str.compare("write") == 0)
                type = 9;
            else if(str.compare("ZAEHLER") == 0 && bS0Zaehler)
            {
                if(nbre < 1 || nbre > m_iAnzZaehler)
                    m_pReadFile->Error(64);
                type = 8;
            }
            else if(str.compare("GSM") == 0)
            {
                // nbr = Nummer des Textes
                // delay = Nummer der Handynummer
                if(m_pGsm == NULL)
                    m_pReadFile->Error(66);
                type = 7;
            }
            else if(str.compare("S") == 0)
            {	// SEingänge
                if(nbre < 1 || nbre > GetSEingAnz() )
                    m_pReadFile->Error(36);
                else
                    type = 6; 
            }
            else if(str.compare("A") == 0)
            {	// Ausgang
                if(nbre < 1 || nbre > GetAusgAnz() )
                    m_pReadFile->Error(5);
                else
                    type = 1; 
            }
            else if(str.compare("M") == 0)
            {	// Merker
                if(nbre < 1 || nbre > GetMerkAnz ())
                    m_pReadFile->Error(10);
                else
                    type = 2;
            }
            else if(str.compare("I") == 0)
            {   // Integer Merker
                if(nbre < 1 || nbre > GetIntegerAnz())
                    m_pReadFile->Error(131);
                else
                    type =16;
            }
            else if(str.compare("T") == 0)
            {	// Sekunden Timer
                if(nbre < 1 || nbre > GetSecTimerAnz())
                    m_pReadFile->Error(11);
                else
                    type = 3;
            }
            else if(str.compare("FA") == 0 || str.compare("FE") == 0)
            {	// EasyWave Ausgänge oder Eingänge
                if(nbre < 1 || nbre > GetEWAnz()) // Ein- und Ausgang identisch
                    m_pReadFile->Error(12);
                else
                {
                    if(str.compare("FA") == 0) 
                        type = 4; // Ausgänge
                    else
                        type = 5; // Eingänge
                    // EW ouput noch A oder C lesen
                    strEW = m_pReadFile->ReadAlpha ();
                    if(strEW.length() == 1)
                    {	cEW = strEW.at(0);
                        switch(cEW) {
                        case 'A':
                            cEW = 0x01;
                            break;
                        case 'B':
                            cEW = 0x02;
                            break;
                        case 'C':
                            cEW = 0x04;
                            break;
                        case 'D':
                            cEW = 0x08;
                            break;
                        case 'E':
                            cEW = 0x10;
                            break;
                        case 'F':
                            cEW = 0x20;
                            break;
                        default:
                            m_pReadFile->Error (15);
                            break;
                        }
                    }
                    else
                        m_pReadFile->Error (15);
                }
            }
            else if(str.compare("IF") == 0)
            {
                type = 100;
                anzIfElse++;
            }
            else if(str.compare("ELSE") == 0)
            {
                type = 101;
                if(anzIfElse <= 0)
                    m_pReadFile->Error (29);
            }
            else if(str.compare("ENDIF") == 0)
            {
                type = 102;
                if(anzIfElse <= 0)
                        m_pReadFile->Error (30);
                anzIfElse--;
            }
            else
                m_pReadFile->Error (6);

            // = Zeichen lesen oder ( bei IF
            switch(type) {
            case 1: // Ausgänge
            case 2: // Merker
            case 3: // Timer
            case 4: // EW(F) Ausgänge
            case 5: // EW(F) Eingänge
            case 6: // SEingänge
            case 7: // GSM
            case 8: // S0Zaehler
            case 10: // Zeitzähler
            case 11: // Hue Element (Lampe oder Gruppe)
            case 12: // Music
            case 13: // Alarm
            case 14: // Sonos
            case 15: // Wecker AlarmClock
            case 16: // Integer Merker
                if(!m_pReadFile->ReadEqual())
                    m_pReadFile->Error(4);
                break;
            case 9: // write
                if(m_pReadFile->GetChar() != '(')
                    m_pReadFile->Error(84);
                break;
            case 100: // IF
                if(m_pReadFile->ReadBuf (buf, '(') != 0)
                    m_pReadFile->Error(24);
                break;
            default:
                break;
            }
            //
            // Operanten und Operatoren lesen
            //
            pcc = new CConfigCalculator(m_pReadFile);
            switch(type) {
            case 1: // Ausgänge
            case 2: // Merker
            case 3: // Timer
            case 4: // EW (F) Ausgänge
            case 5: // EW (F) Eingänge
            case 6: // SEingänge
            case 8: // S0Zaehler
            case 10: // Zeitzähler
            case 11: // HUE Element (Lampe oder Gruppe)
            case 13: // Alarm
            case 14: // Sonos
            case 16: // Integer Merker
            case 100: // IF
                m_pReadFile->ReadBuf (buf, ' ');
                pcc->eval(buf, this, type);
                anz = pcc->GetAnzOper ();
                break;
            case 7: // GSM Nr-Telefon, Nr des Textes der SMS
                nbre = m_pReadFile->ReadZahl();
                if(nbre < 1 || nbre > m_pGsm->GetAnzNummern())
                    m_pReadFile->Error(101);
                i = 0;
                if(m_pReadFile->ReadBuf(buf, ',') != -1)
                    i = m_pReadFile->ReadZahl();
                if(i < 1 || i > m_pGsm->GetAnzTexte())
                    m_pReadFile->Error(47);                
                nbre = nbre * 1000 + i;
                str = to_string(nbre);
                pcc->eval(str.c_str(), this, type);
                anz = pcc->GetAnzOper();
                break;
            case 9: // write
                m_pReadFile->ReadBuf(buf, ',');
                str = buf;
                if(m_pReadFile->ReadBuf(buf, ')') == -1)
                    m_pReadFile->Error(78);
                pcc->eval(buf, this, type);
                anz = pcc->GetAnzOper();
                break;
            case 12: // Musik
                nbre = m_pReadFile->ReadZahl();
                if(nbre < 0 || nbre > m_pMusic->GetAnzahl())
                    m_pReadFile->Error(101);
                i = 0;
                if(m_pReadFile->ReadBuf(buf, ',') != -1)
                {
                    i = m_pReadFile->ReadZahl();
                    if(i)
                        i = 1;
                }
                nbre = i * 1000 + nbre;
                str = to_string(nbre);
                pcc->eval(str.c_str(), this, type);
                anz = pcc->GetAnzOper();
                break;
            case 15: // AC Wecker AlarmClock
                nbre = m_pReadFile->ReadZahl();
                if(nbre < 0 || nbre > 1)
                    m_pReadFile->Error(53);
                str = to_string(nbre);
                pcc->eval(str.c_str(), this, type);
                anz = pcc->GetAnzOper();
                break;            
            default:
                break;
            }

            switch(type) {
            case 1: // Ausgänge
                pOperBase = new COperBase *[anz];
                pAusg = new CBerechneAusg;
                pAusg->setOper(pOperBase);
                pAusg->init(nbre, GetAusgAddress (nbre));
                m_pBerechne[idxBerechne] = pAusg;
                for(i=0; i < anz; i++)
                    pOperBase[i] = pcc->GetOper(i);
                break;
            case 2: // Merker
                pOperBase = new COperBase *[anz];
                pAusg = new CBerechneAusg;
                pAusg->setOper(pOperBase);
                pAusg->init(nbre, GetMerkAddress (nbre));
                m_pBerechne[idxBerechne] = pAusg;
                for(i=0; i < anz; i++)
                    pOperBase[i] = pcc->GetOper(i);
                break;
            case 3: // Sekunden Timer
                pOperBase = new COperBase *[anz];
                pTimer = new CBerechneSecTimer;
                pTimer->setOper(pOperBase);
                pTimer->init(nbre, GetSecTimerAddress (nbre));
                m_pBerechne[idxBerechne] = pTimer;
                for(i=0; i < anz; i++)
                    pOperBase[i] = pcc->GetOper(i);
                break;
            case 4: // EW Ausgänge
                pOperBase = new COperBase *[anz];
                pEWAusg = new CBerechneEWAusg;
                pEWAusg->setOper(pOperBase);
                pEWAusg->init(nbre, GetEWAusgAddress (nbre), cEW, &m_EWAusgFifo);
                m_pBerechne[idxBerechne] = pEWAusg;
                for(i=0; i < anz; i++)
                    pOperBase[i] = pcc->GetOper(i);
                break;
            case 5: // EW Eingänge
                pOperBase = new COperBase *[anz];
                pEWEing = new CBerechneEWEing;
                pEWEing->setOper(pOperBase);
                pEWEing->init(nbre, GetEWEingAddress (nbre), cEW);
                m_pBerechne[idxBerechne] = pEWEing;
                for(i=0; i < anz; i++)
                    pOperBase[i] = pcc->GetOper(i);
                break;
            case 6: // SEingänge
                pOperBase = new COperBase *[anz];
                pAusg = new CBerechneAusg;
                pAusg->setOper(pOperBase);
                pAusg->init(nbre, GetSEingAddress (nbre));
                m_pBerechne[idxBerechne] = pAusg;
                for(i=0; i < anz; i++)
                    pOperBase[i] = pcc->GetOper(i);
                break;
            case 7: // GSM
                pOperBase = new COperBase *[anz];
                pGsm = new CBerechneGSM;
                pGsm->setOper(pOperBase);
                pGsm->init(m_pGsm);
                m_pBerechne[idxBerechne] = pGsm;
                for(i=0; i < anz; i++)
                    pOperBase[i] = pcc->GetOper(i);
                break;
            case 8: // S0Zaehler
                if(m_pZaehler[nbre-1]->IsTyp(2))
                {
                    pOperBase = new COperBase *[anz];
                    pS0Zaehler = new CBerechneS0Zaehler;
                    pS0Zaehler->setOper(pOperBase);
                    pS0Zaehler->init(m_pZaehler[nbre-1]);
                    m_pBerechne[idxBerechne] = pS0Zaehler;
                    for(i=0; i < anz; i++)
                        pOperBase[i] = pcc->GetOper(i);
                }
                else
                    m_pReadFile->Error(87);
                break;
            case 9: //write
                pOperBase = new COperBase *[anz];
                pWrite = new CBerechneWrite;
                pWrite->init(str);
                pWrite->setOper(pOperBase);
                m_pBerechne[idxBerechne] = pWrite;
                for(i=0; i < anz; i++)
                    pOperBase[i] = pcc->GetOper(i);
                break;
            case 10: // Zeitzähler
                if(m_pZaehler[nbre-1]->IsTyp(3))
                {
                    pOperBase = new COperBase *[anz];
                    pZTZaehler = new CBerechneZTZaehler;
                    pZTZaehler->setOper(pOperBase);
                    pZTZaehler->init(m_pZaehler[nbre-1]);
                    m_pBerechne[idxBerechne] = pZTZaehler;
                    for(i=0; i < anz; i++)
                        pOperBase[i] = pcc->GetOper(i);
                }
                else
                    m_pReadFile->Error(87);
                break;
            case 11: // HUE Element (Lampe oder Gruppe)
                pOperBase = new COperBase *[anz];
                pHue = new CBerechneHue;
                pHue->init(m_pHue->GetAddress(nbre));
                pHue->setOper(pOperBase);
                m_pBerechne[idxBerechne] = pHue;
                for(i=0; i < anz; i++)
                    pOperBase[i] = pcc->GetOper(i);
                break;
            case 12: // Music
                pOperBase = new COperBase *[anz];
                pMusic = new CBerechneMusic;
                pMusic->setOper(pOperBase);
                pMusic->init(m_pMusic);
                m_pBerechne[idxBerechne] = pMusic;
                for(i=0; i < anz; i++)
                    pOperBase[i] = pcc->GetOper(i);
                break;
            case 13: // Alarm
                pOperBase = new COperBase *[anz];
                pAlarm = new CBerechneAlarm;
                pAlarm->setOper(pOperBase);
                // m_pHistory wird übergeben um das Lichtprogramm zu beenden
                pAlarm->init(m_pAlarm, m_pHistory);
                m_pBerechne[idxBerechne] = pAlarm;
                for(i=0; i < anz; i++)
                    pOperBase[i] = pcc->GetOper(i);                
                break;
            case 14: // Sonos
                pOperBase = new COperBase *[anz];
                pSonos = new CBerechneSonos;  
                pSonos->setOper(pOperBase);
                pSonos->init(nbre, m_pSonos);
                m_pBerechne[idxBerechne] = pSonos;
                for(i=0; i < anz; i++)
                    pOperBase[i] = pcc->GetOper(i);                 
                break;
            case 15: // AC AlarmClock 
                pOperBase = new COperBase *[anz];
                pAC = new CBerechneAC;
                pAC->setOper(pOperBase);
                pAC->init(m_pAlarmClock);
                m_pBerechne[idxBerechne] = pAC;                
                for(i=0; i < anz; i++)
                    pOperBase[i] = pcc->GetOper(i);                 
                break;
            case 16: // Integer Merker
                pOperBase = new COperBase *[anz];
                pInteger = new CBerechneInteger;
                pInteger->init(GetIntegerAddress (nbre));
                pInteger->setOper(pOperBase);
                m_pBerechne[idxBerechne] = pInteger;                
                for(i=0; i < anz; i++)
                    pOperBase[i] = pcc->GetOper(i);                 
                break;
            case 100: // IF
                pOperBase = new COperBase *[anz];
                pIfElse = new CBerechneBase;
                pIfElse->setIfElse (1);
                pIfElse->setOper(pOperBase);
                m_pBerechne[idxBerechne] = pIfElse;
                for(i=0; i < anz; i++)
                    pOperBase[i] = pcc->GetOper(i);
                break;				
            case 101: // ELSE
                pIfElse = new CBerechneBase;
                pIfElse->setIfElse(2);
                m_pBerechne[idxBerechne] = pIfElse;
                break;
            case 102: // ENDIF
                pIfElse = new CBerechneBase;
                pIfElse->setIfElse(3);
                m_pBerechne[idxBerechne] = pIfElse;
                break;
            default:
                m_pReadFile->Error(21);
                break;
            }

            delete pcc;
        }
        else
            break;
    }
    if(anzIfElse)
        m_pReadFile->Error(28);

    m_pReadFile->Close();
	delete m_pReadFile;
    m_pReadFile = NULL;
}
void CIOGroup::PreReadConfig(char *pProgramPath)
{
    char buf[256];
    
    m_pReadFile = new CReadFile;
    m_pReadFile->OpenRead (pProgramPath, 1);  
    for(;;)
    {	
        if(m_pReadFile->ReadLine())
        {
            m_pReadFile->ReadBuf(buf, ':');
            if(strncmp(buf, "MainboardV1.0", 13) == 0) 
                ext_iI2CSwitchType = 1;                
            else if(strncmp(buf, "MainboardV2.0", 13) == 0)
                ext_iI2CSwitchType = 2;
            else if(strncmp(buf, "WS10", 4) == 0)
                m_iMaxAnzModBusClient++;
            else if(strncmp(buf,"ABBZAEHLER", 10) == 0)
            {   m_iMaxAnzModBusClient++;
                m_iMaxAnzZaehler++;
            }
            else if(strncmp(buf, "S0ZAEHLER", 9) == 0
                    | strncmp(buf, "SUMZAEHLER", 10) == 0
                    | strncmp(buf, "DIFFZAEHLER", 11) == 0
                    | strncmp(buf, "ZTZAEHLER", 9) == 0)
                m_iMaxAnzZaehler++;
            else if(strncmp(buf, "TQS3", 4) == 0 
                    | strncmp(buf, "TQS3CHANGE", 10) == 0
                    | strncmp(buf, "TQS3SQEARCH", 11) == 0)
            {   m_iMaxAnzModBusClient++;
                m_iMaxAnzSensor++;
            }
            else if(strncmp(buf, "TH1", 3) == 0
                    | strncmp(buf, "TH1CHANGE", 9) == 0
                    | strncmp(buf, "TH1SEARCH", 9) == 0)
            {
                m_iMaxAnzModBusClient++;
                m_iMaxAnzSensor++;
            }
            else if(strncmp(buf, "HUELIGHT", 8) == 0
                    | strncmp(buf, "HUEGROUP", 8) == 0)
                m_iMaxAnzHueEntity++;
        }
        else
            break;
    }
    if(!ext_iI2CSwitchType)
        m_pReadFile->Error(80);
    m_pReadFile->Close ();
    delete m_pReadFile;
    m_pReadFile = NULL;
    if(m_iMaxAnzSensor)
        m_pSensor = new CSensor *[m_iMaxAnzSensor];
    if(m_iMaxAnzZaehler)
        m_pZaehler = new CZaehler *[m_iMaxAnzZaehler];
}
                       
void CIOGroup::ReadConfig(char *pProgramPath)
{
    char buf[256];
    int boardtype, pos, offset, address, i;
    int Inh1, Addr2, Inh2, Addr3, Reg, iAddr3;
    string str;
    int AnzAus=0, AnzEin=0, AnzEW=0; 
    int error = 0;
    int iTH1Led = 0, iTH1I2CTakt = 0;
    
    m_MerkAnz = 8;
    m_SecTimerAnz = 8;

    m_pReadFile = new CReadFile;
    m_pReadFile->OpenRead (pProgramPath, 1);

    for(;;)
    {	
        if(m_pReadFile->ReadLine())
        {
            m_pReadFile->ReadBuf(buf, ':');
            if(strncmp(buf, "Integer", 7) == 0) 
            {
                pos = m_pReadFile->ReadNumber ();
                if(pos < 0)
                    m_pReadFile->Error(131); 
                else
                    m_IntegerAnz = pos;               
            }
            else if(strncmp(buf, "HUEUSER", 6) == 0) 
            {
                if(m_pHue == NULL)
                    m_pHue = new CHue((char *)this);
                m_pHue->SetUser(m_pReadFile->ReadText());
            }
            else if(strncmp(buf, "HUEIP", 5) == 0)  
            {
                if(m_pHue == NULL)
                    m_pHue = new CHue((char *)this);
                m_pHue->SetIP(m_pReadFile->ReadText());
            }
            else if(strncmp(buf, "HUEGROUP", 8) == 0)
            {
                if(m_pHue == NULL)
                    error = 92;
                else
                    error = m_pHue->IsDefined();
                if(error)
                    m_pReadFile->Error(error);

                address = m_pReadFile->ReadNumber();
                error = m_pHue->SetEntity(2, address);
                if(error)
                    m_pReadFile->Error(error);
            }            
            else if(strncmp(buf, "HUELIGHT", 8) == 0)
            {
                if(m_pHue == NULL)
                    error = 92;
                else
                    error = m_pHue->IsDefined();
                if(error)
                    m_pReadFile->Error(error);

                address = m_pReadFile->ReadNumber();
                error = m_pHue->SetEntity(1, address);
                if(error)
                    m_pReadFile->Error(error);
            }
            else if(strncmp(buf, "WS10", 4) == 0)
            {
                address = m_pReadFile->ReadNumber ();
                if(m_pModBus != NULL)
                {
                    if(address > 0 && address < 256)
                    {
                        CModBusClient *pClient = m_pModBus->AppendModBus(address, 500000);
                        m_pWStation = new CWStation(address);
                        m_pWStation->SetModBusClient(pClient);
                    }
                    else
                        m_pReadFile->Error(37);
                }
                else
                    m_pReadFile->Error(38);
            }
            else if(strncmp(buf, "TEST", 4) == 0)
            {
                pos = m_pReadFile->ReadNumber ();
                switch (pos) {
                case 1:  // Sensoren
                case 2:  // Zähler
                case 3:  // Wetterstation
                case 4:  // Sonosparameter schreiben
                case 5: // Alle Ausgänge nacheinander ansteuern
                case 10: // I2C
                case 11: // wird ein EingAusg-Modul nicht gefunden, wird diese
                         // kontinuierlich angesteuert (ohne spezielle Anzeige)
                    m_iTest = pos;
                    break;
                default:
                    m_pReadFile->Error(79);
                    break;
                }
            }
            else if(strncmp(buf, "TQS3SEARCH", 10) == 0)
            {
                if(m_iAnzSensor < m_iMaxAnzSensor)
                {
                    if(m_pModBus != NULL)
                    {
                        address = m_pReadFile->ReadNumber ();
                        if(address > 0 && address < 256)
                        {
                            CModBusClient *pClient = m_pModBus->AppendModBus(address, 50000);
                            CTQS3 *pPtr;
                            pPtr = new CTQS3(m_iAnzSensor+1);
                            pPtr->SetModBusClient(pClient);
                            pPtr->SetFunction(2);
                            m_pSensor[m_iAnzSensor] = pPtr;
                            m_iChangeSensorModBusAddress = m_iAnzSensor + 1;
                            m_iAnzSensor++;
                        }
                        else
                            m_pReadFile->Error(37);
                    }    
                    else
                        m_pReadFile->Error(38);                        
                }
                else
                    m_pReadFile->Error(64);

            }
            else if(strncmp(buf, "TQS3CHANGE", 10) == 0)
            {
                if(m_iAnzSensor  < m_iMaxAnzSensor)
                {
                    address = m_pReadFile->ReadNumber ();
                    m_pReadFile->ReadBuf(buf, ',');
                    int newAddress = m_pReadFile->ReadNumber();
                    if(m_pModBus != NULL)
                    {
                        if(address > 0 && address < 256 && newAddress > 0 && newAddress < 256)
                        {
                            CModBusClient *pClient = m_pModBus->AppendModBus(address, 50000);
                            pClient->SetNewAddress(newAddress);
                            CTQS3 *pPtr;
                            pPtr = new CTQS3(m_iAnzSensor+1);
                            pPtr->SetModBusClient(pClient);
                            pPtr->SetFunction(3);
                            m_pSensor[m_iAnzSensor] = pPtr;
                            m_iChangeSensorModBusAddress = m_iAnzSensor + 1;
                            m_iAnzSensor++;
                        }
                        else
                            m_pReadFile->Error(37);
                    }
                    else
                        m_pReadFile->Error(38);
                }
                else
                    m_pReadFile->Error(64);
            }
            else if(strncmp(buf, "TQS3", 4) == 0)
            {
                if(m_iAnzSensor < m_iMaxAnzSensor)
                {
                    address = m_pReadFile->ReadNumber ();
                    if(m_pModBus != NULL)
                    {
                        if(address > 0 && address < 256)
                        {
                            CModBusClient *pClient = m_pModBus->AppendModBus(address, 50000);
                            CTQS3 *pPtr;
                            pPtr = new CTQS3(m_iAnzSensor+1);
                            pPtr->SetModBusClient(pClient);
                            pPtr->SetFunction(1);
                            m_pReadFile->ReadSeparator();
                            str = m_pReadFile->ReadText();
                            if(str == "")
                                str = "Kein Name";
                            pPtr->SetName(str);                            
                            m_pSensor[m_iAnzSensor] = pPtr;
                            m_iAnzSensor++;
                        }
                        else
                            m_pReadFile->Error(37);
                    }
                    else
                        m_pReadFile->Error(38);
                }
                else
                    m_pReadFile->Error(64);
            }
            else if(strncmp(buf, "TH1SEARCH", 10) == 0)
            {
                if(m_iAnzSensor < m_iMaxAnzSensor)
                {
                    if(m_pModBus != NULL)
                    {
                        address = m_pReadFile->ReadNumber ();
                        if(address > 0 && address < 256)
                        {
                            CModBusClient *pClient = m_pModBus->AppendModBus(address, 50000);
                            CTH1 *pPtr;
                            pPtr = new CTH1(m_iAnzSensor+1);
                            pPtr->SetModBusClient(pClient);
                            pPtr->SetFunction(2, iTH1Led, iTH1I2CTakt);
                            m_pSensor[m_iAnzSensor] = pPtr;
                            m_iChangeSensorModBusAddress = m_iAnzSensor + 1;
                            m_iAnzSensor++;
                        }
                        else
                            m_pReadFile->Error(37);
                    }    
                    else
                        m_pReadFile->Error(38);                        
                }
                else
                    m_pReadFile->Error(64);

            }
            else if(strncmp(buf, "TH1CHANGE", 10) == 0)
            {
                if(m_iAnzSensor < m_iMaxAnzSensor)
                {
                    address = m_pReadFile->ReadNumber ();
                    m_pReadFile->ReadBuf(buf, ',');
                    int newAddress = m_pReadFile->ReadNumber();
                    if(m_pModBus != NULL)
                    {
                        if(address > 0 && address < 256 && newAddress > 0 && newAddress < 256)
                        {
                            CModBusClient *pClient = m_pModBus->AppendModBus(address, 50000);
                            pClient->SetNewAddress(newAddress);
                            CTH1 *pPtr;
                            pPtr = new CTH1(m_iAnzSensor+1);
                            pPtr->SetModBusClient(pClient);
                            pPtr->SetFunction(3, 0, 0);
                            m_pSensor[m_iAnzSensor] = pPtr;
                            m_iChangeSensorModBusAddress = m_iAnzSensor + 1;
                            m_iAnzSensor++;
                        }
                        else
                            m_pReadFile->Error(37);
                    }
                    else
                        m_pReadFile->Error(38);
                }
                else
                    m_pReadFile->Error(64);
            }  
            else if(strncmp(buf, "TH1LEDOFF", 6) == 0)
            {
                if(m_iAnzSensor)
                    m_pReadFile->Error(124); // define bef. Sensor
                iTH1Led = 1;
            }
            else if(strncmp(buf, "TH1TEST", 7) == 0)
            {
                if(m_iAnzSensor)
                    m_pReadFile->Error(124); // define bef. Sensor
                iTH1I2CTakt = 1;
            }            
            else if(strncmp(buf, "TH1", 3) == 0)
            {
                if(m_iAnzSensor < m_iMaxAnzSensor)
                {
                    address = m_pReadFile->ReadNumber ();
                    if(m_pModBus != NULL)
                    {
                        if(address > 0 && address < 256)
                        {
                            CModBusClient *pClient = m_pModBus->AppendModBus(address, 50000);
                            CTH1 *pPtr;
                            pPtr = new CTH1(m_iAnzSensor+1);
                            pPtr->SetModBusClient(pClient);
                            pPtr->SetFunction(1, iTH1Led, iTH1I2CTakt);
                            m_pReadFile->ReadSeparator();
                            str = m_pReadFile->ReadText(';');
                            if(str == "")
                                str = "Kein Name";
                            pPtr->SetName(str);
                            if(m_pReadFile->ReadSeparator())
                            {   address = m_pReadFile->ReadZahl(); 
                                if(address)
                                    pPtr->SetKorrTemp(address);
                            }
                            m_pSensor[m_iAnzSensor] = pPtr;
                            m_iAnzSensor++;
                        }
                        else
                            m_pReadFile->Error(37);
                    }
                    else
                        m_pReadFile->Error(38);
                }
                else
                    m_pReadFile->Error(64);
            } 
            else if(strncmp(buf, "SUMZAEHLER", 10) == 0
               || strncmp(buf, "DIFFZAEHLER", 11) == 0)
            {
                int type = 2;
                if(strncmp(buf, "SUMZAEHLER", 10) == 0)
                    type = 1;

                if(m_iAnzZaehler < m_iMaxAnzZaehler)
                {
                    int z1 = m_pReadFile->ReadNumber ();
                    if(z1 < 1 || z1 > m_iAnzZaehler)
                            m_pReadFile->Error(64);
                    m_pReadFile->ReadBuf(buf, ',');
                    int z2 = m_pReadFile->ReadNumber ();
                    if(z2 < 1 || z2 > m_iAnzZaehler)
                            m_pReadFile->Error(64);
                    if(z1 == z2)
                            m_pReadFile->Error(64);
                    int z3 = readAnzeige();
                    if(z3 < 0)
                        m_pReadFile->Error(85);
                    CZaehlerVirt *pPtr;
                    pPtr = new CZaehlerVirt(m_iAnzZaehler+1);
                    m_pZaehler[m_iAnzZaehler] = pPtr;
                    m_iAnzZaehler++;
                    pPtr->init(m_pZaehler[z1-1], m_pZaehler[z2-1], type);
                    pPtr->SetAnzeigeArt(z3);
                }
                else
                    m_pReadFile->Error(37);
            }
            else if(strncmp(buf, "S0ZAEHLER", 9) == 0)
            {
                if(m_iAnzZaehler < m_iMaxAnzZaehler)
                {
                    int z1 = m_pReadFile->ReadNumber ();
                    int z2 = readAnzeige();
                    if(z1 < 1 || z2 < 0)
                        m_pReadFile->Error(85);
                    CZaehlerS0 *pPtr;
                    pPtr = new CZaehlerS0(m_iAnzZaehler+1);
                    m_pZaehler[m_iAnzZaehler] = pPtr;
                    m_iAnzZaehler++;
                    pPtr->SetFactor(z1);
                    pPtr->SetAnzeigeArt(z2);
                }
                else
                    m_pReadFile->Error(37);
            }
            else if(strncmp(buf, "ZTZAEHLER", 9) == 0)
            {
                if(m_iAnzZaehler < m_iMaxAnzZaehler)
                {
                    int z1 = m_pReadFile->ReadNumber ();
                    int z2 = readAnzeige();
                    if(z1 < 1 || z2 < 0)
                        m_pReadFile->Error(85);
                    CZaehlerZT *pPtr;
                    pPtr = new CZaehlerZT(m_iAnzZaehler+1);
                    m_pZaehler[m_iAnzZaehler] = pPtr;
                    m_iAnzZaehler++;
                    pPtr->SetFactor(z1);  
                    pPtr->SetAnzeigeArt(z2);
                }
                else
                    m_pReadFile->Error(37);
            }            
            else if(strncmp(buf, "ABBZAEHLER", 10) == 0)
            {
                if(m_iAnzZaehler < m_iMaxAnzZaehler)
                {
                    address = m_pReadFile->ReadNumber ();
                    int z2 = readAnzeige();
                    if(z2 < 0)
                        m_pReadFile->Error(85);
                    if(m_pModBus != NULL)
                    {
                        if(address > 0 && address < 256)
                        {
                            CModBusClient *pClient = m_pModBus->AppendModBus(address, 50000);
                            CZaehlerABB *pPtr;
                            pPtr = new CZaehlerABB(m_iAnzZaehler+1);
                            pPtr->SetModBusClient(pClient);
                            pPtr->SetFactor(100);
                            pPtr->SetAnzeigeArt(z2);
                            m_pZaehler[m_iAnzZaehler] = pPtr;
                            m_iAnzZaehler++;
                            
                        }
                        else
                            m_pReadFile->Error(37);
                    }
                    else
                        m_pReadFile->Error(38);
                }
                else
                    m_pReadFile->Error(66);

            }
            else if(strncmp(buf, "MODBUS", 6) == 0)
            {
                address = m_pReadFile->ReadNumber ();
                if(m_pModBus == NULL)
                {
                    switch(address) {
                    case 1:
                        if(m_pRS485_1 != NULL)
                        {
                            m_pModBus = new CModBus(m_iMaxAnzModBusClient);
                            m_pModBus->SetCRS485(m_pRS485_1);
                        }
                        else
                            m_pReadFile->Error(41);
                        break;
                    case 2:
                        if(m_pRS485_2 != NULL)
                        {
                            m_pModBus = new CModBus(m_iMaxAnzModBusClient);
                            m_pModBus->SetCRS485(m_pRS485_2);
                        }
                        else
                            m_pReadFile->Error(41);
                        break;                                
                    default:
                        m_pReadFile->Error(41);
                        break;
                    }
                }
                else
                    m_pReadFile->Error(42);
            }
            else if(strncmp(buf, "RS485", 5) == 0) 
            {	int nr;
                int baudrate, bits, stops, zykluszeit;
                char parity;
                if(strncmp(buf, "RS485_1", 7) == 0)
                    nr = 1;
                else if(strncmp(buf, "RS485_2", 7) == 0)
                    nr = 2;
                else
                    m_pReadFile->Error(3);
                baudrate = m_pReadFile->ReadNumber ();
                m_pReadFile->ReadBuf(buf, ',');
                bits = m_pReadFile->ReadNumber ();
                m_pReadFile->ReadBuf(buf, ',');
                m_pReadFile->ReadBuf(buf, ',');
                parity = toupper(buf[0]);
                stops = m_pReadFile->ReadNumber ();

                if(nr == 1)
                {
                    m_pRS485_1 = new CRS485spi;
                    nr = m_pRS485_1->Init(baudrate, bits, stops, parity, m_iZykluszeit);
                }
                else
                {
                    m_pRS485_2 = new CRS485ob;
                    nr = m_pRS485_2->Init(baudrate, bits, stops, parity, m_iZykluszeit);
                }
                if(nr)
                    m_pReadFile->Error(30+nr);
            }
            else if(strncmp(buf, "MainboardV1.0", 13) == 0) 
            {
                m_AusgAnz += PORTANZAHLCHANNEL;
                if(m_AusgAnz > MAXAUSGCHANNEL)
                    m_pReadFile->Error(112);
                m_EingAnz += PORTANZAHLCHANNEL;

                str = m_pReadFile->ReadAlpha ();
                if(str.compare("EW") == 0)
                    m_EWAnzBoard += 1;
            }
            else if(strncmp(buf, "MainboardV2.0", 13) == 0)
            {
                m_AusgAnz += PORTANZAHLCHANNEL;
                if(m_AusgAnz > MAXAUSGCHANNEL)
                    m_pReadFile->Error(112);
                m_EingAnz += PORTANZAHLCHANNEL;

                str = m_pReadFile->ReadAlpha ();
                if(str.compare("EW") == 0)
                    m_EWAnzBoard += 1;
            }                
            else if(strncmp(buf, "Merker", 6) == 0)
            {
                pos = m_pReadFile->ReadNumber ();
                if(pos > 0)
                    // wird auf 8 gerundet
                    m_MerkAnz = (pos / PORTANZAHLCHANNEL) * PORTANZAHLCHANNEL;
                else
                    m_pReadFile->Error(10);
            }
            else if (strncmp(buf, "Timer", 5) == 0)
            {
                pos = m_pReadFile->ReadNumber ();
                if(pos > 0)
                    m_SecTimerAnz = pos;
                else
                    m_pReadFile->Error(11);
            }
            else if(strncmp(buf, "EingAusgV1.0", 12) ==0)
            {
                m_EingAnz += PORTANZAHLCHANNEL;
                m_AusgAnz += PORTANZAHLCHANNEL;
                if(m_AusgAnz > MAXAUSGCHANNEL)
                    m_pReadFile->Error(112);

                pos = m_pReadFile->ReadBuf (buf, ':');
                str = m_pReadFile->ReadAlpha ();
                if(str.compare("EW") == 0)
                    m_EWAnzBoard += 1;

            }
            else if(strncmp(buf, "EingAusgV2.0", 12) ==0)
            {
                m_EingAnz += PORTANZAHLCHANNEL;
                m_AusgAnz += PORTANZAHLCHANNEL;
                if(m_AusgAnz > MAXAUSGCHANNEL)
                    m_pReadFile->Error(112);

                pos = m_pReadFile->ReadBuf (buf, ':');
                while(pos > 0) {
                    pos = m_pReadFile->ReadBuf (buf, ':');
                    if(pos < 0)
                        break;
                    if(strncmp(buf, "EW", 2) == 0)
                        m_EWAnzBoard += 1;
                    else if(strncmp(buf, "EingExpV2.0", 11) == 0)
                        m_EingAnz += PORTANZAHLCHANNEL;
                    else if(strncmp(buf, "AusgExpV2.0", 11) == 0)
                    {
                        m_AusgAnz += PORTANZAHLCHANNEL;
                        if(m_AusgAnz > MAXAUSGCHANNEL)
                            m_pReadFile->Error(112);
                    }
                    else
                        m_pReadFile->Error(7);
                }
            }
            else if(strncmp(buf, "GSMV2.0", 7) == 0)
            {
                m_UartAnz += 1;	
            }
            else if (strncmp(buf, "S", 1) == 0)
            {
                pos = m_pReadFile->ReadNumber ();
                if(pos > 0)
                        m_SEingAnz = pos;
                else
                        m_pReadFile->Error(11);
            }
            else
                m_pReadFile->Error(7);
        }
        else 
            break;
    }

    // Ausgänge
    m_pcAusg = new char[m_AusgAnz / PORTANZAHLCHANNEL];
    m_pcAusgLast = new char[m_AusgAnz / PORTANZAHLCHANNEL];
    m_pcAusgHistory = new char[m_AusgAnz / PORTANZAHLCHANNEL];
    m_pcInvAusgHistoryEnabled = new char[m_AusgAnz / PORTANZAHLCHANNEL];
    
    m_pAusgBoardAddr = new CBoardAddr [m_AusgAnz / PORTANZAHLCHANNEL];
    for(pos=0; pos < m_AusgAnz / PORTANZAHLCHANNEL; pos++)
    {	m_pcAusg[pos] = 0;
        m_pcAusgLast[pos] = 0;
        m_pcAusgHistory[pos] = 0;
        m_pcInvAusgHistoryEnabled[pos] = 0xff;
    }
    // Eingänge
    m_pcEing = new char[m_EingAnz / PORTANZAHLCHANNEL];
    m_pcEingLast = new char[m_EingAnz / PORTANZAHLCHANNEL];
    m_pEingBoardAddr = new CBoardAddr [m_EingAnz / PORTANZAHLCHANNEL];
    m_pEingInterrupt = new int[m_EingAnz / PORTANZAHLCHANNEL];
    for(pos=0; pos < m_EingAnz / PORTANZAHLCHANNEL; pos++)
    {
        m_pcEing[pos] = 0;
        m_pEingInterrupt[pos] = 0;
        m_pcEingLast[pos] = 0;
    }

    // EasyWave
    if(m_EWAnzBoard)
    {
        m_pEWAusg = new char [m_EWAnzBoard *EWANZAHLCHANNEL];
        // EasyWave Eingänge
        m_pEWEing = new char [m_EWAnzBoard *EWANZAHLCHANNEL];
        m_pEWEingLast = new char [m_EWAnzBoard *EWANZAHLCHANNEL];
        m_pEWEingDelay = new int [m_EWAnzBoard *EWANZAHLCHANNEL];
        m_pEWBoardAddr = new CBoardAddr [m_EWAnzBoard];
        // EasyWave Ausgänge
        for(pos=0; pos < m_EWAnzBoard *EWANZAHLCHANNEL; pos++)
        {
            m_pEWAusg[pos] = 0;
            m_pEWEing[pos] = 0;
            m_pEWEingLast[pos] = 0;
            m_pEWEingDelay[pos] = 0;
        }
    // Merker
    }
    if(m_MerkAnz)
    {
        m_pcMerk = new char [m_MerkAnz / PORTANZAHLCHANNEL];
        m_pcMerkLast = new char [m_MerkAnz / PORTANZAHLCHANNEL];
        for(pos=0; pos < m_MerkAnz / PORTANZAHLCHANNEL; pos++)
        {
            m_pcMerk[pos] = 0;
            m_pcMerkLast[pos] = 0;
        }
    }
    // Integer
    if(m_IntegerAnz)
        m_pInteger = new CInteger [m_IntegerAnz];
    
    // Sekunden Timer
    if(m_SecTimerAnz)
        m_pSecTimer = new CSecTimer [m_SecTimerAnz];
    // Software Eingänge
    if(m_SEingAnz)
    {
        m_pcSEing = new char [m_SEingAnz / 8];
        m_pcSEingLast = new char [m_SEingAnz / 8];
        for(pos=0; pos < m_SEingAnz / 8; pos++)
        {
            m_pcSEing[pos] = 0;
            m_pcSEingLast[pos] = 0;
        }
    }
    // UART - Schnittstellen auf I2C-Bus
    if(m_UartAnz)
        m_pGsm = new CGsm;
  
    m_pReadFile->Close ();
    delete m_pReadFile;
    m_pReadFile = NULL;
    
    m_pReadFile = new CReadFile;
    m_pReadFile->OpenRead (pProgramPath, 1);

    int iHeizKoerperNr = 1; 

    for(;;)
    {	if(m_pReadFile->ReadLine())
        {
            m_pReadFile->ReadBuf(buf, ':');
            if(strncmp(buf, "MainboardV1.0", 13) == 0 || strncmp(buf, "MainboardV2.0", 13) == 0) 
                boardtype = 1;
            else if(strncmp(buf, "EingAusgV1.0", 12) == 0)
                boardtype = 20;
            else if(strncmp(buf, "EingAusgV2.0", 12) == 0)
                boardtype = 21;
           else if(strncmp(buf, "GSMV2.0", 7) == 0)
                boardtype = 30;
            else
                boardtype = 0;

            // MainboardV1.0 8 Ausgänge, 8 Eingänge 1 EasyWave Modul
            switch(boardtype) {
            case 1: // Mainboard
                    // 8 Eingänge	
                Inh1 = INHADDRI2C + 3;
                Addr2 = 0;
                Inh2 = 0;
                Addr3 = ADDR23017 + 7;
                initEingAusg(&AnzEin, &AnzAus, Inh1, Addr2, Inh2, Addr3);

                str = m_pReadFile->ReadAlpha ();
                if(str.compare("EW") == 0)
                {   // Inh1, INh2 und Addr2 sind bereits definiert
                    Addr3 = ADDRRTRM08;
                    Reg = 0;
                    init_EW(m_pEWBoardAddr, &AnzEW, Inh1, Addr2, Inh2, Addr3, Reg);
                }
                else
                {
                    BSC_WriteReg (1, ADDRRTRM08, PWD, 0);
                }
                break;
            case 20: // EingAusgV1.0
            case 21: // EinAusgV1.0
                offset = m_pReadFile->ReadBuf (buf, ':');
                if(offset == 2 	&& buf[0] >= 'A' && buf[0] <= 'D' 
                                        && buf[1] >= '1' && buf[1] <= '3')
                {
                    Inh1 = (buf[0] - 'A') + INHADDRI2C;
                    Addr2 = 0;
                    Inh2 = 0;
                    Addr3 = (buf[1] - '1') + ADDR23017;
                }
                else if(offset == 4 
                                && buf[0] >= 'A' && buf[0] <= 'D' 
                                && buf[1] >= '1' && buf[1] <= '3'
                                && buf[2] >= 'a' && buf[2] <= 'd'
                                && buf[3] >= '1' && buf[3] <= '4')
                {
                    Inh1 = (buf[0] - 'A')  + INHADDRI2C ;
                    Addr2 = (buf[1] - '1') + ADDRI2C;
                    Inh2 = (buf[2] - 'a') + INHADDRI2C;
                    Addr3 = (buf[3] - '1') + ADDR23017;
                }
                else 
                    m_pReadFile->Error(13);
                iAddr3 = Addr3;
                initEingAusg(&AnzEin, &AnzAus, Inh1, Addr2, Inh2, Addr3);

                pos = 1;
                while(pos > 0) {
                    pos = m_pReadFile->ReadBuf (buf, ':');
                    if(strncmp(buf, "EW", 2) == 0)
                    {   // Inh1, INh2 und Addr2 sind bereits definiert
                        Addr3 = ADDRRTRM08;
                        Reg = 0;
                        init_EW(m_pEWBoardAddr, &AnzEW, Inh1, Addr2, Inh2, Addr3, Reg);
                    }
                    else if(strncmp(buf, "EingExpV2.0", 11) == 0)
                    {
                        iAddr3 += 4;
                        initEingExp(&AnzEin, Inh1, Addr2, Inh2, iAddr3);
                    } 
                    else if(strncmp(buf, "AusgExpV2.0", 11) == 0)
                    {
                        iAddr3 += 4;
                        initAusgExp(&AnzAus, Inh1, Addr2, Inh2, iAddr3);
                    }
                }
                break;
            case 30: // GSM-Schnittstelle
            {	
                int baudrate, bits, stops, zykluszeit;
                char parity;

                offset = m_pReadFile->ReadBuf (buf, ':');
                if(offset == 2 	&& buf[0] >= 'A' && buf[0] <= 'D' 
                                        && buf[1] >= '1' && buf[1] <= '3')
                {
                    Inh1 = (buf[0] - 'A') + INHADDRI2C;
                    Addr2 = 0;
                    Inh2 = 0;
                    Addr3 = ~(buf[1] - '1');
                    Addr3 = (Addr3 & 0x02)*2 + (Addr3 & 0x01) + ADDRUARTIS750;
                }
                else if(offset == 4 
                                && buf[0] >= 'A' && buf[0] <= 'D' 
                                && buf[1] >= '1' && buf[1] <= '3'
                                && buf[2] >= 'a' && buf[2] <= 'd'
                                && buf[3] >= '1' && buf[3] <= '4')
                {
                    Inh1 = (buf[0] - 'A')  + INHADDRI2C ;
                    Addr2 = (buf[1] - '1') + ADDRI2C;
                    Inh2 = (buf[2] - 'a') + INHADDRI2C;
                    Addr3 = ~(buf[3] - '1');
                    Addr3 = (Addr3 & 0x02)*2 + (Addr3 & 0x01) + ADDRUARTIS750;
                }
                else 
                    m_pReadFile->Error(13);
                baudrate = m_pReadFile->ReadNumber ();
                m_pReadFile->ReadBuf(buf, ',');
                bits = m_pReadFile->ReadNumber ();
                m_pReadFile->ReadBuf(buf, ',');
                m_pReadFile->ReadBuf(buf, ',');
                parity = toupper(buf[0]);
                stops = m_pReadFile->ReadNumber ();
                if(!m_pReadFile->ReadSeparator())
                    m_pReadFile->Error(105);
                str = m_pReadFile->ReadText();
                m_pGsm->SetAddr(Inh1, Addr2, Inh2, Addr3, 0);
                if(!m_pGsm->Init(baudrate, bits, stops, parity, str))
                    m_pReadFile->Error(59);
            }
            break;
            default:
                break;
            }
        }
        else
            break;
    }

    m_pReadFile->Close();
    delete m_pReadFile;
    m_pReadFile = NULL;

}
int CIOGroup::readAnzeige()
{
    char buf[MSGSIZE];
    int iRet = 0;
    
    if(m_pReadFile->ReadBuf(buf, ',') == 0)
    {
        m_pReadFile->ReadBuf(buf, ' ');
        if(buf[0] == 'R')
            iRet = 1;
        else
            iRet = -1;
    }
    return iRet;
}

void CIOGroup::Addr_unique (CBoardAddr *addr, int Anzahl, int Inh1, int Addr2,
                 int Inh2, int Addr3, int Reg)
{
    int i;
    
    if(Inh2)
    {
        i = Inh2 & 0x03;
        Inh2 = 1 << i;
        Inh2 |= 0XF0;
    }
    
    for(i=0; i < Anzahl; i++)
    {
        // wird nicht auf Register getestet !!
        if(Inh1 == (addr + i)->Inh1 && (addr + i)->Addr2 == Addr2
                        && (addr + i)->Inh2 == Inh2 && (addr + i)->Addr3 == Addr3)
            m_pReadFile->Error(8);
    }
}

// Programmiert den 23017-Baustein als Eingang
// Testet ob an der angegebenen Adresse auch ein Baustein vorhanden ist
// Testet ebenfalls die Anzahl der Eingänge

void CIOGroup::initEingAusg(int *AnzEin, int *AnzAus, int Inh1, int Addr2, int Inh2, int Addr3)
{
    int Reg;
    
    Reg = ADDR23017REGB;
    Addr_unique(m_pEingBoardAddr, *AnzEin, Inh1, Addr2, Inh2, Addr3, Reg);
    (m_pEingBoardAddr + *AnzEin)->SetAddr(Inh1, Addr2, Inh2, Addr3, Reg);
    setEingang(m_pEingBoardAddr + *AnzEin);
    *AnzEin += 1;   
    Reg = ADDR23017REGA;
    Addr_unique(m_pAusgBoardAddr, *AnzAus, Inh1, Addr2, Inh2, Addr3, Reg);
    (m_pAusgBoardAddr + *AnzAus)->SetAddr(Inh1, Addr2, Inh2, Addr3, Reg);
    setAusgang(m_pAusgBoardAddr + *AnzAus);
    *AnzAus += 1;   

}

void CIOGroup::initEingExp(int *AnzEin, int Inh1, int Addr2, int Inh2, int Addr3)
{
    int Reg;
    
    Reg = ADDR23017REGB;
    Addr_unique(m_pEingBoardAddr, *AnzEin, Inh1, Addr2, Inh2, Addr3, Reg);
    (m_pEingBoardAddr + *AnzEin)->SetAddr(Inh1, Addr2, Inh2, Addr3, Reg);
    setEingang(m_pEingBoardAddr + *AnzEin);
    *AnzEin += 1;
    Reg = ADDR23017REGA;
    CBoardAddr * pBoardAddr = new CBoardAddr();
    pBoardAddr->SetAddr(Inh1, Addr2, Inh2, Addr3, Reg);
    setAusgang(pBoardAddr);
    delete pBoardAddr;
}

void CIOGroup::initAusgExp(int *AnzAus, int Inh1, int Addr2, int Inh2, int Addr3)
{
    int Reg;
    
    Reg = ADDR23017REGA;
    Addr_unique(m_pAusgBoardAddr, *AnzAus, Inh1, Addr2, Inh2, Addr3, Reg);
    (m_pAusgBoardAddr + *AnzAus)->SetAddr(Inh1, Addr2, Inh2, Addr3, Reg);
    setAusgang(m_pAusgBoardAddr + *AnzAus);
    *AnzAus += 1;
    Reg = ADDR23017REGB;
    CBoardAddr * pBoardAddr = new CBoardAddr();
    pBoardAddr->SetAddr(Inh1, Addr2, Inh2, Addr3, Reg);
    setAusgang(pBoardAddr);
    delete pBoardAddr;
}
void CIOGroup::setEingang(CBoardAddr * pBoardAddr)
{
    int ret;
// MCP23017 I/O
    while(true)
    {
        pBoardAddr->setI2C_gpio(); 

        // IOCON konfigurieren
        //	.BANK = 1 port in different banks;
        //  .MIRROR = 0INTA and INTB not connected
        //  .SEQOP = 0 sequential mode enabled (address pointer incremented
        //  .DISSLW = 1 slew rate disabled (SDA)
        //  .HAEN = 1 enables address pins
        //  .ODR = 1 INT as a, open-drain output
        //  .INTPOL=0 INT active low
        //  .unimplemented
        // IOCON auf Adresse 0x0B denn diese Adresse ist frei wenn Programmierung richtig
        BSC_WriteReg(1, pBoardAddr->Addr3, 0x9C, 0x0B);

        // IOCON lesen und testen ob gleich
        ret = BSC_ReadReg (1, pBoardAddr->Addr3, pBoardAddr->Reg + IOCON);
        if(ret != 0x9C)
        {
            if(m_iTest != 11)
                m_pReadFile->Error(9);
        }
        else
            break;
    }
    // IODIR (0x00) = Input = 1
    BSC_WriteReg(1, pBoardAddr->Addr3, 0xFF, pBoardAddr->Reg + IODIR);
    // IPOL (0x01) = 1 Polarität invertieren : Strom im Optokoppler entspricht high
    BSC_WriteReg(1, pBoardAddr->Addr3, 0xFF, pBoardAddr->Reg + IPOL);
    // GPINTEN (0x02) =1 interrupt on change
    BSC_WriteReg(1, pBoardAddr->Addr3, 0xff, pBoardAddr->Reg + GPINTEN);
    // INTCON (0x04) = 1 control with the DEFVAL Register
    BSC_WriteReg(1, pBoardAddr->Addr3, 0xff, pBoardAddr->Reg + INTCON);
    // GPPU (0x06) = 1 Pull-up with 100k
    BSC_WriteReg (1, pBoardAddr->Addr3, 0xFF, pBoardAddr->Reg + GPPU);

}
// Programmiert den 23017-Baustein als Ausgang
// Testet ob an der angegebenen Adresse auch ein Baustein vorhanden ist
// Testet ebenfalls die Anzahl der Eingänge

void CIOGroup::setAusgang(CBoardAddr *pBoardAddr)
{
    int ret;

    // MCP23017 I/O
    while(true)
    {
        pBoardAddr->setI2C_gpio();

        // IOCON konfigurieren
        //	.BANK = 1 port in different banks																																																																																																																																																																										  IOCON;
        //  .MIRROR = 0INTA and INTB not connected
        //  .SEQOP = 0 sequential mode enabled (address pointer incremented
        //  .DISSLW = 1 slew rate disabled (SDA)
        //  .HAEN = 1 enables address pins
        //  .ODR = 1 INT as a, open-drain output
        //  .INTPOL=0 INT active low
        //  .unimplemented
        // IOCON auf Adresse 0x0B denn diese Adresse ist frei wenn Programmierung richtig
        BSC_WriteReg(1, pBoardAddr->Addr3, 0x9C, 0x0B);
        // IOCON lesen und testen ob gleich
        ret = BSC_ReadReg (1, pBoardAddr->Addr3, pBoardAddr->Reg + IOCON);
        if(ret != 0x9C)
        {
            if(m_iTest != 11)
                m_pReadFile->Error(9);
        }
        else
            break;
    }    
    // IODIR (0x00) = Output = 0
    BSC_WriteReg(1, pBoardAddr->Addr3, 0x00, pBoardAddr->Reg + IODIR);
    // IPOL (0x01) = 0 Polarität nicht invertieren 
    BSC_WriteReg(1, pBoardAddr->Addr3, 0x00, pBoardAddr->Reg + IPOL);
    // GPPU (0x06) = 1 Pull-up with 100k
    BSC_WriteReg (1, pBoardAddr->Addr3, 0xff, pBoardAddr->Reg + GPPU);
    // GPINT (0x02) =0 interrupt on change nicht aktivieren
    BSC_WriteReg(1, pBoardAddr->Addr3, 0x00, pBoardAddr->Reg + GPINTEN);

}

// Easywave
void CIOGroup::init_EW(CBoardAddr *addr, int *anz, int Inh1, int Addr2, 
                int Inh2, int Addr3,int Reg)
{
    int ret;

    Addr_unique(addr, *anz, Inh1, Addr2, Inh2, Addr3, Reg);
    (addr + *anz)->SetAddr(Inh1, Addr2, Inh2, Addr3, Reg);
    (addr + *anz)->setI2C_gpio();

    BSC_WriteReg (1, Addr3, RX_RQ, RX_MODE);
    ret = BSC_ReadReg (1, Addr3, RX_MODE);
    if(ret != RX_RQ)
        m_pReadFile->Error(9);	

    *anz += 1;
}

void CIOGroup::EW_GetBezeichnung(char *ptr, int channel)
{
    int reg, i;
    CBoardAddr *addr;

    if(channel > 0 && channel <= m_EWAnzBoard*EWANZAHLCHANNEL)
    {
        channel--;
        addr = &m_pEWBoardAddr[channel/EWANZAHLCHANNEL];		
        addr->setI2C_gpio();
        channel = channel%EWANZAHLCHANNEL; 
        switch(channel/8) {
        case 0:
            reg = RX_LN_CH0_7;
            break;
        case 1:
            reg = RX_LN_CH8_15;
            break;
        case 2:
            reg = RX_LN_CH16_23;
            break;
        default:
            reg = RX_LN_CH24_31;
            break;
        }
        reg = BSC_ReadReg(1, addr->Addr3, reg);
        channel %= 8;
        for(i = 1; channel > 0 ; i *= 2, channel--);
        if(reg & i)
            sprintf(ptr, "belegt");
        else
            sprintf(ptr, "frei");
    }
}
int CIOGroup::EW_GetAnzahlChannel()
{
    return m_EWAnzBoard * EWANZAHLCHANNEL;
}

void CIOGroup::EW_ResetChannel(int channel)
{
    CBoardAddr *addr;
    int ret;

    if(channel > 0 && channel <= m_EWAnzBoard*EWANZAHLCHANNEL)
    {
        channel--;
        addr = &m_pEWBoardAddr[channel/EWANZAHLCHANNEL];		
        addr->setI2C_gpio();
        ret = BSC_WriteReg(1, addr->Addr3, PWD_RST, RX_MODE);
        ret = BSC_WriteReg (1, addr->Addr3, channel%EWANZAHLCHANNEL, RX_CHANNEL);
        ret = BSC_WriteReg (1, addr->Addr3, DELLN, RX_MODE);
        EW_SetReceive (addr);
    }
}

void CIOGroup::EW_Reset()
{
    int i;

    for(i=0; i < m_EWAnzBoard*EWANZAHLCHANNEL; i++)
        EW_ResetChannel(i+1);
    m_iLastEWButtonErkennen = 0;
}

//
// In dem Empfangsmodus schalten
//
int CIOGroup::EW_SetReceive (CBoardAddr *addr)
{	
    addr->setI2C_gpio();
    return BSC_WriteReg (1, addr->Addr3, RX_RQ, RX_MODE);
}

bool CIOGroup::EW_TransmitFinish(int channel)
{
    CBoardAddr *addr;
    int ret;
    bool bRet= false;

    if(channel > 0 && channel <= m_EWAnzBoard*EWANZAHLCHANNEL)
    {
        channel--;
        addr = &m_pEWBoardAddr[channel/EWANZAHLCHANNEL];		
        addr->setI2C_gpio();
        ret = BSC_ReadReg(1, addr->Addr3, TX_STATUS);
        if(ret & TX_TRANSMIT)
            bRet = false;
        else
            bRet = true;
    }
    return bRet;
}

int CIOGroup::EW_Transmit(int channel, int button)
{
    int ret=0;
    CBoardAddr *addr;

    if(channel > 0 && channel <= m_EWAnzBoard*EWANZAHLCHANNEL)
    {
        channel--;
        addr = &m_pEWBoardAddr[channel/EWANZAHLCHANNEL];		
        addr->setI2C_gpio();
        ret = BSC_WriteReg(3, addr->Addr3, (1*256 + button)*256 + channel, TX_CHANNEL);
    }
    return ret;
}

int CIOGroup::EW_SetLearn(int channel)
{
    int ret = 0;
    CBoardAddr *addr;

    if(channel > 0 && channel <= m_EWAnzBoard*EWANZAHLCHANNEL)
    {
        channel--;
        addr = &m_pEWBoardAddr[channel/EWANZAHLCHANNEL];		
        addr->setI2C_gpio();
        ret = BSC_WriteReg(1, addr->Addr3, channel, RX_CHANNEL);
        // JEN userdata sind noch nicht definiert
        //ret += BSC_WriteReg(2, addr->Addr3, (channel+1)*256 + channel+1, RX_USERDATA0);
        ret += BSC_WriteReg(1, addr->Addr3, LN_RQ, RX_MODE);
    }
    return ret;
}

//
// Taste wird gedrückt und nur wenn diese ANGELERNT wurde, dann werden
//		- RX_TEL_CHANGED = 1
//		- RX_TEL_VALID = 1
// Taste wird wieder losgelassen oder wenn sie 35 Sekunden gedrückt wurde, dann
//		- RX_TEL_CHANGED = 1
//		- RX_TEL_VALID = 0
// Diese Tatsache können wir nutzen um fetszustellen, dass die Taste noch
//    	gedrückt ist
// Es wird immer die Aktion eines Tasters beendet (wie vorher beschrieben, ehe
// ein neues Signal empfangen wird
//
// Ist der Sender nicht angemeldet wird auch RX_TEL_CHANGED gesetzt, RX_TEL_VALID
// aber nicht
//
// Funk mässig präsentiert es sich folgendermassen:
// - beim Taster wird bis maximal 35 Sekunden fortlaufend gesendet
// - beim Schalter ist die Dauer abhängig vom Schalter (siehe Anleitung)
// - der RTRM08 sendet das Signal (~40 msek) 11 x (insgesate Dauer ~440 ms)
// - das Signal wird gesendet ohne zu kontrollieren ob ein Signal vorhanden ist
// 
int CIOGroup::EW_Received(int board)
{	
    int RX_Status;
    int channel, button;
    int ret = 0;
    int les;
    char text[50];

    (&m_pEWBoardAddr[board])->setI2C_gpio();
    RX_Status = BSC_ReadReg(1, m_pEWBoardAddr[board].Addr3, RX_STATUS);
    if(RX_Status & RX_TEL_CHANGED) 
    {	les = BSC_ReadReg(1, m_pEWBoardAddr[board].Addr3, RX_MODE);
        if(les == RX_RQ) // Receive mode
        {   // es ist ein EasyWave telegram gelesen worden
            if(RX_Status & RX_TEL_VALID)
            {	ret = BSC_ReadReg(2, m_pEWBoardAddr[board].Addr3, RX_CHANNEL);
                channel = ret / 256;
                button = ret % 256;
                // FEHLERANALYSE 22.07.16 
                if(channel > 31 || channel < 0)
                {   sprintf(text, "Button pressed 0 < channel > 31 %d", channel);
                    syslog(LOG_INFO, text);
                    channel = 0;
                }
                if(button > 3 || button < 0)
                {   sprintf(text, "Pressed button 0 < number > 3 %d", button);
                    syslog(LOG_INFO, text);
                    button = 0;
                }
                if(m_iLastEWButton)
                    SetEWEingStatus (m_iLastEWButton /256, m_iLastEWButton % 256, false);
                m_iLastEWButton = (board*EWANZAHLCHANNEL + channel + 1)*256 + button;
                m_iLastEWButtonErkennen = m_iLastEWButton;
                SetEWEingStatus (board*EWANZAHLCHANNEL + channel + 1, button, true);
            }
            else
            {	// gedrückte Taste wurde losgelassen !!
                ret = BSC_ReadReg(2, m_pEWBoardAddr[board].Addr3, RX_CHANNEL);
                channel = ret / 256;
                button = ret % 256;
                // FEHLERANALYSE 22.07.16 
                if(channel > 31 || channel < 0)
                {   sprintf(text, "Button released 0 < channel > 31 %d", channel);
                    syslog(LOG_INFO, text);
                    channel = 0;
                }
                if(button > 3 || button < 0)
                {   sprintf(text, "Released button 0 < number > 3 %d", button);
                    syslog(LOG_INFO, text);
                    button = 0;
                }
                // JEN 30.05.16
                if(m_iLastEWButton == (board*EWANZAHLCHANNEL + channel + 1)*256 + button)
                {
                    SetEWEingStatus (board*EWANZAHLCHANNEL + channel + 1, button, false);
                    m_iLastEWButton = 0;
                }
            }
            les = BSC_ReadReg(1, m_pEWBoardAddr[board].Addr3, RX_ACK);
        }
        else if(les == LN_RQ)
        {   if(RX_Status & RX_TEL_VALID)
            {
                ret = BSC_ReadReg(2, m_pEWBoardAddr[board].Addr3, RX_CHANNEL);
                sprintf(text, "Kan.:%d,Tast.:%d", board*EWANZAHLCHANNEL+ret/256+1, ret%256);
                LCD_AnzeigeZeile3 (text);
                les = BSC_ReadReg(1, m_pEWBoardAddr[board].Addr3, RX_ACK);
                EW_SetReceive (&m_pEWBoardAddr[board]);
            }
            else
            {	sprintf(text, "Sender bekannt");
                LCD_AnzeigeZeile3(text);
                les = BSC_ReadReg(1, m_pEWBoardAddr[board].Addr3, RX_ACK);
            }
        }
        else
        {
            EW_SetReceive (&m_pEWBoardAddr[board]);
            les = BSC_ReadReg(1, m_pEWBoardAddr[board].Addr3, RX_ACK);
        }
    }
    return ret;
}

CIOGroup::CIOGroup()
{
    // Eingänge
    m_EingAnz=0;
    m_pcEing = NULL;
    m_pcEingLast = NULL;
    m_pEingInterrupt = NULL;
    m_pEingBoardAddr = NULL;
    // EasyWave
    m_EWAnzBoard=0;
    m_pEWEing = NULL;
    m_pEWEingLast = NULL;
    m_pEWEingDelay = NULL;
    m_pEWBoardAddr = NULL;
    m_iLastEWButton = 0;
    m_iLastEWButtonErkennen = 0;
    // Merker
    m_MerkAnz = 0;
    m_pcMerk = NULL;
    m_pcMerkLast = NULL;
    // Integer
    m_IntegerAnz = 0;
    m_pInteger = NULL;
    // Sekunden Timer
    m_SecTimerAnz = 0;
    m_pSecTimer = NULL;
    // Software-Eingänge
    m_SEingAnz = 0;
    m_pcSEing = NULL;
    m_pcSEingLast = NULL;
    // Ausgänge
    m_AusgAnz = 0;
    m_pcAusg = NULL;
    m_pcAusgLast = NULL;
    m_pcAusgHistory = NULL;
    m_pcInvAusgHistoryEnabled = NULL;
    m_pAusgBoardAddr=NULL;
    // EW EasyWare Ausgänge
    m_pEWAusg = NULL;
    m_pHistory = NULL;
    m_pBerechne = NULL;
    m_iBerechneAnz = 0;
    m_bBerechne = true;
    m_bBerechneHeizung = true;
    m_bHandAusg = false;
    m_bSimEing = false;
    m_iChangeSensorModBusAddress = 0;
    m_pRS485_1 = NULL;
    m_pRS485_2 = NULL;
    m_pModBus = NULL;
    m_iAnzZaehler = 0;
    m_iAnzSensor = 0;
    m_gsmtimeslice = 0;
    m_pGsm = NULL;
    m_UartAnz = 0;
    m_pHeizung = NULL;
    m_pWStation = NULL;
    m_pHue = NULL;
    m_pAlarm = NULL;
    m_pMusic = NULL;
    m_pSonos = NULL;
    m_pBrowserMenu = NULL;
    m_iTest = 0;
    m_iAusgTest = 0;
    m_iControl = 0;
    m_pAlarmClock = NULL;
    m_iMaxAnzModBusClient = 0;
    m_iMaxAnzSensor = 0;
    m_iMaxAnzZaehler = 0;
    m_iMaxAnzHueEntity = 0;
    pthread_mutex_init(&ext_mutexNodejs, NULL);
}

CIOGroup::~CIOGroup()
{
    int i;

    pthread_mutex_destroy(&ext_mutexNodejs);
    if(m_pRS485_1)
    {
        delete m_pRS485_1;
        m_pRS485_1 = NULL;
    }
    if(m_pRS485_2)
    {
        delete m_pRS485_2;
        m_pRS485_2 = NULL;
    }
    if(m_pHeizung)
    {
        delete m_pHeizung;
        m_pHeizung = NULL;
    }
    if(m_pHistory)
    {
        delete m_pHistory;
        m_pHistory = NULL;
    }
    if(m_pHue)
    {
        delete m_pHue;
        m_pHue = NULL;
    }      
    if(m_pAlarm)
    {
        delete m_pAlarm;
        m_pAlarm = NULL;
    }
    if(m_pMusic)
    {
        delete m_pMusic;
        m_pMusic = NULL;
    }
    if(m_pWStation)
    {
        delete m_pWStation;
        m_pWStation = NULL;
    }
    if(m_pHue)
    {
        delete m_pHue;
        m_pHue = NULL;
    }
    if(m_pSonos)
    {
        delete m_pSonos;
        m_pSonos = NULL;
    }
    if(m_pBrowserMenu)
    {
        delete m_pBrowserMenu;
        m_pBrowserMenu = NULL;
    }
    if(m_pAlarmClock)
    {
        delete m_pAlarmClock;
        m_pAlarmClock = NULL;
    }
    if(m_pInteger)
    {
        delete [] m_pInteger;
        m_pInteger = NULL;
    }
    if(m_pSensor)
    {
        for(i=0; i < m_iMaxAnzSensor; i++)
            delete m_pSensor[i];
        delete [] m_pSensor;
        m_pSensor = NULL;
    }
    if(m_pZaehler)
    {
        for(i=0; i < m_iMaxAnzZaehler; i++)
            delete m_pZaehler[i];
        delete [] m_pZaehler;
        m_pZaehler = NULL;
    }
    if(m_pModBus)
    {
        delete m_pModBus;
        m_pModBus = NULL;
    }
}

void CIOGroup::SetBerechne()
{
    m_bBerechne = true;
}

void CIOGroup::SetBerechneHeizung()
{
    m_bBerechneHeizung = true;
}

bool CIOGroup::GetBerechne()
{
    return m_bBerechne;
}

void CIOGroup::LesAlleEing()
{
    int i;

    for(i=0; i < m_EingAnz / PORTANZAHLCHANNEL; i++)
    {
        (&m_pEingBoardAddr[i])->setI2C_gpio();
        m_pcEing[i] = BSC_ReadReg(1, m_pEingBoardAddr[i].Addr3,
                          m_pEingBoardAddr[i].Reg + GPIO);
        BSC_WriteReg(1, m_pEingBoardAddr[i].Addr3, m_pcEing[i],
                                  m_pEingBoardAddr[i].Reg + DEFVAL);
        m_pcEingLast[i] = m_pcEing[i];
    }
}

void CIOGroup::ResetSimulation()
{
    if(m_bSimEing)
    {
        LesAlleEing ();
        m_bSimEing = false;
    }
    m_pUhr->ResetSimulation ();
    m_bBerechne = true;
}

void CIOGroup::SetHandAusg(bool bState)
{
    m_bHandAusg = bState;
    m_bBerechne = true;
}

int CIOGroup::GetSEingAnz()
{
	return m_SEingAnz;
}

char *CIOGroup::GetSEingAddress(int nr)
{
    return &m_pcSEing[(nr-1)/ 8];
}

char *CIOGroup::GetSEingLastAddress(int nr)
{
    return &m_pcSEingLast[(nr-1)/ 8];
}

bool CIOGroup::GetSEingStatus(int nr)
{
    char *ptr;
    int ch, i;

    ptr = &m_pcSEing[(nr-1)/ 8];
    ch = (nr - 1) % 8;
    for(i = 1; ch > 0 ; i *= 2, ch--);
    if(*ptr & i)
        return true;
    else
        return false;

}
void CIOGroup::SetSEingStatus(int nr, bool bState)
{
    char *ptr;
    int ch, i;

    ptr = &m_pcSEing[(nr-1)/ 8];
    ch = (nr - 1) % PORTANZAHLCHANNEL;
    for(i = 1; ch > 0 ; i *= 2, ch--);
    if(bState)
        *ptr |= i;
    else
        *ptr &= ~i;
    m_bBerechne = true;
}

void CIOGroup::SetEWEingStatus(int nr, int button, bool bState)
{
    char zstd;

    // JEN 30.05.16
    if(nr < 1 || nr > m_EWAnzBoard * EWANZAHLCHANNEL)
        return;

    zstd = m_pEWEing[nr - 1];
    switch(button) {
    case 0: // button A
        if(!bState)
                zstd &= 0xFE;
        else
                zstd |= 0x01;
        zstd |= 0x10;
        break;
    case 1: // button B
        if(!bState)
                zstd &= 0xFD;
        else
                zstd |= 0x02;
        zstd &= 0xEF;
        break;
    case 2: // button C
        if(!bState)
                zstd &= 0xFB;
        else
                zstd |= 0x04;
        zstd |= 0x20;
        break;
    case 3: // button D
        if(!bState)
                zstd &= 0xF7;
        else
                zstd |= 0x08;
        zstd &= 0xDF;
        break;
    default:
        zstd = 0;
        break;
    }
    m_pEWEing[nr-1] = zstd;
    if(bState)
            m_pEWEingDelay[nr-1] = EWEINGDELAY;
    else
            m_pEWEingDelay[nr-1] = 0;
    m_bBerechne = true;
}

int CIOGroup::GetAusgAnz()
{
    return m_AusgAnz;
}

char * CIOGroup::GetAusgAddress(int nr)
{
    return &m_pcAusg[(nr-1)/ PORTANZAHLCHANNEL];
}

char *CIOGroup::GetAusgLastAddress(int nr)
{
    return &m_pcAusgLast[(nr-1)/ PORTANZAHLCHANNEL];
}

bool CIOGroup::GetAusgStatus(int nr)
{
    char *ptr;
    int ch, i;

    ptr = GetAusgAddress (nr);
    ch = (nr - 1) % PORTANZAHLCHANNEL;
    for(i = 1; ch > 0 ; i *= 2, ch--);
    if(*ptr & i)
        return true;
    else
        return false;
}

void CIOGroup::SetAusgStatus(int nr, bool bState)
{
    char *ptr;
    int ch, i;

    ptr = GetAusgAddress (nr);
    ch = (nr - 1) % PORTANZAHLCHANNEL;
    for(i = 1; ch > 0 ; i *= 2, ch--);
    if(bState)
        *ptr |= i;
    else
        *ptr &= ~i;
    m_bBerechne = true;
}

CSecTimer *CIOGroup::GetSecTimerAddress(int nr)
{
    return &m_pSecTimer[nr-1];
}

int CIOGroup::GetSecTimerAnz()
{
    return m_SecTimerAnz;
}

int CIOGroup::GetEingAnz()
{
    return m_EingAnz;
}

char * CIOGroup::GetEingAddress(int nr)
{
    return &m_pcEing[(nr-1)/ PORTANZAHLCHANNEL];
}

char * CIOGroup::GetEingLastAddress(int nr)
{
    return &m_pcEingLast[(nr-1)/ PORTANZAHLCHANNEL];
}


bool CIOGroup::GetEingStatus(int nr)
{
    char *ptr;
    int ch, i;

    ptr = GetEingAddress (nr);
    ch = (nr - 1) % PORTANZAHLCHANNEL;
    for(i = 1; ch > 0 ; i *= 2, ch--);
    if(*ptr & i)
        return true;
    else
        return false;
}

void CIOGroup::SetEingStatus(int nr, bool bState)
{
    char *ptr;
    int ch, i;

    ptr = GetEingAddress (nr);
    ch = (nr - 1) % PORTANZAHLCHANNEL;
    for(i = 1; ch > 0 ; i *= 2, ch--);
    if(bState)
        *ptr |= i;
    else
        *ptr &= ~i;
    m_bSimEing = true;
    m_bBerechne = true;
}

int CIOGroup::GetEWAnz()
{
    return m_EWAnzBoard * EWANZAHLCHANNEL;
}

char * CIOGroup::GetEWEingAddress(int nr)
{
    return &m_pEWEing[nr-1];
}

char * CIOGroup::GetEWEingLastAddress(int nr)
{
    return &m_pEWEingLast[nr-1];
}

char * CIOGroup::GetEWAusgAddress(int nr)
{
    return &m_pEWAusg[nr-1];
}

int CIOGroup::GetMerkAnz()
{
    return  m_MerkAnz;
}

char * CIOGroup::GetMerkAddress(int nr)
{
    return &m_pcMerk[(nr-1)/ PORTANZAHLCHANNEL];
}

int CIOGroup::GetIntegerAnz()
{
    return m_IntegerAnz;
}

CInteger * CIOGroup::GetIntegerAddress(int nr)
{
    return &m_pInteger[nr-1];
}
char * CIOGroup::GetMerkLastAddress(int nr)
{
    return &m_pcMerkLast[(nr-1)/ PORTANZAHLCHANNEL];
}

int CIOGroup::GetAnzSensor()
{
    return m_iAnzSensor;
}

CSensor *CIOGroup::GetSensorAddress(int nr)
{
    return m_pSensor[nr-1];
}

int CIOGroup::GetHKAnz()
{
    if(m_pHeizung != NULL)
        return m_pHeizung->GetHKAnz();
    else
        return 0;
}
CHeizung * CIOGroup::GetHZAddress()
{
    return m_pHeizung;
}

CHeizKoerper* CIOGroup::GetHKAddress(int nr)
{
    if(m_pHeizung != NULL)
        return m_pHeizung->GetHKAddress(nr);
    else
        return NULL;
}

CZaehler * CIOGroup::GetZaehler(int nr)
{
    if(nr > 0 && nr <= m_iAnzZaehler)
        return m_pZaehler[nr-1];
    else
        return NULL;
}

int CIOGroup::GetAnzZaehler()
{
    return m_iAnzZaehler;
}
void CIOGroup::LesHeizung(char *pProgramPath)
{
    CUhrTemp* pUhrTemp;
    CHeizProgramm *pHeizProgramm = NULL;
    CHeizProgrammTag *pHeizProgrammTag = NULL;
    CHeizKoerper *pHeizKoerper = NULL;
    
    char buf[256];
    int iTemp, iStunde, iMinute, iTag, iMonat, iJahr, iLen;
    int iAnz, iHeizProgramm=0, iHeizKoerper, iSensor;
    int iCase = 0;
    string str;
 
    m_pReadFile = new CReadFile;
    m_pReadFile->OpenRead (pProgramPath, 6);

    // Eine Zeile entspricht einem Ventil und besteht aus einem Block von Uhrzeit
    // und Temperatur (8h00-21). Die Blöcke müssen durch ein Strichpunkt getrennt sein
    // Warmwasser wird wie ein Heizkoerper verwaltet hat aber keinen Sensor 
    // und wird nur ein- oder ausgeschaltet
    for(;;)
    {	
        iLen = m_pReadFile->ReadLine();
        if(iLen)
        {
            m_pReadFile->ReadBuf(buf, iLen);
            if(strncmp(buf, "HEIZKOERPER", 11) == 0)
                iCase = 1;
            else if(strncmp(buf, "HEIZPROGRAMM", 12) == 0)
            {
                if(iCase == 21)
                {
                    if(pHeizProgramm == NULL)
                        m_pReadFile->Error(77);
                    m_pHeizung->AddHeizProgramm(pHeizProgramm);
                    pHeizProgramm = NULL;
                }
                iCase = 20;
            }
            else if(strncmp(buf, "WARMWASSER", 10) == 0)
                iCase = 2; 
            else
            {
                m_pReadFile->ResetLine();
                switch(iCase) {
                case 1:  // Heizkoerper
                    iLen = m_pReadFile->ReadBuf(buf, ';');
                    str = string(buf);
                    iSensor = m_pReadFile->ReadNumber();
                    if(iSensor > 0 && iSensor <= m_iAnzSensor)
                    {
                        pHeizKoerper = new CHeizKoerper;
                        pHeizKoerper->SetName(str);
                        pHeizKoerper->SetSensor(GetSensorAddress(iSensor), iSensor);
                        m_pHeizung->AddHeizKoerper(pHeizKoerper);
                    }
                    else
                        m_pReadFile->Error(70);
                    break;
                case 2:  // Warmwasser = Heizkoerper ohne Sensor !!
                    iLen = m_pReadFile->ReadBuf(buf, ';');
                    str = string(buf);
                    pHeizKoerper = new CHeizKoerper;
                    pHeizKoerper->SetName(str);
                    m_pHeizung->AddHeizKoerper(pHeizKoerper);
                    break;
                case 20:  // Heizprogramm Name einlesen
                    iLen = m_pReadFile->ReadBuf(buf, ';');
                    str = string(buf);
                    pHeizProgramm = new CHeizProgramm;
                    pHeizProgramm->SetName(str);
                    iHeizProgramm++;
                    // Ist eine Datum und eine Uhrzeit für dieses Programm eingegeben?
                    if(iHeizProgramm > 1 && m_pReadFile->GetChar(iLen) == ';')
                    {         
                        iTag = m_pReadFile->ReadNumber();
                        if(iTag < 1 || iTag > 31)
                            m_pReadFile->Error(78);
                        if(m_pReadFile->GetChar() != ':')
                            m_pReadFile->Error(78);
                        iMonat = m_pReadFile->ReadNumber();
                        if(iMonat < 1 || iMonat > 12)
                            m_pReadFile->Error(78);
                        if(m_pReadFile->GetChar() != ':')
                            m_pReadFile->Error(78);
                        iJahr = m_pReadFile->ReadNumber();
                        if(iJahr < 2018 || iJahr > 2200)
                            m_pReadFile->Error(78);
                        if(m_pReadFile->GetChar() != '-')
                            m_pReadFile->Error(78);
                        iStunde = m_pReadFile->ReadNumber ();
                        if(iStunde < 0 || iStunde > 23)
                            m_pReadFile->Error(78);
                        if(m_pReadFile->GetChar() != 'h')
                            m_pReadFile->Error(72);
                        iMinute = m_pReadFile->ReadNumber();
                        if(iMinute < 0 || iMinute > 59)
                            m_pReadFile->Error(78);
                        pHeizProgramm->m_iDatum = m_pUhr->getDatumTag(iJahr, iMonat, iTag);
                        pHeizProgramm->m_iUhrzeit = iStunde *60 + iMinute;
                    }
                    iCase = 21;
                    break;
                case 21:  // Gültigkeitstage und Name einlesen
                    iLen = m_pReadFile->ReadBuf(buf, ';');
                    for(iTemp = 0, iTag=0; iTemp < 7; iTemp++)
                    {
                        iTag *= 2;
                        if(buf[iTemp] == '1')
                            iTag++;
                    }
                    str = m_pReadFile->ReadText();
                    pHeizProgrammTag = new CHeizProgrammTag;
                    pHeizProgrammTag->CreateUhrTemp(m_pHeizung->GetAnzHeizKoerper());
                    pHeizProgrammTag->SetName(str);
                    pHeizProgrammTag->SetGueltigkeitsTage(iTag);
                    iHeizKoerper = 1;
                    iAnz = 0;
                    iCase = 22;
                    break;
                case 22: // Uhrzeiten und Temperaturen der Heizkoerper einlesen
                    for(;;) {
                        iStunde = m_pReadFile->ReadNumber ();
                        if(m_pReadFile->GetChar() != 'h')
                            m_pReadFile->Error(72);
                        iMinute = m_pReadFile->ReadNumber();
                        if(m_pReadFile->GetChar() != '-')
                            m_pReadFile->Error(73);
                        iTemp = m_pReadFile->ReadNumber();
                        if(iStunde > 24 || iMinute > 60)
                            m_pReadFile->Error(74);
                        if(iTemp > MAXTEMP)
                            m_pReadFile->Error(75);
                        pHeizProgrammTag->SetUhrTemp(iHeizKoerper, iAnz, iStunde*60+iMinute, iTemp);
                        if(m_pReadFile->ReadSeparator())
                        {
                            iAnz++;
                            if(iAnz >= ANZUHRTEMP)
                                m_pReadFile->Error(76);
                        }
                        else
                        {
                            iHeizKoerper++;
                            if(iHeizKoerper > m_pHeizung->GetAnzHeizKoerper())
                            {
                                pHeizProgramm->AddHeizProgrammTag(pHeizProgrammTag);
                                iCase = 21;
                                break;
                            }  
                            iLen = m_pReadFile->ReadLine();
                            if(!iLen)
                                break;
                            iAnz = 0;
                        }
                    }
                    break;
                default:
                    break;
                }
            }  
        }
        else
        {
            if(iCase == 21)
            {
                if(pHeizProgramm == NULL)
                    m_pReadFile->Error(77);
                m_pHeizung->AddHeizProgramm(pHeizProgramm);
            }
            else
                m_pReadFile->Error(00);
            break;
        }
    }

    m_pReadFile->Close();
    delete m_pReadFile;
    m_pReadFile = NULL;   
}

void CIOGroup::LesBrowserMenu(char *pProgramPath)
{
    char buf[256];
    int iNiv1=-1, iNiv2=0, iNiv3=0, iNiv4=0;
    int iSpace, len, nbr, type;
    class CBrowserEntity *pMenu;

    m_pReadFile = new CReadFile;
    m_pReadFile->OpenRead (pProgramPath, 3);
    
    for(nbr = 0;;)
    {
        if(m_pReadFile->ReadLine())
        {
            iSpace = m_pReadFile->CountSpace();
            if(iSpace == 1)
                nbr++;
        }
        else 
            break;
    }
    if(!nbr)
        m_pReadFile->Error(48);
    m_pBrowserMenu = new CBrowserMenu(nbr);
    m_pReadFile->Close();
    delete m_pReadFile;
    
    m_pReadFile = NULL;
    m_pReadFile = new CReadFile;
    m_pReadFile->OpenRead (pProgramPath, 3);
    
    for(;;)
    {
        if(m_pReadFile->ReadLine())
        {
            iSpace = m_pReadFile->CountSpace();
            // Bezeichnung lesen
            m_pReadFile->ReadBuf(buf, ','); 

            switch(iSpace) {
            case 0: // muss die erste Zeile sein
                if(iNiv1 == -1)
                        iNiv1 = 0;
                else
                        m_pReadFile->Error(48);
                break;
            case 1:
                iNiv1 = m_pReadFile->ReadNumber();
                // 1 = Steuerung, 2 = Heizung, 3 = Zähler, 4=Alarm, 5=Wetterstation, 6=Sensoren
                // 7 = AlarmClock
                switch(iNiv1) {
                    case 1: // Steuerung
                        break;
                    case 2:
                        if(m_pHeizung == NULL)
                            m_pReadFile->Error(132);
                        break;
                    case 3:
                        break;
                    case 4:
                        break;
                    case 5:
                        break;
                    case 6:
                        break;
                    case 7:
                        if(m_pAlarmClock == NULL)
                            m_pAlarmClock = new CAlarmClock;
                        else
                            m_pReadFile->Error(130);   
                        break;
                    default:
                        m_pReadFile->Error(64);
                        break;
                }
                if(m_pReadFile->GetChar() != ',') 
                    m_pReadFile->Error(62);
                iNiv2 = 0;
                iNiv3 = 0;
                iNiv4 = 0;
                break;
            case 2:
                if(iNiv1 >= 1)
                {
                    iNiv2++;
                    iNiv3 = 0;
                    iNiv4 = 0;
                }
                else
                    m_pReadFile->Error(51);
                break;
            case 3:
                if(iNiv2 >= 1)
                    iNiv3++;
                else
                    m_pReadFile->Error(52);
                break;
            default:
                m_pReadFile->Error(49);
                break;
            }

           pMenu = new class CBrowserEntity;
           pMenu->SetText(buf);
           pMenu->SetNiv(iNiv1, iNiv2, iNiv3, iNiv4);

            // Wird ein Wert (Zustand oder Zahl abgefragt
            len = m_pReadFile->ReadBuf(buf, ','); 
            if(len >= 0)
            {
                if(!iNiv2 && !iNiv3 && !iNiv4)
                {   // Ebene 1 (1=Steuerung, 2=Heizung, 3=Zähler) 
                    // es handelt sich um einen Bild für die Navigationsbar
                    if(len)
                        pMenu->SetImage(buf);
                }
                else if(iNiv2 && !iNiv3 && !iNiv4)
                {
                    // Ebene 2
                    type = atoi(buf);
                    pMenu->m_iTyp = type;
                    switch(type) {
                        case 1: // Steuerung
                                // es handelt sich um einen Text, mit oder ohne Bild und
                                // Sammelschalter
                            len = m_pReadFile->ReadBuf(buf, ',');
                            if(len >= 0)
                            {
                                if(len)
                                    pMenu->SetImage(buf);
                                len = m_pReadFile->ReadBuf(buf, ',');
                                if(len == 1 && strncmp(buf, "1", 1) == 0)
                                    pMenu->m_bSammelSchalter = true;
                            }
                            break;
                        default:
                            break;
                    }
                }
                else {
                    nbr = atoi(&buf[0]);
                    pMenu->m_iTyp = nbr;
                    m_pReadFile->ReadBuf(buf, ',');
                    switch(nbr) {
                    case 1: // Typ 1 es handelt sich um einen Software-Schalter
                        {
                            if(strncmp(buf, "A", 1) == 0) // Ausgang
                            {
                                nbr = atoi(&buf[1]);
                                if(nbr > 0 && nbr <= GetAusgAnz())
                                { 
                                    CBerechneAusg * pAusg = new CBerechneAusg;
                                    pAusg->init(nbr, GetAusgAddress (nbr));
                                    pMenu->m_pOperState = pAusg;
                                }
                                else
                                    m_pReadFile->Error(54);
                            }
                            else if(strncmp(buf, "M", 1) == 0) // Merker
                            {
                                nbr = atoi(&buf[1]);
                                if(nbr > 0 && nbr <= GetMerkAnz())
                                {   
                                    CBerechneAusg * pAusg = new CBerechneAusg; 
                                    pAusg->init(nbr, GetMerkAddress (nbr));
                                    pMenu->m_pOperState = pAusg;
                                }
                                else
                                    m_pReadFile->Error(55);
                            }
                            else
                                m_pReadFile->Error(53);
                            // soll der Wert auch geändert werden
                            len = m_pReadFile->ReadBuf(buf, ','); 
                            if(len)
                            {
                                nbr = atoi(&buf[1]);
                                if(strncmp(buf, "S", 1) == 0)
                                {
                                    if(nbr > 0 && nbr <= GetSEingAnz())
                                    {
                                        CBerechneAusg * pAusg = new CBerechneAusg; 
                                        pAusg->init(nbr, GetSEingAddress(nbr));
                                        pMenu->m_pOperChange = pAusg;
                                    }
                                    else
                                        m_pReadFile->Error(57);
                                }
                                else
                                    m_pReadFile->Error(56);
                            }
                        }
                        break;
                    case 2: // es handelt sich um eine HUE-Lampe
                        if(strncmp(buf, "H", 1) == 0) // Ausgang
                        {
                            nbr = atoi(&buf[1]);
                            if(m_pHue != NULL && nbr > 0 && nbr <= m_pHue->GetAnz())
                            { 
                                CBerechneHue * pHue = new CBerechneHue;
                                pHue->init(m_pHue->GetAddress(nbr));
                                pMenu->m_pOperState = pHue;
                                
                            }
                            else
                                m_pReadFile->Error(54);
                            len = m_pReadFile->ReadBuf(buf, ','); 
                            if(len)
                            {
                                nbr = atoi(&buf[1]);
                                if(strncmp(buf, "S", 1) == 0)
                                {
                                    if(nbr > 0 && nbr <= GetSEingAnz())
                                    {
                                        CBerechneAusg * pAusg = new CBerechneAusg; 
                                        pAusg->init(nbr, GetSEingAddress(nbr));
                                        pMenu->m_pOperChange = pAusg;
                                    }
                                    else
                                        m_pReadFile->Error(57);
                                }
                                else
                                    m_pReadFile->Error(56);
                            }                            
                        } 
                        else
                            m_pReadFile->Error(53);
                        break;
                    default:
                        m_pReadFile->Error(56);
                        break;
                    }
                }
            }
            m_pBrowserMenu->InsertEntity(pMenu);
        }
        else
            break;
    }
    m_pReadFile->Close();
    delete m_pReadFile;
    m_pReadFile = NULL;

}

void CIOGroup::LesAlarm(char *pProgramPath)
{
    CConfigCalculator *pcc;
    COperBase ** pOperBase;
    CBerechneBase *pBerechne;
    int iLen, nbre, iLines, iAlarme, iTurn, i, anz, iSensor, iDelayAktiv, iDelayAlarm;
    string str, strSensor, strBez;
    int type;
    
    for(iTurn = 0; iTurn < 2; iTurn++)
    {
        m_pReadFile = new CReadFile;
        m_pReadFile->OpenRead (pProgramPath, 7);
        
        if(iTurn)
            m_pAlarm = new CAlarm(iLines, iAlarme);

        iLines = 0;
        iAlarme = 0;
        
        for(;;)
        {	
            iLen = m_pReadFile->ReadLine();
            if(iLen)
            {
                str = m_pReadFile->ReadAlpha ();

                if(str.compare("SENSOR") == 0)
                    type = 0;                
                else if(str.compare("SABOTAGE") == 0)
                    type = 1;
                else if(str.compare("ALARM") == 0)
                    type = 2;
                else if(str.compare("PASSWORD") == 0)
                    type = 3;
                else if(str.compare("TECHPASSWORD") == 0)
                    type = 4;
                else
                    m_pReadFile->Error(6);
            }
            else
                break;
            
            switch(iTurn) {
                case 0: // erster Duchlauf, Anzahl Einträge und Alarme zählen
                    switch(type) {
                        case 0:
                            if(iAlarme)
                                m_pReadFile->Error(92);
                            iLines++;
                            break;
                        case 1: // als erstes und nur einmal
                            if(iAlarme)
                                m_pReadFile->Error(97);  
                            iAlarme++;
                            break;
                        case 2:
                            if(!iAlarme)
                                m_pReadFile->Error(98);
                            iAlarme++;
                            break;
                        default:
                            break;
                    }
                    break;
                case 1: // eingelesene in CAlarm speichern
                    m_pReadFile->ReadSeparator();                    
                    switch(type) {
                        case 0: // Sensoren
                            strSensor = m_pReadFile->ReadText(';');
                            m_pReadFile->ReadSeparator();
                            strBez = m_pReadFile->ReadText(';');
                            m_pReadFile->ReadSeparator();
                            nbre = m_pReadFile->ReadNumber();
                            if(nbre < 1 || nbre > 2)
                                m_pReadFile->Error(96);
                            m_pAlarm->SetSensorBez(iLines, strSensor, strBez, nbre);
                            pcc = new CConfigCalculator(m_pReadFile);  
                            pcc->eval((char *)strSensor.c_str(), this, 100); 
                            anz = pcc->GetAnzOper ();
                            pOperBase = new COperBase *[anz];
                            pBerechne = new CBerechneBase;
                            pBerechne->setIfElse (1);
                            pBerechne->setOper(pOperBase);
                            for(i=0; i < anz; i++)
                                pOperBase[i] = pcc->GetOper(i);
                            m_pAlarm->SetBerechneSensor(iLines, pBerechne);
                            delete pcc;
                            pcc = NULL; 
                            iLines++;
                            break;
                        case 1: // Sabotage
                        case 2: //
                            strBez = m_pReadFile->ReadText(';'); 
                            if(type == 2)
                            {   
                                m_pReadFile->ReadSeparator(); 
                                iDelayAktiv = m_pReadFile->ReadNumber();
                                if(iDelayAktiv < 10)
                                    m_pReadFile->Error(93);
                                m_pReadFile->ReadSeparator();
                                iDelayAlarm = m_pReadFile->ReadNumber();
                                if(iDelayAlarm < 10)
                                    m_pReadFile->Error(94);
                            }
                            for(iSensor = 0; ;iSensor++)
                            {
                                m_pReadFile->ReadSeparator();
                                if(!m_pReadFile->ReadNumber())
                                    break;
                            }
                            if(!iSensor)
                                m_pReadFile->Error(95);
                            m_pReadFile->ResetLine();
                            str = m_pReadFile->ReadText(';');
                            m_pReadFile->ReadSeparator();
                            strBez = m_pReadFile->ReadText(';'); 
                            m_pAlarm->SetAlarmeBez(iAlarme, iSensor, strBez);                             
                            if(type == 2)
                            {
                                m_pReadFile->ReadSeparator();
                                iDelayAktiv = m_pReadFile->ReadNumber();
                                m_pReadFile->ReadSeparator();
                                iDelayAlarm = m_pReadFile->ReadNumber();
                                m_pAlarm->SetDelay(iAlarme, iDelayAktiv, iDelayAlarm);
                            }
                            for(i=0; i < iSensor ; i++)
                            {
                                m_pReadFile->ReadSeparator();
                                nbre = m_pReadFile->ReadNumber();
                                if(!m_pAlarm->SetAlarmeSensor(iAlarme, i, nbre))
                                    m_pReadFile->Error(116);
                            }   
                            iAlarme++;
                            break;
                        case 3: // Passwort
                            strBez = m_pReadFile->ReadText(';');
                            m_pAlarm->SetPwd(strBez);
                            break;
                        case 4: // Techniker Passwort
                            strBez = m_pReadFile->ReadText(';');
                            m_pAlarm->SetTechPwd(strBez);
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            } 
                    
        }

        m_pReadFile->Close();
        delete m_pReadFile;  
        m_pReadFile = NULL;
    }
}

void CIOGroup::LesHistory(char * pProgrammPath)
{
    int iLen, iTyp, iNr;
    char buf[256], ch;
    bool bRet, bLast = false;
    long lFilePos = 0l;
    
    if(m_pHue == NULL)
        iNr = 0;
    else
        iNr = m_pHue->GetAnz();
    m_pHistory = new CHistory(m_AusgAnz, m_EWAnzBoard * EWANZAHLCHANNEL, iNr);
    
    m_pReadFile = new CReadFile;
    m_pReadFile->OpenRead (pProgramPath, 10);  
    
    for(;;)
    {	
        iLen = m_pReadFile->ReadLine();
        if(iLen)
        {
            if(bLast)
                m_pReadFile->Error(115);
            if(m_pReadFile->ReadBuf(buf, ':'))
            {
                if(strncmp(buf, "A", 1) == 0 && strlen(buf) == 1)               
                    iTyp = 1;
                else if (strncmp(buf, "FA", 2) == 0 && strlen(buf) == 2)
                    iTyp = 2;
                else if (strncmp(buf, "H", 1) == 0 && strlen(buf) == 1)
                {
                    iTyp = 3;
                    if(m_pHue == NULL)
                        m_pReadFile->Error(114);
                }
                else if(strncmp(buf, "DIFF", 4) == 0)
                {
                    m_pHistory->SetFilePosAfterDiff(lFilePos);
                    iTyp = 4;
                }
                else
                    m_pReadFile->Error(113);
                lFilePos = m_pReadFile->GetFilePos();                    
                for(;;)
                {
                    iNr = m_pReadFile->ReadNumber();
                    if(!iNr && iTyp != 4)
                        break;
                    switch(iTyp) {
                        case 1:
                            bRet = m_pHistory->InitAddAusg(iNr);
                            break;
                        case 2:
                            bRet = m_pHistory->InitAddEWAusg(iNr);
                            break;
                        case 3:
                            bRet = m_pHistory->InitAddHue(iNr);
                            break;
                        case 4:
                            bRet = m_pHistory->SetDiffTage(iNr);
                            bLast = true;
                            break;
                        default:
                            bRet = false;
                            break;
                    }
                    if(!bRet)
                        m_pReadFile->Error(5);
                    if(!m_pReadFile->ReadSeparator())
                        m_pReadFile->Error(63);
                    if(bLast)
                        break;
                }

            }
            else
                m_pReadFile->Error(113);
        }
        else
            break;
    }
    m_pReadFile->Close();
    delete m_pReadFile;  
    m_pReadFile = NULL;
    m_pHistory->SetInvAusgHistoryEnabled(m_pcInvAusgHistoryEnabled);
}

queue<EWAusgEntity> * CIOGroup::GetEWAusgFifo()
{
    return &m_EWAusgFifo;
}

char * CIOGroup::GetpcAusgHistory()
{
    return m_pcAusgHistory;
}

CAlarmClock * CIOGroup::GetAlarmClockAddress()
{
    return m_pAlarmClock;
}