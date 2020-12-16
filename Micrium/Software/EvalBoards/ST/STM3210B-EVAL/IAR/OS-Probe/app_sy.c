/*
 *********************************************************************************************************
 *                                              TERM PROJECT
 *                          (C) Copyright 2020; Lee Seung Yun; Shim Jae Yeong
 *               Some codes are referenced below.
 * 
 *                                              EXAMPLE CODE
 *
 *                          (c) Copyright 2003-2006; Micrium, Inc.; Weston, FL
 *
 *               All rights reserved.  Protected by international copyright laws.
 *               Knowledge of the source code may NOT be used to develop a similar product.
 *               Please help us continue to provide the Embedded community with the finest
 *               software available.  Your honesty is greatly appreciated.
 *********************************************************************************************************
 */

/*
 *********************************************************************************************************
 *
 *                             High Temperature Entrance Checking Technique
 *
 *                                     ST Microelectronics STM32
 *                                              with the
 *                                   STM3210B-EVAL Evaluation Board
 *
 * Filename      : app.c
 * Version       : V1.0
 * Programmer(s) : Lee Seung Yun, Shim Jae Yeong
 *********************************************************************************************************
 */

/*
 *********************************************************************************************************
 *                                             INCLUDE FILES
 *********************************************************************************************************
 */

#include <includes.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_i2c.h>
#include <stm32f10x_adc.h>
#include <stm32f10x_spi.h>
#include <stm32f10x_tim.h>

/*
 *********************************************************************************************************
 *                                            LOCAL DEFINES
 *********************************************************************************************************
 */

/*
 *********************************************************************************************************
 *                                       LOCAL GLOBAL VARIABLES
 *********************************************************************************************************
 */

// Task Stack (size: 128)
static OS_STK TaskDetectStk[TASK_STK_SIZE];
static OS_STK TaskTemperStk[TASK_STK_SIZE];
static OS_STK TaskPassStk[TASK_STK_SIZE];
static OS_STK TaskDenyStk[TASK_STK_SIZE];


// Message Que
static OS_EVENT *temperQue;
static void *msg[10];

// Event Flags
static OS_FLAG_GRP *flagGroup;
const static OS_FLAGS FLAG_INIT = 0;
const static OS_FLAGS FLAG_DETECT = 1;
const static OS_FLAGS FLAG_DETECT_NOT = 2;
const static OS_FLAGS FLAG_TEMPER_NORMAL = 4;
const static OS_FLAGS FLAG_TEMPER_HIGH = 8;
const static OS_FLAGS FLAG_TEMPER_LOW = 16;

// TODO("Check OS FLAG")
//#define OS_FLAGS_NBITS 8
//#define OS_FLAG_EN 1

const static int DELAY_TIME = 100;

#if ((OS_PROBE_EN == DEF_ENABLED) &&  \
	 (PROBE_COM_EN == DEF_ENABLED) && \
	 (PROBE_COM_STAT_EN == DEF_ENABLED))
static CPU_FP32 ProbeComRxPktSpd;
static CPU_FP32 ProbeComTxPktSpd;
static CPU_FP32 ProbeComTxSymSpd;
static CPU_FP32 ProbeComTxSymByteSpd;

static CPU_INT32U ProbeComRxPktLast;
static CPU_INT32U ProbeComTxPktLast;
static CPU_INT32U ProbeComTxSymLast;
static CPU_INT32U ProbeComTxSymByteLast;

static CPU_INT32U ProbeComCtrLast;
#endif

#if (OS_PROBE_EN == DEF_ENABLED)
static CPU_INT32U ProbeCounts;
static CPU_BOOLEAN ProbeB1;

#endif

/*
 *********************************************************************************************************
 *                                      LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************
 */

// Task function
static void TaskDetect(void *p);
static void TaskTemper(void *p);
static void TaskPass(void *p);
static void TaskDeny(void *p);

// Original function
static void TaskCreate(void);
static void EventCreate(void);

static void TaskStart(void *p_arg);
static void TaskUserIF(void *p_arg);
static void TaskKbd(void *p_arg);

static void DispScr_SignOn(void);
static void DispScr_TaskNames(void);

#if ((PROBE_COM_EN == DEF_ENABLED) || \
	 (OS_PROBE_EN == DEF_ENABLED))
static void InitProbe(void);
#endif

#if (OS_PROBE_EN == DEF_ENABLED)
static void ProbeCallback(void);
#endif

/*
 *********************************************************************************************************
 *                                                main()
 *
 * Description : This is the standard entry point for C code.  It is assumed that your code will call
 *               main() once you have performed all necessary initialization.
 *
 * Argument(s) : none.
 *
 * Return(s)   : none.
 *********************************************************************************************************
 */

int main(void)
{
	CPU_INT08U os_err;

	/* Disable all ints until we are ready to accept them.  */
	BSP_IntDisAll();

	/* Initialize "uC/OS-II, The Real-Time Kernel".         */
	OSInit();

	// Create Message Que, msg : 저장공간, 크기 : 10
	temperQue = OSQCreate(msg, 10);

	// Create Event Flag
	flagGroup = OSFlagCreate(FLAG_INIT, os_err);

	os_err = OSTaskCreateExt((void (*)(void *))TaskDetect,					   // Task가 수행할 함수, 사람의 존재 유/무를 알려주는 Task
							 (void *)0,											   // Task로 넘겨줄 인자
							 (OS_STK *)&TaskDetectStk[TASK_STK_SIZE - 1],  // Task가 할당될 Stack의 Top을 가리키는 주소
							 (INT8U)TASK_DETECT_PRIO,						   // Task의 우선 순위 (MPT)
							 (INT16U)TASK_DETECT_PRIO,						   // Task를 지칭하는 유일한 식별자, Task 갯수의 극복을 위해서 사용할 예정, 현재는 우선 순위와 같게끔 설정
							 (OS_STK *)&TaskDetectStk,						   // Task가 할당될 Stack의 마지막을 가리키는 주소, Stack 검사용으로 사용
							 (INT32U)TASK_STK_SIZE,							   // Task Stack의 크기를 의미
							 (void *)0,											   // Task Control Block 활용시 사용
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK)); // Task 생성 옵션

	os_err = OSTaskCreateExt((void (*)(void *))TaskTemper, // 사람의 온도를 측정하여 통과할지 말지를 결정하는 Task
							 (void *)0,
							 (OS_STK *)&TaskTemperStk[TASK_STK_SIZE - 1],
							 (INT8U)TASK_TEMPER_PRIO,
							 (INT16U)TASK_TEMPER_PRIO,
							 (OS_STK *)&TaskTemperStk,
							 (INT32U)TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

	os_err = OSTaskCreateExt((void (*)(void *))TaskPass, // 정상체온인 사람은 통과를 허가하는 Task
							 (void *)0,
							 (OS_STK *)&TaskPassStk[TASK_STK_SIZE - 1],
							 (INT8U)TASK_PASS_PRIO,
							 (INT16U)TASK_PASS_PRIO,
							 (OS_STK *)&TaskPASSStk,
							 (INT32U)TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

	os_err = OSTaskCreateExt((void (*)(void *))TaskDeny, // 비정상체온인 사람은 통과를 불허하는 Task
							 (void *)0,
							 (OS_STK *)&TaskDenyStk[TASK_STK_SIZE - 1],
							 (INT8U)TASK_DENY_PRIO,
							 (INT16U)TASK_DENY_PRIO,
							 (OS_STK *)&TaskDeNYStk,
							 (INT32U)TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

#if (OS_TASK_NAME_SIZE >= 11)
	OSTaskNameSet(TASK_START_PRIO, (CPU_INT08U *)"Start Task", &os_err);
#endif

	OSStart(); /* Start multitasking (i.e. give control to uC/OS-II).  */

	return (0);
}

/*
 *********************************************************************************************************
 *                                          TaskDetect()
 *
 * Description : Human detecting task. Monitor the existence of people,
 *
 * Argument(s) : p_arg       Argument passed to 'TaskStart()' by 'OSTaskCreate()'.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

// Task가 수행할 함수, 사람의 존재 유/무를 알려주는 Task
static void TaskDetect(void *p)
{
	CPU_INT08U err;


	while (DEF_TRUE)
	{
		if (GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_0) != 0) // when human detected
		{
			OSFlagPost(flagGroup, FLAG_DETECT, OS_FLAG_SET, *err);
		}
		OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
	}
}

/*
 *********************************************************************************************************
 *                                            TaskTemper()
 *
 * Description : Measure a person's temperature
 *
 * Argument(s) : p_arg       Argument passed to 'TaskKbd()' by 'OSTaskCreate()'.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */
// 사람의 온도를 측정하여 통과할지 말지를 결정하는 Task
static void TaskTemper(void *p)
{
	CPU_INT08U err;
	int temp;
	while (DEF_TRUE)
	{
		OSFlagPend(flagGroup, FLAG_DETECT, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, &err);

		temp = TODO("Get temperature");
		if (temp > high) // when temperature is HIGH
		{
			OSQPost(temperQue, temp);
			OSFlagPost(flagGroup, FLAG_TEMPER_HIGH, OS_FLAG_SET, *err);
		}
		else if (temp < low)
		{
			OSQPost(temperQue, temp);
			OSFlagPost(flagGroup, FLAG_TEMPER_LOW, OS_FLAG_SET, *err);
		}
		else
		{
			OSFlagPost(flagGroup, FLAG_TEMPER_NORMAL, OS_FLAG_SET, *err);
		}

		OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
	}
}

/*
 *********************************************************************************************************
 *                                            TaskPass()
 *
 * Description : Those who are at normal body temperature are allowed to pass.
 *
 * Argument(s) : p_arg       Argument passed to 'TaskKbd()' by 'OSTaskCreate()'.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */
// 정상체온인 사람은 통과를 허가하는 Task
static void TaskPass(void *p)
{
	CPU_INT08U err;
	while (DEF_TRUE)
	{
		OSFlagPend(flagGroup, FLAG_TEMPER_NORMAL, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, &err);
		TODO("dot-matrix pass");
		TODO("piezo pass");
		TODO("door open");					// 문 언제 닫음? 열린 뒤 일정 시간 후 문 닫는 태스크 추가 고려
		OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
	}
}

/*
 *********************************************************************************************************
 *                                            TaskDeny()
 *
 * Description : People with abnormal body temperature are not allowed to pass through.
 *
 * Argument(s) : p_arg       Argument passed to 'TaskKbd()' by 'OSTaskCreate()'.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */
// 비정상체온인 사람은 통과를 불허하는 Task
static void TaskDeny(void *p)
{
	CPU_INT08U err;
	int temp = 0;
	while (DEF_TRUE)
	{
		OSFlagPend(flagGroup,
				   FLAG_TEMPER_HIGH + FLAG_TEMPER_LOW,
				   OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME,
				   &err);
		temp = OSQPend(temperQue, 0, &err);
		TODO("dot-matrix deny"); // + 온도 출력 (가능하다면) ㅋㅋ
		TODO("piezo deny");
		OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
	}
}

/*
 *********************************************************************************************************
 *                                            TaskCreate()
 *
 * Description : Create the application tasks.
 *
 * Argument(s) : none.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : TaskStart().
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

/*
 *********************************************************************************************************
 *                                          DispScr_SignOn()
 *
 * Description : Display uC/OS-II system information on the LCD.
 *
 * Argument(s) : none.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : TaskUserIF().
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

static void DispScr_SignOn(void)
{
}

/*
 *********************************************************************************************************
 *                                          DispScr_SignOn()
 *
 * Description : Display uC/OS-II system information on the LCD.
 *
 * Argument(s) : none.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : TaskUserIF().
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

static void DispScr_TaskNames(void)
{
}

/*
 *********************************************************************************************************
 *                                             InitProbe()
 *
 * Description : Initialize uC/Probe target code.
 *
 * Argument(s) : none.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : TaskStart().
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

#if ((PROBE_COM_EN == DEF_ENABLED) || \
	 (OS_PROBE_EN == DEF_ENABLED))
static void InitProbe(void)
{
#if (OS_PROBE_EN == DEF_ENABLED)
	(void)ProbeCounts;
	(void)ProbeB1;

#if ((PROBE_COM_EN == DEF_ENABLED) && \
	 (PROBE_COM_STAT_EN == DEF_ENABLED))
	(void)ProbeComRxPktSpd;
	(void)ProbeComTxPktSpd;
	(void)ProbeComTxSymSpd;
	(void)ProbeComTxSymByteSpd;
#endif

	OSProbe_Init();
	OSProbe_SetCallback(ProbeCallback);
	OSProbe_SetDelay(250);
#endif

#if (PROBE_COM_EN == DEF_ENABLED)
	ProbeCom_Init(); /* Initialize the uC/Probe communications module.       */
#endif
}
#endif

/*
 *********************************************************************************************************
 *                                         AppProbeCallback()
 *
 * Description : uC/Probe OS plugin callback.
 *
 * Argument(s) : none.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : uC/Probe OS plugin task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

#if (OS_PROBE_EN == DEF_ENABLED)
static void ProbeCallback(void)
{
#if ((PROBE_COM_EN == DEF_ENABLED) && \
	 (PROBE_COM_STAT_EN == DEF_ENABLED))
	CPU_INT32U ctr_curr;
	CPU_INT32U rxpkt_curr;
	CPU_INT32U txpkt_curr;
	CPU_INT32U sym_curr;
	CPU_INT32U symbyte_curr;
#endif

	ProbeCounts++;

	ProbeB1 = BSP_PB_GetStatus(1);

#if ((PROBE_COM_EN == DEF_ENABLED) && \
	 (PROBE_COM_STAT_EN == DEF_ENABLED))
	ctr_curr = OSTime;
	rxpkt_curr = ProbeCom_RxPktCtr;
	txpkt_curr = ProbeCom_TxPktCtr;
	sym_curr = ProbeCom_TxSymCtr;
	symbyte_curr = ProbeCom_TxSymByteCtr;

	if ((ctr_curr - ProbeComCtrLast) >= OS_TICKS_PER_SEC)
	{
		ProbeComRxPktSpd = ((CPU_FP32)(rxpkt_curr - ProbeComRxPktLast) / (ctr_curr - ProbeComCtrLast)) * OS_TICKS_PER_SEC;
		ProbeComTxPktSpd = ((CPU_FP32)(txpkt_curr - ProbeComTxPktLast) / (ctr_curr - ProbeComCtrLast)) * OS_TICKS_PER_SEC;
		ProbeComTxSymSpd = ((CPU_FP32)(sym_curr - ProbeComTxSymLast) / (ctr_curr - ProbeComCtrLast)) * OS_TICKS_PER_SEC;
		ProbeComTxSymByteSpd = ((CPU_FP32)(symbyte_curr - ProbeComTxSymByteLast) / (ctr_curr - ProbeComCtrLast)) * OS_TICKS_PER_SEC;

		ProbeComCtrLast = ctr_curr;
		ProbeComRxPktLast = rxpkt_curr;
		ProbeComTxPktLast = txpkt_curr;
		ProbeComTxSymLast = sym_curr;
		ProbeComTxSymByteLast = symbyte_curr;
	}
#endif
}
#endif

/*
 *********************************************************************************************************
 *                                      FormatDec()
 *
 * Description : Convert a decimal value to ASCII (without leading zeros).
 *
 * Argument(s) : pstr            Pointer to the destination ASCII string.
 *
 *               value           Value to convert (assumes an unsigned value).
 *
 *               digits          The desired number of digits.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : various.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                          uC/OS-II APP HOOKS
 *********************************************************************************************************
 *********************************************************************************************************
 */

#if (OS_HOOKS_EN > 0)
/*
 *********************************************************************************************************
 *                                      TASK CREATION HOOK (APPLICATION)
 *
 * Description : This function is cal when a task is created.
 *
 * Argument(s) : ptcb   is a pointer to the task control block of the task being created.
 *
 * Note(s)     : (1) Interrupts are disabled during this call.
 *********************************************************************************************************
 */

void TaskCreateHook(OS_TCB *ptcb)
{
#if ((OS_PROBE_EN == DEF_ENABLED) && \
	 (OS_PROBE_HOOKS_EN == DEF_ENABLED))
	OSProbe_TaskCreateHook(ptcb);
#endif
}

/*
 *********************************************************************************************************
 *                                    TASK DELETION HOOK (APPLICATION)
 *
 * Description : This function is called when a task is deleted.
 *
 * Argument(s) : ptcb   is a pointer to the task control block of the task being deleted.
 *
 * Note(s)     : (1) Interrupts are disabled during this call.
 *********************************************************************************************************
 */

void TaskDelHook(OS_TCB *ptcb)
{
	(void)ptcb;
}

/*
 *********************************************************************************************************
 *                                      IDLE TASK HOOK (APPLICATION)
 *
 * Description : This function is called by OSTaskIdleHook(), which is called by the idle task.  This hook
 *               has been added to allow you to do such things as STOP the CPU to conserve power.
 *
 * Argument(s) : none.
 *
 * Note(s)     : (1) Interrupts are enabled during this call.
 *********************************************************************************************************
 */

#if OS_VERSION >= 251
void TaskIdleHook(void)
{
}
#endif

/*
 *********************************************************************************************************
 *                                        STATISTIC TASK HOOK (APPLICATION)
 *
 * Description : This function is called by OSTaskStatHook(), which is called every second by uC/OS-II's
 *               statistics task.  This allows your application to add functionality to the statistics task.
 *
 * Argument(s) : none.
 *********************************************************************************************************
 */

void TaskStatHook(void)
{
}

/*
 *********************************************************************************************************
 *                                        TASK SWITCH HOOK (APPLICATION)
 *
 * Description : This function is called when a task switch is performed.  This allows you to perform other
 *               operations during a context switch.
 *
 * Argument(s) : none.
 *
 * Note(s)     : (1) Interrupts are disabled during this call.
 *
 *               (2) It is assumed that the global pointer 'OSTCBHighRdy' points to the TCB of the task that
 *                   will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCur' points to the
 *                  task being switched out (i.e. the preempted task).
 *********************************************************************************************************
 */

#if OS_TASK_SW_HOOK_EN > 0
void TaskSwHook(void)
{
#if ((OS_PROBE_EN == DEF_ENABLED) && \
	 (OS_PROBE_HOOKS_EN == DEF_ENABLED))
	OSProbe_TaskSwHook();
#endif
}
#endif

/*
 *********************************************************************************************************
 *                                     OS_TCBInit() HOOK (APPLICATION)
 *
 * Description : This function is called by OSTCBInitHook(), which is called by OS_TCBInit() after setting
 *               up most of the TCB.
 *
 * Argument(s) : ptcb    is a pointer to the TCB of the task being created.
 *
 * Note(s)     : (1) Interrupts may or may not be ENABLED during this call.
 *********************************************************************************************************
 */

#if OS_VERSION >= 204
void TCBInitHook(OS_TCB *ptcb)
{
	(void)ptcb;
}
#endif

/*
 *********************************************************************************************************
 *                                        TICK HOOK (APPLICATION)
 *
 * Description : This function is called every tick.
 *
 * Argument(s) : none.
 *
 * Note(s)     : (1) Interrupts may or may not be ENABLED during this call.
 *********************************************************************************************************
 */

#if OS_TIME_TICK_HOOK_EN > 0
void TimeTickHook(void)
{
#if ((OS_PROBE_EN == DEF_ENABLED) && \
	 (OS_PROBE_HOOKS_EN == DEF_ENABLED))
	OSProbe_TickHook();
#endif
}
#endif
#endif

static void Init_All()
{
	ADC_InitTypeDef adc_init;
	GPIO_InitTypeDef gpio_init;
	I2C_InitTypeDef i2c_init;
	TIM_TimeBaseInitTypeDef tim_timebase_init;
	TIM_OCInitTypeDef tim_piezo_init;
	TIM_OCInitTypeDef tim_motor_init;
	SPI_InitTypeDef spi_init;

	// CLOCK
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	// PIN
	// ADC
	gpio_init.GPIO_Pin = GPIO_Pin_0;
	gpio_init.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOB, &gpio_init);
	// I2C
	gpio_init.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	gpio_init.GPIO_Mode = GPIO_Mode_AF_OD;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio_init);
	// TIM (PWM)
	// Piezo
	gpio_init.GPIO_Pin = GPIO_Pin_8;
	gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio_init);
	// Motor
	gpio_init.GPIO_Pin = GPIO_Pin_9;
	gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio_init);
	// SPI
	GPIO_Init(GPIOB, &gpio_init);
	gpio_init.GPIO_Pin = GPIO_Pin_12;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &gpio_init);
	gpio_init.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio_init);
	GPIO_SetBits(GPIOB, GPIO_Pin_12) // check

	// CONFIG
	// ADC
	adc_init.ADC_Mode = ADC_Mode_Independent;
	adc_init.ADC_ScanConvMode = DISABLE;
	adc_init.ADC_ContinuousConvMode = ENABLE;
	adc_init.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	adc_init.ADC_DataAlign = ADC_DataAlign_Right;
	adc_init.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &adc_init);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_14, 1, ADC_SampleTime_13Cycles5);
	ADC_Cmd(ADC1, ENABLE);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	// I2C
	i2c_init.I2C_Mode = I2C_Mode_I2C;
	i2c_init.I2C_DutyCycle = I2C_DutyCycle_2;
	i2c_init.I2C_OwnAddress1 = 0;
	i2c_init.I2C_Ack = I2C_Ack_Enable;
	i2c_init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	i2c_init.I2C_ClockSpeed = I2C_Speed; // 5000
	I2C_INIT(I2C1, &i2c_init);
	I2C_Cmd(I2C1, ENABLE);
	// TIM (PWM)
	tim_timebase_init.TIM_Prescaler = (uint16_t)(SystemCoreClock / 1000000) - 1;
	tim_timebase_init.TIM_CounterMode = TIM_CounterMode_Up;
	tim_timebase_init.TIM_Period = 20000 - 1;
	tim_timebase_init.TIM_ClockDivision = 0;
	tim_timebase_init.TIM_RepetitionCounter;
	TIM_TimeBaseInit(TIM4, &time_timebase_init);
	/* PIEZO: PWM1 Mode configuration: Channel3 */
	tim_piezo_init.TIM_OCMode = TIM_OCMODE_PWM1;
	tim_piezo_init.TIM_OutputState = TIM_OUTPUTSTATE_Enable;
	tim_piezo_init.TIM_Pulse = 500;
	tim_piezo_init.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC3Init(TIM4, &tim_piezo_init);
	//TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Disable);
	TIM_Cmd(TIM4, ENABLE);
	/* MOTOR: PWM1 Mode configuration: Channel4 */
	tim_motor_init.TIM_OCMode = TIM_OCMODE_PWM1;
	tim_motor_init.TIM_OutputState = TIM_OUTPUTSTATE_Enable;
	tim_motor_init.TIM_Pulse = 1500;
	tim_motor_init.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_PWMIConfig(TIM4, &tim_motor_init);
	TIM_OC4Init(TIM4, &tim_motor_init);
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Disable);
	TIM_ARRPreloadConfig(Tim4, ENABLE);
	TIM_Cmd(TIM4, ENABLE);
	// SPI
	spi_init.SPI_Direction = SPI_Direction_1Line_Tx;
	spi_init.SPI_Mode = SPI_Mode_Master;
	spi_init.SPI_DataSize = SPI_DataSize_16b;
	spi_init.SPI_CPOL = SPI_CPOL_Low;
	spi_init.SPI_CPHA = SPI_CPHA_1Edge;
	spi_init.SPI_NSS = SPI_NSS_Soft;
	spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
	spi_init.SPI_FirstBit = SPI_FirstBit_MSB;
	spi_init.SPI_CRCPolynomial;
	SPI_Init(SPI2, &spi_init);
	SPI_Cmd(SPI2, ENABLE);
}