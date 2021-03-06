#include <stc15f2k60s2.h>
#include "iic.h"
#define u8 unsigned char
#define u16 unsigned int
	
u8 code du[]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
u8 code wei[]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};

void All_Init(void)
{
	//LED
    P2 &= 0X1F;   //关573
	P0 =  0XFF;   //预送数据
	P2 |= 0X80;   //开相应573
	P0 =  0XFF;   //送数据
	P2 &= 0X1F;   //关573
	
	//蜂鸣器
    P2 &= 0X1F;   //关573
	P0 =  0X00;   //预送数据
	P2 |= 0XA0;   //开相应573
	P0 =  0X00;   //送数据
	P2 &= 0X1F;   //关573
	
	//数码管位选
	P2 &= 0X1F;   //关573
	P0 =  0X00;   //预送数据
	P2 |= 0XC0;   //开相应573
	P0 =  0X00;   //送数据
	P2 &= 0X1F;   //关573
	
	//数码管段选
	P2 &= 0X1F;   //关573
	P0 =  0XFF;   //预送数据
	P2 |= 0XE0;   //开相应573
	P0 =  0XFF;   //送数据
	P2 &= 0X1F;   //关573
}

void Timer0Init(void)		//1毫秒@11.0592MHz
{
	AUXR |= 0x80;		//定时器时钟1T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0xCD;		//设置定时初值
	TH0 = 0xD4;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0=1;
	EA=1;
}

//---------------------------四个独立按键--------------------------------
#define btn_io P3
#define btn_state_0 0	  //是否按下
#define btn_state_1 1 	 //是否是抖动
#define btn_state_2 2	  //判定按键有效的种类
#define btn_state_3 3	  //等待按键释放
#define btn_mask 0x0f	  //屏蔽不需要的IO
#define LONG_KEY_TIME 100 //LONG_KEY_TIME*10MS = 3S
#define SINGLE_KEY_TIME 3 //SINGLE_KEY_TIME*10MS = 30MS
char btnkey;
static char btn_state = 0;
char scanbtn(void)
{
	static int key_time; //按键计时变量	
	static char key_value=0;
	char btn_press,btn_return = 0;
	btn_press = btn_io & btn_mask;
	switch(btn_state)
	{
		case btn_state_0:
			key_value=0;
			if(btn_press != btn_mask){
				key_time = 0; // 清零时间间隔计数
				btn_state = btn_state_1;
			}
			break;
		case btn_state_1:
			if(btn_press != btn_mask)
			{
				key_time++; // 一次10ms
				if (key_time >= SINGLE_KEY_TIME) // 消抖时间为：SINGLE_KEY_TIME*10ms = 30ms;
				{
					btn_state = btn_state_2;
				}
			}
			else
				btn_state = btn_state_0;
			break;
		case btn_state_2:
			if(btn_press == btn_mask)
			{
				btn_return=key_value;
				key_value=0;
				btn_state = btn_state_0; // 去状态3，等待按键释放
			}
			else
			{
				if(btn_press == 0x0e) key_value = 1;
				if(btn_press == 0x0d) key_value = 2;
				if(btn_press == 0x0b) key_value = 3;
				if(btn_press == 0x07) key_value = 4;

				key_time++;

				if (key_time >= LONG_KEY_TIME) // 如果按键时间超过 设定的长按时间（LONG_KEY_TIME*10ms=200*10ms=2000ms）, 则判定为 长按
				{
					btn_return =key_value|0x80;
					key_value=0;
					btn_state = btn_state_3; // 去状态3，等待按键释放
				}
			}
			break;
		case btn_state_3:
			if(btn_press == 0x0f)	btn_state = btn_state_0;
			break;
		default:
		break;
	}
	return 	btn_return;
}

u8 on_display[8]={0x00,0x3F|0x80,0x6D,0x3F,0x3F,0x06|0x80,0x3F,0x3F};
u8 off_display[8]={0x00,0x3F|0x80,0x6D,0x3F,0x3F,0x3F|0x80,0x3F,0x3F};
bit water_on;	//是否出水 1出水 0不出水
u16 water_v=0;	//出水量
u8 price;	//价格
sbit relay=P0^4;
sbit buzzer=P0^6;
u8 adc_val;
void main()
{
	All_Init();
	Timer0Init();
	while(1)
	{
		on_display[4]=du[water_v/1000];
		on_display[5]=du[water_v/100%10]|0x80;
		on_display[6]=du[water_v/10%10];
		on_display[7]=du[water_v%10];
		
		EA=0;
		if(adc_val < 64)
		{
			P2=(P2&0x1f)|0x80;P0=~0x01;P2&=0x1f;
		}
		else
		{
			P2=(P2&0x1f)|0x80;P0=~0x00;P2&=0x1f;
		}
		EA=1;
		if(water_v==10000)
		{
			water_on=0;
			relay=0;buzzer=0;P2=(P2&0x1f)|0xa0;relay=0;buzzer=0;P2&=0x1f;
			price=water_v/2;
			off_display[4]=du[5];
			off_display[5]=du[0]|0x80;
			off_display[6]=du[0];
			off_display[7]=du[0];
		}
		switch(btnkey)
		{
			case 1:				//S7
				if(water_on==0)
				{
					water_v=0;
				}
				water_on=1;
				P2=(P2&0x1f)|0xa0;relay=1;buzzer=0;P2&=0x1f;
				btnkey=0;
				break;
			case 2:				//S6
				water_on=0;
				P2=(P2&0x1f)|0xa0;relay=0;buzzer=0;P2&=0x1f;
				price=water_v/2;
				off_display[4]=du[price/1000];
				off_display[5]=du[price/100%10]|0x80;
				off_display[6]=du[price/10%10];
				off_display[7]=du[price%10];	
				btnkey=0;
				break;
			case 3:				//S5
				btnkey=0;
				break;
			case 4:				//S4
				btnkey=0;
				break;
			default:
			break;
		}
	}
}

void time0() interrupt 1
{
	static int smg_count=0,key_count=0,i=0,onesec_count=0,adc_count=0;
	smg_count++; key_count++;onesec_count++;adc_count++;
	if(key_count==8)
	{
		btnkey=scanbtn();
		key_count=0;
	}
	if(adc_count==200)
	{
		adc_val=read_adc(0x01);
		adc_count=0;
	}
	
	if(smg_count==1)
	{
		smg_count=0;
		P2=(P2&0x1f)|0xc0;P0=0x00;P2&=0x1f;
		if(water_on)
		{
			P2=(P2&0x1f)|0xe0;P0=~on_display[i];P2&=0x1f;
		}
		else
		{
			P2=(P2&0x1f)|0xe0;P0=~off_display[i];P2&=0x1f;
		}
		
		P2=(P2&0x1f)|0xc0;P0=wei[i];P2&=0x1f;
		i++;
		if(i==8) i=0;
	}
	
	if(onesec_count==10)
	{
		onesec_count=0;
		if(water_on)
		{
			water_v+=10;
		}
	}
}