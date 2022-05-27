#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<string.h>

#define MEM_MAGIC 'U' //定义一个幻数，而长度正好和ASC码长度一样为8位，所以这里定义个字符
#define USER_ADD _IO(MEM_MAGIC,0)
#define USER_DEL _IO(MEM_MAGIC,1) 
#define USER_MOD _IO(MEM_MAGIC,2)
#define USER_INQ _IO(MEM_MAGIC,3)

struct userdata {
	int index;
	char name[10][20];
};

struct userdata data;

int main()
{
	int fd;
	char buf[1000];
	memset(buf,0,1000);

	if(strlen(buf))
		printf("buf not null\n");

	printf("enter\n");
	fd = open("/dev/chardev0", O_RDWR);//可读可写打开文件
	printf("fd open ok\n");
	//读写测试
	write(fd, "Hello World\n", 12);
	read(fd, buf, 11);
	printf("read buf [%s]\n", buf);
	printf("WR test ok\n");
	//添加
	data.index=0;
	strcpy(data.name[data.index],"zhangsan");
	ioctl(fd, USER_ADD, &data);
	data.index=1;
	strcpy(data.name[data.index],"lisi");
	ioctl(fd, USER_ADD, &data);
	data.index=2;
	strcpy(data.name[data.index],"wangwu");
	ioctl(fd, USER_ADD, &data);
	data.index=2;
	strcpy(data.name[data.index],"wangwu");
	ioctl(fd, USER_ADD, &data);
	printf("useradd ok\n");
	//查询打印
	ioctl(fd, USER_INQ, &data);
	printf("用户查询：\n");
	for(int i=0;i<10;i++)
		if(strlen(data.name[i]))
			printf("name[%d]=%s\n", i, data.name[i]);
	//删除
	data.index=3;
	ioctl(fd, USER_DEL, &data);
	data.index=2;
	ioctl(fd, USER_DEL, &data);
	printf("userdel ok\n");
	//修改
	data.index=1;
	strcpy(data.name[data.index],"lisis");
	ioctl(fd, USER_MOD, &data);
	printf("usermod ok\n");
	//查询打印
	ioctl(fd, USER_INQ, &data);
	printf("用户查询：\n");
	for(int i=0;i<10;i++)
	if(strlen(data.name[i]))
		printf("name[%d]=%s\n", i, data.name[i]);
	close(fd);
}
