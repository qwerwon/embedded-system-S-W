#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<sys/stat.h>
#include<signal.h>
#include<fcntl.h>
#include<errno.h>
#include<dirent.h>
#include<linux/input.h>
#include<termios.h>
#include<time.h>

#define SW_1	1
#define SW_2	2
#define SW_3	4
#define SW_4	8
#define SW_5	16
#define SW_6	32
#define SW_7	64
#define SW_8	128
#define SW_9	256

#define SW_2AND3	6
#define SW_5AND6	48
#define SW_8AND9	384



#define KEY_RELEASE 0
#define KEY_PRESS 1
#define MAX_BUTTON 9
#define READKEY 32
#define FPGA_SWITCH 4
#define BUFF_SIZE 64
#define INPUT_TO_MAIN 1234
#define MAIN_TO_OUTPUT 4321

#define BACK_KEY 158
#define VOL_UP_KEY 115
#define VOL_DOWN_KEY 114

#define ALARM_SETTING 1
#define ALARMING	2
#define ALARM_WAITING	4

#define READKEY_MACRO \
	switch(rcv_msg.data){\
		case BACK_KEY : quit_state=0;\
			break;\
		case VOL_UP_KEY : mode=(mode+1)%5;\
			break;\
		case VOL_DOWN_KEY :	mode--;	if(mode<0)	mode=4;\
			break;\
	}

#define TIME_CALCULATION_MACRO \
	snd_msg.buffer[3]=current_time/60%60%10; \
	snd_msg.buffer[2]=current_time/60%60/10; \
	snd_msg.buffer[1]=current_time/3600%24%10; \
	snd_msg.buffer[0]=current_time/3600%24/10; \
	msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
	
#define TEXT_EDITOR_MACRO(x)\
	if(cur_x==x)\
		cur_y=(cur_y+1)%3;\
	else{\
		cursor++;	cur_x=x;	cur_y=0;\
	}\
	snd_msg.buffer[1]=key_pad[cur_x][cur_y];\
	break;

#define ALARM_MACRO\
	snd_msg.buffer[3]=alarm_time%60%10;\
	snd_msg.buffer[2]=alarm_time%60/10;\
	snd_msg.buffer[1]=alarm_time/60%24%10;\
	snd_msg.buffer[0]=alarm_time/60%24/10;

struct msgbuf{
	long msgtype;
	unsigned int data;
};

struct msgbuf_output{
	long msgtype;
	unsigned char buffer[4];
	int data,mode;
};

int dev_fpga,dev_readkey,dev_led,dev_text,dev_dot,dev_fnd,dev_buzzer,rd;

void input_process(void);
void output_porcess(void);

void main_process(void);

void clock_mode0(void);
void counter_mode1(void);
void text_editor_mode2(void);
void draw_board_mode3(void);
void alarm_mode4(void);
