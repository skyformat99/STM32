/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: portserial.c,v 1.1 2006/08/22 21:35:13 wolti Exp $
 */

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

//STM32�������ͷ�ļ�
#include "stm32f10x.h"
#include "stm32f10x_it.h"

/* ----------------------- static functions ---------------------------------*/
static void prvvUARTTxReadyISR( void );
static void prvvUARTRxISR( void );

/* ----------------------- Start implementation -----------------------------*/
/**
  * @brief  ���ƽ��պͷ���״̬
  * @param  xRxEnable ����ʹ�ܡ�
  *         xTxEnable ����ʹ��
  * @retval None
  */
void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
  if(xRxEnable)
  {
    //ʹ�ܽ��պͽ����ж�
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    //MAX485���� �͵�ƽΪ����ģʽ
    GPIO_ResetBits(GPIOD,GPIO_Pin_8);
  }
  else
  {
    USART_ITConfig(USART2, USART_IT_RXNE, DISABLE); 
    //MAX485���� �ߵ�ƽΪ����ģʽ
    GPIO_SetBits(GPIOD,GPIO_Pin_8);
  }

  if(xTxEnable)
  {
    //ʹ�ܷ�������ж�
    USART_ITConfig(USART2, USART_IT_TC, ENABLE);
  }
  else
  {
    //��ֹ��������ж�
    USART_ITConfig(USART2, USART_IT_TC, DISABLE);
  }
  
}

/**
  * @brief  ���ڳ�ʼ��
  * @param  ucPORT      ���ں�
  *         ulBaudRate  ������
  *         ucDataBits  ����λ
  *         eParity     У��λ 
  * @retval None
  */
BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{

  
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	(void)ucPORT;     //���޸Ĵ���
  (void)ucDataBits; //���޸�����λ����
  (void)eParity;    //���޸�У���ʽ

  //ʹ��USART2��GPIOA
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//ʹ��GPIOAʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);//ʹ��USART2ʱ��

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;	//PA2
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//��������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
   
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;//PA3
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //��������
  GPIO_Init(GPIOA, &GPIO_InitStructure);  
	
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_USART2,ENABLE);//��λ����2
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_USART2,DISABLE);//ֹͣ��λ

  USART_InitStructure.USART_BaudRate = ulBaudRate;            //ֻ�޸Ĳ�����
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  //���ڳ�ʼ��
  USART_Init(USART2, &USART_InitStructure);
  //ʹ��USART2
  USART_Cmd(USART2, ENABLE);
  
  //�趨USART2 �ж����ȼ�
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  

  return TRUE;
}

/**
  * @brief  ͨ�����ڷ�������
  * @param  None
  * @retval None
  */
BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
  //��������
  USART_SendData(USART2, ucByte);
  return TRUE;
}

/**
  * @brief  �Ӵ��ڻ������
  * @param  None
  * @retval None
  */
BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
  //��������
  *pucByte = USART_ReceiveData(USART2);
  return TRUE;
}

/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
static void prvvUARTTxReadyISR( void )
{
  //mb.c eMBInit������
  //pxMBFrameCBTransmitterEmpty = xMBRTUTransmitFSM 
  //����״̬��
  pxMBFrameCBTransmitterEmpty();
}

/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
static void prvvUARTRxISR( void )
{
  //mb.c eMBInit������
  //pxMBFrameCBByteReceived = xMBRTUReceiveFSM
  //����״̬��
  pxMBFrameCBByteReceived();
}

/**
  * @brief  USART2�жϷ�����
  * @param  None
  * @retval None
  */
void USART3_IRQHandler(void)
{
  //���������ж�
  if(USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
  {
    prvvUARTRxISR(); 
    //����жϱ�־λ    
    USART_ClearITPendingBit(USART2, USART_IT_RXNE);   
  }
  
  //��������ж�
  if(USART_GetITStatus(USART2, USART_IT_TC) == SET)
  {
    prvvUARTTxReadyISR();
    //����жϱ�־
    USART_ClearITPendingBit(USART2, USART_IT_TC);
  }
  
  //���Կ��Ƿ����ȥ�� 2012-07-23
  //���-������������Ҫ�ȶ�SR,�ٶ�DR�Ĵ��� �������������жϵ�����
  /*
  if(USART_GetFlagStatus(USART2,USART_FLAG_ORE)==SET)
  {
    USART_ClearFlag(USART2,USART_FLAG_ORE); //��SR
    USART_ReceiveData(USART2);              //��DR
  }
  */
}

