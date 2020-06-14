#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/fb.h>


typedef unsigned int U32;
void swap(int *swapa, int *swapb);
unsigned int random_pixel(void);


unsigned int makepixel(U32 r, U32 g, U32 b)
{
	return (U32)((r<<16)|(g<<8)|b);
}

int main(int argc, char** argv)
{
	int check;
	int frame_fd;
	U32 pixel;
	int offset;
	int posx1,posy1, posx2,posy2;
	int repx, repy, count = 1000;
	unsigned int* pfbdata;
	struct fb_var_screeninfo fvs;
	
	if((frame_fd=open("/dev/fb0",O_RDWR))<0){
		perror("Frame Buffer Open Error!");
		exit(1);
	}
	
	if((check = ioctl(frame_fd,FBIOGET_VSCREENINFO,&fvs))<0){
		perror("Get Information Error - VSCREENINFO!");
		exit(1);
	}
	
	if(fvs.bits_per_pixel != 32){
		perror("Unsupport Mode, 32Bpp only.");
		exit(1);
	}

	pfbdata = (unsigned int*) mmap(0,fvs.xres*fvs.yres*32/8,PROT_READ|\
		PROT_WRITE,MAP_SHARED,frame_fd,0);
	if((unsigned)pfbdata == (unsigned) -1){
		perror("Error Mapping\n");
	}	
	pixel = 0;
	for(repy=0; repy<fvs.yres;repy++){
		offset= repy*fvs.xres;
		for(repx=0; repx< fvs.xres;repx++){
			*(pfbdata+offset+repx) = pixel;
		}
	}	
	munmap(pfbdata,fvs.xres*fvs.yres*32/8);
	close(frame_fd);	
	return 0;
}

unsigned int random_pixel(void)
{
	return rand();
}

void swap(int *swapa, int *swapb){
	int temp;
	if(*swapa > *swapb){
		temp = *swapb;
		*swapb = *swapa;
		*swapa = temp;
	}
}
