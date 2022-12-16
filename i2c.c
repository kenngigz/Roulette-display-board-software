#include "display_slot.h"
#include <intrins.h>
#include <ctype.h>
#define KEYADDR 0x70


#define LONG_P_TIME 80
#define BLINKL   28
#define BLINKS   1
#define BLINKK   5
#define TIMEOUT 80
#define ROW 0xe8

extern unsigned char idata rtc_ram[64];
#define BALLS 0

unsigned char tim;
unsigned char pressed;
unsigned char key;
unsigned char LongPress;
const unsigned char code keytab[12]={'1','2','3','4','5','6','7','8','9','*','0','#'};
unsigned char blinks=BLINKS;
unsigned char blinkl=BLINKL;
int ball;
unsigned int timeout;

static unsigned char testpos;
extern unsigned char xdata screen[];

union pcf {
            struct r 
              {
                unsigned char col:3;
                unsigned char row0:1;
                unsigned char led:1;
                unsigned char row3:1;
                unsigned char row2:1;
                unsigned char row1:1;
              } r;            
              unsigned char c;            
            };

#ifdef USE_NRF24
uint8_t data_array[4];
void    nrf24_send(uint8_t* value);
void    nrf24_getData(uint8_t* dat);
void    test(); //test the LED screen return with reset
bit send_start;
#endif			
						
unsigned char ReadI2C(unsigned char address,char *dat,char num)
{
            STO = 0; // stop condition bit must be zero
            STA = 1; // manually clear STA
						while (!SI); // wait for start to complete					
            SMB0DAT = address  | 1; // A6..A0 + read
						STA = 0; // manually clear STA
            SI = 0; // continue
            while (!SI); // wait for completion
            if (!ACK) // if not acknowledged, stop
             {
							STO = 1; // stop condition bit
							SI = 0; // generate stop condition
							return 0; //return error
             }
						 
						while (num--)
						{
              
              SI = 0; // continue
              while (!SI); // wait for completion
					    *dat=SMB0DAT;
							ACK = (num!=0); // NACK, last byte
							dat++;
					//		ACK = (num!=0); // NACK, last byte
						}
           STO = 1; // stop condition bit
           SI = 0; // generate stop condition
           return 1; //success
}
#ifdef I2CKEY

union pcf kbd;

char write_kbd()
{
		 kbd.c|=ROW;
     STO=0; 
     STA=1;            //generate start
     while(!SI);
     SMB0DAT=KEYADDR;   //Write to keyboard
 
     STA=0;
     SI=0;
     while(!SI);

	  if (!ACK) // if not acknowledged, stop
      {
        STO = 1;    // stop condition bit
        SI = 0;    // generate stop condition
        return 0;  //No ACK
      }
	
     SMB0DAT=kbd.c;   
     SI=0;

    while(!SI);
    STO=1;
    SI=0;
    while(STO);	 
	  return 1;
}


#endif
void init_i2c()
 {
	  	TR0=1; //start timer0 for the I2C
	 
#ifdef I2CKEY
	   kbd.c|=ROW;
     kbd.r.col=0;      //select all colums
     kbd.r.led=0;
	   write_kbd();
#endif	    
 }
 
 
char KbdSetup()
{
  // initialize i2c interface
  tim=BLINKS;
  pressed=0;
  kbd.c|=ROW;
  kbd.r.col=0;  //select all colums
  kbd.r.led=1;
  ball=-1;
	return write_kbd();
}

void addBall(char b)
{
	char d;
	for (d=15;d>0;d--)
	 rtc_ram[BALLS+d]= rtc_ram[BALLS+d-1];
	rtc_ram[BALLS]=b;
	rtc_writeram();
  #ifdef USE_NRF24
   data_array[0] = 0x00;
   data_array[1] = 0xAA;
   data_array[2] = b;
   data_array[3] = -b;
   nrf24_send(data_array); 	
	 send_start=1;
  #endif
}

void delBall()
{
  char d;
	for (d=1;d&0xf;d++)
	 rtc_ram[BALLS+d-1]= rtc_ram[BALLS+d];
	rtc_ram[BALLS+15]=-1;
	rtc_writeram();
}

char KbdLoop()
{
	 char hlp;
	 
   if (ReadI2C(KEYADDR,&kbd.c,1)==0)
		 return 0;
 
   if ((kbd.c&ROW)!=ROW) //key pressed
      {
       if (!pressed)
        { 
          pressed=3;  //the time for realase prevent from double press
          LongPress=LONG_P_TIME;
          hlp=9; //row 3 default value
          if (!kbd.r.row0) hlp=0;
          if (!kbd.r.row1) hlp=3;
          if (!kbd.r.row2) hlp=6;
          // not nessery to check for row 3
          kbd.c|=ROW;
          kbd.r.col=5;    //select second column
					if (write_kbd()==0)
						return 0;
          if (ReadI2C(KEYADDR,&kbd.c,1)==0)
		        return 0;
         if ((kbd.c&ROW)==ROW) 
          { // not the second column 
            // check the third one
             kbd.r.col=3;
					   if (write_kbd()==0)
						   return 0;
             if (ReadI2C(KEYADDR,&kbd.c,1)==0)
		          return 0;
             if ((kbd.c&ROW)!=ROW)   
              hlp+=2;    //Third columno if not the default is first column          
          }
         else
          hlp+=1;
         //now in hlp we have the number of the key
         
         key=keytab[hlp]; // get the right key ASCII
         kbd.r.led=0; //switch led on
         tim=BLINKK;
         if (ball<0)
          {
            switch (ball) 
            {
               case -1:
                if (isdigit(key)) //first digit
                {
                  ball=key-'0';
                  if ( (ball==0) || (ball>3) ) //one digit ball
                   {
                   // Serial.print("Valide ball:");
                   // Serial.println(ball);
										addBall(ball);
                    ball=-1;
                   }
                }
                else
                 if (key=='*') //delete ball
                   ball=-2;
               break;
              case -2:            
                if (key=='#')
                 {
                //delete ball
                    ball=-1;
                    delBall();
                 }
               break;
              case -3: //setup
               switch (key)
                {
                   case '1':
                  //  Serial.println("Switch on/off display");
                   break;

	                      
                   case '2':
                   //  Serial.println("Clear balls");
									    hlp=16;
									    do {
												rtc_ram[BALLS-1+hlp]=-1;
											} while (--hlp);
                    break;

                   case '3':
                   // Serial.println("test");
                    test();
                   break; 

                   case '4':
                   // Serial.println("Switch up/down");
                   break;

                   case '5':
                   // Serial.println("Set brightnes");
                   break;

                   default:
                    ball=-1;
                }
               ball=-1;
              break;
            }
      
           timeout=TIMEOUT;
          }
         else 
          {
            if (isdigit(key)) //second digit
             {
              ball*=10;
              ball+=key-'0';
              if (ball<37) 
               {
								 addBall(ball);
 
               }
               ball=-1;
             }
            else
            {
              if (key=='#')
                 {
									  addBall(ball);
                            
                 }
               ball=-1;
            }
          }
  
        }   //First press
       else
        {   //the key is hold down
           pressed=3;
           if (LongPress!=0)
            {
              LongPress--;          
              if (LongPress==0)
               {

                 if (key=='#') //setup mode
                  {
                    ball=-3;
                    timeout=TIMEOUT; 
										//setup mode
                 
                  }
                 else
                  ball=-1; 
               }
            }
        }
      }
    else
     {
       if (pressed)
        {
          pressed--;
          //if pressed = 0 releaset
          
        }
     }
         
   kbd.r.col=0;
   kbd.c|=ROW;
   if (tim--==0)   
   {
     if (ball<0)
      {
       switch (ball)
        {
           case -2:
            tim=kbd.r.led?blinkl:blinks;
           break;
           case -3:
             tim=blinkl;
           break;
           default:
           tim=kbd.r.led?blinks:blinkl;
           break;
        
        }
      }
     else
      {
       tim=blinks;
      }
     if (timeout!=0)
        {
          if (--timeout==0)
           ball=-1; 
        }
      
     kbd.r.led=!kbd.r.led;
   }
   if (write_kbd()==0)
						return 0;
   return 1;
} 
