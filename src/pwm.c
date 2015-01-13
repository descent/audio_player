#include "pwm.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "misc.h"
#include "filesystem.h"
#include <math.h>
#include "ff.h"
#include "stm32f4xx_dac.h"
#include "gui.h"
uint32_t play_time_other = 0;
uint32_t play_time_sec = 0;

#define BUF_LENGTH 4000

int i;
uint16_t buf[BUF_LENGTH];
uint16_t buf2[BUF_LENGTH];
int br;
int chunck2_size = 0;
const int btr = BUF_LENGTH * sizeof(uint16_t);
uint8_t cur_buf = 0;
uint8_t update_buf = 0;
uint32_t cur_point = 0;
FIL fil;

//uint8_t back[BUF_LENGTH];

//short wave[42000];
void play_test(void *p){
    play("music1.wav");
}

int play(char * name)
{
    if (f_open(&fil, name, FA_OPEN_ALWAYS | FA_READ) != FR_OK) {
        return -1;
    }

    f_lseek(&fil, 40);
    f_read(&fil, &chunck2_size, 4, &br);

    for(i=0;i<BUF_LENGTH;i++)
    {
        buf[i] = 32768;
        buf2[i] = 32768;
    }

    TIM_Cmd(TIM1, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
    DAC_Cmd(DAC_Channel_1, ENABLE);
    DAC_Cmd(DAC_Channel_2, ENABLE);
    return 0;
}

void stop()
{
    TIM_Cmd(TIM1, DISABLE);
    TIM_Cmd(TIM2, DISABLE);
    DAC_Cmd(DAC_Channel_1, DISABLE);
    DAC_Cmd(DAC_Channel_2, DISABLE);
    f_close(&fil);
}

void player_init(void)
{
    PWM_RCC_Configuration();
    DAC_Configuration();
    PWM_GPIO_Configuration();
    PWM_TIM2_Configuration();
    PWM_TIM1_Configuration();
}

void DAC_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

  /* DMA1 clock and GPIOA clock enable (to be used with DAC) */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

  /* DAC Periph clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

  /* DAC channel 1 & 2 (DAC_OUT1 = PA.4)(DAC_OUT2 = PA.5) configuration */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
    // GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* TIM6 Configuration ------------------------------------------------------*/ 

    //DAC_DeInit(); 

     /* DAC channel2 Configuration */
    DAC_InitTypeDef  DAC_InitStructure;
    DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
    // DAC_Init(DAC_Channel_1, &DAC_InitStructure);
    DAC_Init(DAC_Channel_2, &DAC_InitStructure);
    //DAC_DualSoftwareTriggerCmd(ENABLE);
    // DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
    DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);

    // DAC_Cmd(DAC_Channel_1, ENABLE);
    // DAC_Cmd(DAC_Channel_2, ENABLE);
}

void  PWM_RCC_Configuration(void)
{
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOE, ENABLE );//Enalbe AHB for GPIOE
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_TIM1, ENABLE );//Enable APB for TIM1
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2, ENABLE );//Enable APB for TIM2
}


void TIM1_UP_TIM10_IRQHandler(void)
{
    TIM_ClearITPendingBit(TIM1, TIM_FLAG_Update);

    if(cur_buf == 0) 
    {
        //TIM1->CCR1 = buf[cur_point * 2];
        //TIM1->CCR2 = buf[cur_point * 2 + 1];
        int data1 = buf[(cur_point/16)*2] >> 4;
        int data2 = buf[(cur_point/16)*2+1] >> 4;
        if(cur_point%16 == 0)
        {
            // DAC_SetChannel1Data(DAC_Align_12b_R,data1);
            DAC_SetChannel2Data(DAC_Align_12b_R,data2);
        } 
        else 
        {
            if(cur_point%16 == 15 - buf[(cur_point/16)*2] % 16)
            {
                if(data1 < 4094)
                {
                    ++data1;
                    // DAC_SetChannel1Data(DAC_Align_12b_R,data1);
                }
                
            }
        }
    }
    else
    {
        int data1 = buf2[(cur_point/16)*2] >> 4;
        int data2 = buf2[(cur_point/16)*2+1] >> 4;
        if(cur_point%16 == 0)
        {
            
            // DAC_SetChannel1Data(DAC_Align_12b_R,data1);
            DAC_SetChannel2Data(DAC_Align_12b_R,data2);
        } 
        else 
        {
            if(cur_point%16 == 15 - buf2[(cur_point/16)*2+1] % 16)
            {
                if(data2 < 4094)
                {
                    ++data2;
                    DAC_SetChannel2Data(DAC_Align_12b_R,data2);
                }
            }
        }
    }

    ++cur_point;

    if(cur_point == BUF_LENGTH * 8)  {
        cur_buf = !cur_buf;
        update_buf = 1;
        cur_point = 0;
        play_time_sec++;
    }
    //if(play_time_sec > 10 ) TIM1->CCR1 = 0;

    // USART_SendData(USART1, wave[play_time_other]) ;
    //if(play_time_sec >= 10) TIM1->CCR1 = 0;
}

void TIM2_IRQHandler(void)
{
    
    if(update_buf == 1)
    {
        // USART_SendData(USART1, 48) ;
        if(cur_buf == 0)   
        {
            f_read(&fil, buf2, btr, &br);

            int i;
            for(i=0;i<BUF_LENGTH;i++)
            {
                //short big = buf2[i*2] & 0x00FF;
                //short little = (buf2[i*2] & 0xFF00) >> 8;
                //short data = (big << 8) | little;
                buf2[i] = ((short)buf2[i])+32768;
            }


        }
        else
        {
            f_read(&fil, buf, btr, &br);
            
            int i;
            for(i=0;i<BUF_LENGTH;i++)
            {
                // uint16_t big = buf[i*2] & 0x00FF;
                // uint16_t little = (buf[i*2] & 0xFF00) >> 8;
                // short data = (big << 8) | little;
                buf[i] = ((short)buf[i])+32768;
            }           
        }

        update_buf = 0;
        // USART_SendData(USART1, 49) ;
    }
    

    TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
}

void  PWM_TIM2_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

  /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = 39999;
    TIM_TimeBaseStructure.TIM_Prescaler = 99;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;     
    TIM_OCInitStructure.TIM_Pulse = 99; 
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);

    TIM_ARRPreloadConfig(TIM2, ENABLE);

    TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);

 /* TIM2 enable counter */
    TIM_Cmd(TIM2, DISABLE);
}

void  PWM_TIM1_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;

    //TIM_DeInit(TIM1);

    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_TIM10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

  /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = 124;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

  /* PWM1 Mode configuration: Channel1 */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
    //TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    //TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;

    TIM_OC1Init(TIM1, &TIM_OCInitStructure);
    //TIM_OC2Init(TIM1, &TIM_OCInitStructure);

    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    //TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);

    //TIM_ClearFlag(TIM1, TIM_FLAG_Update);
    //TIM_ClearITPendingBit(TIM1,TIM_IT_Update);
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
    //TIM_CtrlPWMOutputs(TIM1, ENABLE);

 /* TIM4 enable counter */
    TIM_Cmd(TIM1, DISABLE);
}

void PWM_GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    // GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    //GPIO_PinAFConfig(GPIOE, GPIO_PinSource9, GPIO_AF_TIM1);
    //GPIO_PinAFConfig(GPIOE, GPIO_PinSource11, GPIO_AF_TIM1);
}