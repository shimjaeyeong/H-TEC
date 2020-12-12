/*
 *********************************************************************************************************
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
 *                                            EXAMPLE CODE
 *
 *                                     ST Microelectronics STM32
 *                                              with the
 *                                   STM3210B-EVAL Evaluation Board
 *
 * Filename      : app.c
 * Version       : V1.10
 * Programmer(s) : BAN
 *********************************************************************************************************
 */


/*
 *********************************************************************************************************
 *                                             INCLUDE FILES
 *********************************************************************************************************
 */

#include <includes.h>
#include  <stm32f10x_tim.h>
#include  <stm32f10x_gpio.h>
#include  <stm32f10x_rcc.h>
#include  <stm32f10x_adc.h>
#include  <stm32f10x_i2c.h>
#include  <stm32f10x_spi.h>


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

static OS_STK App_TaskStartStk[APP_TASK_START_STK_SIZE];
static OS_STK App_TaskUserIFStk[APP_TASK_USER_IF_STK_SIZE];
static OS_STK App_TaskKbdStk[APP_TASK_KBD_STK_SIZE];
// stack (size = ATSSS:128)
static OS_STK tempTaskStartStk[APP_TASK_START_STK_SIZE];
static OS_STK detectTaskStartStk[APP_TASK_START_STK_SIZE];
static OS_STK matrixTaskStartStk[APP_TASK_START_STK_SIZE];
static OS_STK piezoTaskStartStk[APP_TASK_START_STK_SIZE];
static OS_STK motorTaskStartStk[APP_TASK_START_STK_SIZE];

static OS_EVENT      *App_UserIFMbox;
volatile static int tempVal = 0;
volatile static int detactVal = 0;
volatile static int matrix;
volatile static int piezoVal = 0;
volatile static int motorVal = 0;


#if ((APP_OS_PROBE_EN == DEF_ENABLED) && \
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

static void  App_TaskCreate(void);
static void  App_EventCreate(void);

static void  App_TaskStart(void        *p_arg);
static void  App_TaskUserIF(void        *p_arg);
static void  App_TaskKbd(void        *p_arg);

static void  App_DispScr_SignOn(void);
static void  App_DispScr_TaskNames(void);
// INIT
static void RCC_Configure(void);
static void GPIO_Configure(void);
static void ADC_Configure(void);
// TASK
static void tempTask(void *p);
static void detectTask(void *p);
static void matrixTask(void *p);
static void piezoTask(void *p);
static void motorTask(void *p);

#if ((APP_PROBE_COM_EN == DEF_ENABLED) || \
	(APP_OS_PROBE_EN == DEF_ENABLED))
static void  App_InitProbe(void);
#endif

#if (APP_OS_PROBE_EN == DEF_ENABLED)
static void  App_ProbeCallback(void);
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

int  main(void)
{
	CPU_INT08U os_err;

	/* Disable all ints until we are ready to accept them.  */
	BSP_IntDisAll();


	/* Initialize "uC/OS-II, The Real-Time Kernel".         */
	OSInit();

	/* Create the start task.                               */
	/* OSTaskCreatExt()                                     */
	os_err = OSTaskCreateExt((void (*)(void *))App_TaskStart, 
				 (void* )0,                     
				 (OS_STK* )&App_TaskStartStk[APP_TASK_START_STK_SIZE - 1],     
				 (INT8U           )APP_TASK_START_PRIO,
				 (INT16U          )APP_TASK_START_PRIO,
				 (OS_STK* )&App_TaskStartStk[0],
				 (INT32U          )APP_TASK_START_STK_SIZE,
				 (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

#if (OS_TASK_NAME_SIZE >= 11)
	OSTaskNameSet(APP_TASK_START_PRIO, (CPU_INT08U*)"Start Task", &os_err);
#endif

	OSStart();                                              /* Start multitasking (i.e. give control to uC/OS-II).  */

	return(0);
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
static void  App_TaskStart(void *p_arg)
{
	CPU_INT32U i;
	CPU_INT32U j;
	CPU_INT32U dly;


	(void)p_arg;

	BSP_Init();                                             /* Initialize BSP functions.                            */
	OS_CPU_SysTickInit();                                   /* Initialize the SysTick.                              */

#if (OS_TASK_STAT_EN > 0)
	OSStatInit();                                           /* Determine CPU capacity.                              */
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
	while (DEF_TRUE) {
		for (j = 0; j < 4; j++) {
			for (i = 1; i <= 4; i++) {
				BSP_LED_On(i); // LED ON
				dly = (BSP_ADC_GetStatus(1) >> 4) + 2; // ADC ���� �޾ƿ� Delay ũ�� ����
				OSTimeDlyHMSM(0, 0, 0, dly); // Delay��ŭ �ٸ� Task�� ������ ���� �ڿ� �纸
				BSP_LED_Off(i); // LED OFF
				dly = (BSP_ADC_GetStatus(1) >> 4) + 2;
				OSTimeDlyHMSM(0, 0, 0, dly);
			}

			for (i = 3; i >= 2; i--) {
				BSP_LED_On(i);
				dly = (BSP_ADC_GetStatus(1) >> 4) + 2;
				OSTimeDlyHMSM(0, 0, 0, dly);
				BSP_LED_Off(i);
				dly = (BSP_ADC_GetStatus(1) >> 4) + 2;
				OSTimeDlyHMSM(0, 0, 0, dly);
			}
		}

		for (i = 0; i < 4; i++) {
			BSP_LED_On(0);
			dly = (BSP_ADC_GetStatus(1) >> 4) + 2;
			OSTimeDlyHMSM(0, 0, 0, dly * 3);
			BSP_LED_Off(0);
			dly = (BSP_ADC_GetStatus(1) >> 4) + 2;
			OSTimeDlyHMSM(0, 0, 0, dly * 3);
		}
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

static void  App_EventCreate(void)
{
#if (OS_EVENT_NAME_SIZE > 12)
	CPU_INT08U os_err;
#endif

	/* Create MBOX for communication between Kbd and UserIF.*/
	/* Mail Box ����                                         */
	/* ������ ũ���� ������ Task�� Interrupt Service Routine   */
	/* ���� �ٸ� Task ������ �� �����                         */
	App_UserIFMbox = OSMboxCreate((void*)0);
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

static void  App_TaskCreate(void)
{
	CPU_INT08U os_err;

	// LCD�� ���Ž�Ű�� Task ����
	os_err = OSTaskCreateExt((void (*)(void *))App_TaskUserIF,
				 (void* )0,
				 (OS_STK* )&App_TaskUserIFStk[APP_TASK_USER_IF_STK_SIZE - 1],
				 (INT8U           )APP_TASK_USER_IF_PRIO,
				 (INT16U          )APP_TASK_USER_IF_PRIO,
				 (OS_STK* )&App_TaskUserIFStk[0],
				 (INT32U          )APP_TASK_USER_IF_STK_SIZE,
				 (void* )0,
				 (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

#if (OS_TASK_NAME_SIZE >= 9)
	OSTaskNameSet(APP_TASK_USER_IF_PRIO, "User I/F", &os_err);
#endif
	// Keyboard �Է��� �޴� Task ����
	os_err = OSTaskCreateExt((void (*)(void *))App_TaskKbd,
				 (void* )0,
				 (OS_STK* )&App_TaskKbdStk[APP_TASK_KBD_STK_SIZE - 1],
				 (INT8U           )APP_TASK_KBD_PRIO,
				 (INT16U          )APP_TASK_KBD_PRIO,
				 (OS_STK* )&App_TaskKbdStk[0],
				 (INT32U          )APP_TASK_KBD_STK_SIZE,
				 (void* )0,
				 (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
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
static void  App_TaskKbd(void *p_arg)
{
	CPU_BOOLEAN b1_prev;
	CPU_BOOLEAN b1;
	CPU_INT08U key;


	(void)p_arg;

	b1_prev = DEF_FALSE;
	key = 1;

	while (DEF_TRUE) {
		b1 = BSP_PB_GetStatus(BSP_PB_ID_KEY);

		if ((b1 == DEF_TRUE) && (b1_prev == DEF_FALSE)) {
			if (key == 2) {
				key = 1;
			} else {
				key++;
			}
			// MailBox�� Task���� �Է¹��� �� Key�� ����
			OSMboxPost(App_UserIFMbox, (void*)key);
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
static void  App_TaskUserIF(void *p_arg)
{
	CPU_INT08U  *msg;
	CPU_INT08U err;
	CPU_INT32U nstate;
	CPU_INT32U pstate;


	(void)p_arg;


	App_DispScr_SignOn();
	OSTimeDlyHMSM(0, 0, 1, 0);
	nstate = 1;
	pstate = 1;


	while (DEF_TRUE) {
		// �ٸ� Task���� Mailbox�� ������ ����� ���� ����
		msg = (CPU_INT08U*)(OSMboxPend(App_UserIFMbox, OS_TICKS_PER_SEC / 10, &err));
		if (err == OS_NO_ERR) {
			nstate = (CPU_INT32U)msg;
		}

		if (nstate != pstate) {
			pstate = nstate;
		}

		switch (nstate) {
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

static void  App_DispScr_SignOn(void)
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

static void  App_DispScr_TaskNames(void)
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
static void  App_InitProbe(void)
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
	ProbeCom_Init();                                        /* Initialize the uC/Probe communications module.       */
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
static void  App_ProbeCallback(void)
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

	if ((ctr_curr - App_ProbeComCtrLast) >= OS_TICKS_PER_SEC) {
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

void  App_TaskCreateHook(OS_TCB *ptcb)
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

void  App_TaskDelHook(OS_TCB *ptcb)
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
void  App_TaskIdleHook(void)
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

void  App_TaskStatHook(void)
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
void  App_TaskSwHook(void)
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
void  App_TCBInitHook(OS_TCB *ptcb)
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
void  App_TimeTickHook(void)
{
#if ((APP_OS_PROBE_EN == DEF_ENABLED) && \
	(OS_PROBE_HOOKS_EN == DEF_ENABLED))
	OSProbe_TickHook();
#endif
}
#endif
#endif
