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
PROCESS(process_etimer, "ETimer");    // declare a e-timer for buzzer - active state
PROCESS(process_light, "light");      // declare r-timer for light sensor
AUTOSTART_PROCESSES(&process_rtimer); // start r-timer-IMU on system boot

/* IMU */
static int counter_rtimer;                                 // counter var for r-timer - IMU
static struct rtimer timer_rtimer;                         // create a task
static rtimer_clock_t timeout_rtimer = RTIMER_SECOND / 20; // 20 Hz
static bool moved = false;                                 // movement flag
static int moveCount = 0;                                  // counter for detecting significant movement

/* Buzzer */
int buzzerFrequency = 3951; // hgh notes on a piano

/* Light Sensor */
static int counter_light;                                // counter var for light sensor
static struct rtimer light_timer;                        // create a task
static rtimer_clock_t timeout_light = RTIMER_SECOND / 4; // 4Hz
static int prevValue;                                    // var to hold prev Lux value
static bool lightChange = false;                         // flag var to indicate significant light change
static int count_exec = 0;                               // to account for prevValue not derived on first time of execution

static void init_mpu_reading(void);
static void get_mpu_reading(int arr[]);

static void init_opt_reading(void);
static int get_light_reading(void);

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

/* IMU callback */
void do_rtimer_timeout(struct rtimer *timer, void *ptr)
{

    /* rtimer period 50ms = 20Hz*/
    int sensor[6];
    // gx, gy, gz, ax, ay, az
    // read the IMU sensor every 0.05s i.e. 20Hz, 20 Samples per second
    rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);

    // int s, ms1, ms2, ms3;
    // s = clock_time() / CLOCK_SECOND;
    // ms1 = (clock_time() % CLOCK_SECOND) * 10 / CLOCK_SECOND;
    // ms2 = ((clock_time() % CLOCK_SECOND) * 100 / CLOCK_SECOND) % 10;
    // ms3 = ((clock_time() % CLOCK_SECOND) * 1000 / CLOCK_SECOND) % 10;

    counter_rtimer++;
    // printf(": %d (cnt) %d (ticks) %d.%d%d%d (sec) \n", counter_rtimer, clock_time(), s, ms1, ms2, ms3);
    printf("Getting IMU readings\n");
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

/* Light callback */
void do_light(struct rtimer *timer, void *ptr)
{
    /* Re-arm rtimer. Starting up the sensor takes around 125ms */

    int v, diff;

    rtimer_set(&light_timer, RTIMER_NOW() + timeout_light, 0, do_light, NULL);

    // int s, ms1, ms2, ms3;
    // s = clock_time() / CLOCK_SECOND;
    // ms1 = (clock_time() % CLOCK_SECOND) * 10 / CLOCK_SECOND;
    // ms2 = ((clock_time() % CLOCK_SECOND) * 100 / CLOCK_SECOND) % 10;
    // ms3 = ((clock_time() % CLOCK_SECOND) * 1000 / CLOCK_SECOND) % 10;

    counter_light++;
    // printf(": %d (cnt) %d (ticks) %d.%d%d%d (sec) \n", counter_light, clock_time(), s, ms1, ms2, ms3);
    // printf("Getting Light reading\n");
    v = get_light_reading();
    if (count_exec)
    {
        diff = abs(prevValue - v) / 100;
        if (diff > 300)
        {
            lightChange = true;
            return;
        }
    }
    prevValue = v;
    count_exec += 1;
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

static int
get_light_reading()
{
    int value;

    value = opt_3001_sensor.value(0);
    if (value != CC26XX_SENSOR_READING_ERROR)
    {
        printf("OPT: Light=%d.%02d lux\n", value / 100, value % 100);
        return value;
    }
    else
    {
        printf("OPT: Light Sensor's Warming Up\n\n");
    }
    init_opt_reading();
}

static void
init_opt_reading(void)
{
    SENSORS_ACTIVATE(opt_3001_sensor);
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
    init_opt_reading();

    if (process_is_running(&process_etimer))
    {
        process_exit(&process_etimer);
    }

    printf("Started IMU, in IDLE\n");
    // process_event_t movement_detected_event = process_alloc_event();
    while (1)
    {
        if (moved)
        {
            moved = false;
            process_start(&process_light, NULL);
            process_start(&process_etimer, NULL);
            printf("Going to ACTIVE state\n");

            // process_post(&process_etimer, movement_detected_event, NULL);
            break;
        }
        else
        {
            rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);
            // PROCESS_YIELD();
        }
    }
    printf("Out of loop\n");
    PROCESS_EXIT();

    // process_start(&process_etimer, NULL);

    // printf("Exited the process\r\n");
    PROCESS_END();
}

// Buzzer
PROCESS_THREAD(process_etimer, ev, data)
{
    static struct etimer timer_etimer;
    PROCESS_BEGIN();

    if (process_is_running(&process_rtimer))
    {
        process_exit(&process_rtimer);
    }

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

// Light sensor
PROCESS_THREAD(process_light, ev, data)
{
    PROCESS_BEGIN();
    if (process_is_running(&process_rtimer))
    {
        process_exit(&process_rtimer);
    }
    // process_start(&process_etimer, NULL);
    init_opt_reading();
    while (1)
    {
        if (lightChange)
        {
            printf("Light change detected\n");
            lightChange = false;
            count_exec = 0;
            process_exit(&process_etimer);
            process_start(&process_rtimer, NULL);
            printf("Out of the Light process, going to IDLE state\n");
            break;
        }
        else
        {
            rtimer_set(&light_timer, RTIMER_NOW() + timeout_light, 0, do_light, NULL);
            PROCESS_YIELD();
        }
    }
    PROCESS_EXIT();
    PROCESS_END();
}
