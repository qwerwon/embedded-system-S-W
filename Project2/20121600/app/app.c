#include<stdio.h>
#include<stdlib.h>

#include<unistd.h>
#include<syscall.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<linux/ioctl.h>

#define PRJ2_SYSCALL_NUM 376
struct syscall_param{
	int interval;
	int count;
	char fnd_val[4];
};

int main(int argc,char*argv[]){
	struct syscall_param syscall_argument;
	int packed_data=0,dev_fd;
	if(argc!=4){
		printf("argument input error\n");
		return -1;
	}
	syscall_argument.interval=atoi(argv[1]);
	syscall_argument.count=atoi(argv[2]);

	if(syscall_argument.interval>100|syscall_argument.count>100){
		printf("Out of range!(./app [1-100] [1-100] [8000-0001]\n");
		return -1;
	}

	syscall_argument.fnd_val[0]=argv[3][3]-'0';
	syscall_argument.fnd_val[1]=argv[3][2]-'0';
	syscall_argument.fnd_val[2]=argv[3][1]-'0';
	syscall_argument.fnd_val[3]=argv[3][0]-'0';
	printf("interval in user : %d\ncount in user : %d\n",syscall_argument.interval,syscall_argument.count);

	printf("fnd_val = %d%d%d%d\n",syscall_argument.fnd_val[3],syscall_argument.fnd_val[2],syscall_argument.fnd_val[1],syscall_argument.fnd_val[0]);

	syscall(PRJ2_SYSCALL_NUM,&syscall_argument,&packed_data);
	printf("packed_data is %X\n",packed_data);


	dev_fd=open("/dev/dev_driver",O_WRONLY);
	if(dev_fd<0){
		printf("device file open error\n");
		return -1;
	}
	printf("ioctl = %d\n",ioctl(dev_fd,0,packed_data));
	close(dev_fd);
	return 0;
}
