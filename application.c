#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>

// ToneGenerator controlls the dac

#define MAX_INTEGER_SIZE 50	// Buffer size
#define MADAC ((volatile unsigned char *) 0x4000741C) // base adress for DAC

typedef struct {
	
	Object super ;
	int active ;
	int period ;
	int volume ;
	int currentDACState ;	// contains information about last sent info to 
	int delay ;	 //	delay between each note , should shorten the note 
	int beatLength ;	// duration a note is played 
	int pointer ;
	
}Walkman ;

Walkman player = { initObject(), 0, 'X' };


void startStopPlayer(Walkman *self ,int active) ;
void setBeatLength(Walkman *self ,int beatLength) ;
int getBeatLength(Walkman *self,int unused) ;
void setDelay(Walkman *self,int delay) ;
int getDelay(Walkman *self,int unused) ;
void setPeriod(Walkman *self,int period) ;
int getPeriod(Walkman *self,int unused) ;	 
void changeVolume(Walkman *self,int volume) ; // 
void playNext(Walkman *self,int unused) ;  // after




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
    int temp;
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
void printPlayerChanges(App *self, int key); 		// prints changes in vol,period.. etc




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


// Methods for app


void handleInput(App *self, int key){
	
	if (key == '-' && self->digitPointer ){	
		
		ASYNC(self, storeDigit, key);
	
	}else if ( atoi( (char *) key) <= 9 && atoi( (char *) key) >= 0 ){		// Do really need to cast here?
		
		ASYNC(self, storeDigit, key);
		
	}
	
	
	// player bindings
	switch (key){
		
		case 'v':
		ASYNC(&player, changeVolume, 0);
		ASYNC(&player, printPlayerChanges, key);
		break;
		case 'V':
		ASYNC(&player, changeVolume, 1);
		ASYNC(&player, printPlayerChanges, key);
		case 'a':
		ASYNC(&player, startStopPlayer, key);
		ASYNC(&player, printPlayerChanges, key);
		break;
		case 'b':
		ASYNC(&player, setBeatLength, key);
		ASYNC(&player, printPlayerChanges, key);
		case 'p':
		ASYNC(&player, setPeriod, 0);
		ASYNC(&player, printPlayerChanges, key);
		case 'P':
		ASYNC(&player, setPeriod, 1);
		ASYNC(&player, printPlayerChanges, key);
		break;
		case 'd':
		ASYNC(&player, setDelay, 0);
		ASYNC(&player, printPlayerChanges, key);
		break;
		case 'D':
		ASYNC(&player, setDelay, 1);
		ASYNC(&player, printPlayerChanges, key);
		break;
		
	}
	

	
}
void printPlayerChanges(App *self, int key){
	
}
void storeDigit(App *self, int digit){
	
	self->buffer[self->digitPointer] = digit ;
	self->digitPointer ++ ;
	
}


