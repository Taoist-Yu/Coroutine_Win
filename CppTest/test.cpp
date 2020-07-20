#include<stdio.h>

int main()
{
	int i = 3;
	int m = (++i)+(++i)+(++i);
	int j = 3;
	int n = (++j)+(++j);


	printf("%d %d",m,n);
	return 0;
}