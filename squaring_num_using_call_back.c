#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
int squrt_num(int);//function decleration
int call_back(int,int(*p)(int));//call back function
void main()
{
	int i, j, k, n;
	printf("Enter the no.of elements\n");
	scanf("%d",&n);//scaning the no.of inputs from the user.
	int a[n];//declering the array according to user requirement.
	for(i=0; i<n;i++)
	{
		scanf("%d",&a[i]);
	}
	for(i=0; i<n; i++)
	{
		printf("%d ",a[i]);
	}
	printf("\n");
	for(i=0; i<n; i++)
	{
		a[i] = call_back(a[i],squrt_num);//calling the call back function  
	}
	for(i=0; i<n; i++)
	{
		printf("%d ",a[i]);//printing the array after calling the call back function
	}
	printf("\n");
}

int squrt_num(int a)
{
	return a*a;//squaring the number
}

int call_back(int a,int(*p)(int))
{
return p(a);
}
