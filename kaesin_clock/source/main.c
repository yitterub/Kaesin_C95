
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include "config.h"
#include "fontdata.h"

//pin assign
#define push_sw1 PORTAbits.RA3
#define push_sw2 PORTAbits.RA4
#define oled_res LATBbits.LATB5
#define testpin LATBbits.LATB7
#define oled_add 0x78   //set oled I2C address + write mode
#define oled_add_r 0x79 //set oled I2C address + read mode
#define oled_command 0x80   //Co bit=1, D/C# bit=0 next data is command
#define oled_data 0x00   //Co bit=0, D/C# bit=0 next data is data
#define oled_ram 0x40   //Co bit=0, D/C# bit=1 next data is graphic data
#define disp_off 0xAE   //oled display off

/* 
 * File:   main.c
 * Author: Yb
 *
 * Created on 2016/07/10, 22:30
 */

void send_i2c(char, char);   //initialize
void wait_1m(int);  //wait time
void init(void);
void oled_open(void);
void i2c_stop(void);
void oled_close(void);
void oled_on(void);
void oled_set(char *);
void clk_gfx(char, int, int, int);
void wait(int);
void wait_1m(int);

char hour;  //hour
char minute;  //minute
char second;  //second
unsigned char font_index[8]; //font index
char state = 0; //on switch state
//char pushsw = 0; //push sw state

void interrupt high_isr (void)
{
    TMR1H = TMR1H | 0x80;   //set TMR1H
    //timer count
    second++;
    if(second == 60){
        second = 0;
        minute++;
    }
    if(minute == 60){
        minute = 0;
        hour++;
    }
    if(hour == 24){
        hour = 0;
    }
    font_index[0] = 0x30 + hour / 10;
    font_index[1] = 0x30 + hour % 10;
    font_index[2] = 0x3A;
    font_index[3] = 0x30 + minute / 10;
    font_index[4] = 0x30 + minute % 10;
    font_index[5] = 0x3A;
    font_index[6] = 0x30 + second / 10;
    font_index[7] = 0x30 + second % 10;
    //
    if(state == 0x01){
            oled_set(font_index);
        }
    PIR1bits.TMR1IF = 0;    //clear interrupt flag
}

void interrupt low_priority low_isr (void)
{
    char read_port;
    read_port = PORTA;
    //increase time if pushsw pushed
    if((read_port & 0x10) == 0x00){ 
        hour++;
        if(hour > 24)hour = 0;
    }
    if((read_port & 0x08) == 0x00){ 
        minute++;
        if(minute > 60)minute = 0;
    }
    INTCONbits.RABIF = 0;   //flag clear
}


void init(void){
    //Clock set
    OSCCONbits.IRCF = 0x4;  //Internal clock = 2MHz
    //non use module stop
    CM1CON0 = 0x00;     //Compalator disable
    CM2CON0 = 0x00;
    ADCON0bits.ADON = 0;    //A/D converter disable
    ANSEL = 0x00;   //analog stop
    ANSELH = 0x00;  //analog stop
    //GPIO set
    TRISA = 0xFF;   //all pin input
    WPUA = 0x38;    //RA5,4,3 week pull up set
    INTCON2bits.RABPU = 0;  //week pull up enabled
    TRISB = 0x5F;   //RB5 output
    IOCA = 0x18;    //RA4, 3 interrupt set
    LATBbits.LATB5 = 0;  //OLED RESET pin enabled
    //TRISC = 0x7F;   //RC7(T1OSCO) = output, other pin = input 
    
    //i2c port set(master mode)
    SSPCON1 = 0x08; //master mode set
    //SSPCON2bits.RCEN = 1;   //Receive enable
    SSPSTAT = 0x00; //set 100kHz mode
    SSPADD = 0;    //Baud rate set  Fscl = Fosc / (SSPADD + 1)(4) //2:333kHz 9:100kHz
    SSPCON1bits.SSPEN = 1;  //I2C module start
    //oled_res = 0;  //OLED RESET pin enabled
    testpin = 0;
    
    //interrupt set
    INTCON2bits.RABIP = 0;  //set PORT inrerrupt priority = low
    RCONbits.IPEN = 1;  //set IPEN bit.
    PIE1bits.TMR1IE = 1;    //set Timer1 interrupt.
    PIR1bits.TMR1IF = 0;    //clear Timer1 interrupt flag.
    INTCON = 0xC8;  //GIE ON, PEIE ON, RABIE ON.
        
    //timer1 set(8bit mode)
    T1CON = 0x4F;   //8bit mode, 1:1 prescale, OSC ON, unsync, 32.768kHz osc, Timer1 ON.
    TMR1H = TMR1H | 0x80;   //set TMR1H
    
    //sleep enable on
    OSCCONbits.IDLEN = 1;
}

//oled start
void oled_open(void){
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(2,0xAE);   //display off
    
    //Set Display Clock Divide Ratio/Oscillator Frequency
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(1,0xD5);
    send_i2c(1,oled_data);   //send controll byte
    send_i2c(2,0x82);
    
    //Set Multiplex Ratio
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(1,0xA8);
    send_i2c(1,oled_data);   //send controll byte
    send_i2c(2,0x1F);
    
    //Set Display Offset
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(1,0xD3);
    send_i2c(1,oled_data);   //send controll byte
    send_i2c(2,0x00);
    
    //Set Display Start Line
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(2,0x40);
    
    //Set Charge Pump
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(1,0x8D);
    send_i2c(1,oled_data);   //send controll byte
    send_i2c(2,0x14);       //0x10:VCC Supplied Externally, 0x14:VCC Generated by Internal DC/DC Circuit
    
    //Set Segment Re-Map
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(2,0xA1);
    
    //Set COM Output Scan Direction
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(2,0xC8);
    
    //Set COM Pins Hardware Configuration
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(1,0xDA);
    send_i2c(1,oled_data);   //i2c address
    send_i2c(2,0x02);
    
    //Set Contrast Control
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(1,0x81);
    send_i2c(1,oled_data);   //i2c address
    send_i2c(2,0xFF);
    
    //Set Pre-Charge Period
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(1,0xD9);
    send_i2c(1,oled_data);   //i2c address
    send_i2c(2,0xF1);       //0x22:VCC Supplied Externally, 0xF1:VCC Generated by Internal DC/DC Circuit
    
    //Set VCOMH Deselect Level
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(1,0xDB);
    send_i2c(1,oled_data);   //i2c address
    send_i2c(2,0x00);
    
    //Set Entire Display On/Off
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(2,0xA4);
    
    //Set Normal/Inverse Display
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(2,0xA6);
    
    //Set Memory addressing mode
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(1,0x20);
    send_i2c(1,oled_data);   //i2c address
    send_i2c(2,0x00);       //set Horizontal Addressing Mode
    
    //Set Page Start Address for Page Addressing Mode
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(1,0x22);
    send_i2c(1,oled_data);   //i2c address
    send_i2c(1,0x00);       //set Page Start Address
    send_i2c(2,0x03);       //set Page End Address
    
    //wait 100msec
    wait_1m(8);
    
    //Set Display On
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(2,0xAF);
}

//send data for i2c
void send_i2c(char status, char data){ //status= 0:start 1:stay 2:end
    //oled_res = 1;  //OLED RESET pin enabled
    if(status == 0){  //start session
            SSPCON2bits.SEN = 1;    //set start 
            while(1){
                if(SSPCON2bits.SEN==0)break;   //if set SSPIF, goto next process
            }
            PIR1 = PIR1 & 0xF7;  //clear flag
    }
    else if(status == 1){  //restart session
            //SSPCON2bits.RSEN = 1;    //set start 
            while(1){
                if(SSPCON2bits.RSEN==0)break;   //if set SSPIF, goto next process
            }
            PIR1 = PIR1 & 0xF7;  //clear flag
    }
    else if(status == 2){  //if need setting, write below.
    }
    //testpin = 1;
    SSPBUF = data;  //send data
    PIR1bits.SSPIF = 0;  //clear flag
    while(1){
        if(PIR1bits.SSPIF==1)break;   //if set SSPIF, goto next process
    }
    if(status == 0){  //if need setting, write below.
    }
    else if(status == 1){  //if need setting, write below.
    }
    else if(status == 2){
                SSPCON2bits.PEN = 1;    //set stop
                while(1)if(SSPCON2bits.PEN == 0)break;   //if set SSPIF, goto next process
                PIR1bits.SSPIF = 0;  //clear flag
    }
}

void i2c_stop(void){  //set i2c stop condition
    SSPCON2bits.PEN = 1;    //set stop
    while(1)if(SSPCON2bits.PEN == 1)break;   //if set SSPIF, goto next process
    PIR1bits.SSPIF = 0;  //clear flag
}

//oled stop
void oled_close(void){
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(2,0xAE);   //display off
}

void oled_on(void){
    send_i2c(0,oled_add);   //i2c address
    send_i2c(1,oled_command);   //send controll byte
    send_i2c(2,0xAF);
}

//display picture set
void oled_set(char *index){
    const unsigned char *pixelData;
    unsigned char offset;
    char row, column, i, font_point;
    char data;
    data = 0x00;
    pixelData = &pixelData_zero[0];
    send_i2c(0,oled_add);
    send_i2c(1,oled_ram);   //send controll byte
    for(column = 0;column <= 3;column++){
        offset = (0x0f & column) << 4;   //set font point offset
        //send_i2c(0,oled_add);
        //send_i2c(1,oled_ram);   //send controll byte
        for(row = 0;row <= 127;row++){
#ifdef test
            if(i > 7){
                i = 0;
                data = data ^ 0xFF;
            }
            i++;
#endif
            font_point = (0xf0 & row) >> 4;
            if(font_point == 0){
                pixelData = ascii_table[index[0]];
                data = pixelData[offset + row];
            }
            else if(font_point == 1){
                pixelData = ascii_table[index[1]];
                data = pixelData[offset + row - 16];
            }
            else if(font_point == 2){
                pixelData = ascii_table[index[2]];
                data = pixelData[offset + row - 32];
            }
            else if(font_point == 3){
                pixelData = ascii_table[index[3]];
                data = pixelData[offset + row - 48];
            }
            else if(font_point == 4){
                pixelData = ascii_table[index[4]];
                data = pixelData[offset + row - 64];
            }
            else if(font_point == 5){
                pixelData = ascii_table[index[5]];
                data = pixelData[offset + row - 80];
            }
            else if(font_point == 6){
                pixelData = ascii_table[index[6]];
                data = pixelData[offset + row - 96];
            }
            else if(font_point == 7){
                pixelData = ascii_table[index[7]];
                data = pixelData[offset + row - 112];
            }
            else{
                data = 0x00;
            }
            send_i2c(1,data);
        }
    }
    i2c_stop();
}

//create display picture
void clk_gfx(char type, int hour, int minute, int second){
    
}

void wait(int time){
    int i;
    for(i = 0;i < time;i++);    //wait count
}

void wait_1m(int time){
    int i, j;
    for(i = 0;i < 1000;i++){
        for(j = 0;j < time;j++);    //wait count
    }
}

/*
 * 
 */
int main(int argc, char** argv) {
    int count;
    char port;
    state = 0;
    init();      //set IO
    oled_res = 0;  //OLED RESET pin 0
    wait(1000);
    oled_res = 1;  //OLED RESET pin enabled
    //display test
    oled_open();
    //oled_res = 1;  //OLED RESET pin enabled
    //testpin = 1;
    while(1){
        if((state == 0x00) && (PORTAbits.RA5 == 0)){
            oled_on();
            state = 0x01;
        }
        else if((state == 0x01) && (PORTAbits.RA5 == 1)){
            oled_close();
            state = 0x00;
        }
        OSCCONbits.SCS = 1; //set cpu clock = secondary clock  before sleep 
        SLEEP();
        OSCCONbits.SCS = 0; //set cpu clock = primary clock  before sleep
    }
    return (EXIT_SUCCESS);
}

