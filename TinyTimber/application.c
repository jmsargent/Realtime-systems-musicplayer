#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_INTEGER_SIZE 50

typedef struct {
    Object super;
    int count;
    char c;
	
	/*
	 * -- my code beneath --
	 */
	 
	 int digitPointer;
	 char sum[MAX_INTEGER_SIZE];
	 char storedDigits[MAX_INTEGER_SIZE];
	 char arrayToPrint[MAX_INTEGER_SIZE];
	 int key;
	
} App;

App app = { initObject(), 0, 'X', 0 };

void reader(App*, int);
void receiver(App*, int);
void createInteger(App *self,int c);
void printPeriods(App *self, int c);
void resetRunningSum(App *self, int c);
void storeNumber(App *self, int c);
void printRunningSum(App *self, int c);
void printCurrentNumber(App *self, int c);
void addToSum(App *self, int c);

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);



#define MADAC	((volatile unsigned int *)(0x4000741C))	// base adress for DAC

typedef struct {
    Object super;
    int active ; // 1 - if active, 0 otherwise
	int period; // period in microSeconds
	int volume;
	int currentDacState;
	char buffer[MAX_INTEGER_SIZE];
	
} ToneGenerator;

ToneGenerator toneGenerator = {initObject(), 0,1000,5}; // constructor

// functiondeclarations

void ToneGeneratorChangeVolume(ToneGenerator *self, int c);
void ToneGeneratorActive(ToneGenerator *self, int unused);
void ToneGeneratorPlay(ToneGenerator *self, int unused);

typedef struct{
	Object super;
	int active;
	int period;
	int backgroundLoopRange;	// removed underscores to stay in line with the rest of the syntax
	char buffer[MAX_INTEGER_SIZE];
} Dist;

Dist dist = {initObject(), 0,1300, 1000};	// constructor

// declarations

void setPeriod(Dist *self, int period);
int getPeriod(Dist *self, int unused);
void setBackgroundLoopRange(Dist *self, int loopRange);
int getBackgroundLoopRange(Dist *self, int unused);
void toggleDist(Dist *self, int active);
void heyImInUrWay(Dist *self, int unused);
void printBackgroundLoopRange(Dist *self, int unused);

typedef struct{
	Object super;
	int active;
} Deadline;

Deadline deadline = {initObject(),0}; 

void toggleDeadline(Deadline *self, int unused);
int getDeadlineStatus(Deadline *self, int unused);



void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    SCI_WRITE(&sci0, msg.buff);
}

void reader(App *self, int c) {
    SCI_WRITE(&sci0, "Rcv: \'");
    SCI_WRITECHAR(&sci0, c);
    SCI_WRITE(&sci0, "\'\n");
	
	ASYNC(self, createInteger,c);
}

void startApp(App *self, int arg) {
    CANMsg msg;

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    SCI_WRITE(&sci0, "Hello, hello...\n");

    msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 6;
    msg.buff[0] = 'H';
    msg.buff[1] = 'e';
    msg.buff[2] = 'l';
    msg.buff[3] = 'l';
    msg.buff[4] = 'o';
    msg.buff[5] = 0;
    CAN_SEND(&can0, &msg);
}

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}


/*
 * -- MY CODE BELOW --
 */


 
void createInteger(App *self,int c){
	
	switch(c){
		case 'k':
			ASYNC(self, printPeriods, c);
			break;
		case 'F':
			ASYNC(self,resetRunningSum, c);
			ASYNC(self,printRunningSum, c);
			break;
		case 'e':
			ASYNC(self,printCurrentNumber,c);
			break;
		case '-':
			if(self->digitPointer == 0 )
				ASYNC(self, storeNumber, c);
			break;
		case '0':
			ASYNC(self, storeNumber, c);			
			break;
		case '1':
			ASYNC(self, storeNumber, c);
			break;
		case '2':
			ASYNC(self, storeNumber, c);
			break;
		case '3':
			ASYNC(self, storeNumber, c);
			break;
		case '4':
			ASYNC(self, storeNumber, c);
			break;
		case '5':
			ASYNC(self, storeNumber, c);
			break;
		case '6':
			ASYNC(self, storeNumber, c);
			break;
		case '7':
			ASYNC(self, storeNumber, c);
			break;
		case '8':
			ASYNC(self, storeNumber, c);
			break;
		case '9':
			ASYNC(self, storeNumber, c);
			break;
		// labb 1
		case 'h':
			ASYNC(&toneGenerator,ToneGeneratorChangeVolume,c);
			break;
		case 'l':
			ASYNC(&toneGenerator,ToneGeneratorChangeVolume,c);
			break;
		case 'p':
			ASYNC(&toneGenerator,ToneGeneratorActive,c);
			break;
		case 'd': 
			ASYNC(&dist,toggleDist,0);
			break;
		case 'b':
			ASYNC(&dist,setBackgroundLoopRange, c);
			ASYNC(&dist,printBackgroundLoopRange, 0);
			break;
		case 'B':
			ASYNC(&dist,setBackgroundLoopRange, c);
			ASYNC(&dist,printBackgroundLoopRange, 0);
			break;
		case 'x':
			ASYNC(&deadline,toggleDeadline, c);
	}
		
}

void printCurrentNumber(App *self, int c){
	
	self->storedDigits[self->digitPointer] = '\0';
	
    SCI_WRITE(&sci0, "The entered number is:");
	SCI_WRITE(&sci0, self->storedDigits);
	SCI_WRITE(&sci0, "\n");

	self->digitPointer = 0;
	
	ASYNC(self, addToSum, c);
	ASYNC(self, printRunningSum, c);
	
	}
	
void storeNumber(App *self, int c){
	self->storedDigits[self->digitPointer] = c;
	self->digitPointer ++;
}

void printRunningSum(App *self, int c){
	
	SCI_WRITE(&sci0, "The running sum is:");
	SCI_WRITE(&sci0, self->sum);
	SCI_WRITE(&sci0, "\n");
}

void addToSum(App *self, int c){
	int temp;
	temp = atoi(self->sum) + atoi(self->storedDigits);
	snprintf(self->sum, MAX_INTEGER_SIZE,"%d", temp);
}

void resetRunningSum(App *self, int c){
	self->sum[0] = '0';
	self->sum[1] = '\0';
}

 int freqIndices[32] = {0,2,4,0,0,2,4,0,4,5,7,4,5,7,7,9,7,5,4,0,7,9,7,5,4,0,0,-5,0,0,-5,0};
 
 int period[25] = {2025, 1911, 1804, 1703, 1607, 1517, 1432, 1351, 1276, 1204, 
				  1136, 1073, 1012, 956, 902, 851, 804, 758, 716, 676, 638,		// 1x25 vector
				  602, 568, 536, 506};

void printPeriods(App *self, int c){
	
	self->storedDigits[self->digitPointer] = '\0';

    SCI_WRITE(&sci0, "Key: ");
	SCI_WRITE(&sci0, self->storedDigits);
	SCI_WRITE(&sci0, "\n");
	
	self->key = atoi(self->storedDigits);
		
	for(int i = 0; i<=31; i++){
		snprintf(self->storedDigits , MAX_INTEGER_SIZE,"%d", period[(freqIndices[i]+10+self->key)] );
		SCI_WRITE(&sci0, self->storedDigits);
		SCI_WRITE(&sci0, " ");
	}
	
	SCI_WRITE(&sci0, "\n");
	
	SCI_WRITE(&sci0, "\n");

	self->digitPointer = 0;
	
}

void ToneGeneratorActive(ToneGenerator *self, int unused){
	
	if (self->active!= 1){
		self->active = 1;
		ASYNC(self,ToneGeneratorPlay,0);
	}else{
		self->active = 0 ;
	}
	
	
	
	snprintf(self->buffer, MAX_INTEGER_SIZE,"%d", self->active);
	SCI_WRITE(&sci0, "is active:");
	SCI_WRITE(&sci0, self->buffer);
	SCI_WRITE(&sci0, "\n");
}
	
void ToneGeneratorSetPeriod(ToneGenerator *self, int period){
	self->period = period;
}

void ToneGeneratorChangeVolume(ToneGenerator *self, int c){
	if(c == 'h')
		self->volume ++ ;
	if(c == 'l')
		self->volume --;
		
		
	snprintf(self->buffer, MAX_INTEGER_SIZE,"%d", self->volume);
    SCI_WRITE(&sci0, "Current volume:");
	SCI_WRITE(&sci0, self->buffer);
	SCI_WRITE(&sci0, "\n");

}

void ToneGeneratorFlipCurrentState(ToneGenerator *self, int unused){
	
	if(self->currentDacState){
	self->currentDacState = 0;
	}else{
		self->currentDacState = 1;
	}
}

void ToneGeneratorPlay(ToneGenerator *self, int unused){
		
		if(self->active){
			
		if(!self->currentDacState){
			*MADAC = self->volume;
			ASYNC(self, ToneGeneratorFlipCurrentState, 0);
		}else{
			*MADAC = 0;
			ASYNC(self, ToneGeneratorFlipCurrentState, 0);
		}
		if(SYNC(&deadline,getDeadlineStatus,0)){
			
			SEND(USEC(931),USEC(100),&toneGenerator,ToneGeneratorPlay,unused);
			
			}else{
				
				AFTER(USEC(931), &toneGenerator,ToneGeneratorPlay,unused);
				
			}
		}
}

void setPeriod(Dist *self, int period){
	self->period = period;
}

int getPeriod(Dist *self, int unused){
	return self->period;
}

void setBackgroundLoopRange(Dist *self, int incDec){
	if(incDec == 'b'){
		if(self->backgroundLoopRange >= 1000)
			self->backgroundLoopRange -= 500;
	}else{
		self->backgroundLoopRange += 500;
	}
}
int getBackgroundLoopRange(Dist *self, int unused){
	return self->backgroundLoopRange;
}

void toggleDist(Dist *self, int active){
	if(self->active){
		self->active = 0;
		SCI_WRITE(&sci0, " dist disabled \n");
	}else{
		self->active = 1;
		ASYNC(self,heyImInUrWay,0);
		SCI_WRITE(&sci0, " dist enabled \n");

	}
}

void heyImInUrWay(Dist *self, int unused){
	
	if(self->active){
		for(int i = 0; i<= self->backgroundLoopRange; i++){}
		if(SYNC(&deadline,getDeadlineStatus,0)){	// if deadline is active
		SEND(USEC(self->period),USEC(1000),self,heyImInUrWay,unused);
		}else{
			AFTER(USEC(self->period),self,heyImInUrWay,unused);	
		}
	}

}


// rewrite so we can use buffer in other object instead of having multiple buffers
// maby some kind of printmethod which takes a string and a number as that is used many times

void printBackgroundLoopRange(Dist *self, int unused){
	
	snprintf(self->buffer, MAX_INTEGER_SIZE,"%d", self->backgroundLoopRange);
	SCI_WRITE(&sci0, "Current background loop range:");
	SCI_WRITE(&sci0, self->buffer);
	SCI_WRITE(&sci0, "\n");
}

void toggleDeadline(Deadline *self, int unused){
	if(self->active){
		self->active = 0;
		SCI_WRITE(&sci0, " :') disabled \n");
		
	}else{
		self->active = 1;
		SCI_WRITE(&sci0, " :'O enabled \n");

	}
}

int getDeadlineStatus(Deadline *self, int unused){
	return(self->active);
}
