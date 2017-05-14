#include <linux/kernel.h>
#include <asm/uaccess.h>
#define SYS_PRJ2CALL_NUM 376
struct syscall_param{
	int interval;
	int count;
	char fnd_val[4];
};

asmlinkage long sys_prj2call(struct syscall_param*user_data,int*packed_data){
	struct syscall_param kernel_data;
	int packing=0;

	copy_from_user(&kernel_data,user_data,sizeof(kernel_data));

	printk("interval: %d \ncount: %d\nfnd_val: %d%d%d%d\n",kernel_data.interval,kernel_data.count,kernel_data.fnd_val[3],kernel_data.fnd_val[2],kernel_data.fnd_val[1],kernel_data.fnd_val[0]);

	packing=kernel_data.interval<<24;
	packing|=kernel_data.count<<16;

	printk("middle packed_data is %d\n",packing);
	
	if(kernel_data.fnd_val[3]){
		packing|=8;
		packing|=kernel_data.fnd_val[3]<<8;
	}
	else if(kernel_data.fnd_val[2]){
		packing|=4;
		packing|=kernel_data.fnd_val[2]<<8;
	}
	else if(kernel_data.fnd_val[1]){
		packing|=2;
		packing|=kernel_data.fnd_val[1]<<8;
	}
	else{
		packing|=1;
		packing|=kernel_data.fnd_val[0]<<8;
	}
	printk("final packed_data is %d\n",packing);
	copy_to_user(packed_data,&packing,sizeof(int));
	return SYS_PRJ2CALL_NUM;
}
