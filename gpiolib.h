#ifndef gpiolib_h__
#define gpiolib_h__

extern int init_gpio(int on, int iPeriBase);
extern void init_Uhr();
extern void setmode_gpio(int pin, int mode);
extern void setpullUpDown_gpio (int pin, int pud);
extern void seteventdetect_gpio(int pin, int mode);
extern int geteventdetect_gpio(int pin);
extern void switch_gpio(int pin, int val);
extern int check_gpio(int pin);
extern int setIC2_gpio(int nr);
extern int BSC_Read(int len, int addr);
extern int BSC_ReadReg(int len, int addr, int reg);
extern int BSC_Write(int len, int addr, int value);
extern int BSC_WriteReg(int len, int addr, int value, int reg);
extern int BSC_WriteString(int len, int addr, unsigned char *ptr, int reg);
extern void LCD_Init();
extern void LCD_Clear();
extern void LCD_CursorBlinken(int state);
extern void LCD_SetCursorPos(int pos);
extern void LCD_Anzeige(int pos, int len,const char *cptr);
extern void LCD_AnzeigeUhr(time_t zeit, int pos, int type);
extern void LCD_AnzeigeZeile1(const char *cptr);
extern void LCD_AnzeigeZeile2(const char *cptr);
extern void LCD_AnzeigeZeile3(const char *cptr);
extern void delayMicroseconds (unsigned int howLong);
extern void spi_wait_finish();
extern void spiset_ChipSelect(int cs);
extern void spi_setTA(int state);
extern string strZweiStellen(int i);
extern string strDblRunden(double dblValue, int iAnz);

struct bcm2835_peripheral {
	unsigned long addr_p;
	int mem_fd;
	void *map;
	volatile unsigned int *addr;
};


 /* Bitfields in CS */
#define SPI_CS_LEN_LONG         0x02000000
#define SPI_CS_DMA_LEN          0x01000000
#define SPI_CS_CSPOL2           0x00800000
#define SPI_CS_CSPOL1           0x00400000
#define SPI_CS_CSPOL0           0x00200000
#define SPI_CS_RXF              0x00100000
#define SPI_CS_RXR              0x00080000
#define SPI_CS_TXD              0x00040000
#define SPI_CS_RXD              0x00020000
#define SPI_CS_DONE             0x00010000
#define SPI_CS_LEN              0x00002000
#define SPI_CS_REN              0x00001000
#define SPI_CS_ADCS             0x00000800
#define SPI_CS_INTR             0x00000400
#define SPI_CS_INTD             0x00000200
#define SPI_CS_DMAEN            0x00000100
#define SPI_CS_TA               0x00000080
#define SPI_CS_CSPOL            0x00000040
#define SPI_CS_CLEAR_RX         0x00000020
#define SPI_CS_CLEAR_TX         0x00000010
#define SPI_CS_CPOL             0x00000008
#define SPI_CS_CPHA             0x00000004
#define SPI_CS_CS1              0x00000001
#define SPI_CS_CS0              0x00000000

#define SPI_TIMEOUT_MS  30000
#define SPI_MODE_BITS   (SPI_CPOL | SPI_CPHA | SPI_CS_HIGH | SPI_NO_CS)
/* SPI register offsets */
#define SPI_CS                  0x00	// Control and Status
#define SPI_FIFO                0x01	// TX and RX FIFOS
#define SPI_CLK                 0x02	// Clock Divider
#define SPI_DLEN                0x03	// Data Length
#define SPI_LTOH                0x04	// LOSSI mode TOH
#define SPI_DC                  0x05	// DMA DREQ Controls

// I²C Bus register

// set_mode_gpio(int pin, int mode, int eventdetect)
// mode
#define INPUT	0x0
#define OUTPUT	0x1
#define ALT0 	0x4
#define ALT1	0x5
#define ALT2	0x6
#define ALT3	0x7
#define ALT4	0x3
#define ALT5	0x2
// seteventdetect(int mode)
#define NONE		0
#define RISINGEDGE	1
#define FALLINGEDGE	2
#define HIGHDETECT	3
#define LOWDETECT	4

// set_pullUpDown_gpio(int pin, int pud)
#define PUD_OFF		0
#define PUD_DOWN	1
#define PUD_UP		2

class CBoardAddr
{	

public:
    CBoardAddr();
    void setI2C_gpio();
    void SetAddr(int inh1, int addr2, int inh2, int addr3, int reg); 
    char Addr1;
    char Inh1;				// Adresseninhalt (Control Regsiter) erster I2C-Switch			
    char Addr2;				// Adresse des zweiten I2C-Switch
    char Inh2;				// Adresseninhalt (Control Register) des zweiten I2C-Switch
    char Addr3;				// Adresse des Bausteins
    char Reg;				// Register
};

#endif  // gpiolib_h__
