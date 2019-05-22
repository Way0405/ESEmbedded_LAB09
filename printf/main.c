#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include "reg.h"
#include "blink.h"

void init_usart1(void)
{
	//PB6 為TX 
	//PB7 為 RX

	//RCC EN GPIOB //開啟腳位
	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTB));

	//GPIO Configurations//設定腳位功能

	//AF setting//替代功能為 AF7(UART功能)//看第八章
	WRITE_BITS(GPIO_BASE(GPIO_PORTB)+GPIOx_AFRL_OFFSET, AFRLy_3_BIT(7), AFRLy_0_BIT(7), 7);
	WRITE_BITS(GPIO_BASE(GPIO_PORTB)+GPIOx_AFRL_OFFSET, AFRLy_3_BIT(6), AFRLy_0_BIT(6), 7);


	////////////////////////////////////////////////////pin B6///////////////////////////////////////////////////
		//MODER lsded pin = 10 => AF mode//設定mode為"替代功能AF"/
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_1_BIT(6));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_0_BIT(6));

	//OT led pin = 0 => Output push-pull
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OTYPER_OFFSET, OTy_BIT(6));

	//OSPEEDR led pin = 01 => Medium speed
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(6));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(6));

	//PUPDR led pin = 00 => No pull-up, pull-down
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(6));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(6));

	////////////////////////////////////////////////////pin B7///////////////////////////////////////////////////
		//MODER led pin = 10 => AF mode//設定mode為"替代功能AF"/
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_1_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_0_BIT(7));

	//OT led pin = 0 => Output push-pull
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OTYPER_OFFSET, OTy_BIT(7));

	//OSPEEDR led pin = 01 => Medium speed
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(7));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(7));

	//PUPDR led pin = 00 => No pull-up, pull-down
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(7));
///////////////////////////////////////////////
	//RCC EN USART1//
	SET_BIT(RCC_BASE+RCC_APB2ENR_OFFSET,USART1EN);

	//Baud
	const unsigned int BAUD = 115200;
	const unsigned int SYSCLK_MHZ = 168;
	const double USARTDIV = SYSCLK_MHZ * 1.0e6 / 16 / BAUD;

	const uint32_t DIV_MANTISSA = (uint32_t) USARTDIV;//取整數
	const uint32_t DIV_FRACTION = (uint32_t) ((USARTDIV-DIV_MANTISSA)*16);//取小數


	WRITE_BITS(USART1_BASE+USART_BRR_OFFSET,DIV_MANTISSA_11_BIT,DIV_MANTISSA_0_BIT,DIV_MANTISSA);
	WRITE_BITS(USART1_BASE+USART_BRR_OFFSET,DIV_FRACTION_3_BIT,DIV_FRACTION_0_BIT,DIV_FRACTION);
	//USART  Configurations
	SET_BIT(USART1_BASE+USART_CR1_OFFSET, UE_BIT);
	SET_BIT(USART1_BASE+USART_CR1_OFFSET, TE_BIT);
	SET_BIT(USART1_BASE+USART_CR1_OFFSET, RE_BIT);

	//set RXNEIW//RXNE旗幟
	//Recive data to read跟Overrun error detect時，硬體自動設RXNE=1
	SET_BIT(USART1_BASE+USART_CR1_OFFSET,RXNEIE_BIT);

	//中斷向量()vector檔案//usart1_handler為IRQ37
	SET_BIT(NVIC_ISER_BASE + NVIC_ISERn_OFFSET(1), 5); //IRQ37=32*1+5


}

void usart1_send_char(const char ch)
{

	while(!READ_BIT(USART1_BASE+USART_SR_OFFSET,TXE_BIT))
		;//不能傳時(TXE=0)不做事
	REG(USART1_BASE+USART_DR_OFFSET)=ch;//可以傳時，把DATA放到Data reg
	
}

char usart1_receive_char(void)
{
	while(!READ_BIT(USART1_BASE+USART_SR_OFFSET,RXNE_BIT))
		;//不能收時(RXE=0)不做事
	return (char)REG(USART1_BASE+USART_DR_OFFSET);//可以收時，把DATA reg 回傳

}
void usart1_handler(void)//IRQ37中斷轉跳的地方//可能為Recive data to read或Overrun error detect
{

	char ch;
	
	if( READ_BIT(USART1_BASE + USART_SR_OFFSET, ORE_BIT) )//若有Overrun error
		{
			
			char *fuck = "fuck up!\r\n";
			blink_count(LED_RED, 3);
			while (*fuck != '\0')
				usart1_send_char(*fuck++);

			WRITE_BITS(USART1_BASE+USART_DR_OFFSET,8,0,0);//清空Data reg
			usart1_send_char('\n');
		}
	else//Recive data to read
	{	
		ch = usart1_receive_char();
		if (ch == '\r')//enter 時,
			usart1_send_char('\n');//幫你換行

		usart1_send_char(ch);//顯示在電腦上
		blink_count(LED_GREEN, 3);//可以刪掉這行
	}
	

	// clean interrupt flag//清掉中斷旗幟
	CLEAR_BIT(USART1_BASE + USART_SR_OFFSET, RXNE_BIT);
	
}

void *_sbrk(int incr)
{
	extern uint8_t _mybss_vma_end; //Defined by the linker script
	static uint8_t *heap_end = NULL;
	uint8_t *prev_heap_end;

	if (heap_end == NULL)
		heap_end = &_mybss_vma_end;

	prev_heap_end = heap_end;
	if (heap_end + incr > &_mybss_vma_end + HEAP_MAX)
		return (void *)-1;

	heap_end += incr;
	return (void *)prev_heap_end;
}
int _write(int file, char *ptr, int len)
{
	for (unsigned int i = 0; i < len; i++)
		usart1_send_char(*ptr++);

	return len;
}

int _close(int file)
{
	return -1;
}

int _lseek(int file, int ptr, int dir)
{
	return 0;
}

int _read(int file, char *ptr, int len)
{
	return 0;
}

int _fstat(int file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file)
{
	return 1;
}


int main(void)
{
    init_usart1();

    printf("Hello World\r\n");

    int i = 75;

    printf("Decimal: %d  Hexadecimal: 0x%x \r\n", i, i);

    printf("Character: %c\r\n", i);

    blink(LED_BLUE);
}