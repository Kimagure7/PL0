#include<stdio.h>
int main()
{
    int i;
    int* q;
    int* a[10];
    int* (*b[10])[10];
    int* (*(*p)[10])[10];
    i = 100; q = &i; a[1] = q; b[1] = &a; p = &b;
    printf("%d\n",*a[1]);
    printf("%d\n",b[1][0][1][0]);
    printf("%d\n",*((*b[1])+1)[0]);
    printf("%d\n",(*p)[1][0][1][0]);
    printf("%d\n",p[0][1][0][1][0]);
}