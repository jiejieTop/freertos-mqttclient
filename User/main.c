#include "main.h"
/* FreeRTOS头文件 */
#include "FreeRTOS.h"
#include "task.h"

#include "mqttclient.h"

//#define TEST_USEING_TLS  

extern const char *test_ca_get(void);

static TaskHandle_t app_task_create_handle = NULL;/* 创建任务句柄 */
static TaskHandle_t mqtt_task_handle = NULL;/* LED任务句柄 */

static mqtt_client_t client;
static client_init_params_t init_params;

static void app_task_create(void);/* 用于创建任务 */

static void mqtt_task(void* pvParameters);/* mqtt_task任务实现 */

extern void TCPIP_Init(void);

/*****************************************************************
  * @brief  主函数
  * @param  无
  * @retval 无
  * @note   第一步：开发板硬件初始化 
            第二步：创建APP应用任务
            第三步：启动FreeRTOS，开始多任务调度
  ****************************************************************/
int main(void)
{	
  BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */
  
  /* 开发板硬件初始化 */
  BSP_Init();

  /* 创建app_task_create任务 */
  xReturn = xTaskCreate((TaskFunction_t )app_task_create,  /* 任务入口函数 */
                        (const char*    )"app_task_create",/* 任务名字 */
                        (uint16_t       )2048,  /* 任务栈大小 */
                        (void*          )NULL,/* 任务入口函数参数 */
                        (UBaseType_t    )10, /* 任务的优先级 */
                        (TaskHandle_t*  )&app_task_create_handle);/* 任务控制块指针 */ 
  /* 启动任务调度 */           
  if(pdPASS == xReturn)
    vTaskStartScheduler();   /* 启动任务，开启调度 */
  else
    return -1;  
  
  while(1);   /* 正常不会执行到这里 */    
}


/***********************************************************************
  * @ 函数名  ： app_task_create
  * @ 功能说明： 为了方便管理，所有的任务创建函数都放在这个函数里面
  * @ 参数    ： 无  
  * @ 返回值  ： 无
  **********************************************************************/
static void app_task_create(void)
{
    int err;
    
    BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */

    TCPIP_Init();

    mqtt_log_init();
    
    MQTT_LOG_I("\nwelcome to mqttclient test...\n");

    init_params.read_buf_size = 1024;
    init_params.write_buf_size = 1024;

#ifdef TEST_USEING_TLS
    init_params.network.ca_crt = test_ca_get();
    init_params.network.port = "8883";
#else
    init_params.network.port = "1883";
#endif
    init_params.network.host = "www.jiejie01.top"; //"47.95.164.112";//"jiejie01.top"; //"129.204.201.235"; //"192.168.1.101";

    init_params.connect_params.user_name = random_string(10); // random_string(10); //"jiejietop-acer1";
    init_params.connect_params.password = random_string(10); //random_string(10); // "123456";
    init_params.connect_params.client_id = random_string(10); //random_string(10); // "clientid-acer1";
    init_params.connect_params.clean_session = 1;
    
    mqtt_init(&client, &init_params);

    err = mqtt_connect(&client);
    
    MQTT_LOG_I("mqtt_connect err = %d", err);
    
    err = mqtt_subscribe(&client, "freertos-topic", QOS0, NULL);
    
    MQTT_LOG_I("mqtt_subscribe err = %d", err);    


    taskENTER_CRITICAL();           //进入临界区

    /* 创建mqtt_task任务 */
    xReturn = xTaskCreate((TaskFunction_t )mqtt_task, /* 任务入口函数 */
                        (const char*    )"mqtt_task",/* 任务名字 */
                        (uint16_t       )2048,   /* 任务栈大小 */
                        (void*          )NULL,	/* 任务入口函数参数 */
                        (UBaseType_t    )10,	    /* 任务的优先级 */
                        (TaskHandle_t*  )&mqtt_task_handle);/* 任务控制块指针 */
    if(pdPASS == xReturn)
        printf("Create mqtt_task sucess...\r\n");

    vTaskDelete(app_task_create_handle); //删除app_task_create任务

    taskEXIT_CRITICAL();            //退出临界区
}



/**********************************************************************
  * @ 函数名  ： mqtt_task
  * @ 功能说明： mqtt_task任务主体
  * @ 参数    ：   
  * @ 返回值  ： 无
  ********************************************************************/
static void mqtt_task(void* parameter)
{	
    char buf[100] = { 0 };
    mqtt_message_t msg;
    memset(&msg, 0, sizeof(msg));
    sprintf(buf, "welcome to mqttclient, this is a publish test...");

    vTaskDelay(4000);
    
    mqtt_list_subscribe_topic(&client);

    msg.payload = (void *) buf;
    msg.qos = QOS0;
    
    while(1) {
        sprintf(buf, "welcome to mqttclient, this is a publish test, a rand number: %d ...", random_number());

        mqtt_publish(&client, "freertos-topic", &msg);
        
        vTaskDelay(4000);
    }
}
