#define N 11
// #define N 11
typedef struct POINT
{
    int x, y;
    char z[N];
    struct POINT *next;
} DOT;
void f(DOT p)
{
    p.x = 100;
    p.y = sizeof(p);
    p.z[1] = 'A';
    f(*(p.next));
} //�ڶ��� C ����