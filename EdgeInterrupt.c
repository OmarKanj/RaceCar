/*
    Project Name: Race Car
    Authors:
        Omar Kanj
        Fadi Jarray
*/

#include <stdint.h> // C99 data types
#include "tm4c123gh6pm.h"
#include "SysTick.h"

// Port E Def
#define GPIO_PORTE_DATA_R       (*((volatile unsigned long *)0x400243FC))
#define GPIO_PORTE_DIR_R        (*((volatile unsigned long *)0x40024400))
#define GPIO_PORTE_AFSEL_R      (*((volatile unsigned long *)0x40024420))
#define GPIO_PORTE_DEN_R        (*((volatile unsigned long *)0x4002451C))
#define GPIO_PORTE_CR_R         (*((volatile unsigned long *)0x40024524))
#define GPIO_PORTE_AMSEL_R      (*((volatile unsigned long *)0x40024528))
#define GPIO_PORTE_PCTL_R       (*((volatile unsigned long *)0x4002452C))

// Port A Def
#define GPIO_PORTA_DATA_R       (*((volatile unsigned long *)0x400043FC))
#define GPIO_PORTA_DIR_R        (*((volatile unsigned long *)0x40004400))
#define GPIO_PORTA_AFSEL_R      (*((volatile unsigned long *)0x40004420))
#define GPIO_PORTA_DEN_R        (*((volatile unsigned long *)0x4000451C))
#define GPIO_PORTA_CR_R         (*((volatile unsigned long *)0x40004524))
#define GPIO_PORTA_AMSEL_R      (*((volatile unsigned long *)0x40004528))
#define GPIO_PORTA_PCTL_R       (*((volatile unsigned long *)0x4000452C))

// Port F Def
#define GPIO_PORTF_DATA_R       (*((volatile unsigned long *)0x400253FC))
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_AFSEL_R      (*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_PUR_R        (*((volatile unsigned long *)0x40025510))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_LOCK_R       (*((volatile unsigned long *)0x40025520))
#define GPIO_PORTF_CR_R         (*((volatile unsigned long *)0x40025524))
#define GPIO_PORTF_AMSEL_R      (*((volatile unsigned long *)0x40025528))
#define GPIO_PORTF_PCTL_R       (*((volatile unsigned long *)0x4002552C))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
//clock
#define NVIC_PRI7_R             (*((volatile unsigned long *)0xE000E41C))
#define NVIC_EN0_R              (*((volatile unsigned long *)0xE000E100))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))

#define NVIC_ST_CTRL_R          (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R        (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile unsigned long *)0xE000E018))
#define NVIC_ST_CTRL_COUNT      0x00010000  // Count flag
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
#define NVIC_ST_RELOAD_M        0x00FFFFFF  // Counter load value

void move_clkwise();
void move_anticlkwise();

void DisableInterrupts(); // Disable interrupts
void EnableInterrupts();  // Enable interrupts
void WaitForInterrupt();  // Go to low power mode while waiting for the next interrupt

struct State {
	unsigned long Output;
	unsigned long Time;    // 0.1s units
	unsigned long Next[4]; // list of next states
};
typedef const struct State STyp;

#define phase1 0			// Output is 0x03
#define phase2 1		    // Output is 0x06
#define phase3 2			// Output is 0x0C
#define phase4 3		    // Output is 0x09


STyp FSM[4] = {
{ 0x01, 0,{phase1,phase2,phase4,phase1}}, 	      	            // phase1
{ 0x02, 0,{phase2,phase3,phase1,phase2}}, 						// phase2
{ 0x04, 0,{phase3,phase4,phase2,phase3},}, 						// phase3
{ 0x08, 0,{phase4,phase1,phase3,phase4},}						// phase4
};

#define SENSOR 	                (*((volatile unsigned long *)0x40004050))   // Buttons A1 , A2, A3
#define LIGHTS                  (*((volatile unsigned long *)0x400243FC))	// accesses PE5(RedE)-PE4(YellowE)-PE3(GreenE)-PE2(RedS)-PE1(YellowS)�PE0(GreenS)
#define LIGHTS_F                (*((volatile unsigned long *)0x40025078))	// ACCESS F1 and F2

// Function Prototypes
void portA_init(void);
void portF_init(void);
void portE_init(void);
void portB_init(void);
void SysTick_Wait500ms(unsigned long delay);
void PortF_LEDInit();     // Initialize Port F LEDs
void SysTick_Handler();   // Handle SysTick generated interrupts


// FSM variables
unsigned long CS;
unsigned long Input;

// Handler variables
unsigned long i;
unsigned long input_right;
unsigned long input_left;
unsigned long irsensor;
int delay;
int steps;

int main(void){
	portE_init();
  portF_init();
	portA_init();
	EnableInterrupts();
	LIGHTS = FSM[Input].Output;

  while(1) {
    WaitForInterrupt();
  }
}

void portE_init(void){
    volatile unsigned long delay;
	SYSCTL_RCGC2_R |= 0x0000010;    //Port E Clock
    delay = SYSCTL_RCGC2_R;         // delay
    GPIO_PORTE_CR_R = 0xFF;         // allow changes to ALL P0-P7 (LEDs)
    GPIO_PORTE_AMSEL_R = 0x00;      // disables analog functions
    GPIO_PORTE_PCTL_R = 0x00000000; //GPIO clear bit PCTL
    GPIO_PORTE_DIR_R = 0xFF;        // set direction to ouput ALL P0-P7
    GPIO_PORTE_AFSEL_R = 0x00;      // no alternate functions
    GPIO_PORTE_DEN_R = 0xFF;        // enable digital pins ALL P0-P7
}

void portA_init(void){
    volatile unsigned long delay;
	SYSCTL_RCGC2_R |= 0x00000001;   //Port A Clock
    delay = SYSCTL_RCGC2_R;         // delay
    GPIO_PORTA_CR_R = 0x1C;         // allow changes to PA4 - PA2 (buttons)
    GPIO_PORTA_AMSEL_R = 0x00;      // disables analog functions
    GPIO_PORTA_PCTL_R = 0x00000000; //GPIO clear bit PCTL
    GPIO_PORTA_DIR_R = 0x00;        // set direction to input for PA4 - PA2
    GPIO_PORTA_AFSEL_R = 0x00;      // no alternate functions
    GPIO_PORTA_DEN_R = 0x1C;        // enable digital pins  PA4 - PA2
}

void portB_init(void){
    volatile unsigned long delay;
	SYSCTL_RCGC2_R |= 0x00000002;   //Port A Clock
    delay = SYSCTL_RCGC2_R;         // delay
    GPIO_PORTE_CR_R = 0xFF;         // allow changes to ALL P0-P7 (LEDs)
    GPIO_PORTE_AMSEL_R = 0x00;      // disables analog functions
    GPIO_PORTE_PCTL_R = 0x00000000; //GPIO clear bit PCTL
    GPIO_PORTE_DIR_R = 0xFF;        // set direction to ouput ALL P0-P7
    GPIO_PORTE_AFSEL_R = 0x00;      // no alternate functions
    GPIO_PORTE_DEN_R = 0xFF;        // enable digital pins ALL P0-P7
}

void portF_init(void){
    volatile unsigned long delay;
    SYSCTL_RCGC2_R |= 0x00000020;     // 1) F clock
    delay = SYSCTL_RCGC2_R;           // delay
    GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock PortF PF0
    GPIO_PORTF_CR_R = 0x11;           // allow changes to PF4-0
    GPIO_PORTF_AMSEL_R = 0x00;        // 3) disable analog function
    GPIO_PORTF_PCTL_R = 0x00000000;   // 4) GPIO clear bit PCTL
    GPIO_PORTF_DIR_R = 0x00;          // 5) PF4,PF0 input
    GPIO_PORTF_AFSEL_R = 0x00;        // 6) no alternate function
    GPIO_PORTF_PUR_R = 0x11;          // enable pullup resistors on PF4,PF0
    GPIO_PORTF_DEN_R = 0x11;          // 7) enable digital pins PF4-PF0
    GPIO_PORTF_IS_R &= ~0x11;
    GPIO_PORTF_IBE_R &= ~0x11;
    GPIO_PORTF_IEV_R &= ~0x11;
    GPIO_PORTF_ICR_R = 0x11;
    GPIO_PORTF_IM_R |= 0x11;
    NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF) | 0x00020000; // PRIORITY 1
    NVIC_EN0_R = 0x40000000;
}

void GPIOPortF_Handler() {
    GPIO_PORTF_ICR_R |= 0x11;
    input_left = ((GPIO_PORTF_DATA_R&0x10) >> 4);
	input_right =((GPIO_PORTF_DATA_R&0x01));
	if(input_left == 0x01){
		Input = phase1;
		input_left = 0x00;
		input_right = 0x00;
		for(steps = 0; steps < 700; steps++) {
            for(i = 0; i < 4; i++) {
                Input = FSM[Input].Next[1];
                LIGHTS = FSM[Input].Output;
                irsensor = SENSOR&0x04;
                if (!(irsensor == 0x04)){
                    LIGHTS |= 0x20;
                    break;
                }
                for(delay = 0; delay < 5000;delay++);
            }
	    }
	}
	if(input_right == 0x01){
		Input = phase4;
		input_left = 0x00;
		input_right = 0x00;
		for(steps = 0; steps < 250; steps++) {
            for(i = 0; i < 4; i++) {
                Input = FSM[Input].Next[2];
                LIGHTS = FSM[Input].Output;
                for(delay = 0; delay < 5000;delay++);
            }
	    }
	}
}
