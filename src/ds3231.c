
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "i2c.h"
#include "ds3231.h"

//#include "uart.h"

KeyValuePair DayMap[] = {
    {1, "Mon"},
    {2, "Tue"},
    {3, "Wed"},
    {4, "Thu"},
    {5, "Fri"},
    {6, "Sat"},
    {7, "Sun"}
};

KeyValuePair DayFullMap[] = {
    {1, "Monday"},
    {2, "Tuesday"},
    {3, "Wednesday"},
    {4, "Thursday"},
    {5, "Friday"},
    {6, "Saturday"},
    {7, "Sunday"}
};


KeyValuePair MonthMap[] = {
    {1, "Jan"},
    {2, "Feb"},
    {3, "Mar"},
    {4, "Apr"},
    {5, "May"},
    {6, "Jun"},
    {7, "Jul"},
    {8, "Aug"},
    {9, "Sep"},
    {10, "Oct"},
    {11, "Nov"},
    {12, "Dec"}
};

KeyValuePair MonthFullMap[] = {
    {1, "January"},
    {2, "February"},
    {3, "March"},
    {4, "April"},
    {5, "May"},
    {6, "June"},
    {7, "July"},
    {8, "August"},
    {9, "September"},
    {10, "October"},
    {11, "November"},
    {12, "December"}

};



void DS3231_init(void) {
    // Ensure the control register is correctly set
    i2c_start((DS3231_ADDRESS << 1) | I2C_WRITE);
    i2c_write(0x0E); // Address the control register
    i2c_write(0x00); // Set the control register to default
    i2c_stop();
}

void DS3231_getTime(uint8_t* hour, uint8_t* minute, uint8_t* second, uint8_t* day_of_week) {
    //uart_puts("Starting I2C write for time retrieval...\n");
    if (i2c_start((DS3231_ADDRESS << 1) | I2C_WRITE)) {
        //uart_puts("I2C start for write failed!\n");
        return;
    }
    //uart_puts("I2C start for write successful...\n");

    i2c_write(0x00); // Set register pointer to 00h
    //uart_puts("Register pointer set...\n");

    //uart_puts("Starting I2C read for time retrieval...\n");
    if (i2c_start((DS3231_ADDRESS << 1) | I2C_READ)) {
        //uart_puts("I2C start for read failed!\n");
        return;
    }
    //uart_puts("I2C start for read successful...\n");


    *second = bcd_to_decimal(i2c_read_ack());
    *minute = bcd_to_decimal(i2c_read_ack());
    *hour = bcd_to_decimal(i2c_read_ack());
    *day_of_week = bcd_to_decimal(i2c_read_nack());

    i2c_stop();
    //uart_puts("Time retrieval completed successfully.\n");

}



void DS3231_setTime(uint8_t hour, uint8_t minute, uint8_t second, uint8_t day_of_week) {
    // Start I2C communication with DS3231
    i2c_start((DS3231_ADDRESS << 1) | I2C_WRITE);
    i2c_write(0x00); // Set register pointer to 00h
    // The day, month, and year values are written to the corresponding registers (0x00, 0x01, and 0x02).
    // Write the time data to DS3231
    i2c_write(decimal_to_bcd(second));
    i2c_write(decimal_to_bcd(minute));
    i2c_write(decimal_to_bcd(hour));
    i2c_write(decimal_to_bcd(day_of_week)); // Write day of the week to DS3231

    // Stop I2C communication
    i2c_stop();
}

// Date functions

void DS3231_getDate(uint8_t* day, uint8_t* month, uint8_t* year) {
    //uart_puts("Getting date from DS3231...\n");
    
    if (i2c_start((DS3231_ADDRESS << 1) | I2C_WRITE)) {
        //uart_puts("I2C start for write failed!\n");
        return;
    }
    
    i2c_write(0x04); // Set register pointer to 03h (start of day, month, year)
    //uart_puts("Register pointer set to date registers...\n");
    
    if (i2c_start((DS3231_ADDRESS << 1) | I2C_READ)) {
        //uart_puts("I2C start for read failed!\n");
        return;
    }
    //uart_puts("I2C start for read successful...\n");

    *day = bcd_to_decimal(i2c_read_ack());
    //uart_puts("Day read...\n");
    *month = bcd_to_decimal(i2c_read_ack());
    //uart_puts("Month read...\n");
    *year = bcd_to_decimal(i2c_read_nack()); // No ACK after the last byte
    //uart_puts("Year read...\n");

    i2c_stop();
    //uart_puts("Date retrieval completed successfully.\n");
}




void DS3231_setDate(uint8_t day, uint8_t month, uint8_t year) {
    //uart_puts("Setting date on DS3231...\n");

    if (i2c_start((DS3231_ADDRESS << 1) | I2C_WRITE)) {
        //uart_puts("I2C start for write failed!\n");
        return;
    }
    
    i2c_write(0x04); // Set register pointer to 03h (start of day, month, year)
    //uart_puts("Register pointer set to date registers...\n");
    // The day, month, and year values are written to the corresponding registers (0x04, 0x05, and 0x06).
    i2c_write(decimal_to_bcd(day));   // Write day to DS3231
    //uart_puts("Day written...\n");
    i2c_write(decimal_to_bcd(month)); // Write month to DS3231
    //uart_puts("Month written...\n");
    i2c_write(decimal_to_bcd(year));  // Write year to DS3231
    //uart_puts("Year written...\n");

    i2c_stop();
    //uart_puts("Date set successfully.\n");
}


// Alarm functions

void DS3231_setAlarm1(uint8_t hour, uint8_t minute, uint8_t second) {
    // Start I2C communication with DS3231
    i2c_start((DS3231_ADDRESS << 1) | I2C_WRITE);
    i2c_write(0x07); // Set register pointer to 07h
    // The day, month, and year values are written to the corresponding registers (0x07, 0x08, and 0x09).
    // Write the time data to DS3231
    i2c_write(decimal_to_bcd(second));
    i2c_write(decimal_to_bcd(minute));
    i2c_write(decimal_to_bcd(hour));

    // Stop I2C communication
    i2c_stop();
}

void DS3231_setAlarm2(uint8_t hour, uint8_t minute) {
    // Start I2C communication with DS3231
    i2c_start((DS3231_ADDRESS << 1) | I2C_WRITE);
    i2c_write(0x0B); // Set register pointer to 0Bh
    // The day, month, and year values are written to the corresponding registers (0x0B, 0x0C).
    // Write the time data to DS3231
    i2c_write(decimal_to_bcd(minute));
    i2c_write(decimal_to_bcd(hour));

    // Stop I2C communication
    i2c_stop();
}

/*
void DS3231_setAlarm2_interrupt(uint8_t hour, uint8_t minute, uint8_t day) {
    // Start I2C communication with DS3231
    i2c_start((DS3231_ADDRESS << 1) | I2C_WRITE);
    i2c_write(0x0B); // Set register pointer to 0Bh
    // The day, month, and year values are written to the corresponding registers (0x0B, 0x0C, 0x0D).
    // Write the time data to DS3231   
    i2c_write(decimal_to_bcd(minute));
    i2c_write(decimal_to_bcd(hour));
    i2c_write(decimal_to_bcd(day));

    //
    // ALARM 2 REGISTER MASK BITS (BIT 7)
    // DY/DT | A2M4 | A2M3 | A2M2 | ALARM RATE
    // ----------------------------------------------------------------------------
    // X      1      1      1      Alarm once per minute (00 seconds of every minute)
    // X      1      1      0      Alarm when minutes match
    // X      1      0      0      Alarm when hours and minutes match
    // 0      0      0      0      Alarm when date, hours, and minutes match
    // 1      0      0      0      Alarm when day, hours, and minutes match
    //   
    // Set the alarm mode
    // 0 - once per minute, 
    // 1 - when minutes match, 
    // 2 - when hours and minutes match, 
    // 3 - when date, hours, and minutes match, 
    // 4 - when day, hours, and minutes match

    // i2c_write(mode << 7); // Set the mask bits according to the mode
    // Stop I2C communication
    i2c_stop();
}
*/
//

void DS3231_getAlarm1(uint8_t* hour, uint8_t* minute, uint8_t* second) {
    //uart_puts("Starting I2C write for alarm 1 retrieval...\n");
    if (i2c_start((DS3231_ADDRESS << 1) | I2C_WRITE)) {
        //uart_puts("I2C start for write failed!\n");
        return;
    }
    //uart_puts("I2C start for write successful...\n");

    i2c_write(0x07); // Set register pointer to 07h
    //uart_puts("Register pointer set...\n");

    //uart_puts("Starting I2C read for alarm 1 retrieval...\n");
    if (i2c_start((DS3231_ADDRESS << 1) | I2C_READ)) {
        //uart_puts("I2C start for read failed!\n");
        return;
    }
    //uart_puts("I2C start for read successful...\n");

    *second = bcd_to_decimal(i2c_read_ack());
    //uart_puts("Seconds read...\n");
    *minute = bcd_to_decimal(i2c_read_ack());
    //uart_puts("Minutes read...\n");
    *hour = bcd_to_decimal(i2c_read_nack());
    //uart_puts("Hours read...\n");

    i2c_stop();
    //uart_puts("Alarm 1 retrieval completed successfully.\n");
}

void DS3231_getAlarm2(uint8_t* hour, uint8_t* minute) {
    //uart_puts("Starting I2C write for alarm 2 retrieval...\n");
    if (i2c_start((DS3231_ADDRESS << 1) | I2C_WRITE)) {
        //uart_puts("I2C start for write failed!\n");
        return;
    }
    //uart_puts("I2C start for write successful...\n");

    i2c_write(0x0B); // Set register pointer to 0Bh
    //uart_puts("Register pointer set...\n");

    //uart_puts("Starting I2C read for alarm 2 retrieval...\n");
    if (i2c_start((DS3231_ADDRESS << 1) | I2C_READ)) {
        //uart_puts("I2C start for read failed!\n");
        return;
    }
    //uart_puts("I2C start for read successful...\n");

    *minute = bcd_to_decimal(i2c_read_ack());
    //uart_puts("Minutes read...\n");
    *hour = bcd_to_decimal(i2c_read_nack());
    //uart_puts("Hours read...\n");

    i2c_stop();
    //uart_puts("Alarm 2 retrieval completed successfully.\n");
}

void DS3231_clearAlarm1(void) {
    // Start I2C communication with DS3231
    i2c_start((DS3231_ADDRESS << 1) | I2C_WRITE);
    i2c_write(0x0F); // Set register pointer to 0Fh
    // Clear the alarm 1 flag
    i2c_write(0x00);

    // Stop I2C communication
    i2c_stop();
}

void DS3231_clearAlarm2(void) {
    // Start I2C communication with DS3231
    i2c_start((DS3231_ADDRESS << 1) | I2C_WRITE);
    i2c_write(0x0F); // Set register pointer to 0Fh
    // Clear the alarm 2 flag
    i2c_write(0x00);

    // Stop I2C communication
    i2c_stop();
}

// Alarm 2 Interrupt Enable

void init_alarm2_interrupt() {
    // Enable Alarm 2 Interrupt
    i2c_start((DS3231_ADDRESS << 1) | I2C_WRITE);
    i2c_write(0x0E); // Set register pointer to 0Eh (Control Register)
    uint8_t control_reg = i2c_read_nack();
    control_reg |= (1 << A2IE); // Set A2IE bit to enable Alarm 2 interrupt
    i2c_start((DS3231_ADDRESS << 1) | I2C_WRITE);
    i2c_write(0x0E); // Set register pointer to 0Eh again
    i2c_write(control_reg); // Write back modified control register
    i2c_stop();
}

// Temperature function

void DS3231_getTemperature(int8_t* ds3231_temperature) {
    // Start I2C communication with DS3231
    i2c_start((DS3231_ADDRESS << 1) | I2C_WRITE);
    i2c_write(0x11); // Set register pointer to 11h

    // Restart I2C communication
    i2c_start((DS3231_ADDRESS << 1) | I2C_READ);

    // Read the temperature data
    *ds3231_temperature = i2c_read_nack();

    // Stop I2C communication
    i2c_stop();
}

// Convert BCD to decimal

// uint8_t bcd_to_decimal(uint8_t bcd) {
//     return ((bcd / 16 * 10) + (bcd % 16));
// }

// uint8_t decimal_to_bcd(uint8_t decimal) {
//     return ((decimal / 10 * 16) + (decimal % 10));
// }

uint8_t decimal_to_bcd(uint8_t decimal) {
    return ((decimal / 10 * 16) + (decimal % 10));
}

uint8_t bcd_to_decimal(uint8_t bcd) {
    return ((bcd / 16 * 10) + (bcd % 16));
}



