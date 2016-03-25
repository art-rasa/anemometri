#include <avr/io.h>
#include <avr/interrupt.h>
#include "viive.h"



/* Vakioarvot */
#define KELLOTAAJUUS    8000000  /* Kiteen taajuus */
#define JAKSOT_MAX      10       /* Pulssinmittauksen jaksot lkm. */
#define JAKSO_AIKA      3036     /* = 0,5 s */

/* Siirtorekisterin pinnit */
#define SREK_PORTTI     PORTD   /* siirtorekisterin portti */
#define SREK_DATA       PD5     /* datalinja */
#define SREK_KELLO      PD3     /* kellolinja */
#define SREK_NOLLAUS    PD4     /* lähdöt nolliksi */

/* Näytön multipleksauksen pinnit */
#define NAYTTO_PORTTI   PORTD   
#define NAYTTO_A        PD0     /* kun 0, näyttö A päälle */
#define NAYTTO_B        PD1     /* kun 0, näyttö B päälle */


/* Makrot */
/* Siirtorekisterin pinnien ohjailu */
#define SRek_Data_Aseta()\
SREK_PORTTI = (SREK_PORTTI | (1 << SREK_DATA));
#define SRek_Data_Nollaa()\
SREK_PORTTI = (SREK_PORTTI & ~(1 << SREK_DATA));

#define SRek_Kello_Aseta()\
SREK_PORTTI = (SREK_PORTTI | (1 << SREK_KELLO));
#define SRek_Kello_Nollaa()\
SREK_PORTTI = (SREK_PORTTI & ~(1 << SREK_KELLO));

#define SRek_Nollaa()\
SREK_PORTTI = (SREK_PORTTI & ~(1 << SREK_NOLLAUS));\
SREK_PORTTI = (SREK_PORTTI | (1 << SREK_NOLLAUS));


/* Näytön multipleksauksen ohjailu */
#define NayttoA_Paalle()\
NAYTTO_PORTTI = (NAYTTO_PORTTI & ~(1 << NAYTTO_A));
#define NayttoA_Pois()\
NAYTTO_PORTTI = (NAYTTO_PORTTI | (1 << NAYTTO_A));
#define NayttoB_Paalle()\
NAYTTO_PORTTI = (NAYTTO_PORTTI & ~(1 << NAYTTO_B));
#define NayttoB_Pois()\
NAYTTO_PORTTI = (NAYTTO_PORTTI | (1 << NAYTTO_B));



/* Globaalit muuttujat */
volatile unsigned char  JAKSOT;          /* timer1:n ylivuodot lkm. */
volatile unsigned char  PULSSIT_LKM;     /* Pulssilaskurimuuttuja (INT0) */
unsigned char           PULSSIT_NAYTTOA; /* näyttöön menevä luku, ykköset */
unsigned char           PULSSIT_NAYTTOB; /* näyttöön menevä luku, kymmenet */



/* Näytön bitit */
const unsigned char SREKNAYTTO_NUM[10] = {
        0x11,0xD7,0x32,0x92,0xD4,
        0x98,0x1C,0xD3,0x10,0xD0
};


/* Prototyypit */
ISR(TIMER1_OVF1_vect);
ISR(INT0_vect);
void IOAsetukset_Portit(void);
void IOAsetukset_Ajastin0(void);
void IOAsetukset_Ajastin1(void);
void IOAsetukset_Keskeytys(void);
unsigned char Lednaytto_muunna(unsigned char num);
void SRek_Syota(unsigned char luku);
void T0Viive(unsigned char alkuarvo);


int main(void) {
        /*unsigned char testi = 0;*/
        /*unsigned char nayttoon = 0;*/
        unsigned char aika = 0;
        
        PULSSIT_NAYTTOA = 0;
        PULSSIT_NAYTTOA = 0;
        
        NayttoA_Pois();
        NayttoB_Pois();
        
        IOAsetukset_Portit();
        IOAsetukset_Ajastin1();
        IOAsetukset_Keskeytys();
        
        sei();
        
        while(1) {
                
                
                /* testiohjelma */
                
                SRek_Nollaa();
                
                
                SRek_Syota(PULSSIT_NAYTTOA);
                NayttoA_Paalle();
                T0Viive(0);
                NayttoA_Pois();
                
                SRek_Nollaa();
                
                
                SRek_Syota(PULSSIT_NAYTTOB);
                NayttoB_Paalle();
                T0Viive(0);
                NayttoB_Pois();
                
                SRek_Nollaa();
                
                PORTB = 0xFF;
                /* kuinka monta jaksoa on jäljellä */
                aika = (JAKSOT_MAX) - JAKSOT;
                if (aika < 8) {
                        
                        PORTB = (PORTB & ~(1 << aika));
                        
                } 
                
                
                /*viive_ms(5);*/
                
                
                
        }
        
}











/* Tulo/lähtönastojen asetukset */
void IOAsetukset_Portit(void) {
        
        /* PortB 
         * - PB0..7: lähdöt, perustila 1 tällöin ledit on pois
         */
        DDRB    = 0xFF;
        PORTB   = 0xFF;
        
        
        /* PortD
         * - PD0:       lähtö   näytön A ohjaus (0:näyttö päällä)
         * - PD1:       lähtö   näytön B ohjaus (0:näyttö päällä)
         * - PD2:       tulo    ulkoinen keskeytys, ei ylösvetovastusta
         * - PD3:       lähtö   siirtorekisteri: kellosignaali
         * - PD4:       lähtö   siirtorekisteri: nollaus (0:aktiivinen)
         * - PD5:       lähtö   siirtorekisteri: datasignaali
         * - PD6:       ei käytössä, lähtö
         */
        DDRD    = 0x7B;
        PORTD   = 0x13;
        
        
}



/* Ajastimen T0 (8-bittinen) asetukset 
 * 
 */
void IOAsetukset_Ajastin0(void) {
        
        /* Aseta 1 ms ajastin! */
        
        /* TCCR0 timer/counter0 control register */
        TCCR0   = 0x03;  /* 1:64 prescale */
        
        /* alkuarvon asetus */
        TCNT0   = 131;
        
}



/* Ajastimen T1 (16-bittinen) asetukset 
 * - ajastin: nollaa laskurin laskemat pulssit
 */
void IOAsetukset_Ajastin1(void) {
        
        /* Timer 1 
         * - ajastintila 
         * - tuottaa keskeytyksen 0,5 sek. välein 
         * - (8MHz/64)⁻¹ * (2¹⁶ - 3036) = 0,5 s 
         */
        /* Timer/counter1 control register B, prescale: 1:64 */
        TCCR1B  = 0x03;
        
        /* ajastimen alkunollaus */
        TCNT1   = JAKSO_AIKA;
        
        /* timer1:n ylivuotokeskeytys (TOIE1) päälle */
        TIMSK   = 0x80; 
        
}



/* Ulkoinen keskeytys 
 * - laskuritoiminto: laskee INT-nastaan tuotuja pulsseja
 */
void IOAsetukset_Keskeytys(void) {
        
        /* 
         * INT0 -keskeytystulo päälle. Nasta: PD2 
         */
        /* GIMSK: General interrupt mask */
        GIMSK = 0x40;
        
        /* MCUCR: MCU general control register 
         * - ISC00=1, ISC01=1: liipaisu pulssin nousevasta reunasta
         */
        MCUCR = 0x03;
        
}



/* 
 * Keskeytyskäsittelijä -- Timer1
 * - Suoritetaan kun timer1 ylivuotaa 0,5 sek välein
 */
ISR(TIMER1_OVF1_vect) {
        unsigned char ykkoset;
        unsigned char kymmenet;
        
         
        
        /* jaksojen laskenta */
        if (JAKSOT < JAKSOT_MAX) {
                JAKSOT += 1;
        
        /* kun jaksot tulevat täyteen, aseta pulssien lkm. muuttujaan ja 
           nollaa laskurit */
        } else {
                
                /*
                PULSSIT_NAYTTOB = Lednaytto_muunna((PULSSIT_LKM/10)%10);
                PULSSIT_NAYTTOA = Lednaytto_muunna(PULSSIT_LKM);
                */
                
                kymmenet = (PULSSIT_LKM/10)%10;
                ykkoset  = PULSSIT_LKM - (kymmenet*10);
                
                PULSSIT_NAYTTOB = Lednaytto_muunna(kymmenet);
                PULSSIT_NAYTTOA = Lednaytto_muunna(ykkoset);
                
                JAKSOT = 0;
                PULSSIT_LKM = 0;
                /*PORTB = ~PULSSIT_LKM;*/
                
                
        }
        
        /* Aseta timer1:n alkuarvo seuraavalle ajastuskierrolle */
        TCNT1 = JAKSO_AIKA;
        
}



/* 
 * Keskeytyskäsittelijä -- Ulkoinen keskeytys 0
 * - suoritetaan aina kun tuloon T0 tulee nouseva reuna
 */
ISR(INT0_vect) {
        
        /* Keskeytykset pois ennen seuraavaa pulssia */
        /*cli();*/
        
        /* Pulssilaskurin lisäys */
        PULSSIT_LKM += 1;
        
        /*PORTB = ~PULSSIT_LKM;*/
        
        
}


/* Muunna luku (0..9) 7-segmenttinäytön ohjaamiseen sopivaksi heksaluvuksi */
unsigned char Lednaytto_muunna(unsigned char num) {
    unsigned char num_hex = 0x00;
    
    if ((num >= 0) && (num <= 9)) {
        
        /* Siirtorekisterin kautta kytketty näyttö */
        num_hex = SREKNAYTTO_NUM[num];
        
    }
    
    return(num_hex);
    
}



/* Syötä tavu siirtorekisteriin */
void SRek_Syota(unsigned char luku) {
    /*unsigned char temp  = 0;*/
    unsigned char i     = 0;
    unsigned char bitti = 0;
    
    /* Nollaa ensin kellotulo */
    SRek_Kello_Nollaa();
    
    for (i = 0; i < 8; i++) {
        
        /* tarkista tavun bitti #i */
        bitti = (luku & (1 << i));
        
        /* jos oli 1 aseta srek. datatulo */
        if (bitti != 0) { 
            SRek_Data_Aseta();
            
        /* jos oli 0 nollaa srek. datatulo */
        } else {
            SRek_Data_Nollaa();
            
        }
        
        T0Viive(250);
        
        /* Srek. kellotulo ylös, viive ja alas */
        SRek_Kello_Aseta();
        
        T0Viive(250);
        
        SRek_Kello_Nollaa();
        
        /*T0Viive(100);*/
    }
    
}

void T0Viive(unsigned char alkuarvo) {
        TCNT0 = alkuarvo;
        TCCR0 = 0x04;
        while(TCNT0 <= 254) {
                ;
        }
        
        TCCR0 = 0x00;
        TCNT0 = 0;
}

