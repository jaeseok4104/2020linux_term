#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <string.h>
#include <termios.h>

static int handle;
char SERIAL_PORT[20] = "/dev/";
unsigned char NFC_WAKE_SIGNAL[5] = {55,55,0,0,0};

int main(int argc, char **argv)
{
    if(argc == 1){
        printf("not input serial port! \n");
        return 0;
    }

    struct termios oldtio, newtio;
    unsigned char temp_data;
    unsigned char line1_data[] = {"Input txd data = "};
    int retval;
    int x;

    strcat(SERIAL_PORT, argv[1]);
    printf("SERIAL PORT : %s %d\n", SERIAL_PORT, x);
    usleep(1000000);
#ifdef __ARM__
    handle = open("/dev/s3c_serial2", O_RDWR | O_NOCTTY);
#else
    handle = open(SERIAL_PORT, O_RDWR | O_NOCTTY);
#endif
    if (handle < 0)
    {
        printf("Serial Port Open Fail!\n ");
        return -1;
    }

    tcgetattr(handle, &oldtio);
    memset(&newtio, 1, sizeof(newtio));
    memset(&newtio, 1, sizeof(newtio));
    newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR | ICRNL;
    newtio.c_oflag = OPOST | ONLCR;
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;
    tcflush(handle, TCIFLUSH);
    tcsetattr(handle, TCSANOW, &newtio);
    system("clear");
    printf("Serial Communication Test Program! \n");

    // Send Message
    while (line1_data[x] != 0)
    {
        write(handle, &line1_data[x], sizeof(line1_data[x]));
        usleep(20);
        x++;
    }
    
    // Recv Message
    while (1)
    {
        retval = read(handle, &temp_data, sizeof(temp_data));
        if (retval >= 1)
            printf("input data = %c\n\n", temp_data);
        if(temp_data == 'q'){
            printf("Good bye! \n");
            break;
        }
    }
    tcsetattr(handle, TCSANOW, &oldtio);
    close(handle);
    return 0;
}