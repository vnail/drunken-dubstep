#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>

int main()
{
	int fd;
	int i,ret;

	fd_set rds;
	
	//以阻塞的方式打开设备文件
	
	if((ret=open("/dev/s3c2440_buttons",O_RDONLY))<))
	{
		printf("Open Buttons Device Failed\n");
		return -1;
	}

	//多路复用的方式读取键盘状态
	while(1)
	{
		FD_ZERO(&rds);
		FD_SET(fd,&rds);

		ret = select(fd+1,&rds,NULL,NULL,NULL);

		if(ret < 0)
		{
			printf("Read button device Failed\n ");
			return -1;
		}

		if(ret == 0)
		{
			printf("time out\n");
			return -1;
		}

		if(FD_ISSET(fd,&rds))
		{
			ret = read(fd,key_status,sizeof(key_status));

			for(i=0;i<4;i++)
			{
				if(key_status[i] == 0)
				{
					printf("Key %d DOWN\n",i+1);
				}
			}

		}

	}

	close(fd);
	return 0;
}























