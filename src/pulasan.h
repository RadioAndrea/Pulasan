#ifndef PULASAN_H
#define PULASAN_H

#include <modbus.h>

void setup_interupt(void);
int check_watchdog(modbus_t *mb);
void print_intro();
void print_io(uint16_t &input, uint8_t output[]);
int read_inputs(modbus_t *mb, uint16_t *input);
int write_outputs(modbus_t *mb, uint8_t *outputs);

#endif /* PULASAN_H */
