#ifndef MAIN_H
#define MAIN_H


#define F_CPU 16000000UL

#define UART_BAUD_RATE 9600
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1 // For U2X0 = 0
//#define MYUBRR F_CPU/8/BAUD-1 // For U2X0 = 1

#define PRESCALER 1024
#define TARGET_FREQ 1 // Target frequency in Hz for Timer1



void init_watchdog(void);
void init_buttons_interrupts(void);
void timer1_init(void);
void stop_timer1(void);
void start_timer1(void);
void read_datetime(void);
void read_sensors(void);
void check_alarm2(void);
void update_display0(void);
void update_display1(void);
void update_display2(void);
void hello_world(int8_t mode);




#endif // MAIN_H