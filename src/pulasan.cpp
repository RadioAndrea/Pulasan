#include "pulasan.h"
#include <modbus.h>
#include <stdio>
#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <exception>


#define IP_ADDR "137.155.2.170"

const int MODBUS_READ_ADDRESS 100001;
const int MODBUS_WRITE_ADDRESS 1;

class InterruptException : public std::exception
{
public:
  InterruptException(int s) : S(s) {}
  int S;
};

void sig_to_exception(int s)
{
  throw InterruptException(s);
}

int main(){
    modbus_t *mb;
    uint16_t tab_reg[32];

    mb = modbus_new_tcp(IP_ADDR, 502);
    if (ctx == NULL) {
        fprintf(stderr, "Unable to allocate libmodbus context\n");
        return -1;
    }
    if (modbus_connect(mb) == -1){
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sig_to_exception;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    uint8_t inputs[16];
    bool write_bool = false;

    try{
        while (true) {
            modbus_read_input_bits(mb, MODBUS_READ_ADDRESS, 16, inputs);
            write_bool = !write_bool;
            for(int i = MODBUS_WRITE_ADDRESS; i < MODBUS_WRITE_ADDRESS + 16; i++){
                modbus_write_bit(mb, i, write_bool);
                sleep(1);
            }
        }
    }catch(InterruptException& e){
        modbus_close(mb);
        modbus_free(mb);
    }
}
