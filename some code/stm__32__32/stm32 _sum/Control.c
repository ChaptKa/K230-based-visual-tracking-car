#include "stm32f10x.h"                  // Device header
#include "Motor1.h"
#include  "Drive.h"

void Run()
{	
	PWM_SetCompare3(900);
	PWM_SetCompare4(900);
	Left_run();
	Right_run();	
}
void Back()
{
	PWM_SetCompare3(500);
	PWM_SetCompare4(500);
	Left_back();
	Right_back();	
}
void Turn_left()
{
	PWM_SetCompare3(400);
	PWM_SetCompare4(400);
	Left_back();
	Right_run();
	
}
void Turn_right()
{
	PWM_SetCompare3(400);
	PWM_SetCompare4(400);
	Right_back();
	Left_run();
}	
void Step()
{
 Left_step();
 Right_step();
}
