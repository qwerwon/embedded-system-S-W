#include <linux/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int main(void){
	int ret;
	unsigned long n=0;

	ret=open("/dev/stopwatch", O_RDWR);
	if(ret<0){
		printf("open device file fail\n");
		return -1;
	}
	write(ret,&n,sizeof(unsigned long));
	close(ret);
	return 0;
}
