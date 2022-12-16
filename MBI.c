#include "display_slot.h"
#include <intrins.h>

extern unsigned char idata rtc_ram[64];
#define BALLS 0


#ifdef USE_MBI

#define NO_LACH            -1

#define DATA_LACH          1
#define GLOBAL_LACH        3
#define READ_CONFIGURATION 5
#define EABLE_ERRORDETECT  7
#define READ_ERRORSTATUS   9
#define WRITE_CONFIG       11
#define RESET_PWM          13



union ctrl {
              struct control {
							 unsigned int bt0:1; // 0    -> timeout of gck 0 - diable 1 enable	
					     unsigned int bt1:1; // 0    -> thermal protection 0- disable  1-enble
					     unsigned int bt9:8; // 0xab -> current gain adjustment
					     unsigned int bta:1; // 1    -> pwm data sinhronization 0- auto  1- manual
					     unsigned int btb:1; // 0    -> pwm counter reset 0-disable 1- enable
					     unsigned int btc:1; // 1    -> 0 - 6 bit pwm 1 - 12 bit pwm
					     unsigned int btd:1; // 1    -> reserved bit
					     unsigned int bte:1; // 0    -> thermal eror flag 0 - ok readonly							 
					     unsigned int btf:1; // 0    -> Data loading 0- 15 time + 1 datalach  1- 16 time +1 datalach
              } control;
             unsigned int i;
					};
					
union  ctrl ctrl;
// default ctrl 0b0011011010101100 - >	0x36ac	
					
static void sendWord(unsigned int w,char le)
{
	
      w=w;
	    le=le;
#pragma asm
;	     CLR  mbiLE
;	     CLR  Dclk
	
	     MOV A,#10H
	     CLR C
	     SUBB A,R5
	     XCH  A,R5
	
		   MOV R4,#8	
		   MOV A,R6		   ;high byte of w
LOOP1:
		   RLC A
		   MOV sdi,C
		   SETB Dclk
		   DJNZ R5,SETLE1  ;le?
		   SETB mbiLE
SETLE1:	
       CLR  Dclk
		   DJNZ R4,LOOP1
		   
		   MOV R4,#8
		   MOV A,R7	   ;LOW BYTE OF W
LOOP2:		  
		   RLC A
		   MOV  sdi,C
		   SETB Dclk
		   DJNZ R5,SETLE2  ;le?
		   SETB mbiLE
SETLE2:	
       CLR  Dclk
		   DJNZ R4,LOOP2		   
       CLR  mbiLE
#pragma endasm

}

//THIS MBI NOT RECIEVE CONFIG OR I MISTAKE BUT 
// IS SET TO 16 + 1 datalach
void mbiInit()
 {
	 unsigned char c;
	 ctrl.i=0x36ac; // default control
   ctrl.control.btf=1;    
	 ctrl.control.bt9=0xff;
	 for (c=0;c<NUM_OF_MBI-1;c++)
	  {
			sendWord(ctrl.i,NO_LACH);
		}
	 sendWord(ctrl.i,WRITE_CONFIG);
 }
 
 
                                // YU     YD      G      RD     RU
//const unsigned int bright[5] = {0x2500,0x2500,0xcfff,0x1800,0x1800};
//const unsigned int bright[5] = {0xe000,0xe000,0x1000,0x0500,0x0500};
unsigned char xdata screen[NUM_OF_MBI*NUM_OF_COLUMNS*16];

void mbiSend()
 {
	 unsigned char c,d;
	 static column;
//	 unsigned int dat;
   unsigned char *(xdata tmp);
//   dat=bright[column]; 
	 tmp=&screen[column*NUM_OF_MBI*16];
	 for (d=16;d!=0;d--)
	  {
	   for (c=NUM_OF_MBI-1;c!=0;c--)
			  sendWord(((int) *tmp++)*0x100,NO_LACH);
		 sendWord(((int) *tmp++)*0x100,DATA_LACH);
		}
	 CNT=0;
	 _nop_();
	 _nop_();
	 CNT=1;
   _nop_();
	 _nop_();
	 CNT=0;
	 sendWord(((int) (*tmp))*0x100,GLOBAL_LACH);
	 _nop_();
	 _nop_();
	 CNT=1;
	 _nop_();
	 _nop_();
	 CNT=0;
	 if (column==4) 
				{
					column=0;
					RST=1;
					_nop_();
					_nop_();
					_nop_();
					column=0;
					RST=0;
				}
   else
	  column++;				
 }
 
const unsigned char code cc[10]={4,4,2,0,0,3,3,2,1,1};
                         //  a1 b1 c1 d1 e1 f1 g1 dd1
const unsigned char code cs1[8]={ 1, 0,13,12,11, 3, 4, 8};    
                        //  a2 b2 c2 d2 e2 f2 g2 dd2 
const unsigned char code cs2[8]={ 7,  6, 5, 9,10, 2,15,14};
//screen[NUM_OF_MBI*(16 * 4 /* col 1 */ + 0 /*seg 0*/) + 7 /* chip 7*/];

struct caddr {
                unsigned int address;
								unsigned char column;
								unsigned char row;
								unsigned char segment;
							} complicate;
void address()
 {
   unsigned int chip,col,seg;
						   
   chip = complicate.row/2;
   col=cc[complicate.column+5*(complicate.row&1)];
	 switch (complicate.column)
	  {
			case 0:
				seg=cs1[complicate.segment];
			break;
			case 1:
				seg=cs2[complicate.segment];
			break;
			case 2:
	       if (complicate.row&1)
	        seg=cs2[complicate.segment];
	       else
	        seg=cs1[complicate.segment];
			break;
			case 3:
				seg=cs1[complicate.segment];
			break;
			case 4:
				seg=cs2[complicate.segment];
			break;
		}
   complicate.address=NUM_OF_MBI*(16*col+seg)+chip;
}
 

 
const unsigned char code numbers[16]= {0xfc,0x60,0xda,0xf2,0x66,0xb6,0xbe,0xe0,0xfe,0xf6,0xfd,0,0,0,0};


void ptr1(char c,unsigned char k)
 {
	  unsigned char d;
	  if (c>=10)
	   d = numbers[c/10];
		else
		 d = numbers[c];

	  for (complicate.segment=0;complicate.segment<8;complicate.segment++)
	   {			 
 			 address();
			 if ((d&0x80)!=0)
				 screen[complicate.address]=k;
			 else
				 screen[complicate.address]=0;
			 d<<=1;
	   }
		 complicate.column++;
		 if (c<10)
			 return;	   
		 d = numbers[c%10];
	  for (complicate.segment=0;complicate.segment<8;complicate.segment++)
	   {			 
 			 address();
			 if ((d&0x80)!=0)
				 screen[complicate.address]=k;
			 else
				 screen[complicate.address]=0;
			 d<<=1;
	   }		 
		 return;
 }                         //0 1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36
 const char code colors[37]={2,0 ,3 ,0 ,3 ,0 ,3 ,0 , 3, 0 ,3 ,3 ,0 ,3 ,0 ,3 ,0 ,3 ,0 , 0,3 ,0 ,3 ,0 , 3,0 ,3 ,0 , 3,3 ,0 ,3 ,0 , 3,0 ,3 ,0}; 
	#define GCOLOR 255
	#define RCOLOR 90
	#define YCOLOR 170
	const char code intensity[]={RCOLOR,RCOLOR,GCOLOR,YCOLOR,YCOLOR}; 
	
 void ptr(char c)
 {
	    char ball;
	    ball=rtc_ram[BALLS+c];
	 
	    complicate.row=c;		
			complicate.column=0;		
			ptr1(0,0);
			ptr1(0,0);
			ptr1(0,0);
			ptr1(0,0);
			ptr1(0,0);	
     	if (ball == -1)
       return ;					
     complicate.column=colors[ball];
		  ptr1(ball,intensity[complicate.column]);
			 	 
	  return ;
 }

void test()
 {
	 unsigned char repeat,tst;
	 unsigned int r;
	 for (repeat=0;repeat<120;repeat++)
	 {
	   for (r=NUM_OF_MBI*NUM_OF_COLUMNS;r;)
	    {
				--r;
				complicate.row=r;
	      complicate.column=0;
	      ptr1(tst%10,intensity[complicate.column]);
	      complicate.column=1;
	      ptr1(tst%10,intensity[complicate.column]);
	      complicate.column=2;
	      ptr1(tst%10,intensity[complicate.column]);
	      complicate.column=3;
	      ptr1(tst%10,intensity[complicate.column]);
	      complicate.column=4;
	      ptr1(tst%10,intensity[complicate.column]);
		  }
		for (r=0;r<800;r++)
		{
		  WATCHDOG;
		  while (!TF2H); //800HZ refresh rate
		  TF2H=0;
		  mbiSend();
		}
		tst++;
	}
	 while(1);
 }	 
#endif