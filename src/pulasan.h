#ifndef PULASAN_H
#define PULASAN_H

#include <modbus.h>

void setup_interupt(void);
int check_watchdog(modbus_t *mb);
void print_intro();
void print_io(uint16_t &input, uint8_t output[]);
#endif /* PULASAN_H */
