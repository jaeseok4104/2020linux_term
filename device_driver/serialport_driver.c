#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

int open_port(void)
{
    int fd; 
    fd = open("/dev/ttySAC0", O_RDWR | O_NOCTTY| O_NDELAY);
    if (fd ==-1){
        perror(" open_port:unable to open /dev/ttySAC0 - ");
    }
    else
        fnctl(fd, F_SETFL, 0);
    return (fd);
    
}