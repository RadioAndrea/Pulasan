#ifndef PULASAN_H
#define PULASAN_H

#include <modbus.h>
#include <string>


void setup_interupt(void);
int parse_command_line_options(int argc, char *argv[], std::string &ip_address);
int check_watchdog(modbus_t *mb);
void print_intro();
void print_io(uint16_t &input, uint8_t output[]);
int read_inputs(modbus_t *mb, uint16_t *input);
int write_outputs(modbus_t *mb, uint8_t *outputs);

bool get_input_bit(int input, int bit_num);

#endif /* PULASAN_H */
