#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include "FreeRTOSConfig.h"

// shared state
// semaphores make shared state the kernel's problem
SemaphoreHandle_t block_sem;
SemaphoreHandle_t high_sem;
SemaphoreHandle_t mid_sem;
SemaphoreHandle_t low_sem;

void highTask(__unused void* args) {
    xSemaphoreTake(block_sem, portMAX_DELAY);
    xSemaphoreGive(block_sem);
    xSemaphoreGive(high_sem);
    while (1);
}

void midTask(__unused void* args) {
    xSemaphoreGive(mid_sem);
    while (1);
}

void lowTask(__unused void* args) {
    xSemaphoreTake(block_sem, portMAX_DELAY);
    xSemaphoreGive(low_sem);
    vTaskDelay(3000);
    xSemaphoreGive(block_sem);
    while (1);
}

void watchTask(__unused void* args) {
    // give time to open a monitor
    vTaskDelay(10000);
    printf("Starting\n");

    // create dependency inversion
    xTaskCreate(lowTask, "low", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    vTaskDelay(500);
    xTaskCreate(midTask, "mid", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    vTaskDelay(500);
    xTaskCreate(highTask, "high", configMINIMAL_STACK_SIZE, NULL, 3, NULL);

    // output results
    vTaskDelay(5000);
    if (uxSemaphoreGetCount(high_sem)) {
        printf("High priority task ran\n");
    }
    if (uxSemaphoreGetCount(mid_sem)) {
        printf("Mid priority task ran\n");
    }
    if (uxSemaphoreGetCount(low_sem)) {
        printf("Low priority task ran\n");
    }
    printf("Stopping\n");
    while(1);
}

int main () {
    // initialize the pi
    stdio_init_all();
    hard_assert(cyw43_arch_init() == PICO_OK);

    // create the semaphores
    block_sem = xSemaphoreCreateMutex();
    high_sem = xSemaphoreCreateCounting(1, 0);
    mid_sem = xSemaphoreCreateCounting(1, 0);
    low_sem = xSemaphoreCreateCounting(1, 0);
    xSemaphoreGive(block_sem);

    // launch the main task
    xTaskCreate(watchTask, "watch", configMINIMAL_STACK_SIZE, NULL, 4, NULL);
    vTaskStartScheduler();
    return 0;
}