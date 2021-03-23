/***************************************************
//Web: http://www.buydisplay.com
EastRising Technology Co.,LTD
****************************************************/

#include "ER-EPD0583-1.h"
#include "Debug.h"

/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_0583_1_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(200);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(10);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(200);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
static void EPD_0583_1_SendCommand(UBYTE Reg)
{
    DEV_Digital_Write(EPD_DC_PIN, 0);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Reg);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void EPD_0583_1_SendData(UBYTE Data)
{
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Data);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
void EPD_0583_1_WaitUntilIdle(void)
{
    Debug("e-Paper busy\r\n");
    while(DEV_Digital_Read(EPD_BUSY_PIN) == 0) {
        DEV_Delay_ms(10);
    }
    DEV_Delay_ms(200);      
    Debug("e-Paper busy release\r\n");
}

/******************************************************************************
function :	Turn On Display
parameter:
******************************************************************************/
static void EPD_0583_1_TurnOnDisplay(void)
{
    EPD_0583_1_SendCommand(0x12); // DRF: Display Refresh Command
    DEV_Delay_ms(100);
    EPD_0583_1_WaitUntilIdle(); // wait for the electronic paper IC to release the idle signal

}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
UBYTE EPD_0583_1_Init(void)
{
    EPD_0583_1_Reset();

       //Fiti Command
        EPD_0583_1_SendCommand(0xF8);  // Enter FITI Command
        EPD_0583_1_SendData(0x60); 
        EPD_0583_1_SendData(0xA5); 
               
        EPD_0583_1_SendCommand(0xF8);  // Enter FITI Command
        EPD_0583_1_SendData(0x89); 
        EPD_0583_1_SendData(0xA5);      
        
        EPD_0583_1_SendCommand(0xF8);  // Sorting off
        EPD_0583_1_SendData(0xA1);
        EPD_0583_1_SendData(0x00);
        
        EPD_0583_1_SendCommand(0xF8);  //Vcom driving
        EPD_0583_1_SendData(0x73);
        EPD_0583_1_SendData(0x07);
        
        EPD_0583_1_SendCommand(0xF8);  
        EPD_0583_1_SendData(0x76);
        EPD_0583_1_SendData(0x1F);      
        
        EPD_0583_1_SendCommand(0xF8);   //boost constant on time
        EPD_0583_1_SendData(0x7E);
        EPD_0583_1_SendData(0x31); 
        
        EPD_0583_1_SendCommand(0xF8);  
        EPD_0583_1_SendData(0xB8);  
        EPD_0583_1_SendData(0x80);  
        
        EPD_0583_1_SendCommand(0xF8);   //vgl=> GND:08, HZ:00[default]
        EPD_0583_1_SendData(0x92);  
        EPD_0583_1_SendData(0x00);  
        
        EPD_0583_1_SendCommand(0xF8);   //VCOM(2frme off)=> GND:01(0x88=06)[default], HZ:11  
        EPD_0583_1_SendData(0x87);  
        EPD_0583_1_SendData(0x01);
        
        EPD_0583_1_SendCommand(0xF8);   //r_vcom_init_sel
        EPD_0583_1_SendData(0x88);  
        EPD_0583_1_SendData(0x06);        
        
        EPD_0583_1_SendCommand(0xF8);   
        EPD_0583_1_SendData(0xA8);  
        EPD_0583_1_SendData(0x30);


    EPD_0583_1_SendCommand(0x04); // PON: Power ON Command
    EPD_0583_1_WaitUntilIdle();

    EPD_0583_1_SendCommand(0x00); // PSR: Panel setting Register
    EPD_0583_1_SendData(0x4F); // 0100 1111 4F
    // resolution: 640x480
    // use otp lut
    // three colors
    // scan up
    // shift right
    // booster on
    
    EPD_0583_1_SendCommand(0x30); // OSC: OSC control Register
    EPD_0583_1_SendData(0x3C); // 0011 1100
    // framerate: 50hz

    EPD_0583_1_SendCommand(0x61); // TRES: Resolution setting
    EPD_0583_1_SendData(0x02); // 0000 0010
    EPD_0583_1_SendData(0x88); // 1000 1000
    // horizontal res: 648
    EPD_0583_1_SendData(0x01); // 0000 0001
    EPD_0583_1_SendData(0xE0); // 1110 0000
    // vertical res: 480


    return 0;
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void EPD_0583_1_Clear(void)
{
    UWORD Width, Height;
    Width =(EPD_0583_1_WIDTH % 8 == 0)?(EPD_0583_1_WIDTH / 8 ):(EPD_0583_1_WIDTH / 8 + 1);
    Height = EPD_0583_1_HEIGHT;

    UWORD i;
    EPD_0583_1_SendCommand(0x10); // DTM1: Data Start transmission 1 Register
    for(i=0; i<Width*Height; i++) {
        EPD_0583_1_SendData(0xff);

    }
    EPD_0583_1_SendCommand(0x13); // DTM2: Data Start transmission 2 Register
    for(i=0; i<Width*Height; i++)	{
        EPD_0583_1_SendData(0x00);

    }
    EPD_0583_1_TurnOnDisplay();
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_0583_1_Display(const UBYTE *blackimage, const UBYTE *ryimage)
{
    UDOUBLE Width, Height,j,i;
    Width =(EPD_0583_1_WIDTH % 8 == 0)?(EPD_0583_1_WIDTH / 8 ):(EPD_0583_1_WIDTH / 8 + 1);
    Height = EPD_0583_1_HEIGHT;
  
    //send black data
    EPD_0583_1_SendCommand(0x10); // DTM1: Data Start transmission 1 Register
    for ( j = 0; j < Height; j++) {
        for ( i = 0; i < Width; i++) {
            EPD_0583_1_SendData(blackimage[i + j * Width]);
        }
    }

    //send red data
    EPD_0583_1_SendCommand(0x13); // DTM2: Data Start transmission 2 Register
    for ( j = 0; j < Height; j++) {
        for ( i = 0; i < Width; i++) {
            EPD_0583_1_SendData(~ryimage[i + j * Width]);
        }
    }

    EPD_0583_1_TurnOnDisplay();
}

void EPD_0583_1_SendPartialParams(const UDOUBLE x, const UDOUBLE y, const UDOUBLE w, const UDOUBLE h)
{
    EPD_0583_1_SendData(x >> 8);
    EPD_0583_1_SendData(0xF8 & x); // discard last 3 bits

    EPD_0583_1_SendData(y >> 8);
    EPD_0583_1_SendData(0xFF & y);

    EPD_0583_1_SendData(w >> 8);
    EPD_0583_1_SendData(0xF8 & w); // discard last 3 bits

    EPD_0583_1_SendData(h >> 8);
    EPD_0583_1_SendData(0xFF & h);
}

void EPD_0583_1_DisplayFast(const UBYTE *blackimage, const UBYTE *ryimage, const UDOUBLE x, const UDOUBLE w)
{
    UDOUBLE Width, Height,j,i;
    Width =(EPD_0583_1_WIDTH % 8 == 0)?(EPD_0583_1_WIDTH / 8 ):(EPD_0583_1_WIDTH / 8 + 1);
    Height = EPD_0583_1_HEIGHT;
  
    //send black data
    EPD_0583_1_SendCommand(0x10); // DTM1: Data Start transmission 1 Register
    for ( j = 0; j < Height; j++) {
        for ( i = 0; i < Width; i++) {
            EPD_0583_1_SendData(blackimage[i + j * Width]);
        }
    }

    //send red data
    EPD_0583_1_SendCommand(0x13); // DTM2: Data Start transmission 2 Register
    for ( j = 0; j < Height; j++) {
        for ( i = 0; i < Width; i++) {
            EPD_0583_1_SendData(~ryimage[i + j * Width]);
        }
    }
    
    //EPD_0583_1_SendCommand(0x12); // DRF: Display Refresh Command
    
    EPD_0583_1_SendCommand(0x16); // PDTM1: Partial Data Start transmission 1 Register
    EPD_0583_1_SendPartialParams(x, 0, w, EPD_0583_1_HEIGHT);
    DEV_Delay_ms(100);
    EPD_0583_1_WaitUntilIdle(); // wait for the electronic paper IC to release the idle signal
}

void EPD_0583_1_PartialDisplay(const UBYTE *blackimage, const UBYTE *ryimage, const UDOUBLE x, const UDOUBLE y, const UDOUBLE w, const UDOUBLE h)
{
    UDOUBLE Width = w;
    UDOUBLE Height = h;
    UDOUBLE j,i;
    
    // send black data
    EPD_0583_1_SendCommand(0x14); // PDTM1: Partial Data Start transmission 1 Register
    EPD_0583_1_SendPartialParams(x, y, w, h);
    for ( j = 0; j < Height; j++) {
        for ( i = 0; i < Width; i++) {
            EPD_0583_1_SendData(blackimage[i + j * Width]);
        }
    }
    
    EPD_0583_1_SendCommand(0x11); // DSP: Data Stop Command
    
    // send red data
    EPD_0583_1_SendCommand(0x15); // PDTM2: Partial Data Start transmission 2 Register
    EPD_0583_1_SendPartialParams(x, y, w, h);
    for ( j = 0; j < Height; j++) {
        for ( i = 0; i < Width; i++) {
            EPD_0583_1_SendData(~ryimage[i + j * Width]);
        }
    }
    
    EPD_0583_1_SendCommand(0x11); // DSP: Data Stop Command
    
    EPD_0583_1_SendCommand(0x16); // PDRF: Partial Display Refresh Command
    EPD_0583_1_SendPartialParams(x, y, w, h);
    DEV_Delay_ms(100);
    EPD_0583_1_WaitUntilIdle(); // wait for the electronic paper IC to release the idle signal
}

/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
void EPD_0583_1_Sleep(void)
{
    EPD_0583_1_SendCommand(0x02); // POF: Power OFF Command
    EPD_0583_1_WaitUntilIdle();

    EPD_0583_1_SendCommand(0x07); // DSLP: Deep Sleep
    EPD_0583_1_SendData(0xA5);

}
