/*
 * File:   postlab9.c
 * Author: Carolina Paz
 *
 * Created on 29 de abril de 2022, 11:19 AM
 */

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = ON         // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

#include <xc.h>
#include <stdint.h>

/*------------------------------------------------------------------------------
 * CONSTANTES 
 ------------------------------------------------------------------------------*/
#define _XTAL_FREQ 4000000    // Frecuencia de oscilador
#define _tmr0_value 255       // valor para reinicio del TMR0

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);

/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/
int conversion;        // valor para la conversion del ADC
int contador;          // contador para TMR0

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt () isr (void){
    
    if (PIR1bits.ADIF){                           // Revisar si fue interrupción del ADC
        if (ADCON0bits.CHS == 0){                 // Si es el canal de l AN0
            CCPR1L = (ADRESH>>1)+31;              // Corriento de ADRESH de 1 bit (0-127) -- 31 (bit mas sig. controlar el mov. total del servo)
            CCP1CONbits.DC1B = (ADRESH & 0b01);   // Controlar los últimos bit para más prescision (bit menos sig. del ADRESH)
            CCP1CONbits.DC1B0 = (ADRESL>>7);      // Para obtener el bit más sig. del ADRESL
        }
        else if (ADCON0bits.CHS == 1){            // Si es el canal de l AN1
            CCPR2L = (ADRESH>>1)+31;              // Corriento de ADRESH de 1 bit (0-127) -- 31 (bit mas sig. controlar el mov. total del servo)
            CCP1CONbits.DC1B = (ADRESH & 0b01);   // Controlar los últimos bit para más prescision (bit menos sig. del ADRESH)
            CCP1CONbits .DC1B0 = (ADRESL>>7);      // Para obtener el bit más sig. del ADRESL
        }
        else if (ADCON0bits.CHS == 2){            // Si es el canal de l AN2
            conversion= (ADRESH>>6)+1;            // Corriento de ADRESH de 6 bit (0-4)
        }
        PIR1bits.ADIF = 0;                        // Limpiar la interrucpción del ADC
    }
    
    if (INTCONbits.T0IF == 1){                    // Revisar si fue la interrupción del TMR0
        contador++;                               // Aumentar variable contador 
        if (contador > 10){                       // Periodo aprox 4ms
            contador=0;                           // Reiniciar contador 
        }        
        INTCONbits.T0IF = 0;                      // Limpiar la interrupción del TMR0
        TMR0 = _tmr0_value;                       // Reset del TMR0
    }
    
 }

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) {
    setup();                // Llamamos a la función de configuraciones
    ADCON0bits.GO = 1;      // Colocamos en 1 para que comience
    while(1){
        
        if (ADCON0bits.GO == 0)             // Revisar
        {
            if (ADCON0bits.CHS == 0){       // Si el canal es 0
                ADCON0bits.CHS = 1;         // Cambio de canal al 1
            }
            else if (ADCON0bits.CHS == 1){  // Si el canal es 1
                ADCON0bits.CHS = 2;         // Cambio de canal al 2
            }
            else if (ADCON0bits.CHS == 2){  // Si el canal es 2
               ADCON0bits.CHS = 0;          // Cambio de canal al 2
            }
            __delay_us(50);                 // Tiempo de adquisición
            ADCON0bits.GO = 1;              // Iniciamos proceso de conversión
        } 
        
        if(contador > conversion)           // Contador sea mayor a conversion
            PORTDbits.RD0 = 0;              // La led va estar apagada
        else
            PORTDbits.RD0 = 1;              // La led va estar encendida
        PORTB = contador;                   // Colocar contador en puerto B
    }
    return;
}

/*------------------------------------------------------------------------------
 * CONFIGURACION 
 ------------------------------------------------------------------------------*/
void setup(void){
    
    // Configuracion de entradas y salidas
    ANSEL = 0b00000111;         // AN0 como entrada analógica;
    ANSELH = 0;                 // Usaremos I/O digitales
    
    TRISA = 0xFF;               // PORTA como entrada  
    
    TRISB = 0x00;               // PORTB como salida
    PORTB = 0x00;               // Limpiamos PORTB 
    TRISD = 0x00;               // PORTD como salida
    PORTD = 0x00;               // Limpiamos PORTD    
                   
    // Configuracion del reloj
    OSCCONbits.IRCF = 0b0110;   // 4MHz
    OSCCONbits.SCS = 1;         // Oscilador interno
        
    // Configuración ADC
    ADCON1bits.ADFM = 0;        // Justificado a la izquierda
    ADCON1bits.VCFG0 = 0;       // VDD
    ADCON1bits.VCFG1 = 0;       // VSS
    
    ADCON0bits.ADCS = 0b10;     // Fosc/32
    ADCON0bits.CHS = 0;         // Seleccionamos el AN2
    ADCON0bits.ADON = 1;        // Habilitamos modulo ADC
    __delay_us(50);             // Sample time
    
    //Configuración del PWM
    TRISCbits.TRISC2 = 1;       // RC2//CCP1 como entrada
    TRISCbits.TRISC1 = 1;       // RC2//CCP2 como entrada
    PR2 = 249;                  // periodo de 4ms
   
    //Configuración del CCP
    CCP1CONbits.P1M = 0;        // single output 
    CCP1CONbits.CCP1M =0b1100;  // Modo PWM 1100
    CCP2CONbits.CCP2M =0b1100;  // Modo PWM 1100
    
    CCPR1L = 0x0f;              // Ciclo de trabajo inicial
    CCPR2L = 0x0f;              // Ciclo de trabajo inicial
    CCP1CONbits.DC1B = 0;
    
    //Configuracion del TMR2
    PIR1bits.TMR2IF = 0;        // Limpiamos bandera de interrupcion del TMR2
    T2CONbits.T2CKPS = 0b11;    // prescaler 1:16
    T2CONbits.TMR2ON = 1;       // Encendemos TMR2
    while(!PIR1bits.TMR2IF);    // Esperar un cliclo del TMR2
    PIR1bits.TMR2IF = 0;        // Limpiamos bandera de interrupcion del TMR2 nuevamente
    
    TRISCbits.TRISC2 = 0;       // Habilitamos salida de PWM
    TRISCbits.TRISC1 = 0;       // Habilitamos salida de PWM 
    
    //Configacion del TMR0
    OPTION_REGbits.T0CS = 0;    // Timer0 como temporizador
    OPTION_REGbits.PSA = 0;     // Prescaler a TIMER0
    OPTION_REGbits.PS = 0b0111; // Prescaler de 1 : 256
    TMR0 = _tmr0_value;         // Retardo del timer
    
    // Configuracion de Interrupciones
    PIR1bits.ADIF = 0;          // Limpiamos bandera de ADC
    PIE1bits.ADIE = 1;          // Habilitamos interrupcion de ADC
    INTCONbits.PEIE = 1;        // Habilitamos int. de perifericos
    INTCONbits.GIE = 1;         // Habilitamos int. globales  
    INTCONbits.T0IE = 1;        // Habilitamos interrupcion TMR0
    INTCONbits.T0IF = 0;        // Limpiamos bandera de interrupción TMR0
    return;
}