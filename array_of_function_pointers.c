#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
int add(int a, int b)//add function definition
{
	return a+b;
}
int sub(int a, int b)//sub function definition
{
	return a-b;
}
int mul(int a, int b)//mul functuion definition
{
	return a*b;
}
int divi(int a, int b)//div function definition
{
	return a/b;
}
enum {ADD,SUB,MUL,DIV,MAX};//decleration of enum
int main(int argc,char**argv)
{
	int (*p[MAX])(int,int) = {add,sub,mul,divi};//creating the array of unction poiners with sizeof max in ENUM
	int i,j,n,res,index;
	i = atoi(argv[2]);//converting the string into number
	j = atoi(argv[3]);//convering the string into number

	if(strcmp(argv[1],"add") == 0)//comparing the string and deciding the index
	{
		index = ADD;//and assigning the enum value to the index variable according to user.
	}
	else if(strcmp(argv[1],"sub") == 0)
	{
		index = SUB;
	}
	else if(strcmp(argv[1],"mul") == 0)
	{
		index = MUL;
	}
	else if(strcmp(argv[1],"div") == 0)
	{
		index = DIV;
	}
	res = (*p[index])(i,j);//calling the required funtion by passing the index which we stored in index variable 
	printf("RES = %d\n",res);//printing the result.
}
