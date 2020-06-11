#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <linux/input.h>

int main(int argc, char** argv){
	int fd,i=0;
	unsigned short  data[8];
	
	if(argc<2){
		perror("more than 2 arguments need\n");
		exit(1);
	}
	fd = open(argv[1],O_RDONLY);
	if(fd<0){
		printf("%s : Open Error\n",argv[1]);
		exit(1);
	}

	while(1){
		read(fd,data,8*2);
		printf("data :");
		for(i=0;i<8;i++)
			printf("0x%04x  ",data[i]);
		printf("\n");
	}
	close(fd);
	return 0;
}

