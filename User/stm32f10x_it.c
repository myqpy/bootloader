/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "stm32f10x.h"
#include <stdio.h>

void NMI_Handler(void)
{
	
}

void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
	printf("Hard Fault!\r\n");
	NVIC_SystemReset();
  while (1)
  {
  }
}

void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
}

// 串口中断服务函数
void USART1_IRQHandler(void)
{	

}

// 串口中断服务函数
void USART2_IRQHandler(void)
{

}

//串口中断服务函数，接受上位机数据
void USART3_IRQHandler(void)
{

}

void TIM2_IRQHandler(void)
{

}


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
