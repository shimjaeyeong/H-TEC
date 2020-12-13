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
static OS_STK App_TaskDetectStk[APP_TASK_STK_SIZE];
static OS_STK App_TaskTemperStk[APP_TASK_STK_SIZE];
static OS_STK App_TaskPassStk[APP_TASK_STK_SIZE];
static OS_STK App_TaskDenyStk[APP_TASK_STK_SIZE];
// Original Task Stack
static OS_STK App_TaskStartStk[APP_TASK_STK_SIZE];
static OS_STK App_TaskUserIFStk[APP_TASK_USER_IF_STK_SIZE];
static OS_STK App_TaskKbdStk[APP_TASK_KBD_STK_SIZE];

//static OS_EVENT      *App_UserIFMbox; // 메시지 박스

// Message Que
static OS_EVENT *App_msgQue;
static void *msg[10];

// Event Flags
static OS_FLAG_GRP *flagGroup;
static OS_FLAGS check;
// TODO("Check OS FLAG")
//#define OS_FLAGS_NBITS 8
//#define OS_FLAG_EN 1

#if ((APP_OS_PROBE_EN == DEF_ENABLED) &&  \
	 (APP_PROBE_COM_EN == DEF_ENABLED) && \
	 (PROBE_COM_STAT_EN == DEF_ENABLED))
static CPU_FP32 App_ProbeComRxPktSpd;
static CPU_FP32 App_ProbeComTxPktSpd;
static CPU_FP32 App_ProbeComTxSymSpd;
static CPU_FP32 App_ProbeComTxSymByteSpd;

static CPU_INT32U App_ProbeComRxPktLast;
static CPU_INT32U App_ProbeComTxPktLast;
static CPU_INT32U App_ProbeComTxSymLast;
static CPU_INT32U App_ProbeComTxSymByteLast;

static CPU_INT32U App_ProbeComCtrLast;
#endif

#if (APP_OS_PROBE_EN == DEF_ENABLED)
static CPU_INT32U App_ProbeCounts;
static CPU_BOOLEAN App_ProbeB1;

#endif

/*
 *********************************************************************************************************
 *                                      LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************
 */

// Task function
static void App_TaskDetect(void *p);
static void App_TaskTemper(void *p);
static void App_TaskPass(void *p);
static void App_TaskDeny(void *p);

// Original function
static void App_TaskCreate(void);
static void App_EventCreate(void);

static void App_TaskStart(void *p_arg);
static void App_TaskUserIF(void *p_arg);
static void App_TaskKbd(void *p_arg);

static void App_DispScr_SignOn(void);
static void App_DispScr_TaskNames(void);

#if ((APP_PROBE_COM_EN == DEF_ENABLED) || \
	 (APP_OS_PROBE_EN == DEF_ENABLED))
static void App_InitProbe(void);
#endif

#if (APP_OS_PROBE_EN == DEF_ENABLED)
static void App_ProbeCallback(void);
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

	// Create Message Que
	App_msgQue = OSQCreate(msg, 10);

	// Create Event Flag
	flagGroup = OSFlagCreate(check, os_err);

	os_err = OSTaskCreateExt((void (*)(void *))App_TaskDetect,
							 (void *)0,
							 (OS_STK *)&App_TaskDetectStk[APP_TASK_STK_SIZE - 1],
							 (INT8U)APP_TASK_DETECT_PRIO,
							 (INT16U)APP_TASK_DETECT_PRIO,
							 (OS_STK *)&App_TaskDetectStk,
							 (INT32U)APP_TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

	os_err = OSTaskCreateExt((void (*)(void *))App_TaskTemper,
							 (void *)0,
							 (OS_STK *)&App_TaskTemperStk[APP_TASK_STK_SIZE - 1],
							 (INT8U)APP_TASK_TEMPER_PRIO,
							 (INT16U)APP_TASK_TEMPER_PRIO,
							 (OS_STK *)&App_TaskTemperStk,
							 (INT32U)APP_TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

	os_err = OSTaskCreateExt((void (*)(void *))App_TaskPass,
							 (void *)0,
							 (OS_STK *)&App_TaskPassStk[APP_TASK_STK_SIZE - 1],
							 (INT8U)APP_TASK_PASS_PRIO,
							 (INT16U)APP_TASK_PASS_PRIO,
							 (OS_STK *)&App_TaskPASSStk,
							 (INT32U)APP_TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

	os_err = OSTaskCreateExt((void (*)(void *))App_TaskDeny,
							 (void *)0,
							 (OS_STK *)&App_TaskDenyStk[APP_TASK_STK_SIZE - 1],
							 (INT8U)APP_TASK_DENY_PRIO,
							 (INT16U)APP_TASK_DENY_PRIO,
							 (OS_STK *)&App_TaskDeNYStk,
							 (INT32U)APP_TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

#if (OS_TASK_NAME_SIZE >= 11)
	OSTaskNameSet(APP_TASK_START_PRIO, (CPU_INT08U *)"Start Task", &os_err);
#endif

	OSStart(); /* Start multitasking (i.e. give control to uC/OS-II).  */

	return (0);
}

/*
 *********************************************************************************************************
 *                                          App_TaskStart()
 *
 * Description : The startup task.  The uC/OS-II ticker should only be initialize once multitasking starts.
 *
 * Argument(s) : p_arg       Argument passed to 'App_TaskStart()' by 'OSTaskCreate()'.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */
// LED ������ ���� Task
static void App_TaskDetect(void *p_arg)
{
	CPU_INT08U os_err;

	(void)p_arg;

	BSP_Init();			  /* Initialize BSP functions.                            */
	OS_CPU_SysTickInit(); /* Initialize the SysTick.                              */

#if (OS_TASK_STAT_EN > 0)
	OSStatInit(); /* Determine CPU capacity.                              */
#endif

#if ((APP_PROBE_COM_EN == DEF_ENABLED) || \
	 (APP_OS_PROBE_EN == DEF_ENABLED))
	App_InitProbe();
#endif
	/* Create application events.                           */
	/* Task�� ����� ���� MailBox ����                        */
	App_EventCreate();

	/* Create application tasks.                            */
	/* LCD ���� Task, Ű���� �Է� Task ����                    */
	App_TaskCreate();

	/* Task body, always written as an infinite loop.       */
	while (DEF_TRUE)
	{
		if (ADC_GetConversionValue(ADC1) == 1)
		{
			OSFlagPost(flagGroup, check, OS_FLAG_SET, os_err);
		}
		OSTimeDlyHMSM(0, 0, 0, 100);
	}
}

/*
 *********************************************************************************************************
 *                                             App_EventCreate()
 *
 * Description : Create the application events.
 *
 * Argument(s) : none.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : App_TaskStart().
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

static void App_EventCreate(void)
{
#if (OS_EVENT_NAME_SIZE > 12)
	CPU_INT08U os_err;
#endif

	/* Create MBOX for communication between Kbd and UserIF.*/
	/* Mail Box ����                                         */
	/* ������ ũ���� ������ Task�� Interrupt Service Routine   */
	/* ���� �ٸ� Task ������ �� �����                         */
	App_UserIFMbox = OSMboxCreate((void *)0);
#if (OS_EVENT_NAME_SIZE > 12)
	OSEventNameSet(App_UserIFMbox, "User IF Mbox", &os_err);
#endif
}

/*
 *********************************************************************************************************
 *                                            App_TaskCreate()
 *
 * Description : Create the application tasks.
 *
 * Argument(s) : none.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : App_TaskStart().
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

static void App_TaskCreate(void)
{
	CPU_INT08U os_err;

	// LCD�� ���Ž�Ű�� Task ����
	os_err = OSTaskCreateExt((void (*)(void *))App_TaskUserIF,
							 (void *)0,
							 (OS_STK *)&App_TaskUserIFStk[APP_TASK_USER_IF_STK_SIZE - 1],
							 (INT8U)APP_TASK_USER_IF_PRIO,
							 (INT16U)APP_TASK_USER_IF_PRIO,
							 (OS_STK *)&App_TaskUserIFStk[0],
							 (INT32U)APP_TASK_USER_IF_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

#if (OS_TASK_NAME_SIZE >= 9)
	OSTaskNameSet(APP_TASK_USER_IF_PRIO, "User I/F", &os_err);
#endif
	// Keyboard �Է��� �޴� Task ����
	os_err = OSTaskCreateExt(
		(void (*)(void *))App_TaskKbd,
		(void *)0,
		(OS_STK *)&App_TaskKbdStk[APP_TASK_KBD_STK_SIZE - 1],
		(INT8U)APP_TASK_KBD_PRIO,
		(INT16U)APP_TASK_KBD_PRIO,
		(OS_STK *)&App_TaskKbdStk[0],
		(INT32U)APP_TASK_KBD_STK_SIZE,
		(void *)0,
		(INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
#if (OS_TASK_NAME_SIZE >= 9)
	OSTaskNameSet(APP_TASK_KBD_PRIO, "Keyboard", &os_err);
#endif
}

/*
 *********************************************************************************************************
 *                                            App_TaskKbd()
 *
 * Description : Monitor the state of the push buttons and passes messages to AppTaskUserIF()
 *
 * Argument(s) : p_arg       Argument passed to 'App_TaskKbd()' by 'OSTaskCreate()'.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

// Keyboard �Է��� �޴� Task
static void App_TaskKbd(void *p_arg)
{
	CPU_BOOLEAN b1_prev;
	CPU_BOOLEAN b1;
	CPU_INT08U key;

	(void)p_arg;

	b1_prev = DEF_FALSE;
	key = 1;

	while (DEF_TRUE)
	{
		b1 = BSP_PB_GetStatus(BSP_PB_ID_KEY);

		if ((b1 == DEF_TRUE) && (b1_prev == DEF_FALSE))
		{
			if (key == 2)
			{
				key = 1;
			}
			else
			{
				key++;
			}
			// MailBox�� Task���� �Է¹��� �� Key�� ����
			OSMboxPost(App_UserIFMbox, (void *)key);
		}

		b1_prev = b1;

		OSTimeDlyHMSM(0, 0, 0, 20);
	}
}

/*
 *********************************************************************************************************
 *                                            App_TaskUserIF()
 *
 * Description : Updates LCD.
 *
 * Argument(s) : p_arg       Argument passed to 'App_TaskUserIF()' by 'OSTaskCreate()'.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

// LCD ������ ���� Task
static void App_TaskUserIF(void *p_arg)
{
	CPU_INT08U *msg;
	CPU_INT08U err;
	CPU_INT32U nstate;
	CPU_INT32U pstate;

	(void)p_arg;

	App_DispScr_SignOn();
	OSTimeDlyHMSM(0, 0, 1, 0);
	nstate = 1;
	pstate = 1;

	while (DEF_TRUE)
	{
		// �ٸ� Task���� Mailbox�� ������ ����� ���� ����
		msg = (CPU_INT08U *)(OSMboxPend(App_UserIFMbox, OS_TICKS_PER_SEC / 10, &err));
		if (err == OS_NO_ERR)
		{
			nstate = (CPU_INT32U)msg;
		}

		if (nstate != pstate)
		{
			pstate = nstate;
		}

		switch (nstate)
		{
		case 2:
			App_DispScr_TaskNames();
			break;

		case 1:
		default:
			App_DispScr_SignOn();
			break;
		}
	}
}

/*
 *********************************************************************************************************
 *                                          App_DispScr_SignOn()
 *
 * Description : Display uC/OS-II system information on the LCD.
 *
 * Argument(s) : none.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : App_TaskUserIF().
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

static void App_DispScr_SignOn(void)
{
}

/*
 *********************************************************************************************************
 *                                          App_DispScr_SignOn()
 *
 * Description : Display uC/OS-II system information on the LCD.
 *
 * Argument(s) : none.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : App_TaskUserIF().
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

static void App_DispScr_TaskNames(void)
{
}

/*
 *********************************************************************************************************
 *                                             App_InitProbe()
 *
 * Description : Initialize uC/Probe target code.
 *
 * Argument(s) : none.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : App_TaskStart().
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

#if ((APP_PROBE_COM_EN == DEF_ENABLED) || \
	 (APP_OS_PROBE_EN == DEF_ENABLED))
static void App_InitProbe(void)
{
#if (APP_OS_PROBE_EN == DEF_ENABLED)
	(void)App_ProbeCounts;
	(void)App_ProbeB1;

#if ((APP_PROBE_COM_EN == DEF_ENABLED) && \
	 (PROBE_COM_STAT_EN == DEF_ENABLED))
	(void)App_ProbeComRxPktSpd;
	(void)App_ProbeComTxPktSpd;
	(void)App_ProbeComTxSymSpd;
	(void)App_ProbeComTxSymByteSpd;
#endif

	OSProbe_Init();
	OSProbe_SetCallback(App_ProbeCallback);
	OSProbe_SetDelay(250);
#endif

#if (APP_PROBE_COM_EN == DEF_ENABLED)
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

#if (APP_OS_PROBE_EN == DEF_ENABLED)
static void App_ProbeCallback(void)
{
#if ((APP_PROBE_COM_EN == DEF_ENABLED) && \
	 (PROBE_COM_STAT_EN == DEF_ENABLED))
	CPU_INT32U ctr_curr;
	CPU_INT32U rxpkt_curr;
	CPU_INT32U txpkt_curr;
	CPU_INT32U sym_curr;
	CPU_INT32U symbyte_curr;
#endif

	App_ProbeCounts++;

	App_ProbeB1 = BSP_PB_GetStatus(1);

#if ((APP_PROBE_COM_EN == DEF_ENABLED) && \
	 (PROBE_COM_STAT_EN == DEF_ENABLED))
	ctr_curr = OSTime;
	rxpkt_curr = ProbeCom_RxPktCtr;
	txpkt_curr = ProbeCom_TxPktCtr;
	sym_curr = ProbeCom_TxSymCtr;
	symbyte_curr = ProbeCom_TxSymByteCtr;

	if ((ctr_curr - App_ProbeComCtrLast) >= OS_TICKS_PER_SEC)
	{
		App_ProbeComRxPktSpd = ((CPU_FP32)(rxpkt_curr - App_ProbeComRxPktLast) / (ctr_curr - App_ProbeComCtrLast)) * OS_TICKS_PER_SEC;
		App_ProbeComTxPktSpd = ((CPU_FP32)(txpkt_curr - App_ProbeComTxPktLast) / (ctr_curr - App_ProbeComCtrLast)) * OS_TICKS_PER_SEC;
		App_ProbeComTxSymSpd = ((CPU_FP32)(sym_curr - App_ProbeComTxSymLast) / (ctr_curr - App_ProbeComCtrLast)) * OS_TICKS_PER_SEC;
		App_ProbeComTxSymByteSpd = ((CPU_FP32)(symbyte_curr - App_ProbeComTxSymByteLast) / (ctr_curr - App_ProbeComCtrLast)) * OS_TICKS_PER_SEC;

		App_ProbeComCtrLast = ctr_curr;
		App_ProbeComRxPktLast = rxpkt_curr;
		App_ProbeComTxPktLast = txpkt_curr;
		App_ProbeComTxSymLast = sym_curr;
		App_ProbeComTxSymByteLast = symbyte_curr;
	}
#endif
}
#endif

/*
 *********************************************************************************************************
 *                                      App_FormatDec()
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

#if (OS_APP_HOOKS_EN > 0)
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

void App_TaskCreateHook(OS_TCB *ptcb)
{
#if ((APP_OS_PROBE_EN == DEF_ENABLED) && \
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

void App_TaskDelHook(OS_TCB *ptcb)
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
void App_TaskIdleHook(void)
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

void App_TaskStatHook(void)
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
void App_TaskSwHook(void)
{
#if ((APP_OS_PROBE_EN == DEF_ENABLED) && \
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
void App_TCBInitHook(OS_TCB *ptcb)
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
void App_TimeTickHook(void)
{
#if ((APP_OS_PROBE_EN == DEF_ENABLED) && \
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
	tim_timebase_init.TIM_Prescaler = 400 - 1;
	tim_timebase_init.TIM_CounterMode = TIM_CounterMode_Up;
	tim_timebase_init.TIM_Period = 1000 - 1;
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