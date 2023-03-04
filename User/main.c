#include "main.h"
#include "usart.h"
#include "stm32f10x_flash.h"
#include "iwdg.h"
#include "bsp_internal_flash.h"   

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
#define BL_SIZE 							0x4000
#define APP_SIZE 							0x19000
#define ApplicationAddress    (0x08000000+BL_SIZE)
//#define UpdateFlagAddress		(ApplicationAddress-4)//Éý¼¶±êÖ¾Î»´æ´¢µØÖ·£¬BootLoaderÇøÓò×îºóÒ»¸öword
#define UpdateFlagAddress 		0x08038000
//#define FileSizeAddress				(ApplicationAddress-8)//ÎÄ¼þ´óÐ¡ µØÖ·ÎªbootloaderµÄµ¹ÊýµÚ¶þ¸öword

FLASH_Status FLASH_ProgramOptionByteData(uint32_t Address, uint8_t Data);


struct TerminalParameters
{
	// DWORD, ????????(s).
	unsigned int HeartBeatInterval;
	
	//STRING, ??????,IP ???
	unsigned char MainServerAddress[50];

	//DWORD, ??? TCP ??		
	unsigned int ServerPort;

	// DWORD, ????????
	unsigned int DefaultTimeReportTimeInterval;

	// DWORD, ??????, < 180°.
	unsigned int CornerPointRetransmissionAngle;

	// DWORD, ????, km/h.
	unsigned int MaxSpeed;

	// WORD, ??????? ID
  unsigned short ProvinceID;
	
	// WORD, ??????? ID
	unsigned short CityID;

	//STRING, ????????????????
	unsigned char CarPlateNum[12];
	
	//????,?? JT/T415-2006 ? 5.4.12
	unsigned char CarPlateColor;
	
	//	DWORD, ???????????????
	unsigned int initFactoryParameters;
	
	//	DWORD, ??????????
	unsigned long bootLoaderFlag;
	
	//STRING, ???
	unsigned char version[5];
	
	
	unsigned char PhoneNumber[20];
	unsigned char TerminalId[20];
};

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
	uint32_t ReadAddress = ApplicationAddress + APP_SIZE;
	uint32_t WriteAddress = ApplicationAddress;
	
//	printf("ReadAddress : 0x%08X    \r\n",ReadAddress);
//	printf("WriteAddress : 0x%08X    \r\n",WriteAddress);
	
	
	__set_PRIMASK(1); 
	FLASH_Unlock();

//	FileSize = *(__IO uint32_t*)FileSizeAddress;

	//NbrOfPage = FLASH_PagesMask(0x19000);
	FileSize = 0x19000;
	NbrOfPage = 50;
	
	
	for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
	{
		FLASHStatus = FLASH_ErasePage(ApplicationAddress + (PAGE_SIZE * EraseCounter));
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

/*******************************************************************************
* Function Name  : main
* Description    : Ö÷º¯Êý
* Input          : None.
* Return         : None.
*******************************************************************************/
int main(void)
{
	u8 retry=5;
	struct TerminalParameters terminal_parameters;
	USART3_Config(115200);
	USART1_Config(115200);
	
	
	IWDG_Init(6,4095); //Óë·ÖÆµÊýÎª64,ÖØÔØÖµÎª625,Òç³öÊ±¼äÎª1s
	printf("DEBUG-->---------------------------\r\n");
	printf("DEBUG-->                           \r\n");
	printf("DEBUG-->       BOOTLOADER run!     \r\n");
	printf("DEBUG-->                           \r\n");
	printf("DEBUG-->---------------------------\r\n");	

	Internal_ReadFlash((uint32_t) UpdateFlagAddress , (uint8_t *) &terminal_parameters , sizeof(terminal_parameters));
	

	printf("UpdateFlag : 0x%08lX    \r\n",terminal_parameters.bootLoaderFlag);
	if (terminal_parameters.bootLoaderFlag == 0xAAAAAAAA)
	{
		printf("need to update \r\n");
		while(retry--)
		{
			if(Copy_APP2_TO_APP1())
			{
				terminal_parameters.bootLoaderFlag = 0xFFFFFFFF;
				FLASH_WriteByte(UpdateFlagAddress , (uint8_t*)&terminal_parameters , sizeof(terminal_parameters));
				__set_FAULTMASK(1); 
				NVIC_SystemReset();
				break;
			}
		}
	}
	else
	{
		printf("no update \r\n");
	}
		
	if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000)
	{
		printf("DEBUG-->  Jump to APP!  	\r\n");
		/* Jump to user application */
		JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);
		Jump_To_Application = (pFunction) JumpAddress;
		/* Initialize user application's Stack Pointer */
		__set_MSP(*(__IO uint32_t*) ApplicationAddress);
		Jump_To_Application();
	}
	
	while(1)
	{
	
	}
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/




