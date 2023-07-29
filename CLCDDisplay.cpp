#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>

#include "ProWo.h"

#define NUMERISCH		1
#define ZEICHENLISTE		2

#define EINGABEWERT		1
#define EINGABENUMMER		2

#define MODUHRZEIT 		1
#define MODDATUM		2

#define SUBNIVEMPFANG 		101
#define SUBNIVSENDEN 		102
#define SUBNIVHANDAUSG		103
#define SUBNIVSIMEING		104
#define SUBNIVSIMSEING		105
#define SUBNIVSIMEWEING		106

#define EWRESET			201
#define EWRESETCHANNEL		202
#define EWANLERNEN		203
#define EWLERNEN		204
#define EWERKENNEN		205

#define HANDAUSG		301
#define SIMEING			401	
#define SIMSEING		402	
#define SIMEWEING		403	
#define SIMTAGNACHT		404
#define SIMUHRZEIT		405

#define IPADDRESSETH0	501
#define IPADDRESSWLAN0	502
#define VERSIONNR		503
#define MAXZYKLUSZEIT   504

/* menu = NULL es gibt kein Untermenü
 * ID : für die Eingabeart
 * wenn menu != NULL && ID != 0 dann handelt es sich um ein Untermenü, dass
 * um eine Nummer erweitert wird (Eingang, Ausgang, Funk, ...)
 * In diesem Fall wird ID auch gesetzt ohne dass die OK-Taste länger gedrückt ist.
 * 
 * 
 */

// Niveau 3 Hauptmenu-Funkteinstellung-Empfang
struct _menu FunkEmpfang[] = {
    { "  Anlernen", NULL, EWLERNEN, 0 },
    { "  Löschen", NULL, EWRESETCHANNEL, 0 },
    { "", NULL, 0, 0 }
};

// Niveau 3 Hauptmenu-Funkeinstellung-Senden
struct _menu FunkSenden[] = {
    { "  Taste A", NULL, EWANLERNEN, 0 },
    { "  Taste B", NULL, EWANLERNEN, 1 },
    { "  Taste C", NULL, EWANLERNEN, 2 },
    { "  Taste D", NULL, EWANLERNEN, 3 },
    { "", NULL, 0, 0 }
};

// Niveai 3 Hauptmenu - Handsteuerung - Ausgang
struct _menu HandAusg[] = {
    { "  Aus", NULL, HANDAUSG, 0 },
    { "  Ein", NULL, HANDAUSG, 0 },
    { "", NULL, 0, 0 }
};

// Nivea 3 Hauptmenu - Simulation - Eingang
struct _menu SimEing[] = {
    { "  Aus", NULL, SIMEING, 0 },
    { "  Ein", NULL, SIMEING, 0 },
    { "", NULL, 0, 0 }
};

// Nivea 3 Hauptmenu - Simulation - SEingang
struct _menu SimSEing[] = {
    { "  Aus", NULL, SIMSEING, 0 },
    { "  Ein", NULL, SIMSEING, 0 },
    { "", NULL, 0, 0 }
};

// Nivea 3 Hauptmenu - Simulation - EWEingang
struct _menu SimEWEing[] = {
    { "  Taste A", NULL, SIMEWEING, 0 },
    { "  Taste B", NULL, SIMEWEING, 0 },	
    { "  Taste C", NULL, SIMEWEING, 0 },
    { "  Taste D", NULL, SIMEWEING, 0 },
    { "", NULL, 0, 0 }
};

// Nivea 3 Hauptmenu - Simulation - Tag / Nacht
struct _menu SimTagNacht[] = {
    { "   Tag", NULL, SIMTAGNACHT, 0 },
    { "   Nacht", NULL, SIMTAGNACHT, 0 },
    { "", NULL, 0, 0 }
};

// Niveau 2 Hauptmenu - Einstellungen
struct _menu Einstellung[] = {
    { " Uhrzeit", NULL, MODUHRZEIT, 0 },
    { " Datum", NULL, MODDATUM, 0 },
    { "", NULL, 0, 0 }
};
	
// Niveau 2 Hauptmenu - Funkeinstellung
struct _menu Funkeinstellung[] = {
    { " Empfang", FunkEmpfang, SUBNIVEMPFANG, 0 },
    { " Erkennen", NULL, EWERKENNEN, 0 },
    { " Zurücksetzen", NULL, EWRESET, 0 },
    { "", NULL, 0, 0 }
};

// Niveau 2 Hauptmenu - Handsteuerung
struct _menu Handsteuerung[] = {
    { "Ausgang", HandAusg, SUBNIVHANDAUSG, 0 },
    { "Funkausg.", FunkSenden, SUBNIVSENDEN, 0 },
    { "", NULL, 0, 0 }
};

// Niveau 2 Hauptmenu - Simulation
struct _menu Simulation[] = {
    { "Eingang", SimEing, SUBNIVSIMEING, 0 },
    { "SEingang", SimSEing, SUBNIVSIMSEING, 0 },
    { "Funkeing", SimEWEing, SUBNIVSIMEWEING, 0 },
    { "Tag / Nacht", NULL, SIMTAGNACHT, 0 },
    { "Uhrzeit", NULL, SIMUHRZEIT, 0 },
    { "", NULL, 0, 0 }
};

// Niveau 2 Hauptmenu - Systeminfo
struct _menu Systeminfo[] = {
    { "IP-Adresse eth0", NULL, IPADDRESSETH0, 0 },
    { "IP-Adresse wlan0", NULL, IPADDRESSWLAN0, 0 },
    { "Version", NULL, VERSIONNR, 0 },
    { "Max Zykluszeit", NULL, MAXZYKLUSZEIT, 0 },
    { "", NULL, 0, 0 }
};

struct _menu Hauptmenu[] = {
    { "Einstellung", Einstellung, 0, 0 },
    { "Funkeinstellung", Funkeinstellung, 0, 0 },
    { "Handsteuerung", Handsteuerung, 0, 0 },
    { "Simulation", Simulation, 0, 0 },
    { "Systeminfo", Systeminfo, 0, 0 },
    { "", NULL, 0, 0 }
};

struct _zeichen strEinAus[] = {
    { "Aus", 0, 0, 41 },
    { "Ein", 0, 0, 41 },
    { "", 0, 0, 0 }
};

struct _zeichen strJaNein[] = {
    { "    Ja", 0, 0, 41 },
    { "    Nein", 0, 0, 41 },
    { "", 0, 0, 0 }
};

struct _zeichen strTagNacht[] = {
    { "   Nacht", 0, 0, 42 },
    { "    Tag", 0, 0, 41 },
    { "", 0, 0, 0 }
};

struct _zeichen strAnlernen[] = {
    { " Sender drücken", 0, 0, 48 },
    { "", 0, 0, 0 }
};
	
struct _numerisch struhr[] = {
    { 0, 0, 23, 40 },
    { 0, 0, 59, 43 },
    { 0,0,0,0 }
};

struct _numerisch strdatum[] = {
    { 0, 1, 31, 38 },
    { 0, 1, 12, 41 },
    { 0, 2016, 2055, 46 },
    { 0, 0, 0, 0 }
};

struct _numerisch strNummer[] = {
    { 1, 1, 999, 0 },
    { 0, 0, 0, 0 }
};

void CLCDDisplay::VerwaltMenu (int button)
{

    if(button)
    {
        if(!m_niv.eingErfolgt)
            m_niv.eingErfolgt = VerwaltMenuSteuerung(button);
        else
            m_niv.eingErfolgt = VerwaltMenuEingabe(button);
        AnzeigeMenu();
        if(!m_niv.eingErfolgt)
        {
            if(m_niv.ID)
            {
                // Wert lesen, z.B Uhrzeit, Datum, Nummertext, ...
                InitEingabe(m_niv.ID);
                AnzeigeZeile3();
            }
            else
                LCD_AnzeigeZeile3 ((char *)"");
        }
        else
            AnzeigeZeile3();
    }
    else if(!m_niv.niv1)
        AnzeigeUhr();
}

void CLCDDisplay::InitEingabe(int ID)
{
    switch(ID) {
    case MODUHRZEIT:
        {   struct tm t;
            m_pUhr->getUhrzeitStruct(&t);
            struhr[0].wert = t.tm_hour;
            struhr[1].wert = t.tm_min;
        }
        break;
    case SIMUHRZEIT:
        {
            int i = m_pUhr->getUhrmin();
            struhr[0].wert = i / 60;
            struhr[1].wert = i % 60;
        }
        break;
    case MODDATUM:
        {   struct tm t;
            m_pUhr->getUhrzeitStruct(&t);
            strdatum[0].wert = t.tm_mday;
            strdatum[1].wert = t.tm_mon+1;
            strdatum[2].wert = t.tm_year + 1900;
        }
        break;
    case SUBNIVEMPFANG:
    case SUBNIVSENDEN:
        strNummer[0].wert = 1;
        strNummer[0].min = 1;
        strNummer[0].max = m_pIOGroup->EW_GetAnzahlChannel();
            break;
    case SUBNIVHANDAUSG:
        strNummer[0].wert = 1;
        strNummer[0].min = 1;
        strNummer[0].max = m_pIOGroup->GetAusgAnz();
            break;
    case SUBNIVSIMEING:
        strNummer[0].wert = 1;
        strNummer[0].min = 1;
        strNummer[0].max = m_pIOGroup->GetEingAnz();
            break;
    case SUBNIVSIMSEING:
        strNummer[0].wert = 1;
        strNummer[0].min = 1;
        strNummer[0].max = m_pIOGroup->GetSEingAnz();
            break;
    case SUBNIVSIMEWEING:
        strNummer[0].wert = 1;
        strNummer[0].min = 1;
        strNummer[0].max = m_pIOGroup->GetEWAnz();
            break;
    default:
        break;
    }
}


void CLCDDisplay::StartEingabe(int ID)
{
    switch(ID) {
    case EWLERNEN:
        m_pIOGroup->EW_SetLearn(strNummer[0].wert);
        break;
    default:
        break;
    }
}

void CLCDDisplay::BeendeEingabe()
{
    switch(m_niv.ID) {
    case EWANLERNEN:
        m_pIOGroup->EW_Transmit(strNummer[0].wert, 
                Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].menu[m_niv.niv3-1].Nummer);
        break;
    case MODUHRZEIT:
        m_pUhr->setTime(struhr[0].wert, struhr[1].wert);
        break;
    case MODDATUM:
        m_pUhr->setTime(strdatum[0].wert, strdatum[1].wert, strdatum[2].wert);
        break;
    case EWRESET:
        if(m_niv.posEingabe == 0)
            m_pIOGroup->EW_Reset();
        break;
    case EWRESETCHANNEL:
        if(m_niv.posEingabe == 0)
            m_pIOGroup->EW_ResetChannel(strNummer[0].wert);
        break;
    case HANDAUSG:
        if(m_niv.posEingabe == 0)
        {
            m_pIOGroup->SetHandAusg(true);
            m_pIOGroup->SetAusgStatus(strNummer[0].wert, (bool)(m_niv.niv3 - 1));
        }
        break;
    case SIMEING:
        if(m_niv.posEingabe == 0)
            m_pIOGroup->SetEingStatus(strNummer[0].wert, (bool)(m_niv.niv3 - 1));
        break;
    case SIMSEING:
        if(m_niv.posEingabe == 0)
            m_pIOGroup->SetSEingStatus(strNummer[0].wert, (bool)(m_niv.niv3 - 1));
        break;
    case SIMEWEING:
        m_pIOGroup->SetEWEingStatus(strNummer[0].wert, m_niv.niv3-1, m_niv.posEingabe);
        break;
    case SIMTAGNACHT:
        m_pUhr->SetSimTagNacht(m_niv.posEingabe + 1);
        m_pIOGroup->SetBerechne();
        break;
    case SIMUHRZEIT:
        m_pUhr->SetSimUhrzeit(struhr[0].wert*60 + struhr[1].wert);
        m_pIOGroup->SetBerechne();
        break;
    default:
        break;
    }
}

void CLCDDisplay::AnzeigeZeile3()
{
    char text[64];
    char *ptr;

    text[0] = 0;
    switch(m_niv.ID) {
    case IPADDRESSETH0:	// IP-Adresse anzeigen
        GetIPAddress(text, (char *)"eth0");
        break;
    case IPADDRESSWLAN0:	// IP-Adresse anzeigen
        GetIPAddress(text, (char *)"wlan0");
        break;
    case VERSIONNR:
        if(m_pstrVersion != NULL)
            sprintf(text, "%s", m_pstrVersion);
        else
            sprintf(text, "not definded");
        break;
    case MAXZYKLUSZEIT:
        sprintf(text, "%0.3f msek", (float)ext_iMaxZykluszeit / 1000.0);
        break;
    case MODUHRZEIT:
    case SIMUHRZEIT:
        sprintf(text, "     %02d:%02d", struhr[0].wert, struhr[1].wert); 
        break;
    case MODDATUM:
        sprintf(text, "   %02d/%02d/%4d", strdatum[0].wert, strdatum[1].wert,
                                strdatum[2].wert);
        break;
    case SUBNIVEMPFANG:
        m_pIOGroup->EW_GetBezeichnung(text, strNummer[0].wert);
        break;
    case SUBNIVSIMEWEING:
        { 
            char *ptr;
            int ab = 0 , cd = 0;

            ptr = m_pIOGroup->GetEWEingAddress(strNummer[0].wert);
            if(*ptr & 0x10)
                ab = 1;
            if(*ptr & 0x20)
                cd = 1;
            sprintf(text, "E:%s, F:%s", strEinAus[ab].ptr, strEinAus[cd].ptr);
        }
        break;
    case EWRESET:
    case EWRESETCHANNEL:
    case EWANLERNEN:
    case HANDAUSG:
    case SIMEING:
    case SIMSEING:
        if(m_niv.eingErfolgt)
            sprintf(text, "%s", strJaNein[m_niv.posEingabe].ptr);
        break;
    case SIMEWEING:
        if(m_niv.eingErfolgt)
            sprintf(text, "    %s", strEinAus[m_niv.posEingabe].ptr);
        break;
    case EWLERNEN:
        if(m_niv.eingErfolgt)
            sprintf(text, "%s", strAnlernen[0].ptr);
        break;
    case SUBNIVHANDAUSG:
        if(m_pIOGroup->GetAusgStatus (strNummer[0].wert))
            sprintf(text, "    %s", strEinAus[1].ptr);
        else
            sprintf(text, "    %s", strEinAus[0].ptr);
        break;
    case SUBNIVSIMEING:
        if(m_pIOGroup->GetEingStatus (strNummer[0].wert))
            sprintf(text, "    %s", strEinAus[1].ptr);
        else
            sprintf(text, "    %s", strEinAus[0].ptr);
        break;
    case SUBNIVSIMSEING:
        if(m_pIOGroup->GetSEingStatus (strNummer[0].wert))
            sprintf(text, "    %s", strEinAus[1].ptr);
        else
                 sprintf(text, "    %s", strEinAus[0].ptr);
        break;
    case SIMTAGNACHT:
        if(m_niv.eingErfolgt)
            sprintf(text, "%s", strTagNacht[m_niv.posEingabe].ptr);
        else
        {	
            if(m_pUhr->getTagNacht ())
                sprintf(text, "%s", strTagNacht[1].ptr);
            else
                sprintf(text, "%s", strTagNacht[0].ptr);
        }
        break;
    case EWERKENNEN:
        if(m_pIOGroup->m_iLastEWButtonErkennen)
        {
            sprintf(text, "ch=%d, but=%c", m_pIOGroup->m_iLastEWButtonErkennen/256,
                                m_pIOGroup->m_iLastEWButtonErkennen%256 + 'A');
        }
        else
            text[0] = 0;
        break;
    default:
        break;
    }

    LCD_AnzeigeZeile3(text);
    if(m_niv.posCursor)
        LCD_SetCursorPos(m_niv.posCursor);
}

int CLCDDisplay::VerwaltMenuEingabe(int button)
{
    int ret = 1;
    int type;
    struct _numerisch *strnum;
    struct _zeichen *strzeichen;
    int wert;

    switch(m_niv.ID) {
    case MODUHRZEIT:
    case SIMUHRZEIT:
        type = NUMERISCH;
        strnum = struhr;
        break;
    case MODDATUM:
        type = NUMERISCH;
        strnum = strdatum;
        break;
    case SUBNIVEMPFANG:
    case SUBNIVSENDEN:
    case SUBNIVHANDAUSG:
    case SUBNIVSIMEING:
    case SUBNIVSIMSEING:
    case SUBNIVSIMEWEING:
        type = NUMERISCH;
        strnum = strNummer;
        break;
    case EWRESET:
    case EWRESETCHANNEL:
    case EWANLERNEN:
    case HANDAUSG:
    case SIMEING:
    case SIMSEING:
        type = ZEICHENLISTE;
        strzeichen = strJaNein;
            break;
    case SIMEWEING:
        type = ZEICHENLISTE;
        strzeichen = strEinAus;
        break;
    case EWLERNEN:
        type = ZEICHENLISTE;
        strzeichen = strAnlernen;
        break;
    case SIMTAGNACHT:
        type = ZEICHENLISTE;
        strzeichen = strTagNacht;
        break;
    default:
        return 0;
        break;
    }

    switch(button) {
    case BUTTONUP:
        switch (type) {
        case NUMERISCH:
            wert = (strnum+m_niv.posEingabe)->wert;
            wert++;
            if(wert > (strnum+m_niv.posEingabe)->max)
                    wert = (strnum+m_niv.posEingabe)->min;
            (strnum+m_niv.posEingabe)->wert = wert;
            break;
        case ZEICHENLISTE:
            if(m_niv.posEingabe > 0)
                    m_niv.posEingabe--;
            else
                    for(m_niv.posEingabe=0; (strzeichen+m_niv.posEingabe+1)->posCursor; 
                         m_niv.posEingabe++);
            break;
        default:
            break;
        }
        break;
    case BUTTONDOWN:
        switch (type) {
        case NUMERISCH:
            wert = (strnum+m_niv.posEingabe)->wert;
            wert--;
            if(wert < (strnum+m_niv.posEingabe)->min)
                    wert = (strnum+m_niv.posEingabe)->max;
            (strnum+m_niv.posEingabe)->wert = wert;
            break;
        case ZEICHENLISTE:
            if((strzeichen+m_niv.posEingabe+1)->posCursor)
                    m_niv.posEingabe++;
            else
                    m_niv.posEingabe = 0;
            break;
        default:
            break;
        }
        break;
    case BUTTONOK:
        if(m_niv.subniv2 && !m_niv.niv3)
        {
            VerwaltMenuSteuerung(button);
            ret = 0;
        }
        else
        {
            switch (type) {
            case NUMERISCH:
                if((strnum+m_niv.posEingabe+1)->posCursor)
                    m_niv.posEingabe++;
                break;
            case ZEICHENLISTE:
                break;
            default:
                break;
            }					
        }
        break;
    case BUTTONCANCEL:
        if(m_niv.subniv2 && !m_niv.niv3)
        {
            VerwaltMenuSteuerung(button);
            ret = 0;
        }
        else
        {		
            switch (type) {
            case NUMERISCH:
                if(m_niv.posEingabe > 0)
                        m_niv.posEingabe--;
                break;
            case ZEICHENLISTE:
                break;
            default:
                break;
            }					
        }
        break;
    case BUTTONOKHOLD:
        BeendeEingabe ();
        LCD_CursorBlinken (0);
        m_niv.posEingabe = 0;
        ret = 0;
        break;
    case BUTTONCANCELHOLD:
        LCD_CursorBlinken (0);
        m_niv.posEingabe = 0;
        ret =0;
        break;
    default:
        break;
    }

    switch(type) {
    case NUMERISCH:
        m_niv.posCursor = (strnum+m_niv.posEingabe)->posCursor;
        break;
    case ZEICHENLISTE:
        m_niv.posCursor = (strzeichen+m_niv.posEingabe)->posCursor;
        break;
    default:
        m_niv.posCursor = 0;
        break;
    }
    return ret;
}

int CLCDDisplay::VerwaltMenuSteuerung(int button)
{
    int ret = 0;

    switch(button) {
    case BUTTONOK:
        if(!m_niv.niv1)
        {
            m_niv.niv1= 1;
            m_niv.ID = Hauptmenu[m_niv.niv1-1].ID;
        }
        else
        {
            if(!m_niv.niv2 && Hauptmenu[m_niv.niv1-1].menu != NULL)
            {
                m_niv.niv2 = 1;
                if(Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].menu == NULL)
                    m_niv.ID = Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].ID;
                else
                    m_niv.ID = 0;
            }
            else
            {	
                if(!m_niv.subniv2 && Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].menu != NULL 
                        && Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].ID)
                {
                    m_niv.ID = Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].ID;
                    m_niv.subniv2 = m_niv.ID;
                    InitEingabe(m_niv.ID);
                    VerwaltMenuEingabe(0);
                    ret = 1;
                }
                else
                {
                    if(!m_niv.niv3 && Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].menu != NULL)
                    {
                            m_niv.niv3 = 1;
                            m_niv.ID = Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].menu[m_niv.niv3-1].ID;
                    }
                }
            }
        }
        break;
    case BUTTONCANCEL:
        if(m_niv.niv3)
        {
            m_niv.niv3 = 0;
            m_niv.ID = Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].ID;
            if(m_niv.ID) // kann im höheren Niveau nur != 0 sein wenn eine Nummereingabe erfolgt
                ret = 1;
        }
        else
        { 	
            m_niv.ID = 0; // da es auf einem höheren Menuniveau keine Anzeige geben kann
            if(m_niv.niv2)
            {
                if(m_niv.subniv2)
                    m_niv.subniv2 = 0;
                else
                    m_niv.niv2 = 0;
            }
            else
            {	
                if(m_niv.niv1)
                {
                    m_niv.niv1 = 0;
                    m_pIOGroup->SetHandAusg(false);
                    m_pIOGroup->ResetSimulation();
                }
            }
        }
        break;
    case BUTTONUP:
    case BUTTONDOWN:
        if(m_niv.niv3)
        {
            m_niv.niv3 = Blaettere(Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].menu, 
                                      m_niv.niv3, button);
            m_niv.ID = Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].menu[m_niv.niv3-1].ID;
        }
        else
        {
            if(m_niv.niv2)
            {
                m_niv.niv2 = Blaettere(Hauptmenu[m_niv.niv1-1].menu, m_niv.niv2, button);
                if(Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].menu == NULL)
                        m_niv.ID = Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].ID;
                else
                        m_niv.ID = 0;
            }
            else
            {
                if(m_niv.niv1)
                {
                    m_niv.niv1 = Blaettere(Hauptmenu, m_niv.niv1, button);
                    m_niv.ID = Hauptmenu[m_niv.niv1-1].ID;
                }
            }
        }			
        break;
    case BUTTONOKHOLD:
        if(m_niv.ID)
        {
            switch(m_niv.ID) {
                    // Eingabemodus wird nicht aktiviert
            case EWERKENNEN :
                break;
            default:
                StartEingabe(m_niv.ID);
                LCD_CursorBlinken(1);
                m_niv.posEingabe = 0;
                VerwaltMenuEingabe(0);	
                ret = 1;
                break;
            }
        }
        break;
    default:
        break;
    }

    return ret;
}

void CLCDDisplay::AnzeigeUhr()
{
    if(m_pUhr->SekundenTakt())
        LCD_AnzeigeUhr(m_pUhr->getUhrzeit (), 5, 1);
}

void CLCDDisplay::AnzeigeMenu()
{
    string str;

    if(m_niv.niv1 < 0 || m_niv.niv2 < 0 || m_niv.niv3 < 0 || m_niv.niv4 < 0)
        return;

    if(m_niv.niv4)
    {
        LCD_AnzeigeZeile1(Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].menu[m_niv.niv3-1].text);
        LCD_AnzeigeZeile2(Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].menu[m_niv.niv3-1].menu[m_niv.niv4-1].text);
    }
    else if(m_niv.niv3)
    {
        if(m_niv.subniv2)
        {
            str = string(Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].text) + "-Nr: " + to_string(strNummer[0].wert);
            LCD_AnzeigeZeile1(str.c_str());
            LCD_AnzeigeZeile2(Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].menu[m_niv.niv3-1].text);
        }
        else
        {	
            LCD_AnzeigeZeile1(Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].text);
            LCD_AnzeigeZeile2(Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].menu[m_niv.niv3-1].text);
        }
    }
    else
    {
        if(!m_niv.niv1)
        {
            LCD_AnzeigeZeile1("");
            AnzeigeUhr();
        }
        else
            LCD_AnzeigeZeile1((char *)Hauptmenu[m_niv.niv1-1].text);
        if(m_niv.subniv2)
        {
            LCD_AnzeigeZeile2((char *)Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].text);
            str = string(Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].text) + "-Nr: " + to_string(strNummer[0].wert);
            LCD_AnzeigeZeile2(str.c_str());
        }
        else
        {
            if(m_niv.niv2)
                LCD_AnzeigeZeile2((char *)Hauptmenu[m_niv.niv1-1].menu[m_niv.niv2-1].text);
            else
                LCD_AnzeigeZeile2((char *)"");
        }
    }
}

int CLCDDisplay::Blaettere(struct _menu *men, int niv, int button)
{
    int i;

    if(button == BUTTONDOWN)
    {
        if(!strcmp((men+niv)->text, ""))
           niv = 1;
        else
           niv++;
    }
    if(button == BUTTONUP)
    {
        if(niv > 1)
                niv--;
        else
        {
            for(i=0; ;i++)
            {
                if(!strcmp((men+i)->text, ""))
                    break;
            }
            niv = i;
        }
    }			

    return niv;
}

void CLCDDisplay::InitMenu(CIOGroup *pIOGroup, char *argv)
{
    struct stat st;
    struct tm tm;
    char text[50];

    m_pIOGroup = pIOGroup;

    if(stat(argv, &st) == 0)
    {	
        m_pUhr->getUhrzeitStruct(&tm, st.st_ctime);
        sprintf(text, "%d.%d.%d %d:%d", tm.tm_mday, tm.tm_mon+1, 
                tm.tm_year+1900, tm.tm_hour, tm.tm_min);
        m_pstrVersion = new char[strlen(text)+1];
        sprintf(m_pstrVersion, "%s", text);
    }
}

CLCDDisplay::CLCDDisplay()
{
    m_niv.niv1 = 0;
    m_niv.niv2 = 0;
    m_niv.subniv2 = 0;
    m_niv.niv3 = 0;
    m_niv.niv4 = 0;
    m_niv.ID = 0;
    m_niv.posCursor = 0;
    m_niv.posEingabe = 0;
    m_niv.eingErfolgt = 0;
    m_pstrVersion = NULL;
}
void CLCDDisplay::GetIPAddress(char *ptr, char *name)
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    sprintf(ptr, "not defined");;

    if(getifaddrs(&ifaddr) != -1)
    {
        for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if(ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
                continue;

            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host,
                            NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if(strcmp(ifa->ifa_name, name) == 0)
            {	
                sprintf(ptr, "  %s", host);
                break;
            }
        }
    }
}	

