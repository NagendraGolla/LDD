#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
int assending(int a, int b)
{
return a-b;
}

int desending(int a, int b)
{
return b-a;
}
void sort(int*,int,int(*p)(int,int));
void main()
{
	int i,j,k,l,m,n;
	printf("Enter th no.of elements\n");
	scanf("%d",&n);
	int a[n];
	for(i=0; i<n; i++)
	{
		scanf("%d",&a[i]);
	}
	for(i=0; i<n; i++)
	{
		printf("%d ",a[i]);
	}
	printf("\n");
	sort(a,n,assending);
	for(i=0; i<n; i++)
	{
		printf("%d ",a[i]);
	}
	printf("\n");
	sort(a,n,desending);
	for(i=0; i<n; i++)
	{
		printf("%d ",a[i]);
	}
	printf("\n");
	
}

void sort(int *a,int ele,int (*p)(int,int))
{
	int i,j,t;
	for(i=0; i<ele; i++)
	{
		for(j=i+1; j<ele; j++)
		{
			if(p(a[i],a[j])>0)
			{
				t = a[i];
				a[i] = a[j];
				a[j] =t;
			}
		}
	}
}

