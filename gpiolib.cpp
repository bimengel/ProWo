#include <dirent.h>
#include <assert.h>
#include <sys/mman.h>

#include "ProWo.h"

// Definitionen der I²C Schnittstelle
#define BSC1_C 		*(bsc1.addr + 0x00)
#define BSC1_S		*(bsc1.addr + 0x01)
#define BSC1_DLEN 	*(bsc1.addr + 0x02)
#define BSC1_A		*(bsc1.addr + 0x03)
#define BSC1_FIFO 	*(bsc1.addr + 0x04)
#define BSC1_DIV	*(bsc1.addr + 0x05)
#define BSC1_DEL	*(bsc1.addr + 0x06)
#define BSC1_CLKT	*(bsc1.addr + 0x07)
#define BSC_C_I2CEN 	(1 << 15)
#define BSC_C_INTR 		(1 << 10)
#define BSC_C_INTT		(1 << 9)
#define BSC_C_INTD		(1 << 8)
#define BSC_C_ST		(1 << 7)
#define BSC_C_CLEAR		(1 << 4)
#define BSC_C_READ		1
#define CLEAR_WRITE		BSC_C_CLEAR
#define START_READ 	BSC_C_I2CEN | BSC_C_ST | BSC_C_CLEAR | BSC_C_READ
#define START_WRITE BSC_C_I2CEN | BSC_C_ST
#define BSC_S_CLKT 		(1 << 9)
#define BSC_S_ERR		(1 << 8)
#define BSC_S_RXF		(1 << 7)
#define BSC_S_TXE		(1 << 6)
#define BSC_S_RXD		(1 << 5)
#define BSC_S_TXD 		(1 << 4)
#define BSC_S_RXR		(1 << 3)
#define BSC_S_TXW		(1 << 2)
#define BSC_S_DONE 		(1 << 1)
#define BSC_S_TA		1
#define CLEAR_STATUS	BSC_S_CLKT | BSC_S_ERR | BSC_S_DONE

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

struct bcm2835_peripheral gpio;
struct bcm2835_peripheral bsc1;
struct bcm2835_peripheral spi1;
struct bcm2835_peripheral uart;

int map_peripheral(struct bcm2835_peripheral *p);
void unmap_peripheral(struct bcm2835_peripheral *p);
void lcd_display_onoff(struct bcm2835_peripheral *spi, int on);
void lcd_return_home(struct bcm2835_peripheral *spi);
void lcd_clear_display(struct bcm2835_peripheral *spi);
void lcd_output_data(struct bcm2835_peripheral *spi, int ch);
void lcd_output_cmd(struct bcm2835_peripheral *spi, int ch, int delay);
void lcd_output(struct bcm2835_peripheral *spi, int ch, int delay);

//
//	Warten bis der SPI-Bus die Daten übertragen hat
//
void spi_wait_finish()
{
    int i;

    // warten bis Schreiben erfolgreich beendet
    for(i=0; i < 1000 && !(*(spi1.addr + SPI_CS) & SPI_CS_DONE); i++) 
        delayMicroseconds (10);
}

//
// 	TA : Transfer Active
//  CS wird aktiviert und schreiben in das FIFO-Register werden in TXFIFO
//		geleitet und gesendet
//
void spi_setTA(int state)
{ 
    int cs;

    cs = *(spi1.addr + SPI_CS);
    if(state)
        cs |= SPI_CS_TA;
    else
        cs &= ~SPI_CS_TA;
    *(spi1.addr + SPI_CS) = cs;
}

//
// SPI - Schnittstelle
void spiset_ChipSelect(int chipselect)
{
    int cs, i;

    // CSB chip select auf low
    cs = *(spi1.addr + SPI_CS) & 0xFFFFFFFC;
    cs |= chipselect | SPI_CS_CLEAR_RX | SPI_CS_CLEAR_TX;
    *(spi1.addr + SPI_CS) = cs;
    for(i=0; i < 20; i++)
        cs = *(spi1.addr + SPI_FIFO);
}

//
// LCD Anzeige
// Debian arbeitet mit den Zeichensätzen UTF-8 (Anzeige mit 'locale')
// Auf dem Raspberrypi ist par default en_GB eingestellt
// Dies bedeutet, dass die Buchstaben mit Umlauten in 2 bytes abgelegt 
// werden, die Zeichen bis 127 in einem 1 byte
// Das erste byte bei den Umlauten ist immer 195 (0xC2), das LCD arbeitet
// aber mit einem reinen 1 byte System. Aus diesem Grunde wird der Kode 195
// übersprungen und in lcd_output_data wird das korrekte Zeichen für das LCD
// eingesetzt
//
void LCD_Anzeige(int pos, int len, const char *cptr)
{
    int i;
    int cmd = 0x80 + pos -1;
    lcd_output_cmd(&spi1, cmd, 50);
    for(i=0; i < len; i++)
    {
        if(*cptr)
        {	
            if(*cptr == 195)
                i--;
            else
                lcd_output_data (&spi1, *cptr);
            cptr++;
        }
        else
            lcd_output_data(&spi1, ' ');
    }
}

void LCD_AnzeigeZeile1(const char *cptr)
{
    LCD_Anzeige(1, 16, cptr);
}

void LCD_AnzeigeZeile2(const char *cptr)
{
    LCD_Anzeige(17, 16, cptr);
}

void LCD_AnzeigeZeile3(const char *cptr)
{
    LCD_Anzeige(33, 16, cptr);
}

void LCD_CursorBlinken(int state)
{
    if(state)
        lcd_output_cmd(&spi1, 0x0F, 100);
    else
        lcd_output_cmd(&spi1, 0x0C, 100);
}

void LCD_SetCursorPos(int pos)
{
    int cmd = 0x80 + pos -1;
    lcd_output_cmd(&spi1, cmd, 50);
}

void LCD_AnzeigeUhr(time_t zeit, int pos, int art)
{
    struct tm t;
    char ptr[64];

    m_pUhr->getUhrzeitStruct(&t);
    switch(art) {
    case 1: // Uhrzeit
        sprintf(ptr, "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
        LCD_Anzeige (pos, strlen(ptr), ptr);
        break;
    case 2: // Datum	
        sprintf(ptr, "%02d/%02d/%4d", t.tm_mday, t.tm_mon+1, t.tm_year+1900);
        LCD_Anzeige (pos, strlen(ptr), ptr);
        break;
    default:
        break;
    }
}

// LCD DOGM163L-A
void LCD_Init()
{	
    int *ptr;
    int strInit[] = {	
        0x39, // Function set 0,1 8 bits
        0x1D, // Bias set
        0x78, // Contrast set 
        0x50, // Power control 
        0x6C, // Follower control
        0x38, // Function set 0,0 8 bits 
        0x06, // Entry mode
        0x00  // Enderkennung
    };

    for(ptr = strInit; *ptr; ptr++)
        lcd_output_cmd(&spi1, *ptr, 50);
    lcd_clear_display(&spi1);
    lcd_return_home(&spi1);
    lcd_display_onoff(&spi1, 1);
}	

void LCD_Clear()
{
    lcd_clear_display(&spi1);
}

void lcd_display_onoff(struct bcm2835_peripheral *spi, int on)
{
	
    if(on)
        // cmd = 0x0F, Wartezeit = 100µs
        lcd_output_cmd(spi, 0x0C, 100);
    else
        // cmd = 0x08, Wartezeit = 100µs
        lcd_output_cmd(spi, 0x08, 100);
}
void lcd_return_home(struct bcm2835_peripheral *spi)
{
    // cmd = 0x02, Wartezeit = 10ms
    lcd_output_cmd(spi, 0x02, 10000);
}

void lcd_clear_display(struct bcm2835_peripheral *spi)
{
    // cmd = 0x01, Wartezeit = 10ms
    lcd_output_cmd(spi, 0x01, 10000);
}

void lcd_output_data(struct bcm2835_peripheral *spi, int ch)
{
    // RS auf high
    switch_gpio(27, 1);	
    switch(ch) {
    case 0xA4: // ä
        ch = 0x84;
        break;
    case 0x84: // Ä
        ch = 0x8E;
        break;
    case 0xBC: // ü
        ch = 0x81;
        break;
    case 0x9C: // Ü
        ch = 0x9A;
        break;
    case 0xb6: 	// ö
        ch = 0x94;
        break;
    case 0x96: // Ö
        ch = 0x99;
        break;
    case 0xA9: // é
        ch = 0x82;
        break;
    case 0xA8: // è
        ch = 0x8A;
        break;
    case 0xAA: // ê
        ch = 0x88;
        break;
    case 0xA7: // ç
        ch = 0x87;
        break;
    case 0xA2: // â
        ch = 0x83;
        break;
    case 0xAE: // î
        ch = 0x8C;
        break;
    case 0xA0: // à
        ch = 0x85;
        break;
    default:
        break;
    }
    lcd_output(spi, ch, 0);
}
void lcd_output_cmd(struct bcm2835_peripheral *spi, int ch, int delay)
{
    // RS auf low
    switch_gpio(27, 0);	
    lcd_output(spi, ch, delay);
}

void lcd_output(struct bcm2835_peripheral *spi, int ch, int delay)
{
    // CSB chip select auf low
    spiset_ChipSelect (SPI_CS_CS0);
    spi_setTA (true);
    // Daten ausgeben
    *(spi->addr + SPI_FIFO) = ch;
    // warten bis Schreiben erfolgreich beendet
    spi_wait_finish ();
    spi_setTA(false);
    if(delay)
        delayMicroseconds (delay);
    else
        delayMicroseconds(10);
    // CSB chip select auf High schalten
}

void setmode_gpio(int pin, int mode)
{
    *(gpio.addr +(pin/10)) &= ~(7 << (pin %10)*3);
    *(gpio.addr +(pin/10)) |= (mode << (pin%10)*3);
}

void seteventdetect_gpio(int pin, int mode)
{
    int idx;
	
    pin &= 63;
    idx = 1 << (pin%32);
    *(gpio.addr + 19 + (pin/32)) &= ~idx;
    *(gpio.addr + 22 + (pin/32)) &= ~idx;
    *(gpio.addr + 25 + (pin/32)) &= ~idx;
    *(gpio.addr + 28 + (pin/32)) &= ~idx;

    switch(mode) {
    case RISINGEDGE:
        *(gpio.addr + 19 + (pin/32)) |= idx;
        break;
    case FALLINGEDGE:
        *(gpio.addr + 22 + (pin/32)) |= idx;
        break;	
    case HIGHDETECT:
        *(gpio.addr + 25 + (pin/32)) |= idx;
        break;
    case LOWDETECT:
        *(gpio.addr + 28 + (pin/32)) |= idx;
        break;
    }
}
int geteventdetect_gpio(int pin)
{
    int idx, ret = 0;

    pin &= 63;
    idx = 1 << pin % 32;
    if(*(gpio.addr + 16 + pin/32)) // & idx) JEN
    {	ret = 1;
        *(gpio.addr + 16 + pin/32) |= idx;
    }
    return ret;
}

// Ausgang anstellen
void switch_gpio(int pin, int val)
{
    if (val) 
        *(gpio.addr+7) = 1<<pin;
    else 
     *(gpio.addr+10) = 1<<pin;
    delayMicroseconds(10);
} 

// switch_gpio

int check_gpio(int pin)
{
  return (*(gpio.addr + 13) & ( 1 << pin )) >> pin;
} // check_gpio

/*
 * set_pullUpDown_gpio:
 *        Control the internal pull-up/down resistors on a GPIO pin
 *        The Arduino only has pull-ups and these are enabled by writing 1
 *        to a port when in input mode - this paradigm doesn't quite apply
 *        here though.
 *********************************************************************************
 */

void setpullUpDown_gpio (int pin, int pud)
{
    pin &= 63 ;
    pud &=  3 ;

    *(gpio.addr + 37) = pud ;
    delayMicroseconds (5) ;
    *(gpio.addr + 38 + (pin/32)) = 1 << (pin & 31) ;
    delayMicroseconds (5) ;
    *(gpio.addr + 37) = 0 ;
    delayMicroseconds (5) ;
    *(gpio.addr + 38 + (pin/32)) = 0 ;
    delayMicroseconds (5) ;
}

int init_gpio (int on, int iRaspberry)
{
    int iPeriBase, iFrequenz;
    if(on)
    {	// GPIO initialisieren
        switch(iRaspberry) {
            case 3:
                iPeriBase = 0x3F000000;
                iFrequenz = 250000;
                break;
            default:
                iPeriBase = 0x20000000;
                iFrequenz = 150000;
                break;
        }
        gpio.addr_p = iPeriBase + 0x200000;
        if(map_peripheral(&gpio) == -1)
            return -1;

        // I²C Bus initialisieren
        bsc1.addr_p = iPeriBase + 0x804000;
        if(map_peripheral(&bsc1) == -1)
            return -1;
        /* BSC1 is on GPIO 2 & 3 */
        setmode_gpio(2, ALT0);
        setmode_gpio(3, ALT0);

        /* I²C Frequenz auf 100 kHz setzen */
        // Die Clock Frequenz beträgt 150 MHz
        // Der default Wert beträgt 1500 und entspricht 100 kHz
        // Delay Dauer der Daten nach der SCL Flanke auf die Hälfte der Periode
        // 10 kHz über 102m bei Intec getestet (29/04/14)
        // Raspberry PI 2 : 150000/Frequenz
        //  Raspberry PI 3 B+ beträgt die Clockfrequenz 250 MHz
        int div;
        div = iFrequenz/20; // 20 kHz 
        // 10kHz eingestellt sonst Probleme bei beiden RTRM08 Bausteinen in Afst
        // 20kHz 31.09.19 Test mit MainboardV2.0 (50 kHz ging nicht mehr !!!!)
        BSC1_DIV = div;
        div /= 4;
        div = div * 0x10000 + div;
        BSC1_DEL = div;
        BSC1_CLKT = 200;
        //  BSC1_DEL = 0x300030;
        //  BSC1_CLKT = 64;

        // SPI Bus initialisieren
        spi1.addr_p = iPeriBase + 0x204000;
        if(map_peripheral(&spi1) == -1)
            return -1;
        setmode_gpio(7, ALT0);  // SPI_CE1 RS485
        setmode_gpio(8, ALT0);  // SPI_CE0 LCD
        setmode_gpio(9, ALT0);  // SPI_MISO
        setpullUpDown_gpio(9, PUD_UP);
        setmode_gpio(10, ALT0); // SPI_MOSI
        setpullUpDown_gpio(10, PUD_UP);
        setmode_gpio(11, ALT0); // SPI_SCLK
        setpullUpDown_gpio(11, PUD_UP);
        setmode_gpio(27, OUTPUT);
        // SPI_CS_LEN_LONG = 1 : write a single byte to FIFO
        *(spi1.addr + SPI_CS) = SPI_CS_LEN_LONG;
        // CLOCK der SPI Schnittstelle
        // = 250 MHz / eine Zahl zwischen 2 und 65536 (mehrfaches von 2!!)
        //  DIV  FREQUENZ   DAUER Anzeige der Uhr 
        //   50  5 MHz  	0.36msek  Probleme bei verschiedenen LCD-Anzeigen
        //  250  1 MHz      0.36msek
        //  500  500 kHz	0.45msek
        // 2500  100 kHz	1.1msek
        // Die Dauer wird nicht nur von der Frequenz doch auch von Wartezeiten
        // bestimmt. 500 kHz ist ein sinnvoller Kompromiss
        *(spi1.addr + SPI_CLK) = 500;

        // UART initialisieren
        uart.addr_p = iPeriBase + 0x201000;
        if(map_peripheral(&uart) == -1)
            return -1;
        setmode_gpio(17, OUTPUT);
        setmode_gpio(14, ALT0);   // TXD
        setmode_gpio (15, ALT0);  // RXD

    }
    else
    {	unmap_peripheral(&gpio);
        unmap_peripheral(&bsc1);
        unmap_peripheral(&spi1);
        unmap_peripheral(&uart);
    }
    return 0;
}

void unmap_peripheral(struct bcm2835_peripheral *p)
{
    if(p != NULL)
    {	munmap(p->map, BLOCK_SIZE);
        close(p->mem_fd);
        p = NULL;
    }
}

int map_peripheral(struct bcm2835_peripheral *p)
{
    char str[100];

    /* open /dev/mem */
    if ((p->mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) 
    {
        sprintf(str, "can't open /dev/mem try checking permissions\n");
        syslog(LOG_ERR, str);		
        return (-1);
    }

    // mmap GPIO 
    // Allocate MAP block

    p->map = mmap(
        NULL,
        BLOCK_SIZE,
        PROT_READ|PROT_WRITE,
        MAP_SHARED,
        p->mem_fd,
        p->addr_p
    );

    if (p->map == MAP_FAILED) 
    {
        sprintf(str, "mmap error %d\n", (int)mmap);
        syslog(LOG_ERR, str);		
        return (-1);
    }

    // Always use volatile pointer!
    p->addr = (volatile unsigned *)p->map;

    return 0;
   
} // setup_io


/*
 * JEN übernommen aus wiringPi.c
 * delayMicroseconds:
 *        This is somewhat intersting. It seems that on the Pi, a single call
 *        to nanosleep takes some 80 to 130 microseconds anyway, so while
 *        obeying the standards (may take longer), it's not always what we
 *        want!
 *
 *        So what I'll do now is if the delay is less than 100uS we'll do it
 *        in a hard loop, watching a built-in counter on the ARM chip. This is
 *        somewhat sub-optimal in that it uses 100% CPU, something not an issue
 *        in a microcontroller, but under a multi-tasking, multi-user OS, it's
 *        wastefull, however we've no real choice )-:
 *
 *      Plan B: It seems all might not be well with that plan, so changing it
 *      to use gettimeofday () and poll on that instead...
 *********************************************************************************
 */

void delayMicrosecondsHard (unsigned int howLong)
{
    struct timeval tNow, tLong, tEnd ;

    gettimeofday (&tNow, NULL) ;
    tLong.tv_sec  = howLong / 1000000 ;
    tLong.tv_usec = howLong % 1000000 ;
    timeradd (&tNow, &tLong, &tEnd) ;

    while (timercmp (&tNow, &tEnd, <))
        gettimeofday (&tNow, NULL) ;
}

void delayMicroseconds (unsigned int howLong)
{
    struct timespec sleeper ;
                
    if (howLong ==   0)
        return ;
    else if (howLong  < 100)
        delayMicrosecondsHard (howLong) ;
    else
    {
        sleeper.tv_sec  = 0 ;
        sleeper.tv_nsec = (long)(howLong * 1000) ;
        nanosleep (&sleeper, NULL) ; 
    }
}

// Gibt 1 zurück wenn die Operation korrekt beendet wurde !!
int wait_i2c_done()
{
    int timeout = 500;
    while(!(BSC1_S & BSC_S_DONE) && --timeout)
        usleep(100);
    if(timeout == 0)
        return 0;
    else
        return 1;
}

int BSC_ReadReg(int len, int addr, int reg)
{
    BSC_Write(1, addr, reg);
    return BSC_Read(len, addr);
}

int BSC_Read(int len, int addr)
{
    int i, ret=0;

    BSC1_A = addr;
    BSC1_DLEN = len;
    BSC1_S = CLEAR_STATUS;
    // Lesen bis das Empfangsregister leer ist !!
    BSC1_C = START_READ;
    ret = wait_i2c_done();
    for(i=0, ret=0; i < len; i++)
    {	ret *= 256;
        ret += (BSC1_FIFO & 0xFF);
    }
    return ret;
}

int BSC_Write(int len, int addr, int value)
{	
    int i;

    // Status Register initialisieren
    BSC1_S = CLEAR_STATUS;
    // Control Register clear FIFO
    BSC1_C = CLEAR_WRITE;
    BSC1_DLEN = len;
    BSC1_A = addr; 
    for(i=0; i < len; i++)
    {	BSC1_FIFO = value & 0xff;
        value /= 256;
    }
    BSC1_C = START_WRITE;
    return(wait_i2c_done());
}

int BSC_WriteString(int len, int addr, unsigned char *ptr, int reg)
{	
    int i;
    // Status Register initialisieren
    BSC1_S = CLEAR_STATUS;
    // Control Register clear FIFO
    BSC1_C = CLEAR_WRITE;
    BSC1_A = addr; 
    BSC1_DLEN = len + 1;
    BSC1_FIFO = reg;
    for(i=0; i < len; i++)
        BSC1_FIFO = *(ptr + i);
    BSC1_C = START_WRITE;
    return(wait_i2c_done());
}

int BSC_WriteReg(int len, int addr, int value, int reg)
{	
    int i;

    // Status Register initialisieren
    BSC1_S = CLEAR_STATUS;
    // Control Register clear FIFO
    BSC1_C = CLEAR_WRITE;
    BSC1_A = addr; 
    BSC1_DLEN = len + 1;
    BSC1_FIFO = reg;
    for(i=0; i < len; i++)
    {	BSC1_FIFO = value & 0xff;
            value /= 256;
    }
    BSC1_C = START_WRITE;
    return(wait_i2c_done());
}	


CBoardAddr::CBoardAddr()
{
    Inh1 = 0;
    Addr2 = 0;
    Inh2 = 0;
    Addr3 = 0;
    Reg = 0;
}

// 
// Die Adressen werden je nach Mainboard (9544 od 9545) gespeichert
void CBoardAddr::SetAddr(int inh1, int addr2, int inh2, int addr3, int reg)
{
    int i;
    
    if(ext_iI2CSwitchType == 2)
    {
        i = inh1 & 0x03;
        inh1 = 1 << i;
        inh1 |= 0XF0;
        Addr1 = ADDRI2C9545;
    }
    else 
        Addr1 = ADDRI2C9544;
    Inh1 = inh1;
    Addr2 = addr2;
    if(inh2)
    {
        i = inh2 & 0x03;
        inh2 = 1 << i;
        inh2 |= 0XF0;
    }
    Inh2 = inh2;
    Addr3 = addr3;
    Reg = reg;
}


void CBoardAddr::setI2C_gpio()
{

    BSC_Write(1, Addr1, Inh1);
    BSC_Write (1, Addr2, Inh2);
}

string strZweiStellen(int i)
{
    string strRet;

    strRet = to_string(i);
    if(strRet.length() == 1)
        strRet.insert(0,"0"); 

    return strRet;
}

string strDblRunden(double dblValue, int iAnz)
{
    string strRet;

    strRet = to_string(dblValue);
    strRet = strRet.substr(0, strRet.find(".") + iAnz+1);

    return strRet;

}