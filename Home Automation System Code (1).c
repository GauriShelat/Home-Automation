
#include <LPC23xx.h>
#include <math.h>
#include <string.h>

//Uart GSM function declarations
char *fire = "FIRE ALARM";
char *gasleak = "GAS LEAK ALARM";
char *burglar = "BURGLAR ALARM";

int msg_sent=0;
unsigned char *rcvd;

void status_ok(void);
void UartGSMSend(char *msg);
void GsmMsgSend(char *msg);
unsigned char getcharacter(void);
void Uart_terminate(void);
void UnLockGate (void);
void LockGate (void);
//--------------------------------------------------------//

// lock and unlock gate funtions//
void LockGate(void);
void UnLockGate(void);
//---------------------------------------------------------//

// Generic DELAY for all modules
void DELAY(int delayinterval) {     
	unsigned int X = delayinterval;
  while(X--);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//ISR AND FUNCTIONS FOR COUNTER
void Counter0(void);
void Counter1(void);

int Fcounter0 = 0;
int Fcounter1 = 0;
int Blockingmode = 0;
int numpeople = 0;
/*
	1)check if isr is from counter 0 or counter 1
	2)if object is trying to enter
		1)ISR will be due to counter 0
		2)counter0 will check if Fcounter1 and Fcounter0 are both0 and then sets Fcounter0
		3)when object crosses next sensor isr due to Counter1 will be raised
		4)Counter1 checks if Fcounter0 is 1 and Blocking is 0 then calls functions to switch on lightsetc
*/		
//TO DO : ADD FUNCTIONS TO RAISE ALARM SWITCH ON/OFF LIGHTS
__irq void isr_counter (void) {
    if((T0IR & 0x10) == 0x10) {
			//counter 0
			Counter0();	
			T0IR  = 0x10;
			VICAddress = 0;
			return;
    }                                
    
	else if((T0IR & 0x20) == 0x20) {
		//counter 1
		Counter1();
    T0IR = 0x20;
    VICAddress = 0;
	}                 
}    

void Counter0(void) {
	if(Fcounter0 == 1) {
		//Fcounter0 = 0;
		return; 
		}
	if(Fcounter1 ==0 && Fcounter0 == 0) {
		if(!Blockingmode) Fcounter0 =1;
	}
	
	if(Fcounter0 == 0 && Fcounter1 == 1){
	//object leaving
		numpeople--;
		Fcounter1 = 0;
		
	}
	return;
}
        
void Counter1(void) {
	if(Fcounter1 == 1) {
		//Fcounter1 = 0;
		return; 
		}
	if(Fcounter1 ==0 && Fcounter0 == 0) {
		if(!Blockingmode) Fcounter1 =1;
		
	}
	
	if((Fcounter0 == 1 && Fcounter1 == 0) | (Blockingmode == 1)){
	//object entering
		numpeople++;
		Fcounter0 = 0;
		if(Blockingmode == 1) {
		GsmMsgSend(burglar);
		}
	}
	return;
}		

////ISR AND FUNCTIONS FOR IR SENSORS
//int numpeople = 0;
//int fcounter = 0;
//int blockingmode = 0;

//__irq void isr_Counter0(void) {

//	fcounter = 1;
//	VICAddress = 0;
//}

//__irq void isr_Counter1(void) {
//	
//	if(fcounter == 1) {
//		fcounter = 0;
//		numpeople++;
//		//obejct is entering 
//		if(blockingmode == 1) {
//			//forced entery
//			//gsmsend
//		}
//		else if(numpeople == 1) {
//			//switch on lights
//		}
//	}
//	else if(fcounter == 0 ) {
//		//object leaving
//		numpeople--;
//		if(numpeople == 0) {
//			blockingmode = 1;
//			//switch off lights			
//		}			
//	}
//	VICAddress = 0;
//}




//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//ISR AND FUNCTIONS FOR GAS SENSOR ADC

//TO DO : ADD FUNCTIONS TO RAISE ALARM AND SEND MESSAGE AND CHANGE THRESHOLD VALUES OF LPG AND SMOKE

//GAS SENSOR ISR AND FUNCTIONS

//GAS SENSOR ADC ISR
#define         READ_SAMPLE_TIMES          (5)
#define         READ_SAMPLE_INTERVAL       (600000)
#define         THRESHOLD_LPG                             (0)
#define                    THRESHOLD_SMOKE                            (0)
#define                    RL_VALUE                                         (20)


void CheckGasPercentage(void);
float ResistanceCalculation(void);
float ObtainRsRoRatio(void);
void GasDelay(int delayinterval);
int HexToDecimal(int hexvalue);
float ResistanceCalculationRS(void);

float LPGCurve[3] = {2.3,0.21,-0.47};
float COCurve[3] = {2.3,0.72,-0.34};
float SmokeCurve[3] = {2.3,0.53,-0.44};
float R0;

__irq void isr_adc(void)
{
        CheckGasPercentage();

    VICAddress  = 0; //Acknowledge the interrupt
}

void CheckGasPercentage(){
    int lpgppm = 0;
    //int coppm = 0;
    int smokeppm = 0;
    float ratio = 0;
    
    ratio = ObtainRsRoRatio();
    if (ratio < 4) {
			//GsmMsgSend(fire);
		}			
	
    lpgppm = (pow(10,( ((log(ratio)-LPGCurve[1])/LPGCurve[2]) + LPGCurve[0])));
    //coppm  = (pow(10,( ((log(ratio)-COCurve[1])/COCurve[2]) + COCurve[0])));
    smokeppm = (pow(10,( ((log(ratio)-SmokeCurve[1])/SmokeCurve[2]) + SmokeCurve[0])));
    
    if(lpgppm > THRESHOLD_LPG) {
		
        //raise alarm
        //send message         
    }
    
    if(smokeppm > THRESHOLD_SMOKE) {
		//	GsmMsgSend(gasleak);
        //raise alarm
        //send message
    }
}

float ObtainRsRoRatio() {
    int i = 0;
    float val = 0;
    
    for(i=0 ; i<READ_SAMPLE_TIMES; i++) {
        val += ResistanceCalculationRS();
        GasDelay(READ_SAMPLE_INTERVAL);
    }
    
    val = val/READ_SAMPLE_TIMES;
    val = val/R0;
    return val;    
}

float ResistanceCalculationRS() {
     
    int raw_adc = 0;
    int raw_adc_decimal = 0;
    //AD0CR    |= 1<<24;
    GasDelay(2000000);
  raw_adc = ((AD0DR2 >> 6) & 0x3FF);
    
    raw_adc_decimal = HexToDecimal(raw_adc);
  return ( ((float)RL_VALUE*(1023-raw_adc_decimal)/raw_adc));
}

//GAS SENSOR INITIALIZATION MODULES
#define         CALIBRATION_SAMPLE_TIMES     (50) //define how many samples you are going to take in the calibrating

#define         RO_CLEAN_AIR_FACTOR          (9.83)
#define         CALIBRATION_SAMPLE_INTERVAL  (000)
          



void GasDelay(int delayinterval) {
     
    unsigned int X = delayinterval;
    while(X--);
}

float GasSensorInit(void) {
    int i =0;
    float val = 0;
    
    for(i=0; i<CALIBRATION_SAMPLE_TIMES; i++) {
        val = val + ResistanceCalculation();
        GasDelay(CALIBRATION_SAMPLE_INTERVAL);
    }
    
    val = val/CALIBRATION_SAMPLE_TIMES;
    val = val/RO_CLEAN_AIR_FACTOR;
    return val;
}

float ResistanceCalculation() {
     
    int raw_adc = 0;
    int raw_adc_decimal = 0;
    AD0CR    |= 1<<24;
    GasDelay(2000000);
  raw_adc = ((AD0DR2 >> 6) & 0x3FF);
    
    raw_adc_decimal = HexToDecimal(raw_adc);
  return ( ((float)RL_VALUE*(1023-raw_adc_decimal)/raw_adc));
}

int HexToDecimal(int hexvalue) {

    int i = 0;
    int mask[3];
    int temp = 0;
    int val = 0;
    int decimal = 0;
    mask[0] = 0x0f;
    mask[1] = 0xf0;
    mask[2] = 0x300;
        
    for(i=0; i<3; i++) {
         temp = ((hexvalue & mask[i])>>(4*i));
         switch(temp) {
             case 0x0:
                 val = 0;
               break;
             case 0x1 :
                    val = 1;
                    break;
             case 0x2 :
                 val = 2;
               break;
             case 0x3 :
                 val = 3;
               break;
             case 0x04 :
                 val = 4;
               break;
             case 0x05 :
                 val = 5;
               break;
             case 0x06 :
                 val = 6;
               break;
             case 0x07 :
                 val = 7;
               break;
             case 0x08 :
                 val = 8;
                 break;
             case 0x09 :
                    val = 9;
          break;
             case 0x0A:
                 val = 10;
               break;
             case 0x0B :
                 val = 11;
               break;
             case 0x0c :
                 val = 12;
               break;
             case 0x0D :
                 val = 13;
               break;
             case 0x0E :
                 val = 14;
               break;
             case 0x0F :
                 val = 15;
               break;
             default :
                    break;
         }
         decimal += (val * pow(16,i));
    }        
    return decimal;    
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//ISR AND FUNCTIONS FOR GATE CONTROL
//TO DO: 1)REMOVE TEMP AND TEST 2)PUT RESET CONDITION IN MCR REGISTER 
int temp1=0;
__irq void isr_GateTimer(void){
	int c;
	c          = T3TC;   // read TC value
	T3TC       = 0;			 //	reset timer
	T3TCR      = 0;			 // disable timer
	FIO0CLR   |= 0X0E;   // clear motor pins to stop motor
	T3IR       = 0x02;   //clear interrupt Timer 3 channel 1
	VICAddress = 0;
	temp1= 1;	
}

void GATEDELAY() {
	T3TCR = 1; // enable timer	
}

void LockGate(void) {
	 FIO4CLR |= 0X0E;// P4.1->0 P4.2->1 P4.3->1
	FIO4SET  = 0X0A;
	DELAY(72000000);
	FIO4CLR |= 0X0E;
}

void UnLockGate (void) {
	FIO4CLR |= 0X0F;
	FIO4SET |= 0X0C;  // P4.1->1 P4.2->0 P4.3->1
	DELAY(72000000);
	FIO4CLR |= 0X0F;
//	while(temp1== 0); 	
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//ISR and FUNCTIONS OF MATRIX KEYPAD AND EXTERNAL SWITCH AND MANUAL OVERRIDE
//TO DO :ADD FUNCTION TO OPEN DOOR AND RAISE ALARM SEND MESSAGE OWNERS 
#define PASSWORD_SIZE 4
void UnLockGate (void);
char check_key(void);
char PASSWORD[PASSWORD_SIZE] = "1111";
int ATTEMPTS = 0;
//void DELAY();

__irq void isr_ExternalSwitch (void) {
	/*obtain 4 digit password */
	int notmatch = 0;
	int i = 0;
	char password[PASSWORD_SIZE];
	for(i = 0; i<4; i++) {
		password[i] = check_key();
		while(!((FIO3PIN & 0xf0) == 0xf0));
		DELAY(120000);
		while(!((FIO3PIN & 0xf0) == 0xf0));

	}
	
	/*match the password */
	for(i=0; i<PASSWORD_SIZE; i++) {
	 if(password[i] != PASSWORD[i]) {
		 notmatch = 1;
			break;
		}
	}
	if(notmatch == 0) {
		UnLockGate();
	/*if correct password call function to open the gate */		
		VICAddress = 0;
	}
	
	else if(notmatch == 1) {
		if(ATTEMPTS < 3){
			ATTEMPTS++;
		}
		
		else if(ATTEMPTS >= 3) {
			GsmMsgSend(burglar);
			/*call function to raise alarm */
			/*call function to inform owners*/
			ATTEMPTS = 0;
		}
		VICAddress = 0;
	}
		
}
	//keypad module
	#define sw1 0x10
	#define sw2 0x20
	#define sw3 0x40
	#define sw4 0x80

//void DELAY() {
//	unsigned int X=800;
//	while(X--);
//}

char check_key(void) {
	unsigned char c;
	DELAY(120000);	
  while(1) {
		FIO3MASK = 0x0;
		FIO3CLR  = 0X0F;	
		DELAY(800);
		FIO3SET |= 0x0E;
		DELAY(800);
		
    if((FIO3PIN & sw1) == 0) {
			c = '0';
			while((FIO3PIN & sw1) == 0)
			return(c);
  	}  
  
		if((FIO3PIN & sw2) == 0) {
			c = '1';
			while((FIO3PIN & sw2) == 0)
			return(c);
		}

		if((FIO3PIN & sw3) == 0) {
			c = '2';
			while((FIO3PIN & sw3) == 0)
			return(c);
		}

		if((FIO3PIN & sw4) == 0) {
			c = '3';
			while((FIO3PIN & sw4) == 0)
			return(c);
		}
	
		FIO3CLR = 0x0E; 
		DELAY(800);
		FIO3SET = 0x0D; // p2.1 = 0
		DELAY(800);

		if((FIO3PIN & sw1) == 0) {
			c = '4' ;
			while((FIO3PIN & sw1) == 0) 
			return c;
		}
	 
		if((FIO3PIN & sw2) == 0) {
			c = '5';
			while((FIO3PIN & sw2) == 0)
			return(c);
		}	

		if((FIO3PIN & sw3) == 0) {
			c = '6' ;
			while((FIO3PIN & sw3) == 0)
			return(c);
		}
		
		if((FIO3PIN & sw4) == 0) {
			c = '7' ;
			while((FIO3PIN & sw4) == 0)
			return(c);
	 	}
		
		FIO3CLR = 0x0D;
		DELAY(800);
		FIO3SET = 0x0B; // p2.2 = 0
		DELAY(800);
	
		if((FIO3PIN & sw1) == 0) {
			c = '8';
			while((FIO3PIN & sw1) == 0)
			return(c);
		}
		
		if((FIO3PIN & sw2) == 0) {
			c = '9';
			while((FIO3PIN & sw2) == 0)
			return(c);
		}
		
		if((FIO3PIN & sw3) == 0) {
			c = 'A';
			while((FIO3PIN & sw3) == 0)
			return(c);
		}
		
		if((FIO3PIN & sw4) == 0) {
			c = 'B';
			while((FIO3PIN & sw4) == 0)
			return(c);
		}
		
		FIO3CLR = 0x0b;
		DELAY(800);
		FIO3SET = 0x07; //p0.3 = 0
		DELAY(800);

		if((FIO3PIN & sw1) == 0) {
			c = 'C';
			while((FIO3PIN & sw1) == 0)
			return(c);
		}

		if((FIO3PIN & sw2) == 0) {
				c = 'D';
			  while((FIO3PIN & sw2 ) == 0)
				return(c);
		}
		
		if((FIO3PIN & sw3) == 0) {
			c = 'E';
			while((FIO3PIN & sw3) == 0)
			return(c);
		}
		
		if((FIO3PIN & sw4) == 0) {
				c = '*';
			  while((FIO3PIN & sw4) == 0)
				return(c);
		}
		
		FIO3CLR = 0x07;
		DELAY(800);
	}
}

__irq void isr_ManualOverride(void) {
	UnLockGate();
	/*call function to open the gates */
	VICAddress = 0;
}	
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



void GsmMsgSend(char *msg) {
 
    char *temp;
		U1THR = '\n';
    temp = "AT\r";  //AT command for initializing
    UartGSMSend(temp);
    
    DELAY(16000000);
    temp = "ATE0\r"; //echo disable
    UartGSMSend(temp);
    DELAY(16000000);
    temp = "AT+CMGF=1\r"; //text mode
    UartGSMSend(temp);
    //status_ok();
    DELAY(16000000);
    temp = "AT+CSCS=\"GSM\"\r"; // GSM mode
    UartGSMSend(temp);
    //status_ok();
    DELAY(16000000);
    temp = "AT+CMGS=\"08149141050\"\r";
    UartGSMSend(temp);
    //status_ok();
    DELAY(16000000);
    UartGSMSend(msg);
    //status_ok();
    DELAY(16000000);

    Uart_terminate();
		temp = "\r";
    UartGSMSend(temp);
		DELAY(16000000);
}

void Uart_terminate(void) {
  U1THR = 0x1a;
	while(!(U1LSR&0x20));	
}

void UartGSMSend(char *msg) {
	char *temp = msg;
  while(*temp != 0x00) {
		U1THR = *temp;
    while(!(U1LSR & 0x20));
		temp++;
		}
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//BLUETOOTH ISR AND FUNCTIONS
//unsigned char *rcvd;
unsigned char y;
unsigned char c;
    char *temp;
		int i ;
void status_ok(void);
void UartBthSend(char *msg);
void BthMsgSend(char *msg);
void UartBthPin(void);
unsigned char getcharacter(void);
void Uart_terminate(void);
void CompareCommand();
unsigned char rcvdb[2];
int nrcvd =0;


//void getstring(unsigned char *array)
//{
//    unsigned char temp1=0;
//    do{
//        temp1 = getcharacter();
//    *array++ = temp1;
//    }
//		while((temp1 != '\r') && (temp1 != '\n'));
//    *array = '\0';
//}
unsigned char getcharacter (void)  /* Read character from Serial Port */
{    
		
   if ((U0LSR & 0x01 )!=0)
   {	
	    c=U0RBR;
	    return (c);
	 }
		DELAY(1600000);
}
void CompareCommand(void) {
	if(!strncmp(rcvdb, "L0",2) ){
 GsmMsgSend(fire);
	}
	
	if(!strncmp(rcvdb, "L1", 2)) {
		//Lights off
	}
	
	if(!strncmp(rcvdb, "G1", 2)) {
		UnLockGate();
		Blockingmode = 0;
		//gates open
	}
	
	if(!strncmp(rcvdb, "G0", 2)) {
		LockGate();
		Blockingmode = 1;
		//gate lock
//		blockingmode = 1;
	}
	
	if(!strncmp(rcvdb, "A1", 2)) {
		i = 100;
	}
	
	if(!strncmp(rcvdb, "A0", 2)) {
		//AC off
	}

}
__irq void isr_BTH(void){
	y = getcharacter();
	if((y == 'O') || (y == 'K') || (y=='s') || (y=='e') || (y=='t') || (y=='n') || (y=='a') ||(y=='m') || (y=='e')){
		nrcvd=0;
	}
	if(y != '#') {
		rcvdb[nrcvd++] = y;
	}
	else {
		CompareCommand();
		nrcvd = 0;
	}
	VICAddress = 0;
}

// introduce delay


void BthMsgSend(char *msg) {
		char *temp;
		U0THR = '\n';
	
	  DELAY(16000000);
	  
    temp = "AT\r";  //AT command for initializing
    UartBthSend(temp);
    DELAY(16000000);
	
		 
	
//	  temp = "AT+PIN"; //AT command for setting the PIN
//	  UartBthSend(temp);
//		UartBthPin();
//		temp = "\r";
//    UartBthSend(temp);
//    DELAY();
	
		temp = "AT+NAMEhomeauto\r\n"; //AT command for setting the name
	  UartBthSend(temp);
    DELAY(16000000);
	
}

void UartBthPin(void) {
	
    U0THR = 0x5;
		while(!(U0LSR&0x20));	
		U0THR = 0x4;
		while(!(U0LSR&0x20));
		U0THR = 0x3;
		while(!(U0LSR&0x20));
		U0THR = 0x2;
		while(!(U0LSR&0x20));
}

void UartBthSend(char *msg) {
    char *temp = msg;
    while(*temp != 0x00) {
        U0THR = *temp;
        while(!(U0LSR & 0x20));
        temp++;
		}
}
//UART SEND AND RECEIEVE
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int main(void) {
	//initializations for CONTROL MODULE
	
	SCS      |= 1;
	/*1) GPIO pin for light control */
	SCS      |= 1;					
	PINSEL0   |= 00;      // P0.0 ->light control
	PINMODE0 |= 0x03;    // pull down ASSUMING ACTIVE HIGH
	FIO0DIR  |= 0x01;    //output 
	
	/*2) Temperature */
	PINSEL2   |= 0x00; // P1.0
	PINMODE2 |= 0x03; //pull down
	FIO1DIR  |= 0x01; //output
	
	/*3) Gate Control and Timer */
	PCONP     |= (1<<23);     				//enable timer 3
	PINSEL1  |= ((1<<16) | (1<<17));// P0.24 : Timer3 channel 1 CAP3.1
	T3MR1     = 72000000;    				// 6secs 1200000 cycles
	T3CTCR    = 0x00;        				//Timer mode
	T3MCR     = (1<<3)|(1<<5);   		//stop and interrupt
	
	VICIntEnable      |= (1<<27);   //Timer 3
	VICVectPriority27  = 1;			    //
	VICVectAddr27      = (unsigned long) isr_GateTimer;
	
	
	PINSEL8  = 0x00;   	 // p4.1 p4.2 p4.3 :  P4.1 ->2 P4.2->7 P4.3->ENABLE1
	FIO4DIR |= 0x0F;     // output from micr-controller
	FIO4CLR  = 0XE;			 //clear the bits so the motor does not start automatically
	//FIO0SET  = 0X0;
	
	/*4) Counter and IR Sensor */
  /*TIMER0 CH0 and CH1 P1.26, P1.27 */
	PINSEL3  |= 0;
  PINSEL3 |= ((1<<20)| (1<<21) | (1<<22) | (1<<23));  //bit 20 and 21 22,23
  T0CTCR   = 0x2;                                     // Counter Mode ; Falling edge
  T0PR     = 0x0;                                     // increment the count after passed through both the sensors
  T0CCR    = 0x36;                                    //interrupt enable
  T0TCR    = 1 ;                                      // Timer enable  SHIFT THIS TO THE END LATER
  /*Pair fo IR sensor: VCC GND  OUTPUTofmodule(p1.26 p1.27)-input of timer */    
	/*enabling interrupt for the counter */
  VICIntEnable     |= (1<<4);      // TImer 0 interrupt enable
  VICVectPriority4  = 1;
  VICVectAddr4      = (unsigned long) isr_counter;

//	
//	/*4) Counter and IR Sensor */
//	PINSEL4 |= (1<<22) | (1<<24);
//	PINMODE4 |= (1<<22) |(1<<23) | (1<<24) |(1<<25); 
//	IO2_INT_EN_F  = ((1<<11) | (1<<12)); 
//	VICIntEnable |= (1<<15); // EINT1 enabled
//	VICVectPriority15 = 0;   //highest priority to manual override
//	VICVectAddr15 = (unsigned long) isr_Counter0;
//	
//	VICIntEnable |= (1<<16); // EINT2 enabled
//	VICVectPriority16 = 0;   //highest priority to manual override
//	VICVectAddr16 = (unsigned long) isr_Counter1;
	
	//INITIALIZATIONS FOR MONITORING MODULE
	
	 /*1)*Gas Sensor- ADC initialization */
    /*TIMER1*/
    T1MR0   = 	(1+5999999)*6-1 ;       //count for 0.5 minutes: 1Hz frequency
    T1CTCR  = 0x00 ;                    //timer mode 
    T1MCR   = 0x02 ;                    //reset on match
    PINSEL3 |= 0x00003000;     // p1.22(pins 12,13 -1)
    T1EMR  = ((1<<4)|(1<<5)); //toggle thhe output
    
    
    PCONP   |= (1<<12);     //enable power
    PINSEL1 |=  (0x1<<18);      //p0.23 bit 14,15->01 for AD0
    AD0INTEN = (1 << 2);    // CH2 enable interrupt 
    AD0CR    = 0X01200304;  //10BITS SOC MAT1.0 
    
    R0 = GasSensorInit();
	 
	/*2) GPIO pin p0.6 for external switch(for keyboard) and p0.7 for alarm output and p2.10 for manual override */
	PINSEL0  |= 0x00;         //P0.6 p0.7
	PINSEL4  |= ((0<<21) | (1<<20)); // P2.10 af1   
	PINMODE0 |= 0x00;
	FIO0DIR  |= 0x80; 				//p0.6->input p0.7->outpu
	IO0_INT_EN_F = (1<<6);

	VICIntEnable |= (1<<17); //GPIO interrupt enable
	VICVectPriority17 = 1;   //
	VICVectAddr17 = (unsigned long) isr_ExternalSwitch;
	
	VICIntEnable |= (1<<14); // EINT0 enabled
	VICVectPriority14 = 3;   //highest priority to manual override
	VICVectAddr14 = (unsigned long) isr_ManualOverride;

	/*3) MTRIX KEYPAD 4 input 4 output pins p3.0 to p3.7 */
	PINSEL6 = 0x00; 
	FIO3DIR = 0x0F; // p3.0-p3.3 output  p3.4-p3.7 inputs
	FIO3CLR = 0x0F;
	
	//GSM BLUETOOTH - COMMUNICATION MODULE
	/*1) GSM - UART */
	PCONP    |=0x10; 					//Power for UART-1
  PCLKSEL0 |= 0x00; 			  //bits 9:8 for UART1
  PINSEL0  |= 0x40000000; 	//bits 9:8 for UART1
  PINSEL1  |= 0x00000001; 	// P0.15->TXD1 P0.16->RXD1
  U1LCR     = 0X83;       	//Enable divisor latch ,8 bits data
  U1DLM     = 0X00;
  U1DLL     = 0X4E;   			//BAUDRATE:9600
  U1LCR     = 0x03;
  U1FCR     = 0x07;        //FIFO enable : TX and RX FIFO reset
	U1TER 	  = 0x80;			   // transmit register enable

	 /*1)Bluetooh MODULE - UART*/
      PCONP |= 0x08; //Power for UART-0
      PCLKSEL0 |= 0x00; //bits 9:8 for UART1
      PINSEL0 |= 0x00000050;  
      U0LCR    = 0X83;       //Enable divisor latch ,8 bits data
      U0DLM    = 0X00;
      U0DLL    = 0X4E;   //BAUDRATE:9600
      U0LCR    = 0x03;
      U0FCR    = 0x07;            //FIFO enable : TX and RX FIFO reset
	    U0TER = 0x80;
			U0IER = 0x1;
		
	VICIntEnable |= (1<<6); // UART0 enabled
	VICVectPriority6 = 0;   //highest priority to manual override
	VICVectAddr6 = (unsigned long) isr_BTH;
	
    //Set U0IER for raising interrupts on message sent    
    
	
	  AD0CR    = 0X16200304;  //10BITS SOC MAT1.0 
    T1TCR  = 1;               //enable timer
		VICIntEnable      |= (1<<18);
    VICVectPriority18 = 1;
    VICVectAddr18     = (unsigned long) isr_adc ; /* Set Interrupt Vector */
	
	while(1);
}




