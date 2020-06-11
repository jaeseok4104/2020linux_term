#define __ARM__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <string.h>
#include <termios.h>

static int handle;

int main(int argc, char **argv)
{
    struct termios oldtio, newtio;
    unsigned char temp_data;
    unsigned char line1_data[] = {"Input txd data = "};
    int retval;
    int x;
#ifdef __ARM__
    handle = open("/dev/s3c_serial2", O_RDWR | O_NOCTTY);
#else
    handle = open("/dev/ttyS1", O_RDWR | O_NOCTTY);
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
    }
    tcsetattr(handle, TCSANOW, &oldtio);
    close(handle);
    return 0;
}