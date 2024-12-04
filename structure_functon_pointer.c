#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>

int add(int x, int y)
{
	return x+y;
}

int sub(int x, int y)
{
	return x-y;
}
int mul(int x, int y)
{
	return x*y;
}
int divi(int x, int y)
{
	return x/y;
}
struct st
{
	int (*fp)(int,int);
};

void main()
{
	int i,j,k,l,m,n;
	printf("Enter the optin\n");
	printf("1)sum 2)sub 3)mul 4)div\n");
	scanf("%d",&n);
	printf("Enter the values\n");
	scanf("%d%d",&i,&j);
	struct st v;
	int res;
	switch(n)
	{
		case 1:v.fp = add;
		       res = v.fp(i,j);
		       printf("RES = %d\n",res);
		       break;
		case 2:v.fp = sub;
		       res = v.fp(i,j);
		       printf("RES = %d\n",res);
		       break;
		case 3:v.fp = mul;
		       res = v.fp(i,j);
		       printf("RES = %d\n",res);
		       break;
		case 4:v.fp = divi;
		       res = v.fp(i,j);
		       printf("RES = %d\n",res);
		       break;
	}
}
