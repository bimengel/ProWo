#ifndef menu_h__
#define menu_h__

struct _menu {
    const char *text;
    struct _menu *menu;
    int ID;
    int Nummer;
};

struct _numerisch
{
    int wert;
    int min;
    int max;
    int posCursor;
};


struct _zeichen
{
    const char *ptr;
    int zeichenType;
    int zeichenAnzahl;
    int posCursor;
};

struct _niveau
{
    int niv1; 		// Niveau 1
    int niv2; 		// Niveau 2
    int subniv2; 	// Untermenu von Niveau 2 : Eingabe einer Nummer
    int niv3;		// Niveau 3
    int niv4; 		// Niveau 4 noch nicht integriert
    int ID;			// ID der Eingabe
    int posCursor;	// Position des Cursors
    int posEingabe; // Position der Eingabe
    int eingErfolgt;// wenn Eingabemodus aktif ist
};

class CLCDDisplay
{
protected:
    struct _niveau m_niv;
	
protected:
    CIOGroup *m_pIOGroup;
    char *m_pstrVersion;
    void AnzeigeStandardMenu(time_t uhrzeit);
    void AnzeigeMenu();
    void AnzeigeUhr();
    int Blaettere(struct _menu *men, int niv, int button);
    int VerwaltMenuSteuerung(int button);
    int VerwaltMenuEingabe(int button);
    void InitEingabe(int ID);
    void StartEingabe(int ID);
    void BeendeEingabe();
    void AnzeigeZeile3();

public:
    CLCDDisplay();
    void GetIPAddress(char *ptr, char *name);
    void InitMenu(CIOGroup *pIOGroup, char *argv);
    void VerwaltMenu(int button);
};

#endif  // menu_h__