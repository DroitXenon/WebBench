#include "stdio.h"
#include "string.h"
main()
{
	char* p ="abcdefg";
	int len = strlen(p);
	char* t = &p[len-1];
	printf("%c\n",*t);
}
