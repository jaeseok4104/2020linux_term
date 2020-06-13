#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <linux/fb.h>
#include "./glcd-font.h"
#include "./mario.h"
#include "./nfc.h"



#define PUSH_SWITCH_DEVICE "/dev/fpga_push_switch"
#define PUSH_SWITCH_MAX_BUTTON 9

#define SCREEN_BPP 32

#define TOUCH_SCREEN "/dev/input/event1"
#define NFC_PORT "/dev/ttyUSB0"
#define MARIO_STEP 15


typedef unsigned int U32;

typedef struct _U_file{
	FILE *user_file;
	char name[100];
	unsigned char line[100];
	int num;
}U_file;

typedef struct _user_info{
	char name[20];
	char id[20];
	char check;
}U_info;

typedef struct {
	int posX;
	int posY;

	int rows;
	int cols;

	int width;
	int height;

	char **str;  // cell string data [rows][cols]

	int borderWidth;
	int fontSize;

	U32 borderColor;
	U32 bgColor;
	U32 fontColor;
}t_table;

t_table table1;

int frame_fd, push_sw_dev;
struct fb_var_screeninfo fvs;
unsigned int* pfbdata;

static int hNFC;
U_file user_data;
U_file user_check;

int timer;
long int uCnt;

unsigned int initialize();

unsigned int clear_Screen();
unsigned int paint_dot(int x, int y, int width, U32 pixel,int full);
unsigned int paint_square(int x, int y, int width, U32 pixel,int full);
unsigned int paint_font(int num,int x, int y);
unsigned int paint_str(int x, int y,char* str,int length,int fontSize,int fontpadding,U32 pixel,int transparent);
unsigned int paint_mario(int x, int y,int Size,int draw);

unsigned int setTable();
unsigned int paint_table(t_table t);

/*********** NFC_Function   **************/
void made_checkboard(void);
char open_file(U_file* pU_file, char *mode);


unsigned int makePixel(U32 r, U32 g, U32 b){
	//return (U32)((r<<11)|(g<<5)|b);
	return (U32)((r<<16)|(g<<8)|b);
}

unsigned int mario_x,mario_y;
unsigned int mario_x_prev,mario_y_prev;
unsigned int lastPush;
int main(int argc, char** argv){
	int cell =0;
	U32 pixel;

	int push_pid;
	unsigned char push_data[PUSH_SWITCH_MAX_BUTTON];

	// dateTime
	time_t tt;

	char strBuf[30];

	if(argc==4){
		int r = atoi(argv[1]);
		int g = atoi(argv[2]);
		int b = atoi(argv[3]);
		pixel = makePixel(r,g,b);
	}
	else pixel = makePixel(0,255,0);
	
	//Device Init
	if(!initialize()) exit(1);

	strcpy(user_data.name,"linux.txt");
	strcpy(user_check.name,"2020-06-05.txt");


	// Table Data Setting
	setTable();


	// pointer of FrameBuffer 
	pfbdata = (unsigned int*)mmap(0,fvs.xres*fvs.yres*SCREEN_BPP /8,PROT_READ|\
		PROT_WRITE,MAP_SHARED,frame_fd,0);
	if((unsigned)pfbdata == (unsigned)-1){
		perror("Error Mapping");
		exit(1);
	}	

	paint_table(table1);
	mario_x = 850;
	mario_x_prev = 850;
	mario_y = 30;
	mario_y_prev = 30;

	// Alert Chek
	//paint_dot(fvs.xres/2,fvs.yres/2, 200,makePixel(0,0,255),0);

	while(1){
		
		if(uCnt>=1000000){
			uCnt = 0;
		}
		
		read(push_sw_dev,&push_data,sizeof(push_data));

		if(push_data[4] == 1){	// Clear Screen
			printf("Clear Screen\n");
			clear_Screen();
			made_checkboard();
			paint_table(table1);
		}
		else if(push_data[1] == 1){
			if(lastPush == 0) mario_y = (mario_y - MARIO_STEP)<0 ? 0 : mario_y - MARIO_STEP;
			lastPush = 1;
		}
		else if(push_data[7] == 1){
			if(lastPush == 0) mario_y = (mario_y + MARIO_STEP)>(fvs.yres-MARIO_SIZE_Y) ? fvs.yres - MARIO_SIZE_Y : mario_y + MARIO_STEP;
			lastPush = 1;
		}
		else if(push_data[3] == 1){
			if(lastPush == 0) mario_x = (mario_x - MARIO_STEP)<0 ? 0 : mario_x - MARIO_STEP;
			lastPush = 1;
		}
		else if(push_data[5] == 1){
			if(lastPush == 0) mario_x = (mario_x + MARIO_STEP)>(fvs.xres-MARIO_SIZE_X) ? fvs.xres - MARIO_SIZE_X : mario_x + MARIO_STEP;
			lastPush = 1;
		}
		else lastPush = 0;


		time(&tt);
		sprintf(strBuf,"Now :%s",ctime(&tt));
		paint_str(40,50,strBuf,strlen(strBuf),3,3,pixel,0);

		if(mario_x_prev != mario_x || mario_y_prev != mario_y) 
			paint_mario(mario_x_prev,mario_y_prev,6,0);
		paint_mario(mario_x,mario_y,6,1);
		mario_x_prev = mario_x;
		mario_y_prev = mario_y;

		printf("cnt : %d\n",uCnt);
		usleep(1);
		uCnt++;
	} 
	munmap(pfbdata,fvs.xres*fvs.yres*SCREEN_BPP /8); close(frame_fd);
	close(push_sw_dev);
	for(cell =0; cell<table1.rows*table1.cols;cell++){
		free(table1.str[cell]);
	}
	free(table1.str);

	return 0; 
} 

unsigned int initialize(){
	int check,i;
	push_sw_dev = open(PUSH_SWITCH_DEVICE,O_RDONLY);
	if(push_sw_dev<0){
		perror("Push Switch Device open error");
		return 0;
	}

	if((frame_fd=open("/dev/fb0",O_RDWR))<0){
		perror("Frame Buffer Open Error!");
		return 0;
	}
	
	if((check= ioctl(frame_fd,FBIOGET_VSCREENINFO,&fvs))<0){
		perror("Get Information Error - VSCREENINFO!");
		return 0;
	}
	
	if(fvs.bits_per_pixel != SCREEN_BPP){
		perror("Unsupport Mode, 32Bpp Only!");	//return 0;
	}	


	return 1;
}
unsigned int clear_Screen(){
	int repx,repy,offset;
	for(repy = 0; repy<fvs.yres;repy++){
		offset = repy*fvs.xres;
		for(repx=0;repx<fvs.xres;repx++){
			*(pfbdata+offset+repx) = 0;
		}
	}
}
unsigned int paint_dot(int x, int y, int width, U32 pixel,int full){
	int posx1 = x- width/2;
	int posy1 = y - width/2;
	int posx2 = x + width/2;
	int posy2 = y + width/2;

	int repy,repx,offset,dist;
	for(repy = posy1; repy<=posy2;repy++){
		offset = repy*fvs.xres;
		for(repx=posx1;repx<=posx2;repx++){
			dist = (repy-y)*(repy-y) + (repx-x)*(repx-x);
			if(offset+repx>= 1024*600 || (offset+repx)<=0) continue;
			if(full){
				if(dist <= (width*width)/4) *(pfbdata+offset+repx) = (int)pixel;	
			}else{
				if(dist <= (width*width)/4 && dist >= ((width)*(width)/5)) *(pfbdata+offset+repx) = (int)pixel;	
			}
		}
	}
}

unsigned int paint_square(int x, int y, int width, U32 pixel,int full){
	int posx1 = x- width/2;
	int posy1 = y - width/2;
	int posx2 = x + width/2;
	int posy2 = y + width/2;

	int repy,repx,offset;
	int distx,disty;
	for(repy = posy1; repy<=posy2;repy++){
		offset = repy*fvs.xres;
		for(repx=posx1;repx<=posx2;repx++){
			distx = (repx-x)*(repx-x);
			disty = (repy-y)*(repy-y);
			
			if(offset+repx>= 1024*600 || (offset+repx)<0) continue;
			if(full){
				*(pfbdata+offset+repx) = (int)pixel;
			}else{
				if(distx >= (width*width)/5) *(pfbdata+offset+repx) = (int)pixel;
				if(disty >= (width*width)/5) *(pfbdata+offset+repx) = (int)pixel;
			}
		}
	}
}
unsigned int paint_font(int num,int x, int y){
	unsigned char* f = &System5x7[(GLCD_NUMBER+(num%10))*5];
	unsigned char data;
	int i=0,j=0;
	int width = 10;
	U32 pix[2];// = {{0,0,0},{100,100,0}};
	pix[0] = makePixel(0,0,0);
	pix[1] = makePixel(255,0,255);

	for(i =0 ; i<8;i++){
		for(j=0;j<GLCD_FONT_LEN;j++){
			data = f[j];
			if(data&(0x01<<i))paint_dot(x+j*width,y+i*width,width,pix[1],1);
		}
	}
}


unsigned int paint_str(int x, int y,char* str,int length,int fontSize,int fontpadding,U32 pixel,int transparent){
	unsigned char* f;
	unsigned char data;
	int i=0,j=0,c=0;
	int width = fontSize;
	int height = fontSize;
	int padding = fontpadding;
	U32 pix[2];
	pix[0] = makePixel(0,0,0);
	pix[1] = pixel;
	
	for(c=0;c<length;c++){
		f = &System5x7[(str[c]+GLCD_CHAR_OFFSET)*5];
		if(str[c]=='\n'){
			y+=height*8+padding*8;
			x-=(c*(padding+width*5))+(width*GLCD_FONT_LEN+1);
			continue;	
		}
		for(i =0 ; i<8;i++){
			for(j=0;j<GLCD_FONT_LEN;j++){
				data = f[j];
				if(data&(0x01<<i))	paint_dot(x+(j*width-1)+(c*(width*5+padding)),y+(i*height-1),width,pix[1],1);
				else if(!transparent){
					paint_dot(x+(j*width-1)+(c*(padding+width*5)),y+(i*height-1),width,pix[0],1);
				}
				
			}
		}
	}
}


unsigned int paint_mario(int x, int y,int Size,int draw){
	unsigned char* f =(unsigned char*) Mario_map;
	unsigned char data;
	int px =0, py =0;
	int width = Size;
	int height = Size;
	U32 pix;

	for(px =0 ; px<MARIO_SIZE_X;px++){
		for(py=0;py<MARIO_SIZE_Y;py++){
			data = f[py*MARIO_SIZE_X+px];
			if(draw) pix = Mario_Color[data];
			else pix = makePixel(0,0,0);
			if(data != 2) paint_square(x+(px*width-1),y+(py*height-1),width,pix,1);
		}
	}
}

unsigned int setTable(){
	int cell;
	table1.posX = 12;
	table1.posY = 130;
	table1.rows = 7;
	table1.cols = 3;
	table1.width = 1000;
	table1.height = 450;
	table1.borderWidth = 4;
	table1.fontSize = 3;
	table1.borderColor = makePixel(50,50,255);
	table1.bgColor = makePixel(255,255,255);
	table1.fontColor = makePixel(0,0,0);
	table1.str = (char**)malloc(table1.rows*table1.cols*sizeof(char *));
	for(cell =0; cell<table1.rows*table1.cols;cell++){
		table1.str[cell] = (char *)malloc(100 * sizeof(char));
	}
	made_checkboard();
}

unsigned int paint_table(t_table t){
	int r,c;
	int h,w;
	int offsetX = t.posX;
	int offsetY = t.posY;
	int bw = t.borderWidth;
	int cWidth = t.width/t.cols;
	int cHeight = t.height/t.rows;

	char strBuf[100];
	time_t tt;
	if(cWidth*t.cols<t.width){
		cWidth++;
		t.width++;
	}
	if(cHeight*t.rows<t.height){
		cHeight++;
		t.height++;
	}
	time(&tt);
		
	sprintf(strBuf,"#Day:%s",user_check.name);
	paint_str(40,80,strBuf,strlen(strBuf),3,3,makePixel(255,0,0),0);

	//Draw Border
	for(h=0; h<=t.height;h++){
		for(w =0; w<=t.width;w++){
			if((h%(cHeight) ==0)|| (w%(cWidth)==0))
				paint_dot(offsetX+w,offsetY + h,bw,t.borderColor,1);
			else 
				if(h<=cHeight)
					paint_dot(offsetX+w,offsetY + h,1,makePixel(0,0,155),1);
				else
					paint_dot(offsetX+w,offsetY + h,1,t.bgColor,1);
		}
	}

	// Draw String data
	for(r=0;r<t.rows;r++){
		for(c=0;c<t.cols;c++){
			char* s = t.str[r*t.cols + c];
			
			paint_str(2*bw+offsetX+(c*cWidth),bw*2+offsetY+(r*cHeight),s,strlen(s),t.fontSize,1,t.fontColor,1);
			printf("[%d,%d]%s\n",r,c,s);
		}
	}
}


void made_checkboard(void){
    struct _user_info user_info ={{0,},{0,},0};
    char flag = 0;
    char i = 0;
	char* userName;
	char* userID;
	char* userCheck;
	char data[30];
	int idx=0,j=0;

    
    if(!open_file(&user_data, "r"))
        return;
    if(!open_file(&user_check, "r")){
        open_file(&user_check, "w+");

        fprintf(user_check.user_file, "Name\t\t\tid\t\t\tCheck\n");
        while(!feof(user_data.user_file))
        {
            fgets(user_data.line, 100, user_data.user_file);
            sscanf(user_data.line, "%s\t\t\t%s", user_info.name, user_info.id);
            if(!feof(user_data.user_file))
                fprintf(user_check.user_file, "%s\t\t\t%s\t\t\tX\n", user_info.name, user_info.id);
            for (i = 0; i < 20; i++){
                user_info.name[i] = 0;
                user_info.id[i] = 0;
            }
        }
    }
    rewind(user_check.user_file);
    while(!feof(user_check.user_file)){
        fgets(user_check.line, 100, user_check.user_file);
        if(!feof(user_check.user_file)){
            printf("%s",user_check.line);
			userName =(char*)malloc(100*sizeof(char));
			userID=(char*)malloc(100*sizeof(char));
			userCheck =(char*)malloc(3*sizeof(char));

			sscanf(user_check.line,"%s\t\t\t%s\t\t\t%s\n",userName,data,userCheck);
			
			if(idx>0){ 
				sprintf(userID,"%d",data[j]); 
				for(j=1;j<10;j++){ 
					sprintf(userID,"%s %d",userID,data[j]);
				}
				userID[j] = 0x00;
			}
			else{
				sprintf(userID,"%s",data); 
			}
			table1.str[idx*3] = userName;
			table1.str[idx*3+1] = userID;
			table1.str[idx*3+2] = userCheck;
			printf("%s, %s, %s\n", userName,userID,userCheck);
			printf("%d %d %d %d %d %d \n",userName[0],userName[1],userName[2],userName[3],userName[4],userName[5]);
			idx++;
		}
    }
    fclose(user_data.user_file);
    fclose(user_check.user_file);
}

char open_file(U_file* pU_file,char *mode){
	pU_file->user_file = fopen(pU_file->name,mode);
	if(pU_file->user_file == NULL){
		printf("%s file open error\n",pU_file->name);
		return 0;
	}
	return 1;
}
