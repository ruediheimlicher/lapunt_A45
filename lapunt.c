//
//  Created by Sysadmin on 03.10.07.
//  Copyright Ruedi Heimlihcer 2021. All rights reserved.
//

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include "defines.h"
#include <avr/power.h>

//#include "lcd.c"

uint16_t loopCount0=0;
uint16_t loopCount1=0;
uint16_t loopCount2=0;

#define OSZIPORT   PORTB      // Eingang fuer Oszi
#define OSZIDDR   DDRB

#define OSZIA 6            // 
#define OSZIB 7            // 
#define OSZIALO OSZIPORT &= ~(1<<OSZIA)
#define OSZIAHI OSZIPORT |= (1<<OSZIA)
#define OSZIATOG OSZIPORT ^= (1<<OSZIA)

#define OSZIBLO OSZIPORT &= ~(1<<OSZIB)
#define OSZIBHI OSZIPORT |= (1<<OSZIB)
#define OSZIBTOG OSZIPORT ^= (1<<OSZIB)


#define LOOPLED_PORT   PORTB
#define LOOPLED_DDR   DDRB
#define LOOPLED_PIN   2

#define WDT_PORT   PORTB
#define WDT_DDR   DDRB
#define WDT_PIN   PINB

//#define WDT_LED_PIN   2 // LED
#define WDT_INT_PIN   0 // Taster


#define RELAIS_PORT  PORTB
#define RELAIS_DDR   DDRB
#define RELAIS_PIN   PINB

#define RELAIS_ON 3
#define RELAIS_OFF 4

#define RELAIS_ENABLE   2

#define ANZAHL_IMPULSE 2

volatile uint8_t   ThermoStatus=0x00;   // Status des Eingangs

//volatile uint8_t INT0counter = 0; // Anzahl impulse auf Spule

//volatile uint8_t impulscounter = 0; // Anzahl impulse an relais: Sicher ist sicher

volatile uint8_t statusSleep = 0;

volatile uint8_t WDTcounter = 0;

//https://arduino-projekte.webnode.at/meine-projekte/attiny-im-schlafmodus/wecken-mit-pci/

void slaveinit(void)
{
   //  LOOPLED_DDR |= (1<<LOOPLED_PIN);
     OSZIDDR |= (1<<OSZIA);
     OSZIDDR |= (1<<OSZIB);
     
     OSZIPORT |= (1<<OSZIA);
     OSZIPORT |= (1<<OSZIB);
        
     RELAIS_DDR  |= (1<<RELAIS_ENABLE); // output
     RELAIS_PORT &= ~(1<<RELAIS_ENABLE);// HI
     
     
     RELAIS_DDR  |= (1<<RELAIS_ON); // output
     RELAIS_PORT &= ~(1<<RELAIS_ON);// LO
        
     RELAIS_DDR  |= (1<<RELAIS_OFF);// Output
     RELAIS_PORT &= ~(1<<RELAIS_OFF);// LO
        
     WDT_DDR &= ~(1<<WDT_INT_PIN); //PB0  input
     //WDT_PORT |= (1<<WDT_INT_PIN); // pulldown
      
     ThermoStatus = 0;
}
// 
void watchdog_init()
{
  // watchdog konfigurieren
  WDTCR = (1 << WDCE)|(1 << WDE);  // zunaechst Schutz des Registers aufheben gemaess Datasheet
  WDTCR = (1 << WDIE)|(1 << WDP3);  // Interruptmode an, Zeit auf 8s einstellen
//   WDTCSR = (1 << WDIE)|(1 << WDP2)|(1 << WDP1) |(1 << WDP0);  // Interruptmode an, Zeit auf 2s einstellen
}

ISR(WDT_vect)
{
   OSZIATOG;
   
   WDTcounter++;
   if (WDTcounter > ANZAHL_IMPULSE)
   {
     // OSZIATOG;
      statusSleep = 1;
      WDTcounter = 0;
   }
   //OSZIBHI;
} 

void main (void) 
{   
   slaveinit();
   watchdog_init();
   set_sleep_mode(SLEEP_MODE_PWR_DOWN);
   
   sei();
#pragma mark while
   while (1) 
   {
      if (statusSleep)
      {
         //OSZIBTOG;
         RELAIS_PORT |= (1<<RELAIS_ENABLE);// HI
         _delay_ms(50); // Aufladen des Kondensators
         
         if (WDT_PIN & (1<<WDT_INT_PIN)) // Pin ist HI
         {
            //  OSZIATOG;
            if (! (ThermoStatus & (1<<RELAIS_ON))) // Status noch nicht gesetzt
            { 
                ThermoStatus |= (1<<RELAIS_ON); //Zustand merken
               ThermoStatus &= ~(1<<RELAIS_OFF);
               RELAIS_PORT |= (1<<RELAIS_ON); // ON-Spule EIN
               _delay_ms(200);
               RELAIS_PORT &= ~(1<<RELAIS_ON); // ON-Spule AUS
             }
         }
         else
         {
            if (! (ThermoStatus & (1<<RELAIS_OFF))) // Status noch nicht gesetzt
            {
               ThermoStatus |= (1<<RELAIS_OFF); //Zustand merken
               ThermoStatus &= ~(1<<RELAIS_ON);
               RELAIS_PORT |= (1<<RELAIS_OFF);// ON-Spule EIN
               _delay_ms(200);
               RELAIS_PORT &= ~(1<<RELAIS_OFF); // ON-Spule AUS
            }
         }
         RELAIS_PORT &= ~(1<<RELAIS_ENABLE);
         //OSZIBHI;
         statusSleep = 0;
         //OSZIAHI;
      } // if statusSleep
      sleep_mode();
   }
   return ;
}
