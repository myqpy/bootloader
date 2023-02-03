#include "stm32l1xx_it.h"
#include "main.h"
#include "global.h"
#include "systick.h"
#include "TTLM.h"
#include "beep.h"
#include "spi_flash.h"
#include "lcd.h"
#include "BSprotocol.h"
#include "bsp_ctr.h"

void NMI_Handler(void)
{}

void HardFault_Handler(void)
{
	printf("\r\nHardFault Error!\r\n");
	printf("\r\n");
	printf("\r\n");
	NVIC_SystemReset();
	while (1);
}

void MemManage_Handler(void)
{
  while (1)
  {
  }
}

void BusFault_Handler(void)
{
  while (1)
  {
  }
}

void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

void SVC_Handler(void)
{}

void DebugMon_Handler(void)
{}

void PendSV_Handler(void)
{}

void SysTick_Handler(void)
{}

	/*
MCU定时250ms唤醒一次
*/	
void RTC_WKUP_IRQHandler(void)
{
  if(RTC_GetITStatus(RTC_IT_WUT) != RESET)
  {
    RTC_ClearITPendingBit(RTC_IT_WUT);
    EXTI_ClearITPendingBit(EXTI_Line20);

		Tick_250ms++;
//		if(camera_shutdown)
//		{
//			camera_shutdown--;
//			if(camera_shutdown==0)
//				CAMERA_CTR_LOW;
//		}
#if 1
		if(LcdBLTimeout)//按键输入超时，关机
		{
			LcdBLTimeout--;
			if(LcdBLTimeout==0)
			{
				KEY_LP_flag=1;
				LCD_BL_LOW;
				lcdbheverlowflag=1;
				fNotAllowKey=0;
				CAMERA_CTR_LOW;
			}
		}
#endif
  }
}
u32 f_addr= 208*4096;
//串口中断服务函数，接受GPRS数据
void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE)==SET)		   	//判断，一旦接收到数据
	{
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);		   	//清除中断标志位

		rcv_usart1_str_flag=1;					               				//串口数据接收标志置1
		rcv_usart1_over_count=0;			                 				//每来一字节溢出计数标志清0

		Rx1Buf[Rx1Num]= USART_ReceiveData(USART1);
		Rx1Num++;     //记录串口1收到的字节数
		
//		Rx1Buf[0]=USART_ReceiveData(USART1);
//		SPI_FLASH_PageWrite(Rx1Buf, f_addr, 1);
//		f_addr++;
	}
	/*溢出-如果发生溢出需要先读SR,再读DR寄存器 则可清除不断入中断的问题*/
	if(USART_GetFlagStatus(USART1,USART_FLAG_ORE)==SET)
	{
		USART_ClearFlag(USART1,USART_FLAG_ORE);
		USART_ReceiveData(USART1);
	}
}

//串口中断服务函数，接受GPS数据
void USART2_IRQHandler(void)
{
	if(USART_GetITStatus(USART2, USART_IT_RXNE)==SET)		   //判断，一旦接收到数据
	{
		USART_ClearITPendingBit(USART2,USART_IT_RXNE);		   //清除中断标志位

		rcv_usart2_str_flag=1;					               //串口数据接收标志置1
		rcv_usart2_over_count=0;			                 //每来一字节，等待超时计数标志清0

		Rx2Buf[Rx2Num] = USART_ReceiveData(USART2);
		Rx2Num++;     //记录串口2收到的字节数
	}
	/*溢出-如果发生溢出需要先读SR,再读DR寄存器 则可清除不断入中断的问题*/
	if(USART_GetFlagStatus(USART2,USART_FLAG_ORE)==SET)
	{
		USART_ClearFlag(USART2,USART_FLAG_ORE);
		USART_ReceiveData(USART2);
	}
}

//串口中断服务函数，接受上位机数据
void USART3_IRQHandler(void)
{
	if(USART_GetITStatus(USART3, USART_IT_RXNE)==SET)		   //判断，一旦接收到数据
	{
		USART_ClearITPendingBit(USART3,USART_IT_RXNE);		   //清除中断标志位

//		rcv_usart3_str_flag=1;					               //串口数据接收标志置1
//		rcv_usart3_over_count=0;			                 //每来一字节，等待超时计数标志清0
		KEY_LP_flag=0;
		Key_Value = USART_ReceiveData(USART3);
	}
	/*溢出-如果发生溢出需要先读SR,再读DR寄存器 则可清除不断入中断的问题*/
	if(USART_GetFlagStatus(USART3,USART_FLAG_ORE)==SET)
	{
		USART_ClearFlag(USART3,USART_FLAG_ORE);
		USART_ReceiveData(USART3);
	}
}

	/*tim2*/
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		
		r_seed++;
		
		if(rcv_usart1_str_flag)					      //接收到了串口数据
		{
			if(rcv_usart1_over_count < 10)
				rcv_usart1_over_count++;			    //每一个数据接收完了最多等待10ms
			else
			{
				rcv_usart1_over_count = 0;
				rcv_usart1_str_flag=0;            //超过10ms，清标志位
				rcv_usart1_end_flag=1;				    //数据接收结束标志位置1
			}
		}
		if(rcv_usart2_str_flag)					      //接收到了串口数据
		{
			if(rcv_usart2_over_count < 10)
				rcv_usart2_over_count++;			    //每一个数据接收完了最多等待10ms
			else
			{
				rcv_usart2_str_flag=0;            //超过10ms，清标志位
				rcv_usart2_end_flag=1;				    //数据接收结束标志位置1
			}
		}
		if(rcv_usart3_str_flag)					      //接收到了串口数据
		{
			if(rcv_usart3_over_count < 10)
				rcv_usart3_over_count++;			    //每一个数据接收完了最多等待10ms
			else
			{
				rcv_usart3_str_flag=0;            //超过10ms，清标志位
				rcv_usart3_end_flag=1;				    //数据接收结束标志位置1
			}
		}
		//任务标志处理
    TaskReMarks();
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
