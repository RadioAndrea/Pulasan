#include "pulasan.h"
#include <modbus.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <exception>
#include <unistd.h>
#include <cerrno>

#define IP_ADDR "137.155.2.170"

const int MODBUS_READ_ADDRESS = 100001;
const int MODBUS_WRITE_ADDRESS = 1;

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
    if (mb == NULL) {
        fprintf(stderr, "Unable to allocate libmodbus context\n");
        return -1;
    }
    if (modbus_connect(mb) == -1){
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(mb);
        return -1;
    }

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sig_to_exception;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    uint8_t inputs[16] = {0};
    uint8_t outputs[16] = {0};
    bool write_bool = false;
    int  write_num  = MODBUS_WRITE_ADDRESS;
    int  count      = 0;

    std::cout << "Project Pulasan" << std::endl << "Press CTRL-C to exit" << std::endl << std::endl << std::endl << std::endl;

    try{
        while (true) {
            modbus_read_input_bits(mb, MODBUS_READ_ADDRESS-1, 16, inputs);
            modbus_write_bits(mb, MODBUS_WRITE_ADDRESS, 16, outputs);

            // use \e[A to move up a line
            std::cout << "\r\e[A\e[A\e[A"
                      << "         0123456789012345" << std::endl;
            std::cout << "inputs:  ";
            for(int i = 0; i < 16; i++){
                std::cout << (inputs[i]+0);
            }
            std::cout << std::endl;
            std::cout << "outputs: ";
            for(int i = 0; i < 16; i++){
                std::cout << (outputs[i]+0);//(outputs[i] == true) ? "1" : "0";
            }
            std::cout << std::endl << std::flush;

            count++;
            if(count==10){
                outputs[write_num] = write_bool;
                count = 0;
                write_num++;
            }
            if(write_num == 16){
                write_bool = !write_bool;
                write_num = 0;
            }
            usleep(5*1000); //Sleep 5ms
        }
    }catch(InterruptException& e){
        modbus_close(mb);
        modbus_free(mb);
    }
}
