#include <SPI.h>

#define MISO 12
#define MOSI 13
#define SCK 14
#define CS 15
#define RESET 2
#define HRDY 4

#define bcm2835_gpio_write digitalWrite
#define bcm2835_spi_transfer SPI.transfer
#define bcm2835_gpio_lev digitalRead

//Built in I80 Command Code
#define IT8951_TCON_SYS_RUN      0x0001
#define IT8951_TCON_STANDBY      0x0002
#define IT8951_TCON_SLEEP        0x0003
#define IT8951_TCON_REG_RD       0x0010
#define IT8951_TCON_REG_WR       0x0011
#define IT8951_TCON_MEM_BST_RD_T 0x0012
#define IT8951_TCON_MEM_BST_RD_S 0x0013
#define IT8951_TCON_MEM_BST_WR   0x0014
#define IT8951_TCON_MEM_BST_END  0x0015
#define IT8951_TCON_LD_IMG       0x0020
#define IT8951_TCON_LD_IMG_AREA  0x0021
#define IT8951_TCON_LD_IMG_END   0x0022

//I80 User defined command code
#define USDEF_I80_CMD_DPY_AREA     0x0034
#define USDEF_I80_CMD_GET_DEV_INFO 0x0302
#define USDEF_I80_CMD_DPY_BUF_AREA 0x0037

//Rotate mode
#define IT8951_ROTATE_0     0
#define IT8951_ROTATE_90    1
#define IT8951_ROTATE_180   2
#define IT8951_ROTATE_270   3

//Pixel mode , BPP - Bit per Pixel
#define IT8951_2BPP   0
#define IT8951_3BPP   1
#define IT8951_4BPP   2
#define IT8951_8BPP   3

//Waveform Mode
#define IT8951_MODE_0   0
#define IT8951_MODE_1   1
#define IT8951_MODE_2   2
#define IT8951_MODE_3   3
#define IT8951_MODE_4   4

//Endian Type
#define IT8951_LDIMG_L_ENDIAN   0
#define IT8951_LDIMG_B_ENDIAN   1

//Auto LUT
#define IT8951_DIS_AUTO_LUT   0
#define IT8951_EN_AUTO_LUT    1

//LUT Engine Status
#define IT8951_ALL_LUTE_BUSY 0xFFFF

//-----------------------------------------------------------------------
// IT8951 TCon Registers defines
//-----------------------------------------------------------------------
//Register Base Address
#define DISPLAY_REG_BASE 0x1000               //Register RW access for I80 only

//Base Address of Basic LUT Registers
#define LUT0EWHR  (DISPLAY_REG_BASE + 0x00)   //LUT0 Engine Width Height Reg
#define LUT0XYR   (DISPLAY_REG_BASE + 0x40)   //LUT0 XY Reg
#define LUT0BADDR (DISPLAY_REG_BASE + 0x80)   //LUT0 Base Address Reg
#define LUT0MFN   (DISPLAY_REG_BASE + 0xC0)   //LUT0 Mode and Frame number Reg
#define LUT01AF   (DISPLAY_REG_BASE + 0x114)  //LUT0 and LUT1 Active Flag Reg

//Update Parameter Setting Register
#define UP0SR (DISPLAY_REG_BASE + 0x134)      //Update Parameter0 Setting Reg

#define UP1SR     (DISPLAY_REG_BASE + 0x138)  //Update Parameter1 Setting Reg
#define LUT0ABFRV (DISPLAY_REG_BASE + 0x13C)  //LUT0 Alpha blend and Fill rectangle Value
#define UPBBADDR  (DISPLAY_REG_BASE + 0x17C)  //Update Buffer Base Address
#define LUT0IMXY  (DISPLAY_REG_BASE + 0x180)  //LUT0 Image buffer X/Y offset Reg
#define LUTAFSR   (DISPLAY_REG_BASE + 0x224)  //LUT Status Reg (status of All LUT Engines)

#define BGVR      (DISPLAY_REG_BASE + 0x250)  //Bitmap (1bpp) image color table

//-------System Registers----------------
#define SYS_REG_BASE 0x0000

//Address of System Registers
#define I80CPCR (SYS_REG_BASE + 0x04)

//-------Memory Converter Registers----------------
#define MCSR_BASE_ADDR 0x0200
#define MCSR (MCSR_BASE_ADDR  + 0x0000)
#define LISAR (MCSR_BASE_ADDR + 0x0008)

typedef struct {
  uint16_t usPanelW;
  uint16_t usPanelH;
  uint16_t usImgBufAddrL;
  uint16_t usImgBufAddrH;
  uint16_t usFWVersion[8];   //16 Bytes String
  uint16_t usLUTVersion[8];   //16 Bytes String
} IT8951DevInfo;

typedef struct IT8951AreaImgInfo {
  uint16_t usX;
  uint16_t usY;
  uint16_t usWidth;
  uint16_t usHeight;
} IT8951AreaImgInfo;

typedef struct IT8951LdImgInfo {
  uint16_t usEndianType; //little or Big Endian
  uint16_t usPixelFormat; //bpp
  uint16_t usRotate; //Rotate mode
  uint32_t ulStartFBAddr; //Start address of source Frame buffer
  uint32_t ulImgBufBaseAddr;//Base address of target image buffer
} IT8951LdImgInfo;

IT8951DevInfo gstI80DevInfo;
uint8_t* gpFrameBuf;
uint32_t gulImgBufAddr;

#define SYS_REG_BASE 0x0000
#define I80CPCR (SYS_REG_BASE + 0x04)

#define USDEF_I80_CMD_GET_DEV_INFO 0x0302

#define IT8951_TCON_REG_WR       0x0011

uint8_t IT8951_Init()
{
  //bcm2835_spi_begin();
  SPI.begin(SCK, MISO, MOSI, CS);
  //SPI.begin();
  //bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);    //default
  //SPI.setBitOrder(MSBFIRST);
  //bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   //default
  //SPI.setDataMode(SPI_MODE0);
  //bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);    //default
  //SPI.setClockDivider(SPI_CLOCK_DIV8);

  // alternativ:
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));

  //bcm2835_gpio_fsel(CS, BCM2835_GPIO_FSEL_OUTP);
  pinMode(CS, OUTPUT);
  //bcm2835_gpio_fsel(HRDY, BCM2835_GPIO_FSEL_INPT);
  pinMode(HRDY, INPUT);
  //bcm2835_gpio_fsel(RESET, BCM2835_GPIO_FSEL_OUTP);
  pinMode(RESET, OUTPUT);

  bcm2835_gpio_write(CS, HIGH);

  printf("****** IT8951 ******\n");

  bcm2835_gpio_write(RESET, LOW);
  //bcm2835_delay(100);
  delay(1000); //NOTE 100ms like in specs is not enough
  bcm2835_gpio_write(RESET, HIGH);

  //Get Device Info
  GetIT8951SystemInfo(&gstI80DevInfo);

  Serial.print("W: ");
  Serial.print(gstI80DevInfo.usPanelW);
  Serial.print(" H: ");
  Serial.println(gstI80DevInfo.usPanelH);
  if (!gstI80DevInfo.usPanelW || !gstI80DevInfo.usPanelH) {
    return 1;
  }

  gulImgBufAddr = gstI80DevInfo.usImgBufAddrL | ((uint32_t)gstI80DevInfo.usImgBufAddrH << 16);

  //Set to Enable I80 Packed mode
  IT8951WriteReg(I80CPCR, 0x0001);

  //SPI.endTransaction();

  return 0;
}


//-----------------------------------------------------------
//Host controller function 1---Wait for host data Bus Ready
//-----------------------------------------------------------
void LCDWaitForReady()
{
  uint8_t ulData = bcm2835_gpio_lev(HRDY);
  while (ulData == 0)
  {
    ulData = bcm2835_gpio_lev(HRDY);
  }
}

//-----------------------------------------------------------
//Host controller function 2---Write command code to host data Bus
//-----------------------------------------------------------
void LCDWriteCmdCode(uint16_t usCmdCode)
{
  //Set Preamble for Write Command
  uint16_t wPreamble = 0x6000;

  LCDWaitForReady();

  bcm2835_gpio_write(CS, LOW);

  bcm2835_spi_transfer(wPreamble >> 8);
  bcm2835_spi_transfer(wPreamble);

  LCDWaitForReady();

  bcm2835_spi_transfer(usCmdCode >> 8);
  bcm2835_spi_transfer(usCmdCode);

  bcm2835_gpio_write(CS, HIGH);
}

//-----------------------------------------------------------
//Host controller function 3---Write Data to host data Bus
//-----------------------------------------------------------
void LCDWriteData(uint16_t usData)
{
  //Set Preamble for Write Data
  uint16_t wPreamble  = 0x0000;

  LCDWaitForReady();

  bcm2835_gpio_write(CS, LOW);

  bcm2835_spi_transfer(wPreamble >> 8);
  bcm2835_spi_transfer(wPreamble);

  LCDWaitForReady();

  bcm2835_spi_transfer(usData >> 8);
  bcm2835_spi_transfer(usData);

  bcm2835_gpio_write(CS, HIGH);
}

void LCDWriteNData(uint16_t* pwBuf, uint32_t ulSizeWordCnt)
{
  uint32_t i;

  uint16_t wPreamble  = 0x0000;

  LCDWaitForReady();

  bcm2835_gpio_write(CS, LOW);

  bcm2835_spi_transfer(wPreamble >> 8);
  bcm2835_spi_transfer(wPreamble);

  LCDWaitForReady();

  for (i = 0; i < ulSizeWordCnt; i++)
  {
    bcm2835_spi_transfer(pwBuf[i] >> 8);
    bcm2835_spi_transfer(pwBuf[i]);
  }

  bcm2835_gpio_write(CS, HIGH);
}

//-----------------------------------------------------------
//Host controller function 4---Read Data from host data Bus
//-----------------------------------------------------------
uint16_t LCDReadData()
{
  uint16_t wRData;

  uint16_t wPreamble = 0x1000;

  LCDWaitForReady();

  bcm2835_gpio_write(CS, LOW);

  bcm2835_spi_transfer(wPreamble >> 8);
  bcm2835_spi_transfer(wPreamble);

  LCDWaitForReady();

  wRData = bcm2835_spi_transfer(0x00); //dummy
  wRData = bcm2835_spi_transfer(0x00); //dummy

  LCDWaitForReady();

  wRData = bcm2835_spi_transfer(0x00) << 8;
  wRData |= bcm2835_spi_transfer(0x00);

  bcm2835_gpio_write(CS, HIGH);

  return wRData;
}

//-----------------------------------------------------------
//  Read Burst N words Data
//-----------------------------------------------------------
void LCDReadNData(uint16_t* pwBuf, uint32_t ulSizeWordCnt)
{
  uint32_t i;

  uint16_t wPreamble = 0x1000;

  LCDWaitForReady();

  bcm2835_gpio_write(CS, LOW);

  bcm2835_spi_transfer(wPreamble >> 8);
  bcm2835_spi_transfer(wPreamble);

  LCDWaitForReady();

  pwBuf[0] = bcm2835_spi_transfer(0x00); //dummy
  pwBuf[0] = bcm2835_spi_transfer(0x00); //dummy

  LCDWaitForReady();

  for (i = 0; i < ulSizeWordCnt; i++)
  {
    pwBuf[i] = bcm2835_spi_transfer(0x00) << 8;
    pwBuf[i] |= bcm2835_spi_transfer(0x00);
  }

  bcm2835_gpio_write(CS, HIGH);
}

//-----------------------------------------------------------
//Host controller function 5---Write command to host data Bus with aruments
//-----------------------------------------------------------
void LCDSendCmdArg(uint16_t usCmdCode, uint16_t* pArg, uint16_t usNumArg)
{
  uint16_t i;
  //Send Cmd code
  LCDWriteCmdCode(usCmdCode);
  //Send Data
  for (i = 0; i < usNumArg; i++)
  {
    LCDWriteData(pArg[i]);
  }
}

//-----------------------------------------------------------
//Host Cmd 1---SYS_RUN
//-----------------------------------------------------------
void IT8951SystemRun()
{
  LCDWriteCmdCode(IT8951_TCON_SYS_RUN);
}

//-----------------------------------------------------------
//Host Cmd 2---STANDBY
//-----------------------------------------------------------
void IT8951StandBy()
{
  LCDWriteCmdCode(IT8951_TCON_STANDBY);
}

//-----------------------------------------------------------
//Host Cmd 3---SLEEP
//-----------------------------------------------------------
void IT8951Sleep()
{
  LCDWriteCmdCode(IT8951_TCON_SLEEP);
}

//-----------------------------------------------------------
//Host Cmd 4---REG_RD
//-----------------------------------------------------------
uint16_t IT8951ReadReg(uint16_t usRegAddr)
{
  uint16_t usData;

  //Send Cmd and Register Address
  LCDWriteCmdCode(IT8951_TCON_REG_RD);
  LCDWriteData(usRegAddr);
  //Read data from Host Data bus
  usData = LCDReadData();
  return usData;
}
//-----------------------------------------------------------
//Host Cmd 5---REG_WR
//-----------------------------------------------------------
void IT8951WriteReg(uint16_t usRegAddr, uint16_t usValue)
{
  //Send Cmd , Register Address and Write Value
  LCDWriteCmdCode(IT8951_TCON_REG_WR);
  LCDWriteData(usRegAddr);
  LCDWriteData(usValue);
}

//-----------------------------------------------------------
//Host Cmd 6---MEM_BST_RD_T
//-----------------------------------------------------------
void IT8951MemBurstReadTrigger(uint32_t ulMemAddr , uint32_t ulReadSize)
{
  uint16_t usArg[4];
  //Setting Arguments for Memory Burst Read
  usArg[0] = (uint16_t)(ulMemAddr & 0x0000FFFF); //addr[15:0]
  usArg[1] = (uint16_t)( (ulMemAddr >> 16) & 0x0000FFFF ); //addr[25:16]
  usArg[2] = (uint16_t)(ulReadSize & 0x0000FFFF); //Cnt[15:0]
  usArg[3] = (uint16_t)( (ulReadSize >> 16) & 0x0000FFFF ); //Cnt[25:16]
  //Send Cmd and Arg
  LCDSendCmdArg(IT8951_TCON_MEM_BST_RD_T , usArg , 4);
}
//-----------------------------------------------------------
//Host Cmd 7---MEM_BST_RD_S
//-----------------------------------------------------------
void IT8951MemBurstReadStart()
{
  LCDWriteCmdCode(IT8951_TCON_MEM_BST_RD_S);
}
//-----------------------------------------------------------
//Host Cmd 8---MEM_BST_WR
//-----------------------------------------------------------
void IT8951MemBurstWrite(uint32_t ulMemAddr , uint32_t ulWriteSize)
{
  uint16_t usArg[4];
  //Setting Arguments for Memory Burst Write
  usArg[0] = (uint16_t)(ulMemAddr & 0x0000FFFF); //addr[15:0]
  usArg[1] = (uint16_t)( (ulMemAddr >> 16) & 0x0000FFFF ); //addr[25:16]
  usArg[2] = (uint16_t)(ulWriteSize & 0x0000FFFF); //Cnt[15:0]
  usArg[3] = (uint16_t)( (ulWriteSize >> 16) & 0x0000FFFF ); //Cnt[25:16]
  //Send Cmd and Arg
  LCDSendCmdArg(IT8951_TCON_MEM_BST_WR , usArg , 4);
}
//-----------------------------------------------------------
//Host Cmd 9---MEM_BST_END
//-----------------------------------------------------------
void IT8951MemBurstEnd(void)
{
  LCDWriteCmdCode(IT8951_TCON_MEM_BST_END);
}

//-----------------------------------------------------------
//Example of Memory Burst Write
//-----------------------------------------------------------
// ****************************************************************************************
// Function name: IT8951MemBurstWriteProc( )
//
// Description:
//   IT8951 Burst Write procedure
//
// Arguments:
//      uint32_t ulMemAddr: IT8951 Memory Target Address
//      uint32_t ulWriteSize: Write Size (Unit: Word)
//      uint8_t* pDestBuf - Buffer of Sent data
// Return Values:
//   NULL.
// Note:
//
// ****************************************************************************************
void IT8951MemBurstWriteProc(uint32_t ulMemAddr , uint32_t ulWriteSize, uint16_t* pSrcBuf )
{

  uint32_t i;

  //Send Burst Write Start Cmd and Args
  IT8951MemBurstWrite(ulMemAddr , ulWriteSize);

  //Burst Write Data
  for (i = 0; i < ulWriteSize; i++)
  {
    LCDWriteData(pSrcBuf[i]);
  }

  //Send Burst End Cmd
  IT8951MemBurstEnd();
}

// ****************************************************************************************
// Function name: IT8951MemBurstReadProc( )
//
// Description:
//   IT8951 Burst Read procedure
//
// Arguments:
//      uint32_t ulMemAddr: IT8951 Read Memory Address
//      uint32_t ulReadSize: Read Size (Unit: Word)
//      uint8_t* pDestBuf - Buffer for storing Read data
// Return Values:
//   NULL.
// Note:
//
// ****************************************************************************************
void IT8951MemBurstReadProc(uint32_t ulMemAddr , uint32_t ulReadSize, uint16_t* pDestBuf )
{
  //Send Burst Read Start Cmd and Args
  IT8951MemBurstReadTrigger(ulMemAddr , ulReadSize);

  //Burst Read Fire
  IT8951MemBurstReadStart();

  //Burst Read Request for SPI interface only
  LCDReadNData(pDestBuf, ulReadSize);

  //Send Burst End Cmd
  IT8951MemBurstEnd(); //the same with IT8951MemBurstEnd()
}

//-----------------------------------------------------------
//Host Cmd 10---LD_IMG
//-----------------------------------------------------------
void IT8951LoadImgStart(IT8951LdImgInfo* pstLdImgInfo)
{
  uint16_t usArg;
  //Setting Argument for Load image start
  usArg = (pstLdImgInfo->usEndianType << 8 )
          | (pstLdImgInfo->usPixelFormat << 4)
          | (pstLdImgInfo->usRotate);
  //Send Cmd
  LCDWriteCmdCode(IT8951_TCON_LD_IMG);
  //Send Arg
  LCDWriteData(usArg);
}
//-----------------------------------------------------------
//Host Cmd 11---LD_IMG_AREA
//-----------------------------------------------------------
void IT8951LoadImgAreaStart(IT8951LdImgInfo* pstLdImgInfo , IT8951AreaImgInfo* pstAreaImgInfo)
{
  uint16_t usArg[5];
  //Setting Argument for Load image start
  usArg[0] = (pstLdImgInfo->usEndianType << 8 )
             | (pstLdImgInfo->usPixelFormat << 4)
             | (pstLdImgInfo->usRotate);
  usArg[1] = pstAreaImgInfo->usX;
  usArg[2] = pstAreaImgInfo->usY;
  usArg[3] = pstAreaImgInfo->usWidth;
  usArg[4] = pstAreaImgInfo->usHeight;
  //Send Cmd and Args
  LCDSendCmdArg(IT8951_TCON_LD_IMG_AREA , usArg , 5);
}
//-----------------------------------------------------------
//Host Cmd 12---LD_IMG_END
//-----------------------------------------------------------
void IT8951LoadImgEnd(void)
{
  LCDWriteCmdCode(IT8951_TCON_LD_IMG_END);
}

void GetIT8951SystemInfo(void* pBuf)
{
  uint16_t* pusWord = (uint16_t*)pBuf;
  IT8951DevInfo* pstDevInfo;

  //Send I80 CMD
  LCDWriteCmdCode(USDEF_I80_CMD_GET_DEV_INFO);

  //Burst Read Request for SPI interface only
  LCDReadNData(pusWord, sizeof(IT8951DevInfo) / 2); //Polling HRDY for each words(2-bytes) if possible

  //Show Device information of IT8951
  pstDevInfo = (IT8951DevInfo*)pBuf;
  printf("Panel(W,H) = (%d,%d)\r\n",
         pstDevInfo->usPanelW, pstDevInfo->usPanelH );
  printf("Image Buffer Address = %X\r\n",
         pstDevInfo->usImgBufAddrL | (pstDevInfo->usImgBufAddrH << 16));
  //Show Firmware and LUT Version
  printf("FW Version = %s\r\n", (uint8_t*)pstDevInfo->usFWVersion);
  printf("LUT Version = %s\r\n", (uint8_t*)pstDevInfo->usLUTVersion);
}

//-----------------------------------------------------------
//Initial function 2---Set Image buffer base address
//-----------------------------------------------------------
void IT8951SetImgBufBaseAddr(uint32_t ulImgBufAddr)
{
  uint16_t usWordH = (uint16_t)((ulImgBufAddr >> 16) & 0x0000FFFF);
  uint16_t usWordL = (uint16_t)( ulImgBufAddr & 0x0000FFFF);
  //Write LISAR Reg
  IT8951WriteReg(LISAR + 2 , usWordH);
  IT8951WriteReg(LISAR , usWordL);
}

//-----------------------------------------------------------
// 3.6. Display Functions
//-----------------------------------------------------------

//-----------------------------------------------------------
//Display function 1---Wait for LUT Engine Finish
//                     Polling Display Engine Ready by LUTNo
//-----------------------------------------------------------
void IT8951WaitForDisplayReady()
{
  //Check IT8951 Register LUTAFSR => NonZero Busy, 0 - Free
  while (IT8951ReadReg(LUTAFSR));
}

//-----------------------------------------------------------
//Display function 2---Load Image Area process
//-----------------------------------------------------------
void IT8951HostAreaPackedPixelWrite(IT8951LdImgInfo* pstLdImgInfo, IT8951AreaImgInfo* pstAreaImgInfo)
{
  uint32_t i, j;
  //Source buffer address of Host
  uint16_t* pusFrameBuf = (uint16_t*)pstLdImgInfo->ulStartFBAddr;

  //Set Image buffer(IT8951) Base address
  IT8951SetImgBufBaseAddr(pstLdImgInfo->ulImgBufBaseAddr);
  //Send Load Image start Cmd
  IT8951LoadImgAreaStart(pstLdImgInfo , pstAreaImgInfo);
  //Host Write Data
  for (j = 0; j < pstAreaImgInfo->usHeight; j++)
  {
    for (i = 0; i < pstAreaImgInfo->usWidth / 2; i++)
    {
      //Write a Word(2-Bytes) for each time
      LCDWriteData(*pusFrameBuf);
      pusFrameBuf++;
    }
  }
  //Send Load Img End Command
  IT8951LoadImgEnd();
}


//-------------------------------------------------------------------------------------------------------------
//  Command - 0x0037 for Display Base addr by User
//  uint32_t ulDpyBufAddr - Host programmer need to indicate the Image buffer address of IT8951
//                                         In current case, there is only one image buffer in IT8951 so far.
//                                         So Please set the Image buffer address you got  in initial stage.
//                                         (gulImgBufAddr by Get device information 0x0302 command)
//
//-------------------------------------------------------------------------------------------------------------
void IT8951DisplayAreaBuf(uint16_t usX, uint16_t usY, uint16_t usW, uint16_t usH, uint16_t usDpyMode, uint32_t ulDpyBufAddr)
{
  //Send I80 Display Command (User defined command of IT8951)
  LCDWriteCmdCode(USDEF_I80_CMD_DPY_BUF_AREA); //0x0037

  //Write arguments
  LCDWriteData(usX);
  LCDWriteData(usY);
  LCDWriteData(usW);
  LCDWriteData(usH);
  LCDWriteData(usDpyMode);
  LCDWriteData((uint16_t)ulDpyBufAddr);       //Display Buffer Base address[15:0]
  LCDWriteData((uint16_t)(ulDpyBufAddr >> 16)); //Display Buffer Base address[26:16]
}

void IT8951_BMP_Example(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
  IT8951LdImgInfo stLdImgInfo;
  IT8951AreaImgInfo stAreaImgInfo;

  IT8951WaitForDisplayReady();

  //Setting Load image information
  stLdImgInfo.ulStartFBAddr    = (uint32_t)gpFrameBuf;
  stLdImgInfo.usEndianType     = IT8951_LDIMG_L_ENDIAN;
  stLdImgInfo.usPixelFormat    = IT8951_4BPP;
  stLdImgInfo.usRotate         = IT8951_ROTATE_0;
  stLdImgInfo.ulImgBufBaseAddr = gulImgBufAddr;
  //Set Load Area
  stAreaImgInfo.usX      = x;
  stAreaImgInfo.usY      = y;
  stAreaImgInfo.usWidth  = w;
  stAreaImgInfo.usHeight = h;

  //Load Image from Host to IT8951 Image Buffer
  IT8951HostAreaPackedPixelWrite(&stLdImgInfo, &stAreaImgInfo);//Display function 2
  //Display Area ?V (x,y,w,h) with mode 2 for fast gray clear mode - depends on current waveform
  //IT8951DisplayArea(0,0, gstI80DevInfo.usPanelW, gstI80DevInfo.usPanelH, 2);
}

void SPIWrite2Byte(uint16_t data) {
  bcm2835_spi_transfer(data >> 8);
  bcm2835_spi_transfer(data);
}

void IT8951LoadDataStart(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  //Set Image buffer(IT8951) Base address
  IT8951SetImgBufBaseAddr(gulImgBufAddr);

  //Send Load Image Cmd
  LCDWriteCmdCode(IT8951_TCON_LD_IMG_AREA);
  LCDWaitForReady();
  bcm2835_gpio_write(CS, LOW);
  SPIWrite2Byte(0x0000);
  LCDWaitForReady();

  SPIWrite2Byte((IT8951_LDIMG_B_ENDIAN << 8 ) | (IT8951_4BPP << 4) | IT8951_ROTATE_0);
  SPIWrite2Byte(x);SPIWrite2Byte(y);
  SPIWrite2Byte(w);SPIWrite2Byte(h);
  bcm2835_gpio_write(CS, HIGH);
  LCDWaitForReady();

  bcm2835_gpio_write(CS, LOW);
  SPIWrite2Byte(0x0000);
  LCDWaitForReady();
}

void IT8951LoadDataColor(uint8_t color) {
  bcm2835_spi_transfer(color);
}

void IT8951LoadDataColor(uint8_t* colors, uint16_t n) {
  for (int i = 0; i < n; i++)
    bcm2835_spi_transfer(colors[i]);
}

void IT8951LoadDataEnd() {
  bcm2835_gpio_write(CS, HIGH);
  LCDWriteCmdCode(IT8951_TCON_LD_IMG_END);
}

void IT8951DisplayArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t displayMode) {
  //Send I80 Display Command (User defined command of IT8951)
  LCDWriteCmdCode(USDEF_I80_CMD_DPY_AREA);
  LCDWriteData(x);
  LCDWriteData(y);
  LCDWriteData(w);
  LCDWriteData(h);
  LCDWriteData(displayMode);
}
