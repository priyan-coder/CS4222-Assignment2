#include <stdio.h>
#include "contiki.h"
#include "sys/rtimer.h"
#include "board-peripherals.h"
#include <stdint.h>
#include "sys/etimer.h"
#include "buzzer.h"

/* Thresholds for picking up*/
int ACC_X = 20;  // >
int ACC_Y = -11; // <
int ACC_Z = -90; // >

int GYRO_X = -300; // <
int GYRO_Y = 800;  // >
int GYRO_Z = 1000; // >

PROCESS(process_rtimer, "RTimer");    // declare an r-timer for IMU - idle state
PROCESS(process_etimer, "ETimer");    // declare a etimer for buzzer - active state
AUTOSTART_PROCESSES(&process_rtimer); // start r-timer on system boot

static int counter_rtimer;                                 // counter var for r-timer - IMU
static struct rtimer timer_rtimer;                         // create a task
static rtimer_clock_t timeout_rtimer = RTIMER_SECOND / 20; // 20 Hz
static bool moved = false;                                 // movement flag
static int moveCount = 0;                                  // counter for detecting significant movement

static int counter_etimer;  // counter var for e-timer
int buzzerFrequency = 3951; // hgh notes on a piano

static void init_mpu_reading(void);
static void get_mpu_reading(int arr[]);

/*
static void
print_mpu_reading(int reading)
{
    if (reading < 0)
    {
        printf("-");
        reading = -reading;
    }

    printf("%d.%02d", reading / 100, reading % 100);
}
*/

void do_rtimer_timeout(struct rtimer *timer, void *ptr)
{
    /* rtimer period 50ms = 20Hz*/
    int sensor[6];
    // gx, gy, gz, ax, ay, az
    // read the IMU sensor every 0.05s i.e. 20Hz, 20 Samples per second
    rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);

    int s, ms1, ms2, ms3;
    s = clock_time() / CLOCK_SECOND;
    ms1 = (clock_time() % CLOCK_SECOND) * 10 / CLOCK_SECOND;
    ms2 = ((clock_time() % CLOCK_SECOND) * 100 / CLOCK_SECOND) % 10;
    ms3 = ((clock_time() % CLOCK_SECOND) * 1000 / CLOCK_SECOND) % 10;

    counter_rtimer++;
    printf(": %d (cnt) %d (ticks) %d.%d%d%d (sec) \n", counter_rtimer, clock_time(), s, ms1, ms2, ms3);
    get_mpu_reading(sensor);
    if (sensor[0] < GYRO_X || sensor[1] > GYRO_Y || sensor[2] > GYRO_Z || sensor[3] > ACC_X || sensor[4] < ACC_Y || sensor[5] > ACC_Z)
    {
        moveCount += 1;
        if (moveCount > 10)
        {
            moved = true;
            moveCount = 0;
            printf("Detected Motion. Stopped IMU\n");
            return;
        }
    }
}

static void
get_mpu_reading(int arr[])
{
    int value;

    //   printf("MPU Gyro: X=");
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_X);
    arr[0] = value;
    //   print_mpu_reading(value);
    //   printf(" deg/sec\n");

    //   printf("MPU Gyro: Y=");
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Y);
    arr[1] = value;
    //   print_mpu_reading(value);
    //   printf(" deg/sec\n");

    //   printf("MPU Gyro: Z=");
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Z);
    arr[2] = value;
    //   print_mpu_reading(value);
    //   printf(" deg/sec\n");

    //   printf("MPU Acc: X=");
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_X);
    arr[3] = value;
    //   print_mpu_reading(value);
    //   printf(" G\n");

    //   printf("MPU Acc: Y=");
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Y);
    arr[4] = value;
    //   print_mpu_reading(value);
    //   printf(" G\n");

    // printf("MPU Acc: Z=");
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Z);
    arr[5] = value;
    //   print_mpu_reading(value);
    //   printf(" G\n");
}

static void
init_mpu_reading(void)
{
    mpu_9250_sensor.configure(SENSORS_ACTIVE, MPU_9250_SENSOR_TYPE_ALL);
}

// IMU
PROCESS_THREAD(process_rtimer, ev, data)
{
    PROCESS_BEGIN();

    init_mpu_reading();
    // process_event_t movement_detected_event = process_alloc_event();
    while (1)
    {
        if (!moved)
        {
            rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);
        }
        else
        {
            moved = false;
            // process_post(&process_etimer, movement_detected_event, NULL);
            break;
        }
    }
    printf("Out of loop\n");
    process_start(&process_etimer, NULL);
    printf("Started the other process\r\n");
    PROCESS_EXIT();
    printf("Exited the process\r\n");
    PROCESS_END();
}

// Buzzer
PROCESS_THREAD(process_etimer, ev, data)
{
    static struct etimer timer_etimer;
    PROCESS_BEGIN();
    process_exit(&process_rtimer);
    buzzer_init();

    while (1)
    {
        // PROCESS_WAIT_EVENT_UNTIL(ev == movement_detected_event);
        buzzer_start(buzzerFrequency);
        etimer_set(&timer_etimer, 3 * CLOCK_SECOND);
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
        buzzer_stop();
        etimer_set(&timer_etimer, 3 * CLOCK_SECOND);
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
    }

    PROCESS_END();
}
