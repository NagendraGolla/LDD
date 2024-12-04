#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>

void febi(int,int);
void factorial(int,int);
void power(int,int);

void main(int argc,char**argv)
{
	int i,j;
	char s[55];
	void (*p[3])(int,int)={febi,factorial,power};
	printf("Enter the option\n");
	printf("fact  febi  pow\n");
	scanf("%s",s);
	if(strcmp(s,"fact") == 0)
	{
		printf("Enter the values\n");
		scanf("%d",&i);
		(*p[1])(i,1);
	}
	if(strcmp(s,"febi") == 0)
	{
		printf("Enter the values\n");
		scanf("%d%d",&i,&j);
		(*p[0])(i,j);
	}
	if(strcmp(s,"pow") == 0)
	{
		printf("Enter the values\n");
		scanf("%d%d",&i,&j);
		(*p[2])(i,j);
	}
}

void febi(int a, int b)
{
	static int i,j,k,t,t1,c,c1,t2;
	if(c<1)
	{
		printf("%d %d ", a, a+1);
		c++;
	}
	if(c1<1)
	{
		t = a;
	  	t1 = a+1;
		c1++;
	}
	t2 = t+t1;
	t = t1;
	t1 = t2;
	printf("%d ",t2);
	if(t2<b)
	{
		return febi(t1,t2);
	}
}

void factorial(int a,int fact)
{
	fact = fact*a;
		a--;
		if(a>=1)
		{
			return factorial(a,fact);
		}
		printf("FACT = %d\n",fact);
}

void power(int a, int b)
{
	static int po=1, i;
	if(i<b)
	{
		po = po*a;
		i++;
		return power(a,b);
	}
	printf("%d\n",po);
}
