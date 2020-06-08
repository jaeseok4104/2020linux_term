#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char** argv){
	int fd,i=0;
	int tmp =0;
	unsigned short row,col;	
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
		tmp = read(fd,data,8*2);
		printf("[%d]data :",tmp);
		for(i=0;i<8;i++)
			printf("%d\t",data[i]);
		printf("\n");

		if(data[5] == 53) row = data[6];
		else if(data[6] == 54) col = data[6];
	
		
	}
	close(fd);
	return 0;
}

