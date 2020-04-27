// ***** 0. Documentation Section *****
// TableTrafficLight.c for Lab 10
// Runs on LM4F120/TM4C123
// Index implementation of a Moore finite state machine to operate a traffic light.  
// Daniel Valvano, Jonathan Valvano
// January 15, 2016

// east/west red light connected to PB5
// east/west yellow light connected to PB4
// east/west green light connected to PB3
// north/south facing red light connected to PB2
// north/south facing yellow light connected to PB1
// north/south facing green light connected to PB0
// pedestrian detector connected to PE2 (1=pedestrian present)
// north/south car detector connected to PE1 (1=car present)
// east/west car detector connected to PE0 (1=car present)
// "walk" light connected to PF3 (built-in green LED)
// "don't walk" light connected to PF1 (built-in red LED)

// ***** 1. Pre-processor Directives Section *****
#include "TExaS.h"
#include "tm4c123gh6pm.h"
#include "SysTick.h"

// ***** 2. Global Declarations Section *****
#define LIGHT                   (*((volatile unsigned long *)0x400050FC))
#define SENSOR                  (*((volatile unsigned long *)0x4002401C))

// FUNCTION PROTOTYPES: Each subroutine defined
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void PLL_Init(void);
void PortF_Init(void);
void Port_Init(void);

// ***** 3. Subroutines Section *****
// Linked data structure
struct State {
  unsigned long Out; 
  unsigned long Time;  
  unsigned long Next[8];
	unsigned long Out_F;}; 
typedef const struct State STyp;
#define goN   0
#define waitN 1
#define goE   2
#define waitE 3
#define goWalk 4
#define HurryUp0 5
#define HurryUp1 6
#define HurryUp2 7
#define HurryUp3 8
#define HurryUp4 9
#define HurryUp5 10	
STyp FSM[11]={
 {0x21,3000,{goN,waitN,goN,waitN,waitN,waitN,waitN,waitN},0x02}, 
 {0x22, 500,{goE,goE,goE,goE,goWalk,goE,goWalk,goE},0x02},
 {0x0C,3000,{goE,goE,waitE,waitE,waitE,waitE,waitE,waitE},0x02},
 {0x14, 500,{goWalk,goWalk,goN,goN,goWalk,goWalk,goWalk,goWalk},0x02},
 {0x24,3000,{goWalk,HurryUp0,HurryUp0,HurryUp0,goWalk,HurryUp0,HurryUp0,HurryUp0},0x08},
 {0x24,500,{HurryUp1,HurryUp1,HurryUp1,HurryUp1,HurryUp1,HurryUp1,HurryUp1,HurryUp1},0x00},
 {0x24,500,{HurryUp2,HurryUp2,HurryUp2,HurryUp2,HurryUp2,HurryUp2,HurryUp2,HurryUp2},0x02},
 {0x24,500,{HurryUp3,HurryUp3,HurryUp3,HurryUp3,HurryUp3,HurryUp3,HurryUp3,HurryUp3},0x00},
 {0x24,500,{HurryUp4,HurryUp4,HurryUp4,HurryUp4,HurryUp4,HurryUp4,HurryUp4,HurryUp4},0x02},
 {0x24,500,{HurryUp5,HurryUp5,HurryUp5,HurryUp5,HurryUp5,HurryUp5,HurryUp5,HurryUp5},0x00},
 {0x24,500,{goN,goE,goN,goN,goN,goE,goN,goN},0x02}};
unsigned long S;  // index to the current state 
unsigned long Input; 

int main(void)
{    
  TExaS_Init(SW_PIN_PE210, LED_PIN_PB543210,ScopeOff); // activate grader and set system clock to 80 MHz
	Port_Init();        // Call initialization of ports
	PLL_Init();       // 80 MHz, Program 10.1
  SysTick_Init();   // Program 10.2

  S = goN;  
  
  EnableInterrupts();
  while(1){
    LIGHT = FSM[S].Out;  // set lights
		GPIO_PORTF_DATA_R = FSM[S].Out_F;
    SysTick_Wait10ms(FSM[S].Time);
    Input = SENSOR;     // read sensors
    S = FSM[S].Next[Input];  
  }
}
 
// Subroutine to initialize port pins for input and output
// Inputs: None
// Outputs: None
// Notes: None
void Port_Init(void)
{
	volatile unsigned long delay;
	SYSCTL_RCGC2_R |= 0x32;      // 1) B E F
  delay = SYSCTL_RCGC2_R;           // delay   
	
	GPIO_PORTE_AMSEL_R &= ~0x03; // 3) disable analog function on PE1-0
  GPIO_PORTE_PCTL_R &= ~0x000000FF; // 4) enable regular GPIO
  GPIO_PORTE_DIR_R &= ~0x07;   // 5) inputs on PE2-0
  GPIO_PORTE_AFSEL_R &= ~0x07; // 6) regular function on PE2-0
  GPIO_PORTE_DEN_R |= 0x07;    // 7) enable digital on PE2-0
  GPIO_PORTB_AMSEL_R &= ~0x3F; // 3) disable analog function on PB5-0
  GPIO_PORTB_PCTL_R &= ~0x00FFFFFF; // 4) enable regular GPIO
  GPIO_PORTB_DIR_R |= 0x3F;    // 5) outputs on PB5-0
  GPIO_PORTB_AFSEL_R &= ~0x3F; // 6) regular function on PB5-0
  GPIO_PORTB_DEN_R |= 0x3F;    // 7) enable digital on PB5-0
	
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock PortF PF0  
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0       
  GPIO_PORTF_AMSEL_R = 0x00;        // 3) disable analog function
  GPIO_PORTF_PCTL_R = 0x00000000;   // 4) GPIO clear bit PCTL  
  GPIO_PORTF_DIR_R = 0x0E;          // 5) PF4,PF0 input, PF3,PF2,PF1 output   
  GPIO_PORTF_AFSEL_R = 0x00;        // 6) no alternate function
  GPIO_PORTF_PUR_R = 0x11;          // enable pullup resistors on PF4,PF0       
  GPIO_PORTF_DEN_R = 0x1F;          // 7) enable digital pins PF4-PF0      
}
// Color    LED(s) PortF
// dark     ---    0
// red      R--    0x02
// blue     --B    0x04
// green    -G-    0x08
// yellow   RG-    0x0A
// sky blue -GB    0x0C
// white    RGB    0x0E
// pink     R-B    0x06 
 
 
 
 
 /*
int main(void){ 
	volatile unsigned long delay;
	
  TExaS_Init(SW_PIN_PE210, LED_PIN_PB543210,ScopeOff); // activate grader and set system clock to 80 MHz
 
	PLL_Init();       // 80 MHz, Program 10.1
  SysTick_Init();   // Program 10.2
  
	
  S = goN;  
  
  EnableInterrupts();
  while(1){
    //LIGHT = FSM[S].Out;  // set lights
		GPIO_PORTF_DATA_R = FSM[S].Out_F;
    SysTick_Wait10ms(FSM[S].Time);
    //Input = SENSOR;     // read sensors
    S = FSM[S].Next[Input];  
  }
}
*/

