/* Author: Justin Fababier (jfaba001@ucr.edu)
 *
 * Discussion Section: 023
 * Assignment: Custom Laboratory Project Final Submission
 * 
 * I acknowledge all content contained herein, 
 * excluding template or example code, is my own original work.
 * 
 * Demo Link: https://youtu.be/c5ZJR1UjT2s
 */

#include "helper.h"
#include "objects.h"
#include "periph.h"
#include "serialATmega.h"
#include "spiAVR.h"
#include "ST7735.h"
#include "timerISR.h"

#define NUM_TASKS 11

// Task struct for concurrent synchSMs implementations
typedef struct _task{
	signed 		char state;		//Task's current state
	unsigned long period;			//Task period
	unsigned long elapsedTime;		//Time elapsed since last task tick
	int (*TickFct)(int);			//Task tick function
} task;

// Periods for each task
const unsigned long GCD_PERIOD = 1;     // Greatest Common Divisor for task periods
const unsigned long SM1_PERIOD = 1; 	// Period of state machine 1 - Initialize game screen
const unsigned long SM2_PERIOD = 100; 	// Period of state machine 2 - Passive buzzer for background music
const unsigned long SM3_PERIOD = 10;    // Period of state machine 3 - dino_1 jumping mechanic
const unsigned long SM4_PERIOD = 5;     // Period of state machine 4 - dino_1 obstacle generation & animation
const unsigned long SM5_PERIOD = 1;     // Period of state machine 5 - Game start handle
const unsigned long SM6_PERIOD = 1;     // Period of state machine 6 - Game end handle
const unsigned long SM7_PERIOD = 1;     // Period of state machine 7 - LEDs for game state
const unsigned long SM8_PERIOD = 1;     // Period of state machine 8 - Restart game
const unsigned long SM9_PERIOD = 1;     // Period of state machine 9 - Handle introducing player 2
const unsigned long SM10_PERIOD = 10;   // Period of state machine 10 - dino_2 jumping mechanic
const unsigned long SM11_PERIOD = 5;    // Period of state machine 11 - dino_2 obstacle generation & animation

task tasks[NUM_TASKS]; // declared task array with 5 tasks

// Enums and _Tick functions
enum SM1_States {SM1_INIT, SM1_IDLE};
enum SM2_States {SM2_INIT, SM2_IDLE, SM2_PLAY_NOTE};
enum SM3_States {SM3_INIT, SM3_WAIT, SM3_JUMP, SM3_HOLD, SM3_DESCEND};
enum SM4_States {SM4_INIT, SM4_IDLE, SM4_ANIMATE_CACTUS, SM4_ANIMATE_BIRD};
enum SM5_States {SM5_INIT, SM5_IDLE, SM5_HOLD_ON, SM5_HOLD_OFF};
enum SM6_States {SM6_INIT, SM6_IDLE, SM6_CHECK_COLLISION, SM6_GAME_OVER};
enum SM7_States {SM7_INIT, SM7_LED};
enum SM8_States {SM8_INIT, SM8_IDLE, SM8_HOLD_ON, SM8_HOLD_OFF};
enum SM9_States {SM9_INIT, SM9_IDLE, SM9_HOLD_ON, SM9_HOLD_OFF};
enum SM10_States {SM10_INIT, SM10_WAIT, SM10_JUMP, SM10_HOLD, SM10_DESCEND};
enum SM11_States {SM11_INIT, SM11_IDLE, SM11_ANIMATE_CACTUS, SM11_ANIMATE_BIRD};
int SM1_Tick (int state);
int SM2_Tick (int state);
int SM3_Tick (int state);
int SM4_Tick (int state);
int SM5_Tick (int state);
int SM6_Tick (int state);
int SM7_Tick (int state);
int SM8_Tick (int state);
int SM9_Tick (int state);
int SM10_Tick (int state);
int SM11_Tick (int state);

void TimerISR() {
	for (unsigned int i = 0; i < NUM_TASKS; i++) {					// Iterate through each task in the task array
		if (tasks[i].elapsedTime == tasks[i].period) {				// Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state);		// Tick and set the next state for this task
			tasks[i].elapsedTime = 0;								// Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;							// Increment the elapsed time by GCD_PERIOD
	}
}

// Global variables start here
enum GameStates { GAME_IDLE, GAME_IN_PROGRESS, GAME_OVER } gameState;

// Music variables
const int noteICR1[] = {
    159, 159, 145, 145, 129, 129, 122, 122,
    129, 129, 122, 122, 108, 108, 96, 96,
    90, 90, 80, 80, 96, 96, 80, 80,
    108, 108, 122, 122, 129, 129, 145, 145
};
const int numNotes = sizeof(noteICR1) / sizeof(noteICR1[0]);
unsigned char currentNote = 0; // Index to keep track of the current note

// Playable characters
Character dino_1 = {6, 100, 16, 24, true};
Character dino_2 = {6, 48, 16, 24, false};

// Obstacles 
Obstacle bird_1 = {128, 80, 16, 24, true};
Obstacle bird_2 = {128, 28, 24, 16, false};
Obstacle cactus_1 = {128, 104, 24, 16, false};
Obstacle cactus_2 = {128, 52, 16, 24, false};

// Collision variables
bool collisionBird_1, collisionCactus_1, collisionBird_2, collisionCactus_2;

// State machine 1 - Initialize game
int SM1_Tick (int state) {
    switch(state) { // State transitions
        case SM1_INIT:
            Clear_Screen_With_Color(0xFFFF);
            if (dino_1.alive == true) {
                drawDinosaur(dino_1);
            }
            if (bird_1.active == true) {
                drawBird(bird_1);
            }
            if (cactus_1.active == true) {
                drawCactus(cactus_1);
            }
            gameState = GAME_IDLE;
            state = SM1_IDLE;
            break;

        case SM1_IDLE:
            state = SM1_IDLE;
            break;

        default:
            break;
    } // State transitions end

    switch(state) { // State actions
        case SM1_INIT:
            break;

        case SM1_IDLE:
            break;

        default:
            break;
    } // State actions end

    return state;
}

// State machine 2 - Passive buzzer for background music
int SM2_Tick(int state) {
    // Configure Timer1 for Phase and Frequency Correct PWM mode
    TCCR1A = (1 << WGM11) | (1 << COM1A1);
    TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS11);
    TCCR1B = (TCCR1B & 0xF8) | 0x04; // set prescaler

    switch(state) { // State transitions
        case SM2_INIT:
            state = SM2_IDLE;
            break;

        case SM2_IDLE:
            if (gameState == GAME_IN_PROGRESS) {
                state = SM2_PLAY_NOTE;
            } 
            else {
                ICR1 = 0;
                state = SM2_IDLE;
            }
            break;

        case SM2_PLAY_NOTE:
            if (gameState == GAME_IN_PROGRESS) {
                state = SM2_PLAY_NOTE;
            }
            else {
                state = SM2_IDLE;
            }
            break;

        default:
            state = SM2_INIT;
            break;
    } // State transitions end

    switch(state) { // State actions
        case SM2_INIT:
            break;

        case SM2_IDLE:
            break;

        case SM2_PLAY_NOTE:
            // Move to the next note
            currentNote++;
            if (currentNote >= numNotes) {
                currentNote = 0; // Loop back to the first note
            }
            ICR1 = noteICR1[currentNote]; // Update ICR1 to play the next note
            break;

        default:
            break;
    } // State actions end

    return state;
}

// State machine 3 - dino_1 jumping mechanic
int SM3_Tick (int state) {
    static unsigned int i;

    switch(state) { // State transitions
        case SM3_INIT:
            state = SM3_WAIT;
            break;

        case SM3_WAIT:
            if (ADC_read(0) > 960) {
                i = 0;
                state = SM3_JUMP;
            }
            else {
                state = SM3_WAIT;
            }
            break;

        case SM3_JUMP:
            if (i == 16) {
                i = 0;
                state = SM3_HOLD;
            }
            break;

        case SM3_HOLD:
            if (i == 16) {
                i = 0;
                state = SM3_DESCEND;
            }
            break;

        case SM3_DESCEND:
            if (i == 16) {
                i = 0;
                state = SM3_WAIT;
            }
            break;

        default:
            break;
    } // State transitions end

    switch(state) { // State actions
        case SM3_INIT:
            break;

        case SM3_WAIT:
            break;

        case SM3_JUMP:
            deleteBitmap(dino_1.x, dino_1.y, 0, 0);
            dino_1.y = dino_1.y - 2;
            drawDinosaur(dino_1);
            i++;
            break;

        case SM3_HOLD:
            deleteBitmap(dino_1.x, dino_1.y, 0, 0);
            dino_1.y = dino_1.y;
            drawDinosaur(dino_1);
            i++;
            break;

        case SM3_DESCEND:
            deleteBitmap(dino_1.x, dino_1.y, 0, 0);
            dino_1.y = dino_1.y + 2;
            drawDinosaur(dino_1);
            i++;
            break;

        default:
            break;
    } // State actions end

    return state;
}

// State machine 4 - Obstacle animation & generation for player 1
int SM4_Tick (int state) {
    static signed int i;

    switch(state) { // State transitions
        case SM4_INIT:
            state = SM4_IDLE;
            break;

        case SM4_IDLE:
            if (gameState == GAME_IDLE) {
                state = SM4_IDLE;
            }
            if (gameState == GAME_IN_PROGRESS) {
                state = SM4_ANIMATE_CACTUS;
            }
            break;

        case SM4_ANIMATE_CACTUS:
            if (gameState == GAME_OVER) {
                state = SM4_IDLE;
            }
            if (i == 4) {
                i = 0;
                state = SM4_ANIMATE_BIRD;
            }
            break;

        case SM4_ANIMATE_BIRD:
            if (gameState == GAME_OVER) {
                state = SM4_IDLE;
            }
            if (i == 1) {
                i = 0;
                state = SM4_ANIMATE_CACTUS;
            }
            break;

        default:
            break;
    } // State transitions end

    switch(state) { // State actions
        case SM4_INIT:
            break;

        case SM4_IDLE:
            break;

        case SM4_ANIMATE_CACTUS:
            deleteBitmap(cactus_1.x, cactus_1.y, 0, 0);
            cactus_1.x = cactus_1.x - 1;
            if (cactus_1.x == -24) {
                cactus_1.x = 128 - cactus_1.width;
                i++;
            }
            drawCactus(cactus_1);
            break;

        case SM4_ANIMATE_BIRD:
            deleteBitmap(bird_1.x, bird_1.y, 0, 0);
            bird_1.x = bird_1.x - 1;
            if (bird_1.x == -24) {
                bird_1.x = 128 - bird_1.width;
                i++;
            }
            drawBird(bird_1);
            break;

        default:
            break;
    } // State actions end

    return state;
}

// State machine 5 - Game start handler
int SM5_Tick (int state) {
    switch(state) { // State transitions
        case SM5_INIT:
            state = SM5_IDLE;
            break;

        case SM5_IDLE:
            if ((GetBit(PINC, 2) == 1) || (gameState == GAME_IN_PROGRESS) || (gameState == GAME_OVER)) {
                state = SM5_IDLE;
            }
            else if (GetBit(PINC, 2) == 0) {
                state = SM5_HOLD_ON;
            }
            break;

        case SM5_HOLD_ON:
            if (GetBit(PINC, 2) == 0) {
                state = SM5_HOLD_ON;
            }
            else {
                state = SM5_HOLD_OFF;
            }
            break;
        
        case SM5_HOLD_OFF:
            state = SM5_IDLE;
            break;

        default:
            break;
    } // State transitions end

    switch(state) { // State actions
        case SM5_INIT:
            break;

        case SM5_IDLE:
            break;

        case SM5_HOLD_ON:
            break;
        
        case SM5_HOLD_OFF:
            gameState = GAME_IN_PROGRESS;
            break;

        default:
            break;
    } // State actions end

    return state;
}

// State machine 6 - Game end handler
int SM6_Tick (int state) {
    switch(state) { // State transitions
        case SM6_INIT:
            state = SM6_IDLE;
            break;
        
        case SM6_IDLE:
            if ((gameState == GAME_IDLE) || (gameState == GAME_OVER)) {
                state = SM6_IDLE;
            }
            if (gameState == GAME_IN_PROGRESS) {
                state = SM6_CHECK_COLLISION;
            }
            break;

        case SM6_CHECK_COLLISION:
            if (collisionCactus_1 || collisionBird_1 || collisionCactus_2 || collisionBird_2) {
                state = SM6_GAME_OVER;
            }
            else {
                state = SM6_CHECK_COLLISION;
            }
            break;

        case SM6_GAME_OVER:
            if (gameState == GAME_IDLE) {
                state = SM6_IDLE;
            }
            else {
                state = SM6_GAME_OVER;
            }
            break;

        default:
            break;
    } // State transitions end

    switch(state) { // State actions
        case SM6_INIT:
            break;

        case SM6_IDLE:
            break;

        case SM6_CHECK_COLLISION:
            collisionCactus_1 = checkCollision(dino_1.x, dino_1.y, dino_1.width, dino_1.height, cactus_1.x, cactus_1.y, cactus_1.width, cactus_1.height);
            collisionBird_1 = checkCollision(dino_1.x, dino_1.y, dino_1.width, dino_1.height, bird_1.x, bird_1.y, bird_1.width, bird_1.height);
            collisionCactus_2 = checkCollision(dino_2.x, dino_2.y, dino_2.width, dino_2.height, cactus_2.x, cactus_2.y, cactus_2.width, cactus_2.height);
            collisionBird_2 = checkCollision(dino_2.x, dino_2.y, dino_2.width, dino_2.height, bird_2.x, bird_2.y, bird_2.width, bird_2.height);
            break;

        case SM6_GAME_OVER:
            gameState = GAME_OVER;
            break;

        default:
            break;
    } // State actions end

    return state;
}

// State machine 7 - LEDs for game state
int SM7_Tick (int state) {
    switch(state) { // State transitions
        case SM7_INIT:
            state = SM7_LED;
            break;
        
        case SM7_LED:
            state = SM7_LED;
            break;

        default:
            break;
    } // State transitions end

    switch(state) { // State actions
        case SM7_INIT:
            break;

        case SM7_LED:
            if (gameState == GAME_IDLE) {
                PORTD = SetBit(PORTD, 2, 0);
                PORTD = SetBit(PORTD, 3, 1);
                PORTD = SetBit(PORTD, 4, 0);
            }
            if (gameState == GAME_IN_PROGRESS) {
                PORTD = SetBit(PORTD, 2, 0);
                PORTD = SetBit(PORTD, 3, 0);
                PORTD = SetBit(PORTD, 4, 1);
            }
            if (gameState == GAME_OVER) {
                PORTD = SetBit(PORTD, 2, 1);
                PORTD = SetBit(PORTD, 3, 0);
                PORTD = SetBit(PORTD, 4, 0);
            }
            break;

        default:
            break;
    } // State actions end

    return state;
}

// State machine 8 - Restart game
int SM8_Tick (int state) {
    switch(state) { // State transitions
        case SM8_INIT:
            state = SM8_IDLE;
            break;
        
        case SM8_IDLE:
            if (gameState != GAME_OVER) {
                state = SM8_IDLE;
            }
            else if ((gameState == GAME_OVER) && (GetBit(PINC, 2) == 0)) {
                state = SM8_HOLD_ON;
            }
            break;

        case SM8_HOLD_ON:
            if (GetBit(PINC, 2) == 0) {
                state = SM8_HOLD_ON;
            }
            else {
                state = SM8_HOLD_OFF;
            }
            break;

        case SM8_HOLD_OFF:
            state = SM8_IDLE;
            break;

        default:
            break;
    } // State transitions end

    switch(state) { // State actions
        case SM8_INIT:
            break;

        case SM8_IDLE:
            break;

        case SM8_HOLD_ON:
            break;

        case SM8_HOLD_OFF:
            Clear_Screen_With_Color(0xFFFF);
            dino_1.x = 6;
            dino_1.y = 100;;
            bird_1.x = 128;
            bird_1.y = 80;
            cactus_1.x = 128;
            cactus_1.y = 104;
            dino_2.x = 6;
            dino_2.y = 48;
            bird_2.x = 128;
            bird_2.y = 28;
            cactus_2.x = 128;
            cactus_2.y = 52;
            drawDinosaur(dino_1);
            drawBird(bird_1);
            drawCactus(cactus_1);
            drawBird(bird_1);
            if (dino_2.alive) {
                drawDinosaur(dino_2);
            }
            drawCactus(cactus_2);
            gameState = GAME_IDLE;
            break;

        default:
            break;
    } // State actions end

    return state;
}

// State machine 9 - Introduce player two
int SM9_Tick (int state) {
    switch(state) { // State transitions
        case SM9_INIT:
            state = SM9_IDLE;
            break;

        case SM9_IDLE:
            if (gameState != GAME_IDLE) {
                state = SM9_IDLE;
                break;
            }
            else if ((gameState == GAME_IDLE) && (GetBit(PINC, 5) == 0)) {
                state = SM9_HOLD_ON;
            }
            break;

        case SM9_HOLD_ON:
            if (GetBit(PINC, 5) == 0) {
                state = SM9_HOLD_ON;
            }
            else {
                state = SM9_HOLD_OFF;
            }
            break;
        
        case SM9_HOLD_OFF:
            state = SM9_IDLE;
            break;

        default:
            break;
    } // State transitions end

    switch(state) { // State actions
        case SM9_INIT:
            break;

        case SM9_IDLE:
            break;

        case SM9_HOLD_ON:
            break;
        
        case SM9_HOLD_OFF:
            if (dino_2.alive == false) {
                dino_2.alive = true;
                drawDinosaur(dino_2);
            }
            else if (dino_2.alive == true) {
                dino_2.alive = false;
                deleteBitmap(dino_2.x, dino_2.y, 36, 36);
            }
            break;

        default:
            break;
    } // State actions end

    return state;
}

// State machine 10 - dino_2 jumping mechanic
int SM10_Tick(int state) {
    static unsigned int i;

    switch(state) { // State transitions
        case SM10_INIT:
            state = SM10_WAIT;
            break;

        case SM10_WAIT:
            if (dino_2.alive == false) {
                break;
            }
            if (ADC_read(3) > 960) {
                i = 0;
                state = SM10_JUMP;
            }
            else {
                state = SM10_WAIT;
            }
            break;

        case SM10_JUMP:
            if (i == 16) {
                i = 0;
                state = SM10_HOLD;
            }
            break;

        case SM10_HOLD:
            if (i == 16) {
                i = 0;
                state = SM10_DESCEND;
            }
            break;

        case SM10_DESCEND:
            if (i == 16) {
                i = 0;
                state = SM10_WAIT;
            }
            break;

        default:
            break;
    } // State transitions end

    switch(state) { // State actions
        case SM10_INIT:
            break;

        case SM10_WAIT:
            break;

        case SM10_JUMP:
            deleteBitmap(dino_2.x, dino_2.y, 0, 0);
            dino_2.y = dino_2.y - 2;
            drawDinosaur(dino_2);
            i++;
            break;

        case SM10_HOLD:
            deleteBitmap(dino_2.x, dino_2.y, 0, 0);
            dino_2.y = dino_2.y;
            drawDinosaur(dino_2);
            i++;
            break;

        case SM10_DESCEND:
            deleteBitmap(dino_2.x, dino_2.y, 0, 0);
            dino_2.y = dino_2.y + 2;
            drawDinosaur(dino_2);
            i++;
            break;

        default:
            break;
    } // State actions end

    return state;
}

// State machine 11 - Obstacle animation & generation for player 2
int SM11_Tick (int state) {
    static signed int i;

    switch(state) { // State transitions
        case SM11_INIT:
            state = SM4_IDLE;
            break;

        case SM11_IDLE:
            if (dino_2.alive == false) {
                break;
            }
            if (gameState == GAME_IDLE) {
                state = SM11_IDLE;
            }
            if (gameState == GAME_IN_PROGRESS) {
                state = SM11_ANIMATE_CACTUS;
            }
            break;

        case SM11_ANIMATE_CACTUS:
            if (gameState == GAME_OVER) {
                state = SM11_IDLE;
            }
            if (i == 4) {
                i = 0;
                state = SM11_ANIMATE_BIRD;
            }
            break;

        case SM11_ANIMATE_BIRD:
            if (gameState == GAME_OVER) {
                state = SM11_IDLE;
            }
            if (i == 1) {
                i = 0;
                state = SM11_ANIMATE_CACTUS;
            }
            break;

        default:
            break;
    } // State transitions end

    switch(state) { // State actions
        case SM11_INIT:
            break;

        case SM11_IDLE:
            break;

        case SM11_ANIMATE_CACTUS:
            deleteBitmap(cactus_2.x, cactus_2.y, 0, 0);
            cactus_2.x = cactus_2.x - 1;
            if (cactus_2.x == -24) {
                cactus_2.x = 128 - cactus_2.width;
                i++;
            }
            drawCactus(cactus_2);
            break;

        case SM11_ANIMATE_BIRD:
            deleteBitmap(bird_2.x, bird_2.y, 0, 0);
            bird_2.x = bird_2.x - 1;
            if (bird_2.x == -24) {
                bird_2.x = 128 - bird_2.width;
                i++;
            }
            drawBird(bird_2);
            break;

        default:
            break;
    } // State actions end

    return state;
}

int main(void) {
	// Initialize inputs and outputs
	DDRB = 0x2F; PORTB = 0x10;
	DDRC = 0x00; PORTC = 0xFF;
	DDRD = 0x9C; PORTD = 0x00;

	ADC_init();         // initializes ADC
	serial_init(9600);  // initializes serial
	SPI_INIT();         // initalizes SPI
	ST7735_init();      // initializes ST7735
	columnSet();        // initializes LCD columns
	rowSet();           // initializes LCD rows

	// Task 1
	unsigned char i = 0;
	tasks[i].state = SM1_INIT;
	tasks[i].period = SM1_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SM1_Tick;

	// Task 2
	i++;
	tasks[i].state = SM2_INIT;
	tasks[i].period = SM2_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SM2_Tick;

	// Task 3
	i++;
	tasks[i].state = SM3_INIT;
	tasks[i].period = SM3_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SM3_Tick;

	// Task 4
	i++;
	tasks[i].state = SM4_INIT;
	tasks[i].period = SM4_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SM4_Tick;

	// Task 5
	i++;
	tasks[i].state = SM5_INIT;
	tasks[i].period = SM5_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SM5_Tick;

	// Task 6
	i++;
	tasks[i].state = SM6_INIT;
	tasks[i].period = SM6_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SM6_Tick;

	// Task 7
	i++;
	tasks[i].state = SM7_INIT;
	tasks[i].period = SM7_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SM7_Tick;

	// Task 8
	i++;
	tasks[i].state = SM8_INIT;
	tasks[i].period = SM8_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SM8_Tick;

	// Task 9
	i++;
	tasks[i].state = SM9_INIT;
	tasks[i].period = SM9_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SM9_Tick;

	// Task 10
	i++;
	tasks[i].state = SM10_INIT;
	tasks[i].period = SM10_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SM10_Tick;

	// Task 11
	i++;
	tasks[i].state = SM11_INIT;
	tasks[i].period = SM11_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SM11_Tick;

	TimerSet(GCD_PERIOD);
	TimerOn();

	while (1) {}

	return 0;
}
