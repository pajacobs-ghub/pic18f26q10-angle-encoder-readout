// uart.h
// PJ, 2018-01-02, 2018-12-30, 2019-04-15

#ifndef MY_UART
#define MY_UART
void uart1_init(long baud);
void putch(char data);
__bit kbhit(void);
void uart1_flush_rx(void);
int getch(void);
char getche(void);
void uart1_close(void);

#define XON 0x11
#define XOFF 0x13

#endif
