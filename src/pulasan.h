#ifndef PULASAN_H
#define PULASAN_H

#include <modbus.h>

void setup_interupt(void);
int check_watchdog(modbus_t *mb);
void print_intro();

#endif /* PULASAN_H */
