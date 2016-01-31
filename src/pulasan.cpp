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
#include <getopt.h>
#include <string>
#include <chrono>

#define DEFAULT_IP_ADDR "137.155.2.170"

#define AVERAGE_ALPHA 0.05 //Alpha to use when computing the rolling average

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

int main(int argc, char *argv[]){
    std::string ip_address = "";
    int parse_ret = parse_command_line_options(argc, argv, ip_address);
    if(parse_ret != 1)
        return parse_ret;

    if(ip_address.empty()){
        ip_address = DEFAULT_IP_ADDR;
    }

    print_intro();

    modbus_t *mb;

    mb = modbus_new_tcp(DEFAULT_IP_ADDR, 502);
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

    bool write_bool = true;
    int  write_num  = 0;
    unsigned int count = 0;
    uint8_t outputs[16] = {0};

    try{
        while (true) {
            uint16_t input = 0;
            if(read_inputs(mb, &input) == -1)
                break;

            if(write_outputs(mb, outputs) == -1)
                break;

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

/* parses the command line options
   returns 1 on success
   ip_address must be allocated memory
*/
int parse_command_line_options(int argc, char *argv[], std::string &ip_address){
    const char * short_opt = "hi:";
    struct option   long_opt[] ={
        {"help",          no_argument,       NULL, 'h'},
        {"ip",            required_argument, NULL, 'i'},
        {NULL,            0,                 NULL, 0  }
    };
    int c;
    while((c = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1){
        switch(c){
        case -1:       /* no more arguments */
        case 0:        /* long options toggles */
            break;

        case 'i':
            if(!ip_address.empty()){
                std::cout << "You can only enter the IP Address once\n";
                return -2;
            }
            ip_address = optarg;
            break;

        case 'h':
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("  -i, --ip ip_address   the IP for the IO module\n");
            printf("  -h, --help            print this help and exit\n");
            printf("\n");
            return(0);

        case ':':
        case '?':
            fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
            return(-2);

        default:
            fprintf(stderr, "%s: invalid option -- %c\n", argv[0], c);
            fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
            return(-2);
        }
    }
    return 1;
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
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto last_time = start_time;
    start_time = std::chrono::high_resolution_clock::now();
    int diff_us = std::chrono::duration_cast<std::chrono::microseconds>(start_time-last_time).count() ;
    std::cout << "   Cycle time: "
              << std::cout.width(5)
              << diff_us
              << " us";

    std::cout << std::endl
              << "outputs: ";
    for(int i = 0; i < 16; i++){
        std::cout << (output[i]+0);
    }

    static float average_time = diff_us;
    average_time = (AVERAGE_ALPHA * diff_us) + (1.0 - AVERAGE_ALPHA) * average_time;
    std::cout << " Average time: "
              << std::cout.width(6)
              << average_time
              << " us";

    std::cout<< std::endl << std::flush;
}

int read_inputs(modbus_t *mb, uint16_t *input){
    int read_ret = modbus_read_registers(mb, MODBUS_READ_ADDRESS, 1, input);
    if(read_ret == -1){
        //Error status on read
        if(check_watchdog(mb) == -1 ){
            fprintf(stderr, "Read failed: %s\n", modbus_strerror(errno));
            return -1;
        }
    }
    return 0;
}

int write_outputs(modbus_t *mb, uint8_t *outputs){
    int write_ret = modbus_write_bits(mb, MODBUS_WRITE_ADDRESS, 16, outputs);
    if(write_ret == -1){
        //Error status on write
        if(check_watchdog(mb) == -1){
            fprintf(stderr, "Write failed: %s\n", modbus_strerror(errno));
            return -1;
        }
    }
    return 0;
}
