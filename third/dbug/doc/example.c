

/**
    编译命令 gcc example.c -I../inc ../PbDebug.c -o dbug
    若不想启用调试，可以将宏DBUG_OFF放开
*/

#define DBUG_ON

#include "PbDebug.h"

void  Hanoi(int n, char *pDishA, char *pDishB, char *pDishC);

int main (void)
{
	int Cnt = 7;

	DBUG_PUSH ("d:t:O");
	DBUG_PROCESS ("example");

	DBUG_ENTER("Main Function");

    Hanoi(Cnt, "Dish_A", "Dish_B", "Dish_C");

	DBUG_RETURN(0);
}

void  Hanoi(int n, char *pDishA, char *pDishB, char *pDishC)
{
	DBUG_ENTER(__func__);

    if(n==1)
	{
        DBUG_PRINT("\033[31mMOVE DISH\033[0m", ("%s->%s", pDishA, pDishC));
	}
    else
    {
        Hanoi(n-1, pDishA, pDishC, pDishB);
        DBUG_PRINT("\033[31mMOVE DISH\033[0m", ("%s->%s", pDishA, pDishC));
        Hanoi(n-1, pDishB, pDishA, pDishC);
    }
	DBUG_VOID_RETURN;
}
