/*
    Programmer : jonghee minsick jaeseok

    reference https://wiki.dfrobot.com/NFC_Module_for_Arduino__SKU_DFR0231_
    target_board : achro imx6q
                    ubuntu 18.04
*/

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
// #include <unistd.h>

typedef struct _U_file{
    FILE *user_file;
    char name[100]; // file name
    unsigned char line[100];
    int num;
}U_file;

//serail connection integer//
static int handle;
U_file user_data;
U_file user_check; // 출석표 출력

char SERIAL_PORT[20] = "/dev/";

////////////////////////////////////////////////////////protocol//////////////////////////////////////////////////////////
const unsigned char wake[24] = {
    0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x03, 0xfd, 0xd4, 0x14, 0x01, 0x17, 0x00}; //wake up NFC module
const unsigned char firmware[9] = {
    0x00, 0x00, 0xFF, 0x02, 0xFE, 0xD4, 0x02, 0x2A, 0x00}; //
const unsigned char tag[11] = {
    0x00, 0x00, 0xFF, 0x04, 0xFC, 0xD4, 0x4A, 0x01, 0x00, 0xE1, 0x00}; //detecting tag command
const unsigned char std_ACK[25] = {
    0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x0C,
    0xF4, 0xD5, 0x4B, 0x01, 0x01, 0x00, 0x04, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x4b, 0x00, 0x00};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned char old_id[5]; //req_id
unsigned char receive_ACK[25]; //Command receiving buffer
unsigned char id_buff[100];
unsigned char temp_data;//read charactor buff

//////////////////////////NFC_Function/////////////////////////////////////
void copy_id(void)
{ //save old id
    int ai, oi;
    for (oi = 0, ai = 19; oi < 5; oi++, ai++)
    {
        old_id[oi] = receive_ACK[ai];
    }
}

char cmp_id(void)
{ //return true if find id is old
    int ai, oi;
    for (oi = 0, ai = 19; oi < 5; oi++, ai++)
    {
        if (old_id[oi] != receive_ACK[ai])
            return 0;
    }
    return 1;
}

int test_ACK(void)
{ // return true if receive_ACK accord with std_ACK
    int i;
    for (i = 0; i < 19; i++)
    {
        if (receive_ACK[i] != std_ACK[i])
            return 0;
    }
    return 1;
}

void send_id(void)
{ //send id to PC
    int i;
    printf("ID: ");
    for (i = 19; i <= 23; i++)
    {
        printf("%x", receive_ACK[i]);
        printf(" ");
    }
    printf("\n");
}

void UART1_Send_Byte(unsigned char command_data)
{ //send byte to device
    write(handle, &command_data, sizeof(command_data));
}
unsigned char temp_data;

void UART_Send_Byte(unsigned char command_data)
{ //send byte to PC
    printf("%.2x ", command_data);
    printf(" ");
}

void read_ACK(unsigned char temp)
{ //read ACK into reveive_ACK[]
    unsigned char i;
    for (i = 0; i < temp; i++)
    {
        if(read(handle, &temp_data, sizeof(temp_data)))
            receive_ACK[i] = temp_data;
    }
}

void wake_card(void)
{ //send wake[] to device
    unsigned char i;
    for (i = 0; i < 24; i++) //send command
        write(handle, &wake[i], sizeof(wake[i]));
}

void firmware_version(void)
{ //send fireware[] to device
    unsigned char i;
    for (i = 0; i < 9; i++) //send command
        UART1_Send_Byte(firmware[i]);
}

void send_tag(void)
{ //send tag[] to device
    unsigned char i;
    for (i = 0; i < 11; i++) //send command
    {
        if(write(handle, &tag[i], sizeof(tag[i])));
            // printf("%.2x  ",tag[i]);
    }
    // printf("\n");
}

void display(unsigned char tem)
{ //send receive_ACK[] to PC
    unsigned char i;
    for (i = 0; i < tem; i++) //send command
        UART_Send_Byte(receive_ACK[i]);
        // UART_Send_Byte(test[i]);

    printf("\n");
}

char check_id(void)
{
    for(char i=0;i<10;i++){
        if(id_buff[i] != 0)
            return 1;
    }
    return 0;
}

char open_file(U_file* pU_file, char *mode){
    pU_file->user_file = fopen(pU_file->name, mode);
    if(pU_file->user_file == NULL){
        printf("%s file open error\n", pU_file->name);
        return 0;
    }
    return 1;
}

void add_user(void)
{
    char exist_name_flag = 1;
    char add_Name[30]    = {0,};
    char ack_buff1[5]    = {0,};
    char ack_buff2[5]    = {0,};
    char check_arr[5]    = {0,};
    char flag = 0;

    open_file(&user_data,  "a+");
    if((open_file(&user_check, "r"))){
        fclose(user_check.user_file);
        open_file(&user_check, "a+");
        flag = 1;
    }

    printf("Taging please\n");
    while(1){
        send_tag();
        for (char i = 0; i < 25; i++)
            receive_ACK[i] = 0;
        read_ACK(25);
        for(char i=0; i<5;i++)
            ack_buff1[i] = receive_ACK[i+19];
        usleep(100000);
        
        send_tag();
        for (char i = 0; i < 25; i++)
            receive_ACK[i] = 0;
        read_ACK(25);
        for (char i = 0; i < 5; i++)
            ack_buff2[i] = receive_ACK[i+19];
        if(!strcmp(ack_buff1,check_arr)){
            for(char i = 0; i<5; i++){
                id_buff[i]   = ack_buff2[i];
                id_buff[i+5] = ack_buff1[i];
            }
        }
        else{
            for (char i = 0; i < 5; i++){
                id_buff[i]     = ack_buff1[i];
                id_buff[i + 5] = ack_buff2[i];
            }
        }
        usleep(100000);

        if(check_id()){
            while(!feof(user_data.user_file)){
                fgets(user_data.line, 100, user_data.user_file);
                if(strstr(user_data.line, id_buff) != NULL){
                   printf("already exist ID : %s \n", user_data.line);
                   exist_name_flag = 0;
                   getchar();
                   break;
                }
            }
            if(exist_name_flag){
                printf("add Name : \n");
                gets(add_Name);
                fprintf(user_data.user_file,  "%s\t\t\t%s\n", add_Name, id_buff);
                if(flag) fprintf(user_check.user_file, "\n%s\t\t\t%s\t\t\tX\n",add_Name, id_buff);
            }
            break;
        }
    }
    read_ACK(25);
    for(char i=0; i<100; i++){
        id_buff[i] = 0;
    }

    fclose(user_data.user_file);
    if(flag) fclose(user_check.user_file);
}

void made_checkboard(void)
{
    struct _user_info{
        char name[20];
        char id[20];
    };
    struct _user_info user_info ={{0,},{0,}};
    char flag = 0;
    
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
            for (char i = 0; i < 20; i++){
                user_info.name[i] = 0;
                user_info.id[i] = 0;
            }
        }
    }
    rewind(user_check.user_file);
    while(!feof(user_check.user_file)){
        fgets(user_check.line, 100, user_check.user_file);
        if(!feof(user_check.user_file))
            printf("%s",user_check.line);
    }
    getchar();
    system("clear");
    fclose(user_data.user_file);
    fclose(user_check.user_file);
}

void callName_user(void)
{
}

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        printf("not input serial port! \n");
        return 0;
    }

    struct termios oldtio, newtio;
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

    char func = 0;

    strcpy(user_data.name, "linux.txt");
    strcpy(user_check.name,"2020-06-05.txt"); //현재 날짜로 이름 설정 ex) 2020-06-12.txt

    wake_card();
    usleep(100000);
    read_ACK(15);
    usleep(100000);
    for(char i = 0; i<25; i++)
        receive_ACK[i] = 0;
    while(1){
        system("clear");
        printf("///////CallName/////////\n");
        printf("insert func: \n");
        scanf("%d", &func);
        getchar();
        switch(func){
            case 1:
                add_user();
                break;
            case 2:
                callName_user();
                break;
            case 3:
                made_checkboard();
                break;
            default:
                break;
        }
    }

    tcsetattr(handle, TCSANOW, &oldtio);
    close(handle);
    return 0;
}