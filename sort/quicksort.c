#include<stdio.h>
#include<stdlib.h>
#include<time.h>

void display(int *a,int n)
{
	int i,j;

	for(i=0;i<10;i++){
	for(j=0;j<10;j++)
	{
		printf("%6d",a[i*10+j]);
	}
	printf("\n");
	}
}

void swap(int *a,int *b)
{
	int temp;

	temp = *a;
	*a = *b;
	*b = temp;
	printf("swap:%d,%d\n",*a,*b);
}

int patition(int *a,int low,int high)
{
	int i,j;
	int pivotkey;

	pivotkey =a[low];

	while(low<high){
	while((low<high)&&(a[high]>pivotkey)) high--;
	a[low]=a[high];
	while((low<high)&&(a[low]<pivotkey)) low++;
	a[high] = a[low];
	}

	a[low] = pivotkey;
	return low;

}

void my_quick_sort(int *a,int low,int high)
{
	int pivotloc;
	
	if(low<high)
	{
		pivotloc = patition(a,low,high);
		my_quick_sort(a,low,pivotloc-1);
		my_quick_sort(a,pivotloc+1,high);

	}
}
int main(int argc,char **argv)
{
	int i;
	int a[100];

	srand((unsigned int)time(0));

	for(i=0;i<100;i++)
	{
		a[i] = rand()%1000+1;
	}
	printf("the origin numbers:\n");
	display(a,100);
	printf("the sort result:\n");
	my_quick_sort(a,0,atoi(argv[1]));
    display(a,100);
	return 0;
}
