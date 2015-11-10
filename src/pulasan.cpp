#include "pulasan.h"
#include <modbus.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <exception>
#include <unistd.h>
#include <cerrno>
#include <bitset>

#define IP_ADDR "137.155.2.170"

const int MODBUS_READ_ADDRESS = 00000;
const int MODBUS_WRITE_ADDRESS = 00000;

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

    uint16_t input = 0;
    uint8_t outputs[16] = {0};
    bool write_bool = true;
    int  write_num  = 0;
    int  count      = 0;

    std::cout << "Project Pulasan" << std::endl << "Press CTRL-C to exit" << std::endl << std::endl << std::endl << std::endl;

    try{
        while (true) {
            int read_ret = modbus_read_registers(mb, MODBUS_READ_ADDRESS, 1, &input);
            if(read_ret == -1){
                //Error status on read
                if(check_watchdog(mb) == 0){
                    continue;
                }
                fprintf(stderr, "Read failed: %s\n", modbus_strerror(errno));
                break;
            }
            int write_ret = modbus_write_bits(mb, MODBUS_WRITE_ADDRESS, 16, outputs);
            if(write_ret == -1){
                //Error status on write
                if(check_watchdog(mb) == 0){
                    continue;
                }
                fprintf(stderr, "Write failed: %s\n", modbus_strerror(errno));
                break;
            }

            // use \e[A to move up a line
            std::cout << "\r\e[A\e[A\e[A"
                      << "         0123456789012345" << std::endl;
            std::cout << "inputs:  " << std::bitset<16>(input) << std::endl;
            std::cout << "outputs: ";
            for(int i = 0; i < 16; i++){
                std::cout << (outputs[i]+0);//(outputs[i] == true) ? "1" : "0";
            }
            std::cout << std::endl << std::flush;

            count++;
            if(count==50){
                outputs[write_num] = write_bool;
                count = 0;
                write_num++;
            }
            if(write_num==16){
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

/* Checks if the error is a watchdog error
   If the error is a watchdog timeout, resets it and returns 0
   Otherwise return -1
*/
int check_watchdog(modbus_t *mb){
    if(errno != EMBXSFAIL)
        //Not a watchdog error
        return -1;
    uint16_t wd_status = 0;
    int read_status = modbus_read_registers(mb, 4108, 1, &wd_status);
    if (read_status == -1)
        return -1;
    if(modbus_write_register(mb, 4385, 0xBECF) == -1)
        return -1;
    if(modbus_write_register(mb, 4385, 0xAFFE) == -1)
        return -1;
    return 0;
}
