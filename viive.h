/*
 * Funktio viiveiden tekemiseen
 */

void viive_ms(unsigned char t) {
    unsigned int i;
    unsigned int aika;
    
    aika = 8000/3;  /* Kidetaajuus 8 MHz */
    
    while (t--) 
        for (i = 0; i < aika; i++) {
            asm volatile ("nop"::);
            if (t == 0) return;
        }
}

