#include "pulasan.h"
#include <modbus.h>
#include <stdio.h>
#include <iostream>
#include <sys/ioctl.h>
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

/* used for ctrl-c interupt */
void sig_to_exception(int s)
{
  throw InterruptException(s);
}

int main(){


    modbus_t *mb;

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

    setup_interupt();
    print_intro();

    bool write_bool = true;
    int  write_num  = 0;
    unsigned int count = 0;
    uint8_t outputs[16] = {0};

    try{
        while (true) {
            uint16_t input = 0;
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

            print_io(input, outputs);

            count++;
            if(count%50==0){
                outputs[write_num] = write_bool;
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

/* Sets up the interupt to make an exception */
void setup_interupt(void){
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sig_to_exception;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
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

void print_intro(){
    //Get the window size
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    for(int i = 0; i <w.ws_col; i++)
        std::cout << "=";
    std::cout << std::endl << "Project Pulasan" << std::endl << "Press CTRL-C to exit" << std::endl;
    for(int i = 0; i <w.ws_col; i++)
        std::cout << "=";
    std::cout<< std::endl << std::endl << std::endl << std::endl;
}

/* Prints IO data
   output must be an array of length 16
*/
void print_io(uint16_t &input, uint8_t output[]){
    // use \e[A to move up a line
    std::cout << "\r\e[A\e[A\e[A"
              << "         0123456789012345" << std::endl
              << "inputs:  ";
    std::bitset<16> inputs(input);
    for(int i = 0; i < 16; i++){
        std::cout << inputs[i];
    }
    std::cout << std::endl
              << "outputs: ";
    for(int i = 0; i < 16; i++){
        std::cout << (output[i]+0);
    }
    std::cout << std::endl << std::flush;
}
