#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/fb.h>

typedef unsigned int U32;

unsigned int makePixel(U32 r, U32 g, U32 b){
	return (U32)((r<<16)|(g<<8)|b);
}

int main(int argc, char** argv){
	int check,i;
	unsigned int width;
	int frame_fd, touch_fd;
	U32 pixel;
	int offset;
	int posx1,posy1, posx2, posy2;
	int repx, repy;
	unsigned int* pfbdata;
	unsigned short row,col,touch=0;
	unsigned short  data[8];
	struct fb_var_screeninfo fvs;
	
	if((frame_fd=open("/dev/fb0",O_RDWR))<0){
		perror("Frame Buffer Open Error!");
		exit(1);
	}
	
	if((check= ioctl(frame_fd,FBIOGET_VSCREENINFO,&fvs))<0){
		perror("Get Information Error - VSCREENINFO!");
		exit(1);
	}
	
	if(fvs.bits_per_pixel != 32){
		perror("Unsupport Mode, 32Bpp Only!");	
		exit(1);
	}	
	if(argc<2){
		perror("more than 2 arguments need\n");
		exit(1);
	}
	if(argc>=3) width = atoi(argv[2]);
	else width = 30;
	touch_fd = open(argv[1],O_RDONLY);
	if(touch_fd<0){
		printf("%s : Open Error\n",argv[1]);
		exit(1);
	}

	pfbdata = (unsigned int*)mmap(0,fvs.xres*fvs.yres*32/8,PROT_READ|\
		PROT_WRITE,MAP_SHARED,frame_fd,0);
	if((unsigned)pfbdata == (unsigned)-1){
		perror("Error Mapping");
		exit(1);
	}	
	pixel = makePixel(255,0,0);

	while(1){
		read(touch_fd,data,8*2);
		printf("data :");
		for(i=0;i<8;i++)
			printf("%d\t",data[i]);
		printf("\n");

		if(data[5] == 53){
			row = data[6];
			touch++;
		}
		else if(data[5] == 54){
			col = data[6];
			touch++;
		}
		if(touch>=2){
			posx1 = (int)((row-width/2)<=0 ? 0 : row-width/2);
			posy1 = (int)((col-width/2)<=0 ? 0 : col-width/2);
			posx2 = (int)((row+width/2)<=0 ? 0 : row+width/2);
			posy2 = (int)((col+width/2)<=0 ? 0 : col+width/2);
			printf("center[%d] : %d,%d\n",touch,row,col);
			printf("Rect : %d,%d  %d,%d\n",posx1,posy1,posx2,posy2);
			
		
			for(repy = posy1; repy<posy2;repy++){
				offset = repy*fvs.xres;
				for(repx=posx1;repx<posx2;repx++){
					*(pfbdata+offset+repx) = pixel;
				}
			}
			touch = 0;
		}
	}
	
	munmap(pfbdata,fvs.xres*fvs.yres*32/8);
	close(frame_fd);
	close(touch_fd);
	return 0;
}


