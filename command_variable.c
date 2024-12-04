#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>

struct st
{
	int *p;
};

int add_generic(struct st v,int n)
{
	int i=0,sum=0;
	for(i=0; i<n; i++)
	{
		sum = sum+v.p[i];
	}
	return sum;
}

void main(int argc, char**argv)
{
	int (*fp)(struct st,int);
	int c=0,i,j,k,l,m;
	for(i=2,c=0;i<argc;i++,c++);
	struct st v;
	v.p = malloc(sizeof(int)*c);
	for(i=2,j=0; i<argc; i++)
	{
		v.p[j++] = atoi(argv[i]);
	}
	if(strcmp(argv[1],"add") == 0)
	{
		fp = add_generic;
			int res = fp(v,c);
		printf("RES = %d\n",res);
	}
}

