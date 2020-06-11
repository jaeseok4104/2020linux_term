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

char scan(void)
{

}

static int handle;
char SERIAL_PORT[20] = "/dev/";
    unsigned char wake[24] = {0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x03, 0xfd, 0xd4, 0x14, 0x01, 0x17, 0x00};

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
    printf("value : %d \n", wake[1]);

    // printf("%d \n",NFC_WAKE_SIGNAL[x]);
    // Send Message
    for(int i=0; i<24; i++){
        printf("%x ", wake[i]);
        write(handle, &wake[i], sizeof(wake[i]));
    }
    printf("\n");
    
    char pid = 0;
    // Recv Message
    while (1){
        pid = fork();
        retval = read(handle, &temp_data, sizeof(temp_data));
        if (retval != 0)
            printf("%x \n", temp_data);
        // else if(retval == 0)
        //         break;
        // else break;
    }
    printf("\n");
    tcsetattr(handle, TCSANOW, &oldtio);
    close(handle);
    return 0;
}