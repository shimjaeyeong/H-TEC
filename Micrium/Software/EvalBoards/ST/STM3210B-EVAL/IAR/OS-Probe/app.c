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
static OS_STK detectTaskStack[TASK_STK_SIZE];
static OS_STK temperatureTaskStack[TASK_STK_SIZE];
static OS_STK passTaskStack[TASK_STK_SIZE];
static OS_STK denyTaskStack[TASK_STK_SIZE];
static OS_STK checkTaskStack[TASK_STK_SIZE];
static OS_STK startTaskStack[TASK_STK_SIZE];

// Event Flags
static OS_FLAG_GRP *flagGroup;
const static int FLAG_INIT = 0;
const static int FLAG_DETECT = 1;
const static int FLAG_DETECT_NOT = 2;
const static int FLAG_TEMPER_NORMAL = 4;
const static int FLAG_TEMPER_HIGH = 8;
const static int FLAG_TEMPER_LOW = 16;

// Que
static OS_EVENT *tempQue;
static void *tempBuffer[10];

// time
//static OS_EVENT *sem;
static int count = 0;
static int check = 0;
const static int DELAY_TIME = 1000;

static int ADC_value = 0;
/*
static GPIO_TypeDef *orangeTypes[8] = {GPIOC, GPIOA, GPIOA, GPIOA, GPIOB, GPIOC, GPIOA, GPIOA};
const static int orangePins[8] = {GPIO_Pin_7, GPIO_Pin_8, GPIO_Pin_11, GPIO_Pin_14, GPIO_Pin_1, GPIO_Pin_4, GPIO_Pin_5, GPIO_Pin_2};

static GPIO_TypeDef *greenTypes[8] = {GPIOC, GPIOC, GPIOA, GPIOA, GPIOB, GPIOC, GPIOA, GPIOA};
const static int greenPins[8] = {GPIO_Pin_6, GPIO_Pin_9, GPIO_Pin_10, GPIO_Pin_13, GPIO_Pin_2, GPIO_Pin_5, GPIO_Pin_6, GPIO_Pin_3};

static GPIO_TypeDef *lineTypes[8] = {GPIOC, GPIOA, GPIOA, GPIOA, GPIOB, GPIOA, GPIOA, GPIOA};
const static int linePins[8] = {GPIO_Pin_8, GPIO_Pin_9, GPIO_Pin_12, GPIO_Pin_15, GPIO_Pin_0, GPIO_Pin_7, GPIO_Pin_4, GPIO_Pin_1};

const static char shapeX[8][8] = {
	{1, 0, 0, 0, 0, 0, 0, 1},
	{0, 1, 0, 0, 0, 0, 1, 0},
	{0, 0, 1, 0, 0, 1, 0, 0},
	{0, 0, 0, 1, 1, 0, 0, 0},
	{0, 0, 0, 1, 1, 0, 0, 0},
	{0, 0, 1, 0, 0, 1, 0, 0},
	{0, 1, 0, 0, 0, 0, 1, 0},
	{1, 0, 0, 0, 0, 0, 0, 1}};
const static char shapeO[8][8] = {
	{0, 0, 0, 1, 1, 0, 0, 0},
	{0, 1, 1, 0, 0, 1, 1, 0},
	{0, 1, 0, 0, 0, 0, 1, 0},
	{1, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 1},
	{0, 1, 0, 0, 0, 0, 1, 0},
	{0, 1, 1, 0, 0, 1, 1, 0},
	{1, 0, 0, 1, 1, 0, 0, 1}};
*/
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
static void detectTask(void *p);
static void temperTask(void *p);
static void passTask(void *p);
static void denyTask(void *p);
static void checkTask(void *p);
static void startTask(void *p);

static void App_DispScr_SignOn(void);
static void DispScr_TaskNames(void);

static int readTemperature(void);
static void stopAlert();
static void startAlert();
static void stopNotice();
static void startNotice();
static void initAll();

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

	initAll();

	os_err = OSTaskCreateExt((void (*)(void *))detectTask,						   // Task가 수행할 함수, 사람의 존재 유/무를 알려주는 Task
							 (void *)0,											   // Task로 넘겨줄 인자
							 (OS_STK *)&detectTaskStack[TASK_STK_SIZE - 1],		   // Task가 할당될 Stack의 Top을 가리키는 주소
							 (INT8U)TASK_DETECT_PRIO,							   // Task의 우선 순위 (MPT)
							 (INT16U)TASK_DETECT_PRIO,							   // Task를 지칭하는 유일한 식별자, Task 갯수의 극복을 위해서 사용할 예정, 현재는 우선 순위와 같게끔 설정
							 (OS_STK *)&detectTaskStack,						   // Task가 할당될 Stack의 마지막을 가리키는 주소, Stack 검사용으로 사용
							 (INT32U)TASK_STK_SIZE,								   // Task Stack의 크기를 의미
							 (void *)0,											   // Task Control Block 활용시 사용
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK)); // Task 생성 옵션

	os_err = OSTaskCreateExt((void (*)(void *))temperTask, // 사람의 온도를 측정하여 통과할지 말지를 결정하는 Task
							 (void *)0,
							 (OS_STK *)&temperatureTaskStack[TASK_STK_SIZE - 1],
							 (INT8U)TASK_TEMPER_PRIO,
							 (INT16U)TASK_TEMPER_PRIO,
							 (OS_STK *)&temperatureTaskStack,
							 (INT32U)TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

	os_err = OSTaskCreateExt((void (*)(void *))passTask, // 정상체온인 사람은 통과를 허가하는 Task
							 (void *)0,
							 (OS_STK *)&passTaskStack[TASK_STK_SIZE - 1],
							 (INT8U)TASK_PASS_PRIO,
							 (INT16U)TASK_PASS_PRIO,
							 (OS_STK *)&passTaskStack,
							 (INT32U)TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

	os_err = OSTaskCreateExt((void (*)(void *))denyTask, // 비정상체온인 사람은 통과를 불허하는 Task
							 (void *)0,
							 (OS_STK *)&denyTaskStack[TASK_STK_SIZE - 1],
							 (INT8U)TASK_DENY_PRIO,
							 (INT16U)TASK_DENY_PRIO,
							 (OS_STK *)&denyTaskStack,
							 (INT32U)TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

	os_err = OSTaskCreateExt((void (*)(void *))checkTask, // 알림 장치 작동 중지하는 Task
							 (void *)0,
							 (OS_STK *)&checkTaskStack[TASK_STK_SIZE - 1],
							 (INT8U)TASK_CHECK_PRIO,
							 (INT16U)TASK_CHECK_PRIO,
							 (OS_STK *)&checkTaskStack,
							 (INT32U)TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

	os_err = OSTaskCreateExt((void (*)(void *))startTask, // 초기화 일회용 Task
							 (void *)0,
							 (OS_STK *)&startTaskStack[TASK_STK_SIZE - 1],
							 (INT8U)TASK_START_PRIO,
							 (INT16U)TASK_START_PRIO,
							 (OS_STK *)startTaskStack,
							 (INT32U)TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

/*	os_err = OSTaskCreateExt((void (*)(void *))displayTask, // dot-matrix 표시하는 Task
							 (void *)0,
							 (OS_STK *)&displayTaskStack[TASK_STK_SIZE - 1],
							 (INT8U)TASK_DISPLAY_PRIO,
							 (INT16U)TASK_DISPLAY_PRIO,
							 (OS_STK *)&displayTaskStack,
							 (INT32U)TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
*/
#if (OS_TASK_NAME_SIZE >= 11)
	OSTaskNameSet(TASK_DETECT_PRIO, (CPU_INT08U *)"Detect Task", &os_err);
	OSTaskNameSet(TASK_TEMPER_PRIO, (CPU_INT08U *)"Temperature Task", &os_err);
	OSTaskNameSet(TASK_PASS_PRIO, (CPU_INT08U *)"Pass Task", &os_err);
	OSTaskNameSet(TASK_DENY_PRIO, (CPU_INT08U *)"Deny Task", &os_err);
	OSTaskNameSet(TASK_CHECK_PRIO, (CPU_INT08U *)"Check Task", &os_err);
	OSTaskNameSet(TASK_START_PRIO, (CPU_INT08U *)"Start Task", &os_err);
	//OSTaskNameSet(TASK_DISPLAY_PRIO, (CPU_INT08U *)"Display Task", &os_err);
#endif

	OSStart(); /* Start multitasking (i.e. give control to uC/OS-II).  */

	return (0);
}

/*
 *********************************************************************************************************
 *                                          detectTask()
 *
 * Description : Human detecting task. Monitor the existence of people,
 *
 * Argument(s) : p
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

// Task가 수행할 함수, 사람의 존재 유/무를 알려주는 Task
static void detectTask(void *p)
{
	CPU_INT08U err;

	while (DEF_TRUE)
	{
		int exist = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);

		if (exist != 0) // when human detected
		{
			OSFlagPost(flagGroup, (OS_FLAGS)FLAG_DETECT, OS_FLAG_SET, &err);
		}
		else
		{
			OSFlagPost(flagGroup, (OS_FLAGS)FLAG_DETECT_NOT, OS_FLAG_SET, &err);
		}

		OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
	}
}

/*
 *********************************************************************************************************
 *                                            temperTask()
 *
 * Description : Measure a person's temperature
 *
 * Argument(s) : p
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

// 사람의 온도를 측정하여 통과할지 말지를 결정하는 Task
static void temperTask(void *p)
{
	INT8U err;
	int temp;
	int high = 40;
	int low = 20;
	while (DEF_TRUE)
	{
		temp = 30; // readTemperature()

		if (temp > high)
		{
			OSFlagPost(flagGroup, (OS_FLAGS)FLAG_TEMPER_HIGH, OS_FLAG_SET, &err);
		}
		else if (temp < low)
		{
			OSFlagPost(flagGroup, (OS_FLAGS)FLAG_TEMPER_LOW, OS_FLAG_SET, &err);
		}
		else
		{
			OSFlagPost(flagGroup, (OS_FLAGS)FLAG_TEMPER_NORMAL, OS_FLAG_SET, &err);
		}

		//OSQPost(tempQue, (void *)temp);
		OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
	}
}
static int readTemperature()
{
	while (I2C_GetFlagStatus(((I2C_TypeDef *)I2C1_BASE), I2C_FLAG_BUSY))
		I2C_GenerateSTART(((I2C_TypeDef *)I2C1_BASE), ENABLE);
	while (!I2C_CheckEvent(((I2C_TypeDef *)I2C1_BASE), I2C_EVENT_MASTER_MODE_SELECT))
		;
	I2C_Send7bitAddress(((I2C_TypeDef *)I2C1_BASE), 0x74, I2C_Direction_Transmitter);
	while (!I2C_CheckEvent(((I2C_TypeDef *)I2C1_BASE), I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
		;
	I2C_SendData(((I2C_TypeDef *)I2C1_BASE), 0x07);
	while (!I2C_CheckEvent(((I2C_TypeDef *)I2C1_BASE), I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		;
	I2C_GenerateSTOP(((I2C_TypeDef *)I2C1_BASE), ENABLE);
	I2C_GenerateSTART(((I2C_TypeDef *)I2C1_BASE), ENABLE);
	while (!I2C_CheckEvent(((I2C_TypeDef *)I2C1_BASE), I2C_EVENT_MASTER_MODE_SELECT))
		;
	I2C_Send7bitAddress(((I2C_TypeDef *)I2C1_BASE), 0x75, I2C_Direction_Receiver);
	while (!I2C_CheckEvent(((I2C_TypeDef *)I2C1_BASE), I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
		;
	int low = I2C_ReceiveData(((I2C_TypeDef *)I2C1_BASE));
	if (I2C_GetLastEvent((I2C_TypeDef *)I2C1_BASE) & 0x40 != 0x40)
	{
		int high = I2C_ReceiveData(((I2C_TypeDef *)I2C1_BASE));
		if (high & 0x80 != 0)
		{
			return 20;
		}
		else
		{
			return (high << 8 + low) * 0.02 - 273.15;
		}
	}

	I2C_AcknowledgeConfig(((I2C_TypeDef *)I2C1_BASE), ENABLE);
}

/*
 *********************************************************************************************************
 *                                            passTask()
 *
 * Description : Those who are at normal body temperature are allowed to pass.
 *
 * Argument(s) : p
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */
// 정상체온인 사람은 통과를 허가하는 Task
static void passTask(void *p)
{
	int err;
	while (DEF_TRUE)
	{
		OSFlagPend(flagGroup, (OS_FLAGS)(FLAG_DETECT + FLAG_TEMPER_NORMAL), OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 100, (INT8U *)&err);
		if (count == 0){
                startNotice();
		stopAlert();
                //OSSemPend(sem, 0, (INT8U *)&err);
		count = 1;
		//OSSemPost(sem);
                }
		/*
		OSFlagPend(flagGroup, FLAG_DETECT, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, (INT8U *)&err);
		int temp = (int)OSQPend(tempQue, 0, (INT8U *)&err);
		if (temp > 40)
		{
			startAlert();
		}
		else if (temp > 30 && temp <= 40)
		{
			startNotice();
		}
		if (count == 0)
		{
			count = 1;
		}
		*/
		OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
	}
}

/*
 *********************************************************************************************************
 *                                            denyTask()
 *
 * Description : People with abnormal body temperature are not allowed to pass through.
 *
 * Argument(s) : p
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */
// 비정상체온인 사람은 통과를 불허하는 Task
static void denyTask(void *p)
{
	int err;
	while (DEF_TRUE)
	{
		OSFlagPend(flagGroup,
				   (OS_FLAGS)(FLAG_TEMPER_HIGH + FLAG_DETECT),
				   OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME,
				   100,
				   (INT8U *)&err);
                if (count == 0) {
		startAlert();
		stopNotice();
		//OSSemPend(sem, 0, (INT8U *)&err);
		count = 1;
		//OSSemPost(sem);
                }
		OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
	}
}

/*
 *********************************************************************************************************
 *                                            checkTask()
 *
 * Description : Check dot-matrix, piezo, motor.
 *
 * Argument(s) : p
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */
// 경고를 일정 시간 후 정지하도록 하는 Task
static void checkTask(void *p)
{
	CPU_INT08U err;
	stopAlert();
	stopNotice();
	while (DEF_TRUE)
	{
		if (count != 0)
		{
			check++;
			if (check > 3)
			{
				stopAlert();
				stopNotice();
				//OSSemPend(sem, 0, &err);
				count = 0;
				//OSSemPost(sem);
				check = 0;
			}
		}

		OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
	}
}

// Stop all
static void stopAlert()
{
	// LED
	GPIO_ResetBits(GPIOC, GPIO_Pin_12);
	// piezo
	GPIO_ResetBits(GPIOB, GPIO_Pin_8);
}

static void startAlert()
{
	// LED
	GPIO_SetBits(GPIOC, GPIO_Pin_12);
	// piezo
	GPIO_SetBits(GPIOB, GPIO_Pin_8);
}

static void stopNotice()
{
	GPIO_ResetBits(GPIOC, GPIO_Pin_11);
}

static void startNotice()
{
	GPIO_SetBits(GPIOC, GPIO_Pin_11);
}
/*
 *********************************************************************************************************
 *                                            displayTask()
 *
 * Description : display with dot-matrix.
 *
 * Argument(s) : p
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */
/*
// dot-matrix 출력
static void
displayTask(void *p)
{
	CPU_INT08U err;
	int color = 0; // green
	int shape = 0; // O
	while (DEF_TRUE)
	{
		GPIO_SetBits(GPIOC, GPIO_Pin_6);
		GPIO_SetBits(GPIOC, GPIO_Pin_8);
		
		for (int i = 0; i < 8; i++)
		{
			GPIO_SetBits(lineTypes[i], linePins[i]);
			for (int j = 0; j < 8; j++)
			{
				GPIO_SetBits(orangeTypes[i], orangePins[j]);
				
				if (shape == 0 && shapeO[i][j] == 1)
				{

					GPIO_SetBits(orangeTypes[j], orangePins[j]);
				}
				else if (shape == 1 && shapeX[i][j] == 1)
				{

					GPIO_SetBits(orangeTypes[j], orangePins[j]);
				}
				else
				{
					//GPIO_ResetBits(orangeTypes[j], orangePins[j]);
					//GPIO_ResetBits(greenTypes[j], greenPins[j]);
				}
				
			}

			GPIO_ResetBits(lineTypes[i], linePins[i]);
		}
		
		OSTimeDlyHMSM(0, 0, 0, 30); // To run other tasks
	}
}
*/

/*
 *********************************************************************************************************
 *                                            startTask()
 *
 * Description : Init task.
 *
 * Argument(s) : p
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */
// 경고를 일정 시간 후 정지하도록 하는 Task
static void startTask(void *p)
{
	CPU_INT08U err;

	BSP_Init();
	OS_CPU_SysTickInit();

	// Create Event Flag
	flagGroup = OSFlagCreate(0, &err);

	// Create msg que
	//tempQue = OSQCreate(&tempBuffer[0], 10);

	// Create semaphore
	//sem = OSSemCreate(0);

	while (DEF_TRUE)
	{
		OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
	}

#if (OS_TASK_STAT_EN > 0)
	OSStatInit(); /* Determine CPU capacity.                              */
#endif

#if ((APP_PROBE_COM_EN == DEF_ENABLED) || \
	 (APP_OS_PROBE_EN == DEF_ENABLED))
	App_InitProbe();
#endif
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
 * Caller(s)   : TaskUserIF().
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
 * Caller(s)   : TaskUserIF().
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

static void initAll()
{
	ADC_InitTypeDef adc_init;
	GPIO_InitTypeDef gpio_init;
	I2C_InitTypeDef i2c_init;
	SPI_InitTypeDef spi_init;

	// CLOCK
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	// PIN
	// ADC / 온도
	gpio_init.GPIO_Pin = GPIO_Pin_0;
	gpio_init.GPIO_Mode = GPIO_Mode_AIN;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio_init);
	// 인체 감지
	gpio_init.GPIO_Pin = GPIO_Pin_1;
	gpio_init.GPIO_Mode = GPIO_Mode_AIN;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio_init);
	// I2C
	gpio_init.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	gpio_init.GPIO_Mode = GPIO_Mode_AF_OD;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio_init);
	// Piezo
	gpio_init.GPIO_Pin = GPIO_Pin_8;
	gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio_init);
	// SPI
	gpio_init.GPIO_Pin = GPIO_Pin_12;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &gpio_init);
	gpio_init.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio_init);

	// light
	gpio_init.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
	gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &gpio_init);

	/*
	// dot-matrix
	gpio_init.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &gpio_init);

	gpio_init.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_0;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &gpio_init);

	gpio_init.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &gpio_init);
*/
	/*
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  	GPIO_Init(GPIOD, &GPIO_InitStructure);
*/
	// CONFIG
	// ADC
	adc_init.ADC_Mode = ADC_Mode_Independent;
	adc_init.ADC_ScanConvMode = ENABLE;
	adc_init.ADC_ContinuousConvMode = ENABLE;
	adc_init.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	adc_init.ADC_DataAlign = ADC_DataAlign_Right;
	adc_init.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &adc_init);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_41Cycles5);
	ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
	ADC_Cmd(ADC1, ENABLE);

	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1) != RESET)
		;
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1) != RESET)
		;
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	// I2C
	i2c_init.I2C_Mode = I2C_Mode_I2C;
	i2c_init.I2C_DutyCycle = I2C_DutyCycle_2;
	i2c_init.I2C_OwnAddress1 = 0;
	i2c_init.I2C_Ack = I2C_Ack_Enable;
	i2c_init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	i2c_init.I2C_ClockSpeed = 100000;
	I2C_Init(((I2C_TypeDef *)I2C1_BASE), &i2c_init);
	I2C_Cmd(((I2C_TypeDef *)I2C1_BASE), ENABLE);
	/*
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
	*/
}