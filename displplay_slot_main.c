#include "display_slot.h"
#include <intrins.h>
bit dir;
void mbiSend();
extern unsigned char xdata screen[];
extern unsigned char idata rtc_ram[64];
#define BALLS 0
#define START_TIME  17
#define FULL_TIME   21
void ptr(char);

#ifdef USE_NRF24
const uint8_t code tx_address[5] = {0xE7,0xE7,0xE7,0xE7,0xE7};
const uint8_t code rx_address[5] = {0xD7,0xD7,0xD7,0xD7,0xD7};
void    nrf24_init();
void    nrf24_rx_address(uint8_t* adr);
void    nrf24_tx_address(uint8_t* adr);
void    nrf24_config(uint8_t channel, uint8_t pay_length);
extern bit send_start;
extern uint8_t data_array[4];
uint8_t nrf24_isSending();
uint8_t nrf24_dataReady();
void    nrf24_powerUpRx();
void    nrf24_getData(uint8_t* dat);
uint8_t nrf24_lastMessageStatus();
void addBall(char b);
void    nrf24_send(uint8_t* value);
#endif

void main() 
{

  int	i;
	unsigned char d;
	char mb;
  
	Init_Device();
	WATCHDOG;
	//init MBI
	mbiLE=0;
	Dclk=0;
	sdi=0;						
	RST=1;
  _nop_();
  _nop_();
  _nop_();
	RST=0;
  _nop_();
  _nop_();
  _nop_();
	CNT=0;
	_nop_();
	_nop_();
  _nop_();
	CNT=1;
  _nop_();
	_nop_();
	_nop_();
	CNT=0;
	//switch off all segments while init
	
  mbiInit();
  init_i2c();
  rtc_init();
	
	// ------	
	WATCHDOG;
	for (i=0;i<(NUM_OF_MBI*NUM_OF_COLUMNS*16);i++)
	 screen[i]=0x00;
	CNT=0;
  RST=0;
	d=16;
	mb=0;
 #ifdef USE_NRF24
    /* init hardware pins */
  nrf24_init();
    
    /* Channel #2 , payload length: 4 */
  nrf24_config(2,4);

    /* Set the device addresses */
  nrf24_tx_address(tx_address);
  nrf24_rx_address(rx_address);   
   data_array[0] = 0x00;
   data_array[1] = 0xAA;
   data_array[2] = 5;
   data_array[3] = -5;
   nrf24_send(data_array);
   WATCHDOG;   
	 while(nrf24_isSending())
		  WATCHDOG;
	 mb=nrf24_lastMessageStatus(); //i can check the status
	 nrf24_powerUpRx();
 #endif
 if (!rtc_readram())
	{
		 //crc is not good
		
		for (d=0;d<37;d++)
		 {
		   rtc_ram[BALLS+d] = -1;
		  
		 }
		*((unsigned long *)(&rtc_ram[FULL_TIME]))=0;
	//	rtc_writeTime(0);
		rtc_writeram();
	}
	WATCHDOG;
  for (d=0;d<37;d++)
		 {
		   if (rtc_ram[BALLS+d]>36)
   				 rtc_ram[BALLS+d]= -1;
		   WATCHDOG;
		 }
 // *((unsigned long *)(&rtc_ram[START_TIME]))=rtc_readTime();
	TR2=1; //start timer 2 to 800Hz
	WATCHDOG;
	d=0;
	KbdSetup();
  led=0;
	while (1) {
		  WATCHDOG;
		  while (!TF2H); //800HZ refresh rate
		  TF2H=0;
		  mbiSend();		 
		 
		  if (d>16)
			{
				d=0;
#ifdef USE_NRF24				
				if (_testbit_(send_start))
				{
					if (!nrf24_isSending())
					{
						//transmith finished
						mb=nrf24_lastMessageStatus(); //i can check the status
						nrf24_powerUpRx(); //go to recieve mode
						
					}
				}
				else
				{
					if(nrf24_dataReady())
					{
						nrf24_getData(data_array); 
						if ( (data_array[0]==0) && (data_array[1]==0xaa) && (data_array[2]==-data_array[3]))
						{
							 //ball recieved from external
							 led=!led;
						//	 addBall(data_array[2]);
						}
					}
				}
#endif			
				
				KbdLoop();
			}
			else
       ptr(d&0xf);
			d++;
		
	}

}