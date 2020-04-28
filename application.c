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
	int delay;	 //	delay between each note , should shorten the note 
	int beatLength;	// duration a note is played 
	int pointer;
	int key;
	int notes[32];
	int noteLenght[32];
	
} Walkman;

Walkman player = { initObject(), 0, 0, 0, 0, 0};


void startStopPlayer(Walkman *self ,int active);
void setBeatLength(Walkman *self ,int beatLength);
void setDelay(Walkman *self,int delay);
void playNext(Walkman *self,int unused);
void setKey(Walkman *self, int key);



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
	int digitPointer;
	int playerActiveStatus;	// vill igentligen ha i player, men blir mycket smidigare kod här
	char buffer[MAX_INTEGER_SIZE]; // funderar på att lagra bakifrån med andra tecken? eller ba delvis ta bort grejjer ur array o ställa tillbaka pekare ett steg

} App;

App app = { initObject(), 0, 'X',0,0 };

void reader(App*, int);
void receiver(App*, int);
void handleInput(App *self, int key); 	// som din gamla create integer använd if /elseif ist för case med ASCII villkor för att korta ned
void storeDigit(App *self, int digit);
void printPeriods(App *self, int key);
void printPlayerChanges(App *self, int key);  // prints changes in vol,period.. etc




typedef struct {
	Object super;
	int period;
	int volume;
	int currentDacState;
} ToneGenerator;

ToneGenerator tonegenerator = {initObject(), 0, 1, 0};

void changeVolume(ToneGenerator *self,int volume);
void setPeriod(ToneGenerator *self,int period); 




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
	
	ASYNC(self, handleInput, c);
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

void handleInput(App *self, int ikey){
	
	char key = (char) ikey;
	
	if (key == '-' && (self->digitPointer == 0) ){	
		self->buffer[self->digitPointer] = key ;
		self->digitPointer ++ ;
	}else if ( key <= 57 && key >= 48 ){	// ASCI CODE FOR [0,9] 	
		self->buffer[self->digitPointer] = key ;
		self->digitPointer ++ ;
	}
	
	
	// player bindings
	// takes stored number and sends to different walkman functions,
	// takes the same number and prints change in consol
	

	
	switch (key){
	
		case('v'):
		// store value as integer
		self->changeToValue = atoi(self->buffer);

		// check if valid volume
		if(self->changeToValue <20 && self->changeToValue > 0){
			
			// change volume
			ASYNC(&player, changeVolume, self->changeToValue);
			
			
			//write new volume in consol
			SCI_WRITE(&sci0, "New volume: ");
			SCI_WRITE(&sci0, self->buffer);						
			SCI_WRITE(&sci0, "\n");		
			self->digitPointer = 0;			
			
			}else{
				//write error message
				SCI_WRITE(&sci0, "Please use a value within the interval (0,20) \n");
				SCI_WRITE(&sci0, "you used value: ");	
				SCI_WRITE(&sci0, self->buffer);
				SCI_WRITE(&sci0, "\n");
			
			}
		break;
		
		case('a'):
		
			/* To start or stop player there is no need to send any special value,
			 if player is started, same binding will stop player */
			
			ASYNC(&player, startStopPlayer, self->buffer);
			
			// Track player status in current obj
			
			if(self->playerActiveStatus){			
				self->playerActiveStatus = 0;			
			}else{
				self->playerActiveStatus = 1;
			}
			
			// print in terminal
				
			snprintf(self->buffer,MAX_INTEGER_SIZE,"%d", self->playerActiveStatus);		
			SCI_WRITE(&sci0, "active status: \'");
			SCI_WRITE(&sci0, self->buffer);						
			SCI_WRITE(&sci0, "\'\n");		
		break;
		
		case('k'):

			self->changeToValue = atoi(self->buffer);
			if(self->changeToValue <= 5 && self->changeToValue){
				ASYNC(&player,setKey, self->changeToValue);
				SCI_WRITE(&sci0, "playing in key: ");
				SCI_WRITE(&sci0, self->buffer);						
				SCI_WRITE(&sci0, "\n");	
				self->digitPointer = 0;
			}else{
				SCI_WRITE(&sci0, "please use a value within [0,5] \n ");

			}
			
		break;
		
		case('b'):
			self->changeToValue = atoi(self->buffer);
			if(self->changeToValue >= 0){
			ASYNC(&player,setBeatLength, self->changeToValue);
			SCI_WRITE(&sci0, "New beatlength in milliseconds: ");
			SCI_WRITE(&sci0, self->buffer);						
			SCI_WRITE(&sci0, "\n");	
			self->digitPointer = 0;
			}else{
			SCI_WRITE(&sci0, "Please use a positive value: ");	
			}
			break;
				
		case('d'):
			self->changeToValue = atoi(self->buffer);
			if(self->changeToValue >= 0){
			ASYNC(&player,setBeatLength, self->changeToValue);
			SCI_WRITE(&sci0, "New deadline in milliseconds: ");
			SCI_WRITE(&sci0, self->buffer);						
			SCI_WRITE(&sci0, "\n");	
			self->digitPointer = 0;
			}else{
			SCI_WRITE(&sci0, "Please use a positive value: ");	
			}
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

void setDelay(Walkman *self, int delay){
	self -> delay = delay;
}

void setBeatLength(Walkman *self, int bl){
	self -> beatLength = bl;
}

void setKey(Walkman *self, int key){
	self->key = key;
}

// tonegenerator functions

void setPeriod(ToneGenerator *self, int period){
	self -> period = period;
}

void changeVolume(ToneGenerator *self,int volume){
	if(volume > 0 && volume < 20)
		self -> volume = volume;
}