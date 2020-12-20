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
static OS_STK detectTaskStack[TASK_STK_SIZE];
static OS_STK temperatureTaskStack[TASK_STK_SIZE];
static OS_STK passTaskStack[TASK_STK_SIZE];
static OS_STK denyTaskStack[TASK_STK_SIZE];
static OS_STK checkTaskStack[TASK_STK_SIZE];

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

// time
static OS_EVENT *sem;
static int count = 0;
const static int TIME_COUNT = 9; // 100ms * 10 = 1Ï¥à
const static int DELAY_TIME = 100;

<<<<<<< HEAD
#if ((OS_PROBE_EN == DEF_ENABLED) &&  \
    (PROBE_COM_EN == DEF_ENABLED) && \
    (PROBE_COM_STAT_EN == DEF_ENABLED))
static CPU_FP32 ProbeComRxPktSpd;
static CPU_FP32 ProbeComTxPktSpd;
static CPU_FP32 ProbeComTxSymSpd;
static CPU_FP32 ProbeComTxSymByteSpd;
=======
#if ((APP_OS_PROBE_EN == DEF_ENABLED) &&  \
	 (APP_PROBE_COM_EN == DEF_ENABLED) && \
	 (PROBE_COM_STAT_EN == DEF_ENABLED))
static CPU_FP32 App_ProbeComRxPktSpd;
static CPU_FP32 App_ProbeComTxPktSpd;
static CPU_FP32 App_ProbeComTxSymSpd;
static CPU_FP32 App_ProbeComTxSymByteSpd;
>>>>>>> 12f609e231801fbf08d93cb7ba0b89eff1ec66db

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

static void App_DispScr_SignOn(void);
static void DispScr_TaskNames(void);

static int readTemperature(void);
static void stopAll();
static void initAll();

<<<<<<< HEAD
#if ((PROBE_COM_EN == DEF_ENABLED) || \
    (OS_PROBE_EN == DEF_ENABLED))
static void InitProbe(void);
=======
#if ((APP_PROBE_COM_EN == DEF_ENABLED) || \
	 (APP_OS_PROBE_EN == DEF_ENABLED))
static void App_InitProbe(void);
>>>>>>> 12f609e231801fbf08d93cb7ba0b89eff1ec66db
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
<<<<<<< HEAD
   CPU_INT08U os_err;

   /* Disable all ints until we are ready to accept them.  */
   BSP_IntDisAll();

   /* Initialize "uC/OS-II, The Real-Time Kernel".         */
   OSInit();

   initAll();

   // Create Message Que, msg : ¿˙¿Â∞¯∞£, ≈©±‚ : 10
   temperQue = OSQCreate(msg, 10);

   // Create Event Flag
   flagGroup = OSFlagCreate(FLAG_INIT, &os_err);

   // Create semaphore
   sem = OSSemCreate(0);

   os_err = OSTaskCreateExt((void (*)(void *))detectTask,                     // Task∞° ºˆ«‡«“ «‘ºˆ, ªÁ∂˜¿« ¡∏¿Á ¿Ø/π´∏¶ æÀ∑¡¡÷¥¬ Task
                      (void *)0,                                    // Task∑Œ ≥—∞‹¡Ÿ ¿Œ¿⁄
                      (OS_STK *)&detectTaskStack[TASK_STK_SIZE - 1],         // Task∞° «“¥Áµ… Stack¿« Top¿ª ∞°∏Æ≈∞¥¬ ¡÷º“
                      (INT8U)TASK_DETECT_PRIO,                        // Task¿« øÏº± º¯¿ß (MPT)
                      (INT16U)TASK_DETECT_PRIO,                        // Task∏¶ ¡ˆƒ™«œ¥¬ ¿Ø¿œ«— Ωƒ∫∞¿⁄, Task ∞πºˆ¿« ±ÿ∫π¿ª ¿ß«ÿº≠ ªÁøÎ«“ øπ¡§, «ˆ¿Á¥¬ øÏº± º¯¿ßøÕ ∞∞∞‘≤˚ º≥¡§
                      (OS_STK *)&detectTaskStack,                     // Task∞° «“¥Áµ… Stack¿« ∏∂¡ˆ∏∑¿ª ∞°∏Æ≈∞¥¬ ¡÷º“, Stack ∞ÀªÁøÎ¿∏∑Œ ªÁøÎ
                      (INT32U)TASK_STK_SIZE,                           // Task Stack¿« ≈©±‚∏¶ ¿«πÃ
                      (void *)0,                                    // Task Control Block »∞øÎΩ√ ªÁøÎ
                      (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK)); // Task ª˝º∫ ø…º«

   os_err = OSTaskCreateExt((void (*)(void *))temperTask, // ªÁ∂˜¿« ø¬µµ∏¶ √¯¡§«œø© ≈Î∞˙«“¡ˆ ∏ª¡ˆ∏¶ ∞·¡§«œ¥¬ Task
                      (void *)0,
                      (OS_STK *)&temperatureTaskStack[TASK_STK_SIZE - 1],
                      (INT8U)TASK_TEMPER_PRIO,
                      (INT16U)TASK_TEMPER_PRIO,
                      (OS_STK *)&temperatureTaskStack,
                      (INT32U)TASK_STK_SIZE,
                      (void *)0,
                      (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

   os_err = OSTaskCreateExt((void (*)(void *))passTask, // ¡§ªÛ√ºø¬¿Œ ªÁ∂˜¿∫ ≈Î∞˙∏¶ «„∞°«œ¥¬ Task
                      (void *)0,
                      (OS_STK *)&passTaskStack[TASK_STK_SIZE - 1],
                      (INT8U)TASK_PASS_PRIO,
                      (INT16U)TASK_PASS_PRIO,
                      (OS_STK *)&passTaskStack,
                      (INT32U)TASK_STK_SIZE,
                      (void *)0,
                      (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

   os_err = OSTaskCreateExt((void (*)(void *))denyTask, // ∫Ò¡§ªÛ√ºø¬¿Œ ªÁ∂˜¿∫ ≈Î∞˙∏¶ ∫“«„«œ¥¬ Task
                      (void *)0,
                      (OS_STK *)&denyTaskStack[TASK_STK_SIZE - 1],
                      (INT8U)TASK_DENY_PRIO,
                      (INT16U)TASK_DENY_PRIO,
                      (OS_STK *)&denyTaskStack,
                      (INT32U)TASK_STK_SIZE,
                      (void *)0,
                      (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

   os_err = OSTaskCreateExt((void (*)(void *))checkTask, // æÀ∏≤ ¿Âƒ° ¿€µø ¡ﬂ¡ˆ«œ¥¬ Task
                      (void *)0,
                      (OS_STK *)&checkTaskStack[TASK_STK_SIZE - 1],
                      (INT8U)TASK_CHECK_PRIO,
                      (INT16U)TASK_CHECK_PRIO,
                      (OS_STK *)&checkTaskStack,
                      (INT32U)TASK_STK_SIZE,
                      (void *)0,
                      (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
=======
	CPU_INT08U os_err;

	/* Disable all ints until we are ready to accept them.  */
	BSP_IntDisAll();

	/* Initialize "uC/OS-II, The Real-Time Kernel".         */
	OSInit();

	initAll();

	// Create Message Que, msg : Ï†ÄÏû•Í≥µÍ∞Ñ, ÌÅ¨Í∏∞ : 10
	//temperQue = (OS_EVENT *)OSQCreate(msg, 10);

	// Create Event Flag
	flagGroup = OSFlagCreate(FLAG_INIT, &os_err);

	// Create semaphore
	sem = OSSemCreate(0);

	os_err = OSTaskCreateExt((void (*)(void *))detectTask,						   // TaskÍ∞Ä ÏàòÌñâÌï† Ìï®Ïàò, ÏÇ¨ÎûåÏùò Ï°¥Ïû¨ Ïú†/Î¨¥Î•º ÏïåÎ†§Ï£ºÎäî Task
							 (void *)0,											   // TaskÎ°ú ÎÑòÍ≤®Ï§Ñ Ïù∏Ïûê
							 (OS_STK *)&detectTaskStack[TASK_STK_SIZE - 1],		   // TaskÍ∞Ä Ìï†ÎãπÎê† StackÏùò TopÏùÑ Í∞ÄÎ¶¨ÌÇ§Îäî Ï£ºÏÜå
							 (INT8U)TASK_DETECT_PRIO,							   // TaskÏùò Ïö∞ÏÑ† ÏàúÏúÑ (MPT)
							 (INT16U)TASK_DETECT_PRIO,							   // TaskÎ•º ÏßÄÏπ≠ÌïòÎäî Ïú†ÏùºÌïú ÏãùÎ≥ÑÏûê, Task Í∞ØÏàòÏùò Í∑πÎ≥µÏùÑ ÏúÑÌï¥ÏÑú ÏÇ¨Ïö©Ìï† ÏòàÏ†ï, ÌòÑÏû¨Îäî Ïö∞ÏÑ† ÏàúÏúÑÏôÄ Í∞ôÍ≤åÎÅî ÏÑ§Ï†ï
							 (OS_STK *)&detectTaskStack,						   // TaskÍ∞Ä Ìï†ÎãπÎê† StackÏùò ÎßàÏßÄÎßâÏùÑ Í∞ÄÎ¶¨ÌÇ§Îäî Ï£ºÏÜå, Stack Í≤ÄÏÇ¨Ïö©ÏúºÎ°ú ÏÇ¨Ïö©
							 (INT32U)TASK_STK_SIZE,								   // Task StackÏùò ÌÅ¨Í∏∞Î•º ÏùòÎØ∏
							 (void *)0,											   // Task Control Block ÌôúÏö©Ïãú ÏÇ¨Ïö©
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK)); // Task ÏÉùÏÑ± ÏòµÏÖò

	os_err = OSTaskCreateExt((void (*)(void *))temperTask, // ÏÇ¨ÎûåÏùò Ïò®ÎèÑÎ•º Ï∏°Ï†ïÌïòÏó¨ ÌÜµÍ≥ºÌï†ÏßÄ ÎßêÏßÄÎ•º Í≤∞Ï†ïÌïòÎäî Task
							 (void *)0,
							 (OS_STK *)&temperatureTaskStack[TASK_STK_SIZE - 1],
							 (INT8U)TASK_TEMPER_PRIO,
							 (INT16U)TASK_TEMPER_PRIO,
							 (OS_STK *)&temperatureTaskStack,
							 (INT32U)TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

	os_err = OSTaskCreateExt((void (*)(void *))passTask, // Ï†ïÏÉÅÏ≤¥Ïò®Ïù∏ ÏÇ¨ÎûåÏùÄ ÌÜµÍ≥ºÎ•º ÌóàÍ∞ÄÌïòÎäî Task
							 (void *)0,
							 (OS_STK *)&passTaskStack[TASK_STK_SIZE - 1],
							 (INT8U)TASK_PASS_PRIO,
							 (INT16U)TASK_PASS_PRIO,
							 (OS_STK *)&passTaskStack,
							 (INT32U)TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

	os_err = OSTaskCreateExt((void (*)(void *))denyTask, // ÎπÑÏ†ïÏÉÅÏ≤¥Ïò®Ïù∏ ÏÇ¨ÎûåÏùÄ ÌÜµÍ≥ºÎ•º Î∂àÌóàÌïòÎäî Task
							 (void *)0,
							 (OS_STK *)&denyTaskStack[TASK_STK_SIZE - 1],
							 (INT8U)TASK_DENY_PRIO,
							 (INT16U)TASK_DENY_PRIO,
							 (OS_STK *)&denyTaskStack,
							 (INT32U)TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

	os_err = OSTaskCreateExt((void (*)(void *))checkTask, // ÏïåÎ¶º Ïû•Ïπò ÏûëÎèô Ï§ëÏßÄÌïòÎäî Task
							 (void *)0,
							 (OS_STK *)&checkTaskStack[TASK_STK_SIZE - 1],
							 (INT8U)TASK_CHECK_PRIO,
							 (INT16U)TASK_CHECK_PRIO,
							 (OS_STK *)&checkTaskStack,
							 (INT32U)TASK_STK_SIZE,
							 (void *)0,
							 (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
>>>>>>> 12f609e231801fbf08d93cb7ba0b89eff1ec66db

#if (OS_TASK_NAME_SIZE >= 11)
   OSTaskNameSet(TASK_DETECT_PRIO, (CPU_INT08U *)"Detect Task", &os_err);
   OSTaskNameSet(TASK_TEMPER_PRIO, (CPU_INT08U *)"Temperature Task", &os_err);
   OSTaskNameSet(TASK_PASS_PRIO, (CPU_INT08U *)"Pass Task", &os_err);
   OSTaskNameSet(TASK_DENY_PRIO, (CPU_INT08U *)"Deny Task", &os_err);
   OSTaskNameSet(TASK_CHECK_PRIO, (CPU_INT08U *)"Check Task", &os_err);
#endif

   OSStart(); /* Start multitasking (i.e. give control to uC/OS-II).  */

<<<<<<< HEAD
   return (0);
=======
	BSP_Init();
	OS_CPU_SysTickInit();
	#if (OS_TASK_STAT_EN > 0)
	OSStatInit();                                           /* Determine CPU capacity.                              */
#endif

#if ((APP_PROBE_COM_EN == DEF_ENABLED) || \
	(APP_OS_PROBE_EN == DEF_ENABLED))
	App_InitProbe();
#endif

	return (0);
>>>>>>> 12f609e231801fbf08d93cb7ba0b89eff1ec66db
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

// TaskÍ∞Ä ÏàòÌñâÌï† Ìï®Ïàò, ÏÇ¨ÎûåÏùò Ï°¥Ïû¨ Ïú†/Î¨¥Î•º ÏïåÎ†§Ï£ºÎäî Task
static void detectTask(void *p)
{
<<<<<<< HEAD
   CPU_INT08U err;

   while (DEF_TRUE)
   {
      if (GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_0) != 0) // when human detected
      {
         OSFlagPost(flagGroup, FLAG_DETECT, OS_FLAG_SET, &err);
      }
      OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
   }
=======
	CPU_INT08U err;
	while (DEF_TRUE)
	{
		if (ADC_GetConversionValue(ADC1) != 0) // when human detected
		{
			BSP_LED_On(0);
			OSFlagPost(flagGroup, FLAG_DETECT, OS_FLAG_SET, &err);
		}
		else
		{
			BSP_LED_On(1);
			OSFlagPost(flagGroup, FLAG_DETECT_NOT, OS_FLAG_SET, &err);
		}
		OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
	}
>>>>>>> 12f609e231801fbf08d93cb7ba0b89eff1ec66db
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
// ÏÇ¨ÎûåÏùò Ïò®ÎèÑÎ•º Ï∏°Ï†ïÌïòÏó¨ ÌÜµÍ≥ºÌï†ÏßÄ ÎßêÏßÄÎ•º Í≤∞Ï†ïÌïòÎäî Task
static void temperTask(void *p)
{
<<<<<<< HEAD
   INT8U err;
   int temp;
   int high = 39;
   int low = 34;
   while (DEF_TRUE)
   {
      OSFlagPend(flagGroup, FLAG_DETECT, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, &err);
      temp = readTemperature();
      if (temp > high) // when temperature is HIGH
      {
         OSQPost(temperQue, temp);
         OSFlagPost(flagGroup, FLAG_TEMPER_HIGH, OS_FLAG_SET, &err);
      }
      else if (temp < low)
      {
         OSQPost(temperQue, temp);
         OSFlagPost(flagGroup, FLAG_TEMPER_LOW, OS_FLAG_SET, &err);
      }
      else
      {
         OSFlagPost(flagGroup, FLAG_TEMPER_NORMAL, OS_FLAG_SET, &err);
      }

      OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
   }
=======
	INT8U err;
	int temp;
	int high = 39;
	int low = 34;
	while (DEF_TRUE)
	{
		temp = readTemperature();
		if (temp > high) // when temperature is HIGH
		{
			//OSQPost(temperQue, temp);
			BSP_LED_On(2);
			OSFlagPost(flagGroup, FLAG_TEMPER_HIGH, OS_FLAG_SET, &err);
		}
		else if (temp < low)
		{
			//OSQPost(temperQue, temp);
			BSP_LED_On(2);
			OSFlagPost(flagGroup, FLAG_TEMPER_LOW, OS_FLAG_SET, &err);
		}
		else
		{
			BSP_LED_On(3);
			OSFlagPost(flagGroup, FLAG_TEMPER_NORMAL, OS_FLAG_SET, &err);
		}

		OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
	}
>>>>>>> 12f609e231801fbf08d93cb7ba0b89eff1ec66db
}

static int readTemperature()
{
   // int high, low;
   // int tmp = 0;

   // while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
   //    ;
   // I2C_GenerateSTART(I2C1, ENABLE);
   // while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
   //    ;
   // I2C_Send7bitAddress(I2C1, addr, I2C_Direction_Transmitter);
   // while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
   //    ;
   // I2C_SendData(I2C1, 0x0);
   // while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
   //    ;
   // I2C_GenerateSTOP(I2C1, ENABLE);

   // I2C_GenerateSTART(I2C1, ENABLE);
   // while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
   //    ;
   // I2C_Send7bitAddress(I2C1, addr, I2C_Direction_Receiver);
   // while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
   //    ;
   // while ((I2C_GetLastEvent(I2C1) & I2C_FLAG_RXNE) != I2C_FLAG_RXNE)
   //    ; /* Poll on RxNE */
   // high = I2C_ReceiveData(I2C1);
   // I2C_AcknowledgeConfig(I2C1, DISABLE);
   // I2C_GenerateSTOP(I2C1, ENABLE);

   // while ((I2C_GetLastEvent(I2C1) & I2C_FLAG_RXNE) != I2C_FLAG_RXNE)
   //    ; /* Poll on RxNE */

   // low = I2C_ReceiveData(I2C1);
   // I2C_AcknowledgeConfig(I2C1, ENABLE);
   // tmp = (uint16_t)(high << 8);

   // tmp |= low;
   // return tmp >> 7;
   return 36;
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
// Ï†ïÏÉÅÏ≤¥Ïò®Ïù∏ ÏÇ¨ÎûåÏùÄ ÌÜµÍ≥ºÎ•º ÌóàÍ∞ÄÌïòÎäî Task
static void passTask(void *p)
{
<<<<<<< HEAD
   int err;
   while (DEF_TRUE)
   {
      OSFlagPend(flagGroup, FLAG_TEMPER_NORMAL, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, (INT8U *)&err);
      // dot-matrix
      TODO("dot-matrix pass");
      // piezo
      GPIO_SetBits(GPIOB, GPIO_Pin_8);
      // door
      for (int i = TIM2->CCR1; i < 2300; i += 2) // 1500 -> 2300
      {
         TIM2->CCR1 = i;
      }

      // stop setting
      OSSemPend(sem, 0, (INT8U *)&err);
      count = 1;
      OSSemPost(sem);
      OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
   }
=======
	int err;
	while (DEF_TRUE)
	{
		OSFlagPend(flagGroup, FLAG_DETECT + FLAG_TEMPER_NORMAL, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, (INT8U *)&err);
		// dot-matrix
		// TODO("dot-matrix pass");
		// piezo
		GPIO_SetBits(GPIOB, GPIO_Pin_8);
		// door
		for (int i = TIM2->CCR1; i < 2300; i += 2) // 1500 -> 2300
		{
			TIM2->CCR1 = i;
		}

		// stop setting
		OSSemPend(sem, 0, (INT8U *)&err);
		count = 1;
		OSSemPost(sem);
		OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
	}
>>>>>>> 12f609e231801fbf08d93cb7ba0b89eff1ec66db
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
// ÎπÑÏ†ïÏÉÅÏ≤¥Ïò®Ïù∏ ÏÇ¨ÎûåÏùÄ ÌÜµÍ≥ºÎ•º Î∂àÌóàÌïòÎäî Task
static void denyTask(void *p)
{
<<<<<<< HEAD
   int err;
   int temp = 0;
   while (DEF_TRUE)
   {
      OSFlagPend(flagGroup,
               FLAG_TEMPER_HIGH + FLAG_TEMPER_LOW,
               OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME,
               0,
               (INT8U *)&err);
      temp = OSQPend(temperQue, 0, &err);
      // dot-matrix
      TODO("dot-matrix deny"); // + ø¬µµ √‚∑¬ (∞°¥…«œ¥Ÿ∏È) §ª§ª
      // piezo
      GPIO_SetBits(GPIOB, GPIO_Pin_8);
      // Stop setting
      OSSemPend(sem, 0, (INT8U *)&err);
      count = 1;
      OSSemPost(sem);
      OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
   }
=======
	int err;
	int temp = 0;
	while (DEF_TRUE)
	{
		int flags =
			OSFlagPend(flagGroup,
					   FLAG_TEMPER_HIGH + FLAG_TEMPER_LOW + FLAG_DETECT_NOT,
					   OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME,
					   0,
					   (INT8U *)&err);
		if ((flags & FLAG_TEMPER_HIGH) == FLAG_TEMPER_HIGH)
		{
			// dot-matrix
			// TODO("dot-matrix deny");
			// piezo
			GPIO_SetBits(GPIOB, GPIO_Pin_8);
		}
		else if ((flags & FLAG_TEMPER_LOW) == FLAG_TEMPER_LOW)
		{
			// dot-matrix
			// TODO("dot-matrix deny");
		}
		OSSemPend(sem, 0, (INT8U *)&err);
		count = 1;
		OSSemPost(sem);
		OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
	}
>>>>>>> 12f609e231801fbf08d93cb7ba0b89eff1ec66db
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
// dot-matrix, piezo, motorÎ•º 1Ï¥à ÌõÑ Ï†ïÏßÄÌïòÎèÑÎ°ù ÌïòÎäî Task
static void checkTask(void *p)
{
   CPU_INT08U err;
   int isStop = 0;
   while (DEF_TRUE)
   {
      if (count != 0)
      {
         OSSemPend(sem, 0, &err);
         if (count > TIME_COUNT)
         {
            isStop = 1; // Use flag / Don't do a lot of work in sem
            count = 0;   // init time counter
         }
         count++;
         OSSemPost(sem);

         // STOP: Do out of sem
         if (isStop == 1)
         {
            stopAll();
            isStop = 0;
         }
      }

      OSTimeDlyHMSM(0, 0, 0, DELAY_TIME); // To run other tasks
   }
}

// Stop all
static void stopAll()
{
   // dot-matrix

   // piezo
   GPIO_ResetBits(GPIOB, GPIO_Pin_8);
   // motor
   for (int i = TIM2->CCR1; i > 1500; i -= 2) // 2300 -> 1500
   {
      TIM2->CCR1 = i;
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

<<<<<<< HEAD
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
=======
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
>>>>>>> 12f609e231801fbf08d93cb7ba0b89eff1ec66db
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
<<<<<<< HEAD
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
=======
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
>>>>>>> 12f609e231801fbf08d93cb7ba0b89eff1ec66db
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
<<<<<<< HEAD
#if ((OS_PROBE_EN == DEF_ENABLED) && \
    (OS_PROBE_HOOKS_EN == DEF_ENABLED))
   OSProbe_TaskCreateHook(ptcb);
=======
#if ((APP_OS_PROBE_EN == DEF_ENABLED) && \
	 (OS_PROBE_HOOKS_EN == DEF_ENABLED))
	OSProbe_TaskCreateHook(ptcb);
>>>>>>> 12f609e231801fbf08d93cb7ba0b89eff1ec66db
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
<<<<<<< HEAD
#if ((OS_PROBE_EN == DEF_ENABLED) && \
    (OS_PROBE_HOOKS_EN == DEF_ENABLED))
   OSProbe_TaskSwHook();
=======
#if ((APP_OS_PROBE_EN == DEF_ENABLED) && \
	 (OS_PROBE_HOOKS_EN == DEF_ENABLED))
	OSProbe_TaskSwHook();
>>>>>>> 12f609e231801fbf08d93cb7ba0b89eff1ec66db
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
<<<<<<< HEAD
#if ((OS_PROBE_EN == DEF_ENABLED) && \
    (OS_PROBE_HOOKS_EN == DEF_ENABLED))
   OSProbe_TickHook();
=======
#if ((APP_OS_PROBE_EN == DEF_ENABLED) && \
	 (OS_PROBE_HOOKS_EN == DEF_ENABLED))
	OSProbe_TickHook();
>>>>>>> 12f609e231801fbf08d93cb7ba0b89eff1ec66db
#endif
}
#endif
#endif

static void initAll()
{
<<<<<<< HEAD
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
   GPIO_SetBits(GPIOB, GPIO_Pin_12); // check

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
   i2c_init.I2C_ClockSpeed = 100000;
   I2C_Init((I2C_TypeDef *)I2C1, &i2c_init);
   I2C_Cmd((I2C_TypeDef *)I2C1, ENABLE);
   // TIM (PWM)
   tim_timebase_init.TIM_Prescaler = (72000000 / 1000000) - 1; // set to 1MHz Counter Clock
   tim_timebase_init.TIM_Period = 20000 - 1;               // set to 50Hz pulse with 1MHz Counter Clock
   tim_timebase_init.TIM_ClockDivision = 0;
   tim_timebase_init.TIM_CounterMode = TIM_CounterMode_Down;
   tim_timebase_init.TIM_RepetitionCounter;
   TIM_TimeBaseInit(TIM4, &tim_timebase_init);
   /* PIEZO: PWM1 Mode configuration: Channel3 */
   tim_piezo_init.TIM_OCMode = TIM_OCMode_PWM1;
   tim_piezo_init.TIM_OutputState = TIM_OutputState_Enable;
   tim_piezo_init.TIM_Pulse = 500;
   tim_piezo_init.TIM_OCPolarity = TIM_OCPolarity_High;
   TIM_OC3Init(TIM4, &tim_piezo_init);
   //TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Disable);
   TIM_Cmd(TIM4, ENABLE);
   /* MOTOR: PWM1 Mode configuration: Channel4 */
   tim_motor_init.TIM_OCMode = TIM_OCMode_PWM1;
   tim_motor_init.TIM_OutputState = TIM_OutputState_Enable;
   tim_motor_init.TIM_Pulse = 1500; // 50 % duty cylce value
   tim_motor_init.TIM_OCPolarity = TIM_OCPolarity_High;
   //TIM_PWMIConfig(TIM4, &tim_motor_init);
   TIM_OC4Init(TIM4, &tim_motor_init);
   TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Disable);
   TIM_ARRPreloadConfig(TIM4, ENABLE);
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
=======
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
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
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
	GPIO_SetBits(GPIOB, GPIO_Pin_12); // check

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
	// TIM (PWM)
	tim_timebase_init.TIM_Prescaler = (72000000 / 1000000) - 1; // set to 1MHz Counter Clock
	tim_timebase_init.TIM_Period = 20000 - 1;					// set to 50Hz pulse with 1MHz Counter Clock
	tim_timebase_init.TIM_ClockDivision = 0;
	tim_timebase_init.TIM_CounterMode = TIM_CounterMode_Down;
	tim_timebase_init.TIM_RepetitionCounter;
	TIM_TimeBaseInit(TIM4, &tim_timebase_init);
	/* PIEZO: PWM1 Mode configuration: Channel3 */
	tim_piezo_init.TIM_OCMode = TIM_OCMode_PWM1;
	tim_piezo_init.TIM_OutputState = TIM_OutputState_Enable;
	tim_piezo_init.TIM_Pulse = 500;
	tim_piezo_init.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC3Init(TIM4, &tim_piezo_init);
	//TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Disable);
	TIM_Cmd(TIM4, ENABLE);
	/* MOTOR: PWM1 Mode configuration: Channel4 */
	tim_motor_init.TIM_OCMode = TIM_OCMode_PWM1;
	tim_motor_init.TIM_OutputState = TIM_OutputState_Enable;
	tim_motor_init.TIM_Pulse = 1500; // 50 % duty cylce value
	tim_motor_init.TIM_OCPolarity = TIM_OCPolarity_High;

	//TIM_PWMIConfig(TIM4, &tim_motor_init);
	TIM_OC4Init(TIM4, &tim_motor_init);
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Disable);
	TIM_ARRPreloadConfig(TIM4, ENABLE);
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
>>>>>>> 12f609e231801fbf08d93cb7ba0b89eff1ec66db
}