#include <msp430.h>

#define PCF 0x3f //endereço do LCD
#define a 440 //nota musical

void config_pinos(void);
void config_I2C(void);
void LCD_BL_on(void);
void LCD_BL_off(void);
void LCD_00(void);
int  PCF_read(void);
void PCF_write(char dado);
void delay(long limite);
void lcd_wr_aberto(void);
void lcd_wr_abrindo(void);
void lcd_print_text(void);
void lcd_print_text2(void);

void init_lcd(void);
void lcd_wr_byte(char byte);
void pcf_wr_cmd(char byte);
void lcd_clear(void);
void cursor_begin();
void next_line();

void delay1(int n);
void delay2(int n);
void motor_ccw(void);
void motor_cw(void);

void pin_inic(void);
void t2_inic(void);
void nota(int freq, int tempo);

int porta = 0;    //ultimo valor escrito na porta
int sinal = 0;

void conf_serial(void);
void conf_leds(void);
void conf_pinos(void);

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    pin_inic();
    t2_inic();

    conf_serial();
    conf_leds();
    conf_pinos();
    UCA0CTL1 &= ~UCSWRST; //encerrar comunicação serial

    config_pinos();
    config_I2C();
    lcd_clear();
    init_lcd();

    volatile int i;
        for (i = 0; i< 100; i++){
            while(!(UCA0IFG & UCTXIFG)); //Is this necessary?
            UCA0TXBUF = 0x55;

            while(!(UCA0IFG & UCRXIFG));
            if(UCA0RXBUF == 0x55){
                lcd_wr_abrindo();
                int j;
                for (j = 128; j > 0; j--)
                    motor_cw();
                nota(a, 50);
                nota(0,100);        //Pausa
                nota(a, 100);
                nota(0,100);        //Pausa
                nota(a, 200);
                P2SEL &= ~BIT5;
                lcd_wr_aberto();
                __delay_cycles(2000000);
                for (j = 128; j > 0; j--)
                    motor_ccw();
                __delay_cycles(2000000);
                sinal = 0;
            }
            else{
                lcd_print_text();
                int z;
                sinal = sinal + 1;
                if (sinal == 3){
                    for (z = 10; z > 0; z--){
                        lcd_print_text2();
                        nota(a, 100);
                        nota(0, 100);
                        nota(a, 100);
                        nota(0, 100);
                        nota(a, 100);
                        nota(0, 100);
                    }
                    P2SEL &= ~BIT5;
                    lcd_print_text2();
                    sinal = 0;

                }
                __delay_cycles(2000000);
            }

            P1OUT &= ~BIT0;
            P4OUT &= ~BIT7;
        }
        while(1);
}

void conf_leds(void){
    P4DIR |= BIT7;
    P1DIR |= BIT0;
}

void conf_pinos(void){
    //Alarme
    P2DIR |= BIT5;  //Saída PWM
    P2OUT &= ~BIT5;

    //Bluetooth
    P3DIR |= BIT3;
    P3DIR &= ~BIT4;
    P3REN |= BIT4;
    P3OUT |= BIT4;
    P3SEL |= BIT3;
    P3SEL |= BIT4;

    //Motor de Passos
    P4DIR |= BIT0;
    P4OUT |= BIT0;

    P4DIR |= BIT1;
    P4OUT |= BIT1;

    P4DIR |= BIT2;
    P4OUT |= BIT2;

    P4DIR |= BIT3;
    P4OUT |= BIT3;
}

void conf_serial(void){
    //configurador do UART para o Bluetooth
    UCA0CTL1 = UCSWRST;
    UCA0CTL1 = UCSSEL_2 | UCSWRST;

    UCA0BRW = 6;
    UCA0MCTL = UCBRF_13 | UCBRS_0 | UCOS16;

}

// Zerar toda a porta
void LCD_00(void){
    porta=0;
    PCF_write(porta);
}

//vc cria uma variavel que vai mandar seu dado, a variavel pode ser chamada anything, você a mascara, roda ela,
// Escrever dado na porta
void PCF_write(char dado){
    UCB0I2CSA = PCF;                            //Endereço do Escravo

    UCB0CTL1 |= UCTR    |                       //Mestre transmissor
                UCTXSTT;                        //Gerar START e envia endereço

    while ( (UCB0IFG & UCTXIFG) == 0);          //Esperar TXIFG (completar transm.)

    if ( (UCB0IFG & UCNACKIFG) == UCNACKIFG){   //NACK?
        P1OUT |= BIT0;                       //Acender LED Vermelho
        while(1);                               //Se NACK, prender
    }

    UCB0TXBUF = dado;                           //Dado a ser escrito

    while ( (UCB0IFG & UCTXIFG) == 0);          //Esperar Transmitir

    UCB0CTL1 |= UCTXSTP;                        //Gerar STOP

    while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP);   //Esperar STOP

    delay(50);                                  //Atraso p/ escravo perceber stop
}

// Configurar Pinos I2C - UCSB0
// P3.0 --> SDA
// P3.1 --> SCL
void config_I2C(void){
    P3SEL |=  BIT0;    // Usar módulo dedicado
    P3REN |=  BIT0;    // Habilitar resistor
    P3OUT |=  BIT0;    // Pull-up

    P3SEL |=  BIT1;    // Usar módulo dedicado
    P3REN |=  BIT1;    // Habilitar resistor
    P3OUT |=  BIT1;    // Pull-up

    UCB0CTL1 |= UCSWRST;    // UCSI B0 em reset

    UCB0CTL0 = UCSYNC |     //Síncrono
               UCMODE_3 |   //Modo I2C
               UCMST;       //Mestre

    UCB0BRW = 10;       //100 kbps
    //UCB0BRW = BR20K;      // 20 kbps
    //UCB0BRW = 100;      // 10 kbps
    UCB0CTL1 = UCSSEL_2;   //SMCLK e remove reset
}

// Configurar portas
void config_pinos(void){
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;
    P4DIR |= BIT7;
    P4OUT &= ~BIT7;
}

void delay(long limite){
    volatile long cont=0;
    while (cont++ < limite) ;
}

void init_lcd(void){
    PCF_write(0x38);
    PCF_write(0x3c);
    PCF_write(0x38);

    delay(100);
    PCF_write(0x38);
    PCF_write(0x3c);
    PCF_write(0x38);

    delay(100);
    pcf_wr_cmd(0x32);
    pcf_wr_cmd(0x28);


    pcf_wr_cmd(0x0c);
    pcf_wr_cmd(0x01);

    pcf_wr_cmd(0x06);

    pcf_wr_cmd(0x0F);
}

void lcd_wr_byte(char byte){
    char msb, lsb;
    msb = (byte & 0xF0);
    lsb = (byte & 0x0F);
    lsb = (byte << 4);

    char msb_unable = (msb | 0x09);
    char msb_enable = (msb | 0x0d);

    char lsb_unable = (lsb | 0x09);
    char lsb_enable = (lsb | 0x0d);

    PCF_write(msb_unable);
    PCF_write(msb_enable);
    PCF_write(msb_unable);
    delay(10);
    PCF_write(lsb_unable);
    PCF_write(lsb_enable);
    PCF_write(lsb_unable);
}

void pcf_wr_cmd(char byte){
    char msb, lsb;
    msb = (byte & 0xF0);
    lsb = (byte & 0x0F);
    lsb = (byte << 4);

    char msb_unable = (msb | 0x08);
    char msb_enable = (msb | 0x0c);

    char lsb_unable = (lsb | 0x08);
    char lsb_enable = (lsb | 0x0c);

    PCF_write(msb_unable);
    PCF_write(msb_enable);
    PCF_write(msb_unable);
    delay(10);
    PCF_write(lsb_unable);
    PCF_write(lsb_enable);
    PCF_write(lsb_unable);
}

void lcd_clear(void){
    pcf_wr_cmd(0x01);
}

void cursor_begin(){
    pcf_wr_cmd(0x02);
}

void next_line(){
    int i;
    for(i = 56; i > 0; i--){
        pcf_wr_cmd(0x0A);
    }
}

void lcd_wr_aberto(){
    //escrever aberto no LCD
    pcf_wr_cmd(0x01); // clear

    lcd_wr_byte(65);
    lcd_wr_byte(66);
    lcd_wr_byte(69);
    lcd_wr_byte(82);
    lcd_wr_byte(84);
    lcd_wr_byte(79);

    pcf_wr_cmd(0x0E);
    //cursor_begin();
}

void lcd_wr_abrindo(){
    //escrever abrindo no LCD
    pcf_wr_cmd(0x01); // clear

    lcd_wr_byte(65);
    lcd_wr_byte(66);
    lcd_wr_byte(82);
    lcd_wr_byte(73);
    lcd_wr_byte(78);
    lcd_wr_byte(68);
    lcd_wr_byte(79);

    pcf_wr_cmd(0x0E);
    //cursor_begin();
}

void lcd_print_text(){
    //escrever SENHA INCORRETA no LCD
    pcf_wr_cmd(0x01); // clear

    char linetxt[] = "SENHA INCORRETA";

    //print array
    int line = 1;
    int i;
    for(i = 0; i < ((sizeof(linetxt)/sizeof(linetxt[0]))-1); i++){
        char letter = linetxt[i];
        if(((i > 0) && (i%16) == 0 && line == 1)){
            pcf_wr_cmd(0xc0); //skip line 2
            line = 2;
        }
        else if(((i > 0) && (i%32) == 0 && line == 2)){
            pcf_wr_cmd(0x80); //skip line 1
            line = 1;
            delay(160000);
            pcf_wr_cmd(0x01); // clear
        }
        lcd_wr_byte(letter);
        if((letter == ' ')){
            delay(4000); //delay opcional apos cada palavra
        }
    }
    pcf_wr_cmd(0x0E);
    //cursor_begin();
}

void lcd_print_text2(){
    //escrever ALERTA DE INTRUSO no LCD
    pcf_wr_cmd(0x01); // clear

    char linetxt[] = "   ALERTA DE        INTRUSO";

    //print array
    int line = 1;
    int i;
    for(i = 0; i < ((sizeof(linetxt)/sizeof(linetxt[0]))-1); i++){
        char letter = linetxt[i];
        if(((i > 0) && (i%16) == 0 && line == 1)){
            pcf_wr_cmd(0xc0); //skip line 2
            line = 2;
        }
        else if(((i > 0) && (i%32) == 0 && line == 2)){
            pcf_wr_cmd(0x80); //skip line 1
            line = 1;
            delay(160000);
            pcf_wr_cmd(0x01); // clear
        }
        lcd_wr_byte(letter);
        if((letter == ' ')){
            delay(4000); //delay opcional apos cada palavra
        }
    }
    pcf_wr_cmd(0x0E);
    //cursor_begin();
}

void delay1(int n){
    //delay para o motor de passos, sentido horário
    int i;
    for(i=0;i<n;i++)
        asm("nop;");
}

void delay2(int n){
    //delay para o motor de passos, sentido anti-horário
    int i; for(i=0;i<n;i++)
    {
        delay1(10);
    }
}

void motor_cw (void){
    //função para o motor de passos girar no sentido horário
    unsigned int k;
    for (k = 0x01;
            k <= 0x08;
            k <<= 1)
    {
        delay2(250);
        P4OUT |= ( k & 0x0F);
        delay2(250);
        P4OUT &= (~k & 0x0F);
    }
}

void motor_ccw (void){
    //função para o motor de passos girar no sentido anti-horário
    unsigned int k;
    for (k = 0x08;
            k >= 0x01;
            k >>= 1)
    {
        delay2(250);
        P4OUT |= ( k & 0x0F);
        delay2(250);
        P4OUT &= (~k & 0x0F);
    }
}

void nota(int freq, int tempo){
    //nota musical
    volatile int cont=0,comp0,comp2;
    if (freq != 0){
        comp0=TA2CCR0 = 1048576L/freq;
        TA2CCR0 = comp0;                //Prog freq

        comp2 = TA2CCR0/2;
        TA2CCR2 = comp2;                //Carga 50%
        cont = freq*(tempo/1000.);      //Quantidade de períodos
    }
    else{
        P2SEL &= ~BIT5;                 //Desligar saída
        comp0=TA2CCR0 = 1048576L/1000;  //Programar 1 kHz
        comp2=TA2CCR2 = TA2CCR0/2;      //Carga  50%
        cont = tempo;                   //Quantidade de períodos
    }
    while(cont != 0){
        while( (TA2CCTL0&CCIFG) == 0);  //Usar CCIFG para
        TA2CCTL0 &= ~CCIFG;             //contar quantidade
        cont--;                         //de períodos
    }
    P2SEL |= BIT5;
}


void pin_inic(void){
    P2DIR |= BIT5;  //Saída PWM
    P2OUT &= ~BIT5;
}

void t2_inic(void){
    TA2CTL = TASSEL_2|ID_0|MC_1|TACLR;
    TA2EX0 = TAIDEX_0;
    TA2CCTL2 = OUTMOD_6;
    TA2CCR0 = 2*1048;
    TA2CCR2 = TA2CCR0/2;
}
