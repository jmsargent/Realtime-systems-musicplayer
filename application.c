#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>

// ToneGenerator controlls the dac

#define MAX_INTEGER_SIZE 50	// Buffer size
#define MADAC ((volatile unsigned char *) 0x4000741C) // base adress for DAC

typedef struct {
	
	Object super;
	int active;
	int period;
	int volume;
	int currentDACState;	// contains information about last sent info to 
	int delay;	 //	delay between each note , should shorten the note 
	int beatLength;	// duration a note is played 
	int pointer;
	
} Walkman;

Walkman player = { initObject(), 0, 0, 0, 0, 0, 0};


void startStopPlayer(Walkman *self ,int active);
int getPlayerActiveStatus(Walkman *self, int unused);
void setBeatLength(Walkman *self ,int beatLength);
int getBeatLength(Walkman *self,int unused);
void setDelay(Walkman *self,int delay);
int getDelay(Walkman *self,int unused);
void setPeriod(Walkman *self,int period);
int getPeriod(Walkman *self,int unused);	 
void changeVolume(Walkman *self,int volume); // 
void playNext(Walkman *self,int unused);  // after




	/*
	 * 
	 * 
	SEND
	Baseline = delay
	Deadline = beatLength
	argument = period
	SEND(delay,beatLength,self,playNext,period)
	
	 * 
	 * 
	 * */ 

// Handles anything to do with writing to and from the consol

typedef struct {
    Object super;
    int changeToValue;
    char c;
	int buffer[MAX_INTEGER_SIZE]; // funderar på att lagra bakifrån med andra tecken? eller ba delvis ta bort grejjer ur array o ställa tillbaka pekare ett steg
	int digitPointer;
} App;

App app = { initObject(), 0, 'X' };

void reader(App*, int);
void receiver(App*, int);
void handleInput(App *self, int key); 	// som din gamla create integer använd if /elseif ist för case med ASCII villkor för att korta ned
void storeDigit(App *self, int digit);
void printPeriods(App *self, int key);
void printPlayerChanges(App *self, int key);  // prints changes in vol,period.. etc




Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);

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


// app functions

void handleInput(App *self, int key){
	
	// handles numbers
	if (key == '-' && self->digitPointer ){	
		
		ASYNC(self, storeDigit, key);
		
	}else if ( atoi( (char *) key) <= 9 && atoi( (char *) key) >= 0 ){		// Do I really need to typecast here?

		ASYNC(self, storeDigit, key);
		
		// updates integerversion of number
		// change later
//		snprintf(self->buffer, MAX_INTEGER_SIZE, "&d", self->changeToValue);
	}
	
	
	
	// player bindings
	// takes stored number and sends to different walkman functions,
	// takes the same number and prints change in consol
	
	switch (key){
		
		case('v'):
		ASYNC(&player, changeVolume, self->buffer[self->digitPointer]);
		ASYNC(&player, printPlayerChanges, key);
		break;
		
		case('a'):
		// To start or stop player there is no need to send any special value,
		// if player is started, same binding will stop player
		ASYNC(&player, startStopPlayer, self->buffer[self->digitPointer]);
		ASYNC(&player, printPlayerChanges, key);
		break;
		
		case('b'):
		ASYNC(&player, setBeatLength, key);
		ASYNC(&player, printPlayerChanges, key);
		break;
		
		case('p'):
		ASYNC(&player, setPeriod, key);
		ASYNC(&player, printPlayerChanges, key);
		break; 
		
		case('d'):
		ASYNC(&player, setDelay, key);
		ASYNC(&player, printPlayerChanges, key);
		break;
		
	}
}

// Prints the value and what function the value was sent to, to the consol 
void printPlayerChanges(App *self, int key){
	
	switch (key){
		
		case 'v':
		    SCI_WRITE(&sci0, "Volume: \'");
			SCI_WRITE(&sci0, self->buffer);
			SCI_WRITE(&sci0, "\'\n");
		break;
		
		case 'a':
			SCI_WRITE(&sci0, "active status: \'");
			SCI_WRITE(&sci0, self->buffer);
			SCI_WRITE(&sci0, "\'\n");
		break;
		
		case 'b':
			SCI_WRITE(&sci0, "BeatLength: \'");
			SCI_WRITE(&sci0, self->buffer);
			SCI_WRITE(&sci0, "\'\n");
		break;
		
		case 'p':
			SCI_WRITE(&sci0, "New period length:  \'");
			SCI_WRITE(&sci0, self->buffer);
			SCI_WRITE(&sci0, "\'\n");
		break; 
		
		case 'd':
			SCI_WRITE(&sci0, "Delay: \'");
			SCI_WRITE(&sci0, self->buffer);
			SCI_WRITE(&sci0, "\'\n");
		break;
	
	}
}

// player functions

void startStopPlayer(Walkman *self, int unused){
	if(self -> active){
		self ->active = 0;
	}else{
		self -> active = 1;
	}
}

void storeDigit(App *self, int digit){
	
	self->buffer[self->digitPointer] = digit ;
	self->digitPointer ++ ;
	
}

void setDelay(Walkman *self, int delay){
	self -> delay = delay;
}

int getPlayerActiveStatus(Walkman *self, int unused){
	return(self->active); 
}

void changeVolume(Walkman *self,int volume){
	if(volume > 0 && volume < 20)
		self -> volume = volume;
}

void setBeatLength(Walkman *self, int bl){
	self -> beatLength = bl;
}

void setPeriod(Walkman *self, int period){
	self -> period = period;
}

