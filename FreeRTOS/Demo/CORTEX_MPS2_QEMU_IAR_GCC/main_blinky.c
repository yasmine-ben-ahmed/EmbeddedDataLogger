#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Task priorities */
#define SENSOR_TASK_PRIO       (tskIDLE_PRIORITY + 3)
#define PROCESS_TASK_PRIO      (tskIDLE_PRIORITY + 2)
#define MONITOR_TASK_PRIO      (tskIDLE_PRIORITY + 2)
#define COMMAND_TASK_PRIO      (tskIDLE_PRIORITY + 2)
#define LOGGER_TASK_PRIO       (tskIDLE_PRIORITY + 1)
#define ALERT_TASK_PRIO        (tskIDLE_PRIORITY + 1)

/* Queue lengths */
#define SENSOR_QUEUE_LENGTH    10
#define ALERT_QUEUE_LENGTH     5
#define COMMAND_QUEUE_LENGTH   5

/* Structures */
typedef struct {
    int temp;      // Temperature * 10
    int humidity;  // Humidity * 100
    int light;     // Lux
    TickType_t tick;
} SensorData_t;

typedef struct {
    char message[64];
} AlertData_t;

typedef struct {
    char command[32];
    uint32_t cmd_id;
} CommandData_t;

/* Static queue buffers */
static SensorData_t sensorQueueBuffer[SENSOR_QUEUE_LENGTH];
static StaticQueue_t sensorQueueStruct;

static AlertData_t alertQueueBuffer[ALERT_QUEUE_LENGTH];
static StaticQueue_t alertQueueStruct;

static CommandData_t commandQueueBuffer[COMMAND_QUEUE_LENGTH];
static StaticQueue_t commandQueueStruct;

/* Queue handles */
static QueueHandle_t xSensorQueue;
static QueueHandle_t xAlertQueue;
static QueueHandle_t xCommandQueue;

/* Task stacks and TCBs */
static StackType_t sensorStack[configMINIMAL_STACK_SIZE];
static StaticTask_t sensorTCB;

static StackType_t processorStack[configMINIMAL_STACK_SIZE];
static StaticTask_t processorTCB;

static StackType_t monitorStack[configMINIMAL_STACK_SIZE];
static StaticTask_t monitorTCB;

static StackType_t commandStack[configMINIMAL_STACK_SIZE];
static StaticTask_t commandTCB;

static StackType_t loggerStack[configMINIMAL_STACK_SIZE];
static StaticTask_t loggerTCB;

static StackType_t alertStack[configMINIMAL_STACK_SIZE];
static StaticTask_t alertTCB;

/* Prototypes */
static void SensorReaderTask(void *pvParameters);
static void DataProcessorTask(void *pvParameters);
static void SystemMonitorTask(void *pvParameters);
static void CommandHandlerTask(void *pvParameters);
static void DataLoggerTask(void *pvParameters);
static void AlertTask(void *pvParameters);

/* Random number generator (LFSR) */
static uint32_t lfsr = 0xACE1u;
static uint16_t simple_rand(void) {
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
    return lfsr & 0xFFFF;
}

/* Simulate sensor reading */
static SensorData_t readSensor(TickType_t tick) {
    SensorData_t data;
    data.temp = 150 + (simple_rand() % 200);        // 15.0C - 35.0C
    data.humidity = 3000 + (simple_rand() % 4000); // 30% - 70%
    data.light = 300 + simple_rand() % 500;        // 300 - 800 lux
    data.tick = tick;
    return data;
}

/*-----------------------------------------------------------*/
void main_blinky(void) {
    printf("QEMU RTOSdemo started\n");
    printf("--- SENSOR LOG START ---\n");
    printf("| Tick | Temp(°C) | Humidity(%%) | Light(lux) |\n");
    printf("---------------------------------------------\n");

    /* Create static queues */
    xSensorQueue  = xQueueCreateStatic(SENSOR_QUEUE_LENGTH, sizeof(SensorData_t),
                                       (uint8_t*)sensorQueueBuffer, &sensorQueueStruct);
    xAlertQueue   = xQueueCreateStatic(ALERT_QUEUE_LENGTH, sizeof(AlertData_t),
                                       (uint8_t*)alertQueueBuffer, &alertQueueStruct);
    xCommandQueue = xQueueCreateStatic(COMMAND_QUEUE_LENGTH, sizeof(CommandData_t),
                                       (uint8_t*)commandQueueBuffer, &commandQueueStruct);

    /* Create tasks */
    xTaskCreateStatic(SensorReaderTask, "SensorReader", configMINIMAL_STACK_SIZE,
                      NULL, SENSOR_TASK_PRIO, sensorStack, &sensorTCB);

    xTaskCreateStatic(DataProcessorTask, "DataProcessor", configMINIMAL_STACK_SIZE,
                      NULL, PROCESS_TASK_PRIO, processorStack, &processorTCB);

    xTaskCreateStatic(SystemMonitorTask, "SystemMonitor", configMINIMAL_STACK_SIZE,
                      NULL, MONITOR_TASK_PRIO, monitorStack, &monitorTCB);

    xTaskCreateStatic(CommandHandlerTask, "CommandHandler", configMINIMAL_STACK_SIZE,
                      NULL, COMMAND_TASK_PRIO, commandStack, &commandTCB);

    xTaskCreateStatic(DataLoggerTask, "DataLogger", configMINIMAL_STACK_SIZE,
                      NULL, LOGGER_TASK_PRIO, loggerStack, &loggerTCB);

    xTaskCreateStatic(AlertTask, "AlertTask", configMINIMAL_STACK_SIZE,
                      NULL, ALERT_TASK_PRIO, alertStack, &alertTCB);

    vTaskStartScheduler();
}

/*-----------------------------------------------------------*/
static void SensorReaderTask(void *pvParameters) {
    (void) pvParameters;
    TickType_t tick = 1;

    for (;;) {
        SensorData_t data = readSensor(tick);
        xQueueSend(xSensorQueue, &data, portMAX_DELAY);

        printf("| %4lu | %8.1f | %10.2f | %10d |\n",
               (unsigned long)data.tick,
               data.temp / 10.0f,
               data.humidity / 100.0f,
               data.light);

        tick++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*-----------------------------------------------------------*/
static void DataProcessorTask(void *pvParameters) {
    (void) pvParameters;
    SensorData_t data;
    int avgTemp = 0, avgHum = 0;
    int samples = 0;
    int prevTemp = -1; // initialize to invalid value

    for (;;) {
        if (xQueueReceive(xSensorQueue, &data, portMAX_DELAY)) {
            avgTemp = (avgTemp * samples + data.temp) / (samples + 1);
            avgHum  = (avgHum * samples + data.humidity) / (samples + 1);
            samples++;

            printf("[PROCESS] Avg Temp: %5.1f°C Avg Hum: %5.2f%% (Samples: %d)\n",
                   avgTemp / 10.0f, avgHum / 100.0f, samples);

            if (prevTemp >= 0 && abs(data.temp - prevTemp) > 50) { // >5°C sudden change
                AlertData_t alert;
                snprintf(alert.message, sizeof(alert.message),
                         "\x1B[31mRapid temp change: %5.1f°C -> %5.1f°C\x1B[0m",
                         prevTemp / 10.0f, data.temp / 10.0f);
                xQueueSend(xAlertQueue, &alert, portMAX_DELAY);
            }
            prevTemp = data.temp;
        }
    }
}

/*-----------------------------------------------------------*/
static void AlertTask(void *pvParameters) {
    (void) pvParameters;
    AlertData_t alert;

    for (;;) {
        if (xQueueReceive(xAlertQueue, &alert, portMAX_DELAY)) {
            printf("[ALERT] %s\n", alert.message);
        }
    }
}

/*-----------------------------------------------------------*/
static void SystemMonitorTask(void *pvParameters) {
    (void) pvParameters;
    TickType_t tick;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        tick = xTaskGetTickCount();

        printf("[MONITOR] Tick:%lu Heap:%d%% Errors:%d\n",
               (unsigned long)tick,
               70 + simple_rand() % 10,
               simple_rand() % 5);
    }
}

/*-----------------------------------------------------------*/
static void CommandHandlerTask(void *pvParameters) {
    (void) pvParameters;
    CommandData_t cmd;
    const char *commands[] = {"STATUS", "READ_SENSORS", "CALIBRATE"};
    TickType_t tick;

    for (uint32_t cmd_id = 0;; cmd_id++) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        snprintf(cmd.command, sizeof(cmd.command), "%s", commands[cmd_id % 3]);
        cmd.cmd_id = cmd_id;
        tick = xTaskGetTickCount();

        printf("[COMMAND] Tick:%lu Processing:%s -> OK (Cmd #%lu)\n",
               (unsigned long)tick, cmd.command, (unsigned long)cmd.cmd_id);
    }
}

/*-----------------------------------------------------------*/
static void DataLoggerTask(void *pvParameters) {
    (void) pvParameters;
    SensorData_t data;

    for (;;) {
        if (xQueueReceive(xSensorQueue, &data, portMAX_DELAY)) {
            // Optionally log data to flash or SD card
        }
    }
}
