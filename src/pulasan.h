#ifndef PULASAN_H
#define PULASAN_H

#include <modbus.h>

void setup_interupt(void);
int check_watchdog(modbus_t *mb);

#endif /* PULASAN_H */
