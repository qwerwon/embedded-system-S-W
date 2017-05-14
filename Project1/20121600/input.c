#include"header.h"

void input_process(){
	struct input_event ev[BUFF_SIZE];
	struct msgbuf snd_msg;
	key_t key_id=msgget((key_t)INPUT_TO_MAIN,IPC_CREAT|0666);
	unsigned char push_sw_buff[MAX_BUTTON];
	int msgbuf_size=sizeof(struct msgbuf)-sizeof(long),i;
	int buff_size=sizeof(push_sw_buff);
	int size = sizeof(struct input_event),temp;

	while(1){	
		usleep(100000);
		rd=read(dev_readkey,ev,size * BUFF_SIZE);
		if((rd < sizeof(struct input_event)))
			exit(0);	
		if(rd>=READKEY&&ev[0].value==KEY_PRESS){//When Readkey input occurs && key pressed
			snd_msg.msgtype=READKEY;
			snd_msg.data=ev[0].code;
			temp=msgsnd(key_id,&snd_msg,msgbuf_size,IPC_NOWAIT);
			if(snd_msg.data==BACK_KEY)//Exit key
				break;	
			continue;
		}
		read(dev_fpga,&push_sw_buff,buff_size);
		i=MAX_BUTTON;
		do{				//Loop unrolled by 3
			if(push_sw_buff[i-1]){
				snd_msg.msgtype=FPGA_SWITCH;
				if(push_sw_buff[i-2])	//when sw2&3 or sw5&6 or sw8&9 pressed simultaneously
					snd_msg.data=(1<<i-1)+(1<<i-2);
				else
					snd_msg.data=1<<i-1;
				temp=msgsnd(key_id,&snd_msg,msgbuf_size,IPC_NOWAIT);
			}
			else if(push_sw_buff[i-2]){
				snd_msg.msgtype=FPGA_SWITCH;
				if(push_sw_buff[i-1])//when pressed simultaneously
					snd_msg.data=(1<<i-1)+(1<<i-2);
				else
					snd_msg.data=1<<i-2;
				temp=msgsnd(key_id,&snd_msg,msgbuf_size,IPC_NOWAIT);
			}
			else if(push_sw_buff[i-3]){
				snd_msg.msgtype=FPGA_SWITCH;
				snd_msg.data=1<<i-3;
				temp=msgsnd(key_id,&snd_msg,msgbuf_size,IPC_NOWAIT);
			}
		}while(i-=3);
	}
	printf("-----------------Program Exit---------------\n");
}
