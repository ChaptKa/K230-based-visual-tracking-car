#include "Motor.h"
#include "PWM.h"
#include "tim.h"



void car_go(void)
{
	LF_Motor(35);
	LB_Motor(35);
	RF_Motor(35);	
	RB_Motor(35);
	
}	


void car_go_2 (void)
{
	car_go();
}
void car_turnleft_s_2(void)
{
	LF_Motor(25);
	LB_Motor(25);
	RF_Motor(35);	
	RB_Motor(35);
}
void car_turnright_s_2(void)
{
	LF_Motor(35);
	LB_Motor(35);
	RF_Motor(25);	
	RB_Motor(25);
}

void car_back(void)
{
	LF_Motor(-50);
	LB_Motor(-50);
	RF_Motor(-50);	
	RB_Motor(-50);
}

void car_stop(void)
{
	LF_Motor(0);
	LB_Motor(0);
	RF_Motor(0);	
	RB_Motor(0);
}

void car_turnleft_s(void)
{
	LF_Motor(25);
	LB_Motor(25);
	RF_Motor(35);	
	RB_Motor(35);

}

void car_turnright_s(void)
{
	LF_Motor(35);
	LB_Motor(35);
	RF_Motor(25);	
	RB_Motor(25);

}
void car_turnleft_b(void)
{
	LF_Motor(-25);
	LB_Motor(-30);
	RF_Motor(25);	
	RB_Motor(20);

}
void car_turnright_b(void)
{
	LF_Motor(20);
	LB_Motor(25);
	RF_Motor(-30);	
	RB_Motor(-25);

}

void car_turnright_save(void)
{
	LF_Motor(15);
	LB_Motor(15);
	RF_Motor(-15);	
	RB_Motor(-15);
}

void car_turnleft_save(void)
{
	LF_Motor(-15);
	LB_Motor(-15);
	RF_Motor(15);	
	RB_Motor(15);
}
