#include "usart.h"
//#include "stm32f10x_flash.h"
//#include "iwdg.h"
#include "adc.h"
#include "sys.h"
#include "delay.h"
//#include "bsp_internal_flash.h"   

typedef  void (*pFunction)(void);
pFunction Jump_To_Application;
uint32_t JumpAddress;
uint32_t EraseCounter = 0x0;
uint32_t NbrOfPage = 0;
FLASH_Status FLASHStatus = FLASH_COMPLETE;
uint32_t FileSize=0;

#if defined (STM32F10X_MD) || defined (STM32F10X_MD_VL)
 #define PAGE_SIZE                         (0x400)    /* 1 Kbyte */
 #define FLASH_SIZE                        (0x10000)  /* 64 KBytes */
#elif defined STM32F10X_CL
 #define PAGE_SIZE                         (0x800)    /* 2 Kbytes */
 #define FLASH_SIZE                        (0x40000)  /* 256 KBytes */
#elif defined STM32F10X_HD || defined (STM32F10X_HD_VL)
 #define PAGE_SIZE                         (0x800)    /* 2 Kbytes */
 #define FLASH_SIZE                        (0x80000)  /* 512 KBytes */
#elif defined STM32F10X_XL
 #define PAGE_SIZE                         (0x800)    /* 2 Kbytes */
 #define FLASH_SIZE                        (0x100000) /* 1 MByte */
#else 
 #error "Please select first the STM32 device to be used (in stm32f10x.h)"    
#endif


//#define PAGE_SIZE                         (0x800)    /* 2 Kbytes */
//#define FLASH_SIZE                        (0x80000)  /* 512 KBytes */

/*
bootload   app1       app2       data      total
 0x5000   0x19000    0x19000    0x9000     40000
   20k      100k      100k       36k        256k
*/
#define BL_SIZE 								0x4000
#define APP_SIZE 								0x10000
#define Application1Address    	(0x08000000+BL_SIZE)
//#define Application2Address    	(Application1Address+APP_SIZE)
#define Application2Address    	(0x08000000+0x10000+0x4000)
#define Application3Address    	(Application2Address+APP_SIZE)
#define DataStorageAddress			(Application3Address+APP_SIZE)

FLASH_Status FLASH_ProgramOptionByteData(uint32_t Address, uint8_t Data);

uint32_t FLASH_PagesMask(__IO uint32_t Size)
{
  uint32_t pagenumber = 0x0;
  uint32_t size = Size;

  if ((size % PAGE_SIZE) != 0)
  {
    pagenumber = (size / PAGE_SIZE) + 1;
  }
  else
  {
    pagenumber = size / PAGE_SIZE;
  }
  return pagenumber;
}

u8 Copy_APP2_TO_APP1(void)
{
	uint32_t i=0,NumOfWord=0;
	uint32_t ReadAddress = Application3Address;
	uint32_t WriteAddress = Application2Address;
	
//	printf("ReadAddress : 0x%08X    \r\n",ReadAddress);
//	printf("WriteAddress : 0x%08X    \r\n",WriteAddress);
	
	
	__set_PRIMASK(1); 
	FLASH_Unlock();

//	FileSize = *(__IO uint32_t*)FileSizeAddress;

	//NbrOfPage = FLASH_PagesMask(0x19000);
	FileSize = 0x10000;
	NbrOfPage = 50;
	
	
	for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
	{
		FLASHStatus = FLASH_ErasePage(Application2Address + (PAGE_SIZE * EraseCounter));
	}
	
	if(FLASHStatus != FLASH_COMPLETE)
	{
		printf("FLASH_ErasePage error\r\n");
		return 0;
	}
	printf("FLASH_ErasePage OK\r\n");
	
	if(FileSize%4)
	{
		NumOfWord= FileSize/4+1;
	}
	else
	{
		NumOfWord= FileSize/4;
	}

	for(i = 0; i < NumOfWord; i++)
	{
		FLASH_ProgramWord(WriteAddress, *(uint32_t*)ReadAddress);
		if (*(uint32_t*)WriteAddress != *(uint32_t*)ReadAddress)
		{
			printf("Program Word ERROR %d\r\n",i);
			return 0;
		}
		WriteAddress += 4;
		ReadAddress += 4;
	}
	printf("ProgramWord OK\r\n");
	
	FLASH_Lock();
  __set_PRIMASK(0);
	return 1;
}

void Delay(__IO uint32_t nCount)	 //简单的延时函数
{
	for(; nCount != 0; nCount--);
}

/*******************************************************************************
* Function Name  : main
* Description    : ??????
* Input          : None.
* Return         : None.
*******************************************************************************/
int main(void)
{
//	u8 retry=5;
	u8 VoltageAD = 0;
	
	uart_init(115200);


	printf("DEBUG-->---------------------------\r\n");
	printf("DEBUG-->                           \r\n");
	printf("DEBUG-->       BOOTLOADER run!     \r\n");
	printf("DEBUG-->                           \r\n");
	printf("DEBUG-->---------------------------\r\n");	


	Adc_Init();
	VoltageAD = (float) (Get_Adc_Average(ADC_Channel_6,10) * 3.3 /4096) ;
	
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);
	Delay(10000000);
	GPIO_SetBits(GPIOB, GPIO_Pin_12);

	if (VoltageAD>1.7) 
	{
		GPIO_ResetBits(GPIOF, GPIO_Pin_7);
		printf("\r\n 正常模式 \r\n");
//		printf("0x%08x \r\n",((*(__IO uint32_t*)Application1Address) & 0x2FFE0000 ));
		if (((*(__IO uint32_t*)Application1Address) & 0x2FFE0000 ) == 0x20000000)
		{
			printf("DEBUG-->  Jump to APP1!  \r\n");
			/* Jump to user application */
			JumpAddress = *(__IO uint32_t*) (Application1Address + 4);
			Jump_To_Application = (pFunction) JumpAddress;
			/* Initialize user application's Stack Pointer */
			__set_MSP(*(__IO uint32_t*) Application1Address);
			Jump_To_Application();
		}
	}
	
	else
	{
		GPIO_SetBits(GPIOF, GPIO_Pin_7);
		printf("\r\n 低功耗模式 \r\n");
//		printf("0x%08x \r\n",((*(__IO uint32_t*)Application2Address)));
		if (((*(__IO uint32_t*)Application2Address) & 0x2FFE0000 ) == 0x20000000)
		{
			printf("DEBUG-->  Jump to APP2!  	\r\n");
			/* Jump to user application */
			JumpAddress = *(__IO uint32_t*) (Application2Address + 4);
			Jump_To_Application = (pFunction) JumpAddress;
			/* Initialize user application's Stack Pointer */
			__set_MSP(*(__IO uint32_t*) Application2Address);
			Jump_To_Application();
		}
	}

//	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET) //看门狗复位
//	{
//		RCC_ClearFlag();
//		GPIO_SetBits(GPIOF, GPIO_Pin_7);
//		Delay(30000000);
//		GPIO_ResetBits(GPIOF, GPIO_Pin_7);
//		printf("\r\n看门狗复位\r\n");
//		if (((*(__IO uint32_t*)Application1Address) & 0x2FFE0000 ) == 0x20000000)
//		{
//			printf("DEBUG-->  Jump to APP1!  \r\n");
//			/* Jump to user application */
//			JumpAddress = *(__IO uint32_t*) (Application1Address + 4);
//			Jump_To_Application = (pFunction) JumpAddress;
//			/* Initialize user application's Stack Pointer */
//			__set_MSP(*(__IO uint32_t*) Application1Address);
//			Jump_To_Application();
//		}
//	}
//	
//	else
//	{
//		GPIO_ResetBits(GPIOF, GPIO_Pin_7);
//		printf("\r\n 正常模式 \r\n");
////		printf("0x%08x \r\n",((*(__IO uint32_t*)Application1Address) & 0x2FFE0000 ));
//		if (((*(__IO uint32_t*)Application1Address) & 0x2FFE0000 ) == 0x20000000)
//		{
//			printf("DEBUG-->  Jump to APP1!  \r\n");
//			/* Jump to user application */
//			JumpAddress = *(__IO uint32_t*) (Application1Address + 4);
//			Jump_To_Application = (pFunction) JumpAddress;
//			/* Initialize user application's Stack Pointer */
//			__set_MSP(*(__IO uint32_t*) Application1Address);
//			Jump_To_Application();
//		}
//	}

	while(1)
	{
	
	}
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/




