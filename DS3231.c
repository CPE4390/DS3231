
#include <xc.h>
#include "DS3231.h"
#include <stdio.h>

void InitDS3231(void) {
    TRISD = 0b01100000;
    SSP2STATbits.SMP = 0;
    SSP2ADD = 0x18; //400kHz
    SSP2CON1bits.SSPM = 0b1000; //I2C Master mode
    SSP2CON1bits.SSPEN = 1; //Enable MSSP2
}

unsigned char ReadClockRegister(unsigned char address) {
    unsigned char rx;
    SSP2CON2bits.SEN = 1;
    while (SSP2CON2bits.SEN == 1);
    SSP2BUF = I2C_ADDRESS;
    while (SSP2STATbits.BF || SSP2STATbits.R_W);
    SSP2BUF = address;
    while (SSP2STATbits.BF || SSP2STATbits.R_W);
    SSP2CON2bits.RSEN = 1;
    while (SSP2CON2bits.RSEN == 1);
    SSP2BUF = I2C_ADDRESS | 0x01;
    while (SSP2STATbits.BF || SSP2STATbits.R_W);
    SSP2CON2bits.RCEN = 1;
    while (!SSP2STATbits.BF);
    rx = SSP2BUF;
    SSP2CON2bits.ACKDT = 1;
    SSP2CON2bits.ACKEN = 1;
    while (SSP2CON2bits.ACKEN != 0);
    SSP2CON2bits.PEN = 1;
    while (SSP2CON2bits.PEN == 1);
    return rx;
}

void WriteClockRegister(unsigned char address, unsigned char data) {
    SSP2CON2bits.SEN = 1;
    while (SSP2CON2bits.SEN == 1);
    SSP2BUF = I2C_ADDRESS;
    while (SSP2STATbits.BF || SSP2STATbits.R_W);
    SSP2BUF = address;
    while (SSP2STATbits.BF || SSP2STATbits.R_W);
    SSP2BUF = data;
    while (SSP2STATbits.BF || SSP2STATbits.R_W);
    SSP2CON2bits.PEN = 1;
    while (SSP2CON2bits.PEN == 1);
}

void ReadClockBytes(unsigned char address, unsigned char *buffer, int len) {
    int i;
    SSP2CON2bits.SEN = 1;
    while (SSP2CON2bits.SEN == 1);
    SSP2BUF = I2C_ADDRESS;
    while (SSP2STATbits.BF || SSP2STATbits.R_W);
    SSP2BUF = address;
    while (SSP2STATbits.BF || SSP2STATbits.R_W);
    SSP2CON2bits.RSEN = 1;
    while (SSP2CON2bits.RSEN == 1);
    SSP2BUF = I2C_ADDRESS | 0x01;
    while (SSP2STATbits.BF || SSP2STATbits.R_W);
    for (i = 0; i < len; ++i) {
        SSP2CON2bits.RCEN = 1;
        while (!SSP2STATbits.BF);
        buffer[i] = SSP2BUF;
        SSP2CON2bits.ACKDT = (i != len - 1 ? 0 : 1);
        SSP2CON2bits.ACKEN = 1;
        while (SSP2CON2bits.ACKEN != 0);
    }
    SSP2CON2bits.PEN = 1;
    while (SSP2CON2bits.PEN == 1);
}

void WriteClockBytes(unsigned char address, unsigned char *buffer, int len) {
    int i;
    SSP2CON2bits.SEN = 1;
    while (SSP2CON2bits.SEN == 1);
    SSP2BUF = I2C_ADDRESS;
    while (SSP2STATbits.BF || SSP2STATbits.R_W);
    SSP2BUF = address;
    while (SSP2STATbits.BF || SSP2STATbits.R_W);
    for (i = 0; i < len; ++i) {
        SSP2BUF = buffer[i];
        while (SSP2STATbits.BF || SSP2STATbits.R_W);
    }
    SSP2CON2bits.PEN = 1;
    while (SSP2CON2bits.PEN == 1);
}

void ReadClock(DateTime *data) {
    ReadClockBytes(0x00, data->bytes, 7);
}

void SetClock(DateTime *data) {
    WriteClockBytes(0x00, data->bytes, 7);
}

void FormatDate(DateTime date, char *str, bool useDOW) {
    const char *dow_str[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}; 
    if (useDOW) {
        sprintf(str, "%d%d/%d%d/%d%d %s", date.month10, date.month, date.date10, date.date, date.year10, date.year, dow_str[date.day - 1]);
    } else {
        sprintf(str, "%d%d/%d%d/%d%d", date.month10, date.month, date.date10, date.date, date.year10, date.year);
    }
}

void FormatTime(DateTime time, char *str) {
    if (time._12_24) {
        if (time.AM_PM == 0) {
            sprintf(str, "%d%d:%d%d:%d%d am", time.hour10, time.hour, time.minutes10, time.minutes, time.seconds10, time.seconds);
        } else {
            sprintf(str, "%d%d:%d%d:%d%d pm", time.hour10, time.hour, time.minutes10, time.minutes, time.seconds10, time.seconds);
        }
    } else {
        sprintf(str, "%d%d:%d%d:%d%d", time.hour10 + (time.AM_PM * 2), time.hour, time.minutes10, time.minutes, time.seconds10, time.seconds);
    }
}

void FillTime(DateTime *dt, int hour, int minutes, int seconds, bool pm, bool _12hour) {

    if (_12hour) {
        dt->hour10 = hour / 10;
        dt->_12_24 = 1;
        if (pm) {
            dt->AM_PM = 1;
        } else {
            dt->AM_PM = 0;
        }
    } else {
        dt->hour10 = hour / 10;
        dt->AM_PM = (hour / 10) >> 1;
        dt->_12_24 = 0;
    }
    dt->hour = hour % 10;
    dt->minutes10 = minutes / 10;
    dt->minutes = minutes % 10;
    dt->seconds10 = seconds / 10;
    dt->seconds = seconds % 10;
}

void FillDate(DateTime *dt, int month, int date, int year, int dayOfWeek) {
    dt->month10 = month / 10;
    dt->month = month % 10;
    dt->date10 = date / 10;
    dt->date = date % 10;
    dt->year10 = year / 10;
    dt->year = year % 10;
    dt->day = dayOfWeek;
    dt->century = 0;
}

void EnableSWOut(bool out, unsigned char freq) {
    
}