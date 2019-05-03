/*
 * Sailing_timer_v2.c
 *
 * Created: 16.04.2019 14:01:44
 * Author : leskovolleg
 */ 

#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
//timer lib
#include "Timer.h"

void BuzzerSound(void);
void TimeCalc(void);
void Mode_select (void);
void DinamicInd(void);

uint8_t Mode = 0; //modes of operation: 0- NU, 1 - 3 min, 2 - 5 min, 3 - 7 min
uint8_t Segment = 0; //0-2 - segments counter
uint8_t StartFlag = 0;//start flag
uint8_t InProgressFlag = 0;//in-progress flag

//table of symbols
char SEGMENT[ ] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 0b01101101, 0b01111101,
0b00000111, 0b01111111, 0b01101111};
char Dot = 0b10000000;

int SimForInd[] = {0,0,0}; // characters for displaying on indicators

/*
	segments
	B0-B2
	
	buttons
	б0-б2
	C0 - start
	C1 - stop
	C2 - mode select
		
	аб4 - klaxon
*/
Time time1 = 0; //for time calculation 
Time time2 = 0; //for dynamic indication
Time time3 = 0;//for little buzzer 
Time time4 = 0;//for klaxon

Time ButtTime = 0;//for buttons delay

uint16_t Inittime1 = 180;
uint16_t Inittime2 = 300;
uint16_t Inittime3 = 420;

uint16_t TimeToCalc = 0;

//buzzer sound generation
ISR( TIMER2_COMPA_vect)
{
	TCNT2 = 0;
	PORTC^=(1<<PC5);
}

//flag for buzzer enable
uint8_t BuzzEnable = 0;

//buzzer counter
uint8_t BuzzCount = 0;

//klaxon enable flag
uint8_t ClaksonFlag = 0;
uint16_t ClakDelay = 1000;

//klaxon long signal flag
uint8_t ClakLong = 0;



int main(void)
{
    //sound pins
	DDRC = 0b00110000;
	
	//timer T2 for sound
	TCCR2B|=(1<<CS20)|(1<<CS21);
	OCR2A=48;
	
	//button settings C0-C3
	PORTC = 0b00000111;//pull-up enable
	
	//digit pins
	DDRB = 0b0000111;
	
	//segment settings
	DDRD = 0xFF;
	
	//enable global interrupt
	sei();
	
	//timers init
	Timer0_Init();
	Timer0_StartTimer(&time2);//dynamic indication
	Timer0_StartTimer(&ButtTime);//button delay timer
	Timer0_StartTimer(&time3);//buzzer 

		
    while (1) 
    {
		//mode select pin PC2
		if((~PINC & (1<<2))&&(InProgressFlag == 0))
		{
			if(Timer0_TimeIsOut(&ButtTime,200))
			{
				Mode_select();
				Timer0_StartTimer(&ButtTime);	
			}
		}		
			
		//start button C0
		if((~PINC & (1<<0))&&(Mode != 0))
		{
			if(Timer0_TimeIsOut(&ButtTime,200))
			{
				if(InProgressFlag == 0)
				{
				StartFlag = 1;
				Timer0_StartTimer(&ButtTime);
				}
				//enable klaxon
				Timer0_StartTimer(&time4);	
				ClaksonFlag = 1;				
			}
		}
		
		//stop button
		if(~PINC & (1<<1))
		{
			if(Timer0_TimeIsOut(&ButtTime,200))
			{
				InProgressFlag = 0;	
				Timer0_StartTimer(&ButtTime);
			}				
		}
		
		//if start button was pressed
		if(StartFlag == 1)
		{
			StartFlag = 0;
			InProgressFlag = 1;
			Timer0_StartTimer(&time1);//start time calculation
		}
		
		//in progress 
		if(InProgressFlag == 1)
		{			
			if(Timer0_TimeIsOut(&time1,1000))
			{
				TimeCalc();
			}				
		}
		
		//segment switching
		if(Timer0_TimeIsOut(&time2,3))
		{
			Timer0_StartTimer(&time2);	
			DinamicInd();
		}
		
		BuzzerSound();

    }
}

//timing function
void TimeCalc(void)
{
		TimeToCalc = TimeToCalc - 1;
		SimForInd[2] =TimeToCalc/60; // min
		SimForInd[1] =(TimeToCalc%60)/10; //0.1 min;
		SimForInd[0] =(TimeToCalc%60)%10; // sec
		
		//timer restart
		if(TimeToCalc == 0)
		{
			switch (Mode)
			{
				case 1: { TimeToCalc = Inittime1;  break;}
				case 2: { TimeToCalc = Inittime2;  break;}
				case 3: { TimeToCalc = Inittime3;  break;}
			}
		}
		Timer0_StartTimer(&time1);
}

//mode selection function
void Mode_select (void)
{
	if(Mode >= 3)
	{		
		Mode = 0;
	}
	Mode = Mode+1;
	
	switch (Mode)
	{
		case 1: { TimeToCalc = Inittime1;  break;}
		case 2: { TimeToCalc = Inittime2;  break;}
		case 3: { TimeToCalc = Inittime3;  break;}
	}
	SimForInd[2] =TimeToCalc/60; // min
	SimForInd[1] =(TimeToCalc%60)/10; //0.1 min;
	SimForInd[0] =(TimeToCalc%60)%10; // seё
}

void DinamicInd(void)
{	
	Segment = Segment +1;
	switch(Segment)
	{
		case 3:
		{
		//minutes
		PORTD = SEGMENT[SimForInd[0]]|Dot;
		PORTB = 0b00000001;//0
		Segment = 0;
		break;
		}		
	case 2:
		{
		//tens of seconds
		PORTD = SEGMENT[SimForInd[1]]|Dot;
		PORTB = 0b00000010;
		break; 
		}
	case 1:
		{
		//seconds
		PORTD = SEGMENT[SimForInd[2]]|Dot;
		PORTB = 0b00000100;
		break;
		}
	}		
}

//sounds function
void BuzzerSound(void)
{

	//time for the start buzzer signal
	if((TimeToCalc == 305)||(TimeToCalc == 245)||(TimeToCalc == 65)||(TimeToCalc == 5))
	{
		BuzzEnable = 1;
	}
	
	if(Mode == 1)
	{
		if(TimeToCalc == 120)
		{
			Timer0_StartTimer(&time4);
			ClaksonFlag = 1;
		}
	}
	
	//time for the klaxon signal
	if(((TimeToCalc == 300)||(TimeToCalc == 240)||(TimeToCalc == 60)||((SimForInd[0] == 0)&&(SimForInd[1] == 0)&&(SimForInd[2] == 0)))&&(InProgressFlag == 1))//preduprezhdenie,podgotovka,minuta,start
	{
		if(TimeToCalc == 60)
		{
			ClakLong = 1;
		}
		Timer0_StartTimer(&time4);
		ClaksonFlag = 1;		
	}
	// 2 minutes signal for 3 minutes mode
	if(Mode == 1)
	{
		if(TimeToCalc == 125)
		{
			BuzzEnable = 1;
		}
	}
	//count of the last 5 seconds before the horn signal
	if(BuzzEnable == 1)
	{				
		if(BuzzCount <= 6)
		{
			if(Timer0_TimeIsOut(&time3,500))
			{
			TIMSK1 |= (1<<OCIE1A)|(0<<TOIE1);
			TIMSK2|=1<<OCIE2A;
			}
			if(Timer0_TimeIsOut(&time3,1000))
			{
				BuzzCount = BuzzCount +1;
				TIMSK2&=~(1<<OCIE2A);
				PORTC&=~(1<<PC5);
				Timer0_StartTimer(&time3);				
			}			
		}
		else
		{
			TIMSK2&=~(1<<OCIE2A);
			PORTC&=~(1<<PC5);
			BuzzEnable = 0;
			BuzzCount = 0;			
		}		
		
		if(BuzzCount > 6)
		{
			BuzzCount = 0;
			BuzzEnable = 0;
		}
	}
		
		//klaxon long or short signal
		if(ClaksonFlag == 1)
		{				
			if(ClakLong == 1)
			{
				ClakDelay = 3000;				
			}
			else
			{
				ClakDelay = 1000;
			}
			
			if(Timer0_TimeIsOut(&time4,ClakDelay))
			{
				PORTC &=~(1<<PC4);
				ClaksonFlag = 0;
				ClakLong = 0;
			}
			else
			{
				PORTC |=1<<PC4;
			}			
		}		
}