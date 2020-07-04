#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_exti.h"
#include "misc.h"
#include "core_cm3.h"
#include "lcd.h"
#include <string.h>
#include "stm32f10x_usart.h"
int keypad_input[4] = {-1, -1, -1, -1}; // Ű�е� �Է� ����
int password[4] = {1, 2, 3, 4};         // ��й�ȣ �迭
// int temp_password[4] = {
//     0,
// };
char LCD_output[5] = {'\0', '\0', '\0', '\0', '\0'};   // LCD output�� ����
int status = 0;       // 0 �̸� �������, 1 �̸� ������
int fail_counter = 0; // ��й�ȣ Ʋ�� Ƚ�� ī����
int motion_flag = 0;  // ������ ���� Ž�� ���� (1�̸� Ž��)
int door_flag = 0;    // �� ���� ���� Ž�� ���� (1�̸� ��������)
int fail_flag = 0;    // ��й�ȣ 3ȸ �̻� Ʋ���� fail_flag = 1
int flag = 1;

int i = 0, j = 0;
int value = -1;
int password_index = -1;

int cnt = 0, returnLine = 0;
uint16_t buf[500] = {
    0,
};
uint16_t Pulse[2] = {2300, 700};

void openDoor();
void closeDoor();
void TIM_Init();

void RCC_Configure(void)
{
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2ENR_AFIOEN, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2ENR_IOPAEN, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2ENR_IOPDEN, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2ENR_IOPCEN, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2ENR_IOPEEN, ENABLE);

   RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
   // led �׽�Ʈ��
}

void GPIO_Configure(void)
{
	// 4���� output, 4���� input
	GPIO_InitTypeDef GPIO_InitStructure;

	//USART1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);


	// input
	GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_11
	| GPIO_Pin_12); // �̷��� �����ص� �Ǵ°����� �� �𸣰���
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	// pull-down down�� �� low
	// GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);


	// output
	GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6
	| GPIO_Pin_7);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// led 1 �׽�Ʈ��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4
	| GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, ENABLE);

	// PIR sensor
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//Door sensor
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource1);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource2);


}

void NVIC_Configuration()
{

   NVIC_InitTypeDef NVIC_InitStructure;

   NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

   NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;

   NVIC_Init(&NVIC_InitStructure);

   NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;

   NVIC_Init(&NVIC_InitStructure);

   //USART1
   NVIC_EnableIRQ(USART1_IRQn);
   NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // TODO
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;        // TODO
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);

   //USART2
   NVIC_EnableIRQ(USART2_IRQn);
   NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // TODO
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;        // TODO
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);
}

void USART1_Init(void)
{
   USART_InitTypeDef USART1_InitStructure;

   // Enable the USART2 peripheral
   USART_Cmd(USART1, ENABLE);

   USART1_InitStructure.USART_BaudRate = 115200;
   USART1_InitStructure.USART_WordLength = USART_WordLength_8b;
   USART1_InitStructure.USART_StopBits = USART_StopBits_1;
   USART1_InitStructure.USART_Parity = USART_Parity_No;
   USART1_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
   USART1_InitStructure.USART_Mode = (USART_Mode_Rx | USART_Mode_Tx);

   // TODO: Initialize the USART using the structure 'USART_InitTypeDef' and the function 'USART_Init'
   USART_Init(USART1, &USART1_InitStructure);

   // TODO: Enable the USART2 RX interrupts using the function 'USART_ITConfig' and the argument value 'Receive Data register not empty interrupt'
   USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
}

void USART2_Init(void)
{
   USART_InitTypeDef USART1_InitStructure;

   // Enable the USART2 peripheral
   USART_Cmd(USART2, ENABLE);

   USART1_InitStructure.USART_BaudRate = 115200;
   USART1_InitStructure.USART_WordLength = USART_WordLength_8b;
   USART1_InitStructure.USART_StopBits = USART_StopBits_1;
   USART1_InitStructure.USART_Parity = USART_Parity_No;
   USART1_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
   USART1_InitStructure.USART_Mode = (USART_Mode_Rx | USART_Mode_Tx);

   // TODO: Initialize the USART using the structure 'USART_InitTypeDef' and the function 'USART_Init'
   USART_Init(USART2, &USART1_InitStructure);

   // TODO: Enable the USART2 RX interrupts using the function 'USART_ITConfig' and the argument value 'Receive Data register not empty interrupt'
   USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
}

void EXTI_Configuration()
{

   EXTI_InitTypeDef EXTI_InitStructure;

   EXTI_InitStructure.EXTI_Line = EXTI_Line1;
   EXTI_InitStructure.EXTI_LineCmd = ENABLE;
   EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
   EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;

   EXTI_Init(&EXTI_InitStructure);

   EXTI_InitStructure.EXTI_Line = EXTI_Line2;
   EXTI_InitStructure.EXTI_LineCmd = ENABLE;
   EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
   EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;

   EXTI_Init(&EXTI_InitStructure);
}

void sendUSART1(uint16_t data)
{
   while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
   {
   }

   USART_SendData(USART1, data);
   while ((USART1->SR & USART_SR_TC) == 0)
   {
   };
}

void sendUSART2(uint16_t data)
{
   while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
   {
   }

   USART_SendData(USART2, data);
   while ((USART2->SR & USART_SR_TC) == 0)
   {
   };
}

void USART1_IRQHandler()
{
   uint16_t word;

   if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
   {
      word = USART_ReceiveData(USART1);
      while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
      {
      }
      {
         LCD_ShowNum(100, 130, word, 10, BLACK, WHITE);
         USART_SendData(USART1, word);
      }
   }

   USART_ClearITPendingBit(USART1, USART_IT_RXNE);
}

void USART2_IRQHandler()
{
   uint16_t word;

   if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
   {
      word = USART_ReceiveData(USART2);

      if (word == '\n')
      {
         returnLine = 1;
         cnt++;
         buf[cnt] = 0;
         cnt = 0;
      }
      else
      {
         buf[cnt] = word;
         cnt++;
      }

      while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
      {
      }
      USART_SendData(USART1, word);
   }

   USART_ClearITPendingBit(USART2, USART_IT_RXNE);
}

void EXTI1_IRQHandler(void)
{

   if (EXTI_GetITStatus(EXTI_Line1) != RESET)
   { /* motion sensing */

      if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1))
      {
    	 if(status == 1){
    		 GPIO_SetBits(GPIOD, GPIO_Pin_2);
         	 motion_flag = 1;
         	/* ��޸�� signal */
    	 }
         /* signal "people exist" */
      }
      else if (!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1))
      {

         motion_flag = 0;
         GPIO_ResetBits(GPIOD, GPIO_Pin_2);
      }
      EXTI_ClearITPendingBit(EXTI_Line1);
   }
}

void EXTI2_IRQHandler(void)
{

   if (EXTI_GetITStatus(EXTI_Line2) != RESET)
   {

      /* signal "door open" */
      if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2))
      {
    	 if(status == 1){
    		 GPIO_SetBits(GPIOD, GPIO_Pin_3);
    		 /* ��޸�� signal */
    	 }
         /* signal "door open" */
         door_flag = 1;

      }
      else if (!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2))
      {

         GPIO_ResetBits(GPIOD, GPIO_Pin_3);
         door_flag = 0;
         /* signal "door close" */
      }

      EXTI_ClearITPendingBit(EXTI_Line2);
   }
}

void delay(int time)
{
   int i;
   int delay_time = 50000 * time;
   for (i = 0; i < delay_time; i++)
   {
   }
}

/*
 ����� : PC9
 ������ : VCC
 ������ : Ground
 */

void openDoor()
{
   TIM_OCInitTypeDef TIM_OCInitStructure;
   TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
   TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
   TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
   TIM_OCInitStructure.TIM_Pulse = Pulse[0];
   TIM_OC4Init(TIM3, &TIM_OCInitStructure);
}

void closeDoor()
{
   TIM_OCInitTypeDef TIM_OCInitStructure;
   TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
   TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
   TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
   TIM_OCInitStructure.TIM_Pulse = Pulse[1];
   TIM_OC4Init(TIM3, &TIM_OCInitStructure);
}

void TIM_Init()
{

   TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

   TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);
   TIM_ARRPreloadConfig(TIM3, ENABLE);
   TIM_Cmd(TIM3, ENABLE);

   TIM_TimeBaseStructure.TIM_Period = 20000 - 1;
   TIM_TimeBaseStructure.TIM_Prescaler = (uint16_t)(SystemCoreClock / 1000000) - 1;
   TIM_TimeBaseStructure.TIM_ClockDivision = 0;
   TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

   TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
}

void INPUT_PASSWORD()
{


	GPIO_ResetBits(GPIOA, GPIO_Pin_4);
	GPIO_SetBits(GPIOA, GPIO_Pin_5);
	GPIO_SetBits(GPIOA, GPIO_Pin_6);
	GPIO_SetBits(GPIOA, GPIO_Pin_7);
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)) {
	value = 'D';
	delay(10);
	}
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1)) {
	value = '#';
	delay(10);
	}
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11)) {
	value = 0;
	delay(10);
	}
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12)) {
	value = '*';
	delay(10);
	}

	GPIO_SetBits(GPIOA, GPIO_Pin_4);
	GPIO_ResetBits(GPIOA, GPIO_Pin_5);
	GPIO_SetBits(GPIOA, GPIO_Pin_6);
	GPIO_SetBits(GPIOA, GPIO_Pin_7);
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)) {
	value = 'C';
	delay(10);
	}
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1)) {
	value = 9;
	delay(10);
	}
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11)) {
	value = 8;
	delay(10);
	}
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12)) {
	value = 7;
	delay(10);
	}

	GPIO_SetBits(GPIOA, GPIO_Pin_4);
	GPIO_SetBits(GPIOA, GPIO_Pin_5);
	GPIO_ResetBits(GPIOA, GPIO_Pin_6);
	GPIO_SetBits(GPIOA, GPIO_Pin_7);
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)) {
	value = 'B';
	delay(10);
	}
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1)) {
	value = 6;
	delay(10);
	}
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11)) {
	value = 5;
	delay(10);
	}
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12)) {
	value = 4;
	delay(10);
	}

	GPIO_SetBits(GPIOA, GPIO_Pin_4);
	GPIO_SetBits(GPIOA, GPIO_Pin_5);
	GPIO_SetBits(GPIOA, GPIO_Pin_6);
	GPIO_ResetBits(GPIOA, GPIO_Pin_7);
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)) {
	value = 'A';
	delay(10);
	}
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1)) {
	value = 3;
	delay(10);
	}
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11)) {
	value = 2;
	delay(10);
	}
	while (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12)) {
	value = 1;
	delay(10);
	}

	if ((value >= 0 && value < 10) || value == 'A' || value == 'B' || value == 'C' || value == 'D')
	   {
	      password_index = (password_index + 1) % 4;
	      // 10�� �Ѿ�� � ������� ��ȣ�� ��
	      keypad_input[password_index] = value;
	   }


	for (i = 0; i<4 ; i++) {
		  if(keypad_input[i] == -1)
			  LCD_output[i]= '\0';
		  else if (keypad_input[i] == 'A' || keypad_input[i] == 'B' || keypad_input[i] == 'C' || keypad_input[i] == 'D' )
			  LCD_output[i] = keypad_input[i];
		  else {
			  LCD_output[i] = keypad_input[i] + 48;
		  }
	   }

	LCD_ShowString(10, 100, LCD_output, BLACK, WHITE);

}


void sendWIFI(char *str)
{
   int len = strlen(str);

   for (i = 0; i < len; i++)
   {
      sendUSART2((uint16_t)str[i]);
   }

   sendUSART2('\r');
   sendUSART2('\n');
}

void Check_Password()
{


   // ������� �Է� ����(keypad_input) LCD ȭ�鿡 ���

   //LCD_ShowCharString(10, 100, LCD_output, BLACK, WHITE);
   //LCD_ShowString(10, 100, LCD_output, BLACK, WHITE);

   if (value == '*')
   {
      // �н����� �Է� ����
      password_index = -1;
      for (i = 0; i < 4; i++) {
         LCD_output[i] = '\0';
      }

      for (i = 0; i < 4; i++)
      {
         if (keypad_input[i] != password[i])
         {
            fail_counter++;

            for (j = 0; j < 4; j++)
            {
               keypad_input[j] = -1;
               LCD_output[j] = '\0';
               LCD_ShowString(10, 100, LCD_output, BLACK, WHITE);
            } // �Է� ���� �ʱ�ȭ
            return;
         }
      }

      /* ��й�ȣ �¾����� */
      for (j = 0; j < 4; j++)
      {
         keypad_input[j] = -1;
      } // �Է� ���� �ʱ�ȭ
      LCD_ShowString(10, 100, LCD_output, BLACK, WHITE);
      status = 0;
      fail_flag = 0;
      fail_counter = 0;
      openDoor();
   }

   if (fail_counter >= 3)
   {
      /* ��й�ȣ ���� */
      sendWIFI("passwordfail");
   }
}


void sendTTL(char *str)
{
   int len = strlen(str);

   for (i = 0; i < len; i++)
   {
      sendUSART1((uint16_t)str[i]);
   }
}

void recvWIFI(char *str)
{
   if (buf[0] == '+' && buf[1] == 'I' && buf[2] == 'P' && buf[3] == 'D')
   {
      str[0] = buf[7];
      str[1] = buf[8];
      str[2] = buf[9];
      str[3] = buf[10];
   }
   else
      str = "Fail";

   //LCD_ShowString(200,0, buf, BLACK, WHITE);
}

void waitOK()
{
   while (1)
   {
      LCD_ShowNum(100, 100, buf[0], 4, BLACK, WHITE);
      LCD_ShowNum(100, 120, buf[1], 4, BLACK, WHITE);
      if (buf[0] == 'O' && buf[1] == 'K')
         break;
   }
}

void wait(uint16_t str)
{
   while (1)
   {
      if (buf[0] == str)
         break;

      LCD_ShowNum(100, 100, buf[0], 4, BLACK, WHITE);
      LCD_ShowNum(100, 120, str, 4, BLACK, WHITE);
   }
}

void WIFI_Init()
{
   sendWIFI("AT+CWMODE=3");
   waitOK();
   sendWIFI("AT+CWJAP=\"ABC\",\"123456789a\"");
   waitOK();
   sendWIFI("AT+CIPMUX=0");
   waitOK();
   sendWIFI("AT+CIPSTATUS");
   waitOK();
}

void getPassword()
{
   char pass[4];
   LCD_Clear(WHITE);
   sendWIFI("AT+CIPSTART=\"TCP\",\"server.gomsoup.com\",4000");
   waitOK();

   sendWIFI("AT+CIPSEND=4");

   wait('>');
   delay(100);

   sendWIFI("PASS");
   delay(500);
   recvWIFI(pass);

   if (pass != "Fail")
   {
      password[0] = pass[0];
      password[1] = pass[1];
      password[2] = pass[2];
      password[3] = pass[3];
   }

   LCD_Clear(WHITE);
   LCD_ShowString(200, 100, pass, BLACK, WHITE);

   sendWIFI("AT+CIPSEND=4");
   wait('>');
   delay(100);
   sendWIFI("CLOS");
}

void getTempPassword()
{
   char pass[4];
   LCD_Clear(WHITE);
   sendWIFI("AT+CIPSTART=\"TCP\",\"server.gomsoup.com\",4000");
   waitOK();

   sendWIFI("AT+CIPSEND=4");

   wait('>');
   delay(100);

   sendWIFI("TEMP");
   delay(500);
   recvWIFI(pass);

   if (pass != "Fail")
   {
      password[0] = pass[0];
      password[1] = pass[1];
      password[2] = pass[2];
      password[3] = pass[3];
   }

   sendWIFI("AT+CIPSEND=4");
   wait('>');
   delay(100);
   sendWIFI("CLOS");
}

/* main */
int main(void)
{
   int p = 0;
   int buf_index;
   SystemInit();
   RCC_Configure();
   GPIO_Configure();
   TIM_Init();
   NVIC_Configuration();
   EXTI_Configuration();
   USART1_Init();
   USART2_Init();
   LCD_Init();
   LCD_Clear(WHITE);

    delay(10);
    WIFI_Init();
   // getTempPassword();

   // while (1)
   // {
   // }

   //closeDoor();

   if (motion_flag == 1)
   {
   }
   else if (motion_flag == 0)
   {
   }
   if (door_flag == 1)
   {
   }
   else if (door_flag == 0)
   {
   }

   while (1)
   {


      value = -1;
      if (status == 0)
      {
         GPIO_SetBits(GPIOD, GPIO_Pin_4);
         GPIO_ResetBits(GPIOD, GPIO_Pin_7);
         // ������ ������ ��
         INPUT_PASSWORD();

         if (value == '#')
         {
            if (door_flag == 0)
            {
               closeDoor();
               status = 1;
            }
         }

         if (value == '*')
         { // �� ���� ���¿��� #�� ������ ��й�ȣ ���� ���·� ��
            for (i = 0; i < 4; i++)
            {
               password[i] = -1;
            } // �н����� �ʱ�ȭ
            password_index = -1;

            buf_index = -1;
            while (1)
            {
               value = -1;
               INPUT_PASSWORD();

               if ((value >= 0 && value < 10) || value == 'A' || value == 'B' || value == 'C' || value == 'D')
               { // #�̶� * ���� ��� ����
                  buf_index = (buf_index + 1) % 4;
                  password[buf_index] = value;
               }
               if (value == '*')
               {
                  break;
               } // ��й�ȣ ���� ���¿��� �ٽ� * �ԷµǸ� ��й�ȣ ���� ��

               // LCD ȭ�鿡 �н����� �Է� ���� ���
            }
         }
      }
      else if (status == 1)
      { // �� ��� (������)

         GPIO_SetBits(GPIOD, GPIO_Pin_7);
         GPIO_ResetBits(GPIOD, GPIO_Pin_4);
         if (fail_flag == 1)
         {
         }
         if (door_flag == 1)
         {
         }
         if (motion_flag == 1)
         {
            motion_flag = 0;
         }

         INPUT_PASSWORD();
         Check_Password();
      }
   }

   return 0;
}