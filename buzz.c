#include <stdint.h>
#include <stdio.h>

#include "board-peripherals.h"
#include "buzzer.h"
#include "contiki.h"
#include "stdlib.h"
#include "sys/etimer.h"
#include "sys/rtimer.h"

/* Thresholds for picking up*/
int ACC_X = 20;   // >
int ACC_Y = -11;  // <
int ACC_Z = -90;  // >

int GYRO_X = -300;  // <
int GYRO_Y = 800;   // >
int GYRO_Z = 1000;  // >

PROCESS(process_rtimer, "RTimer");  // declare a r-timer for IMU - IDLE state
PROCESS(process_etimer,
        "ETimer");                     // declare an e-timer for buzzer - ACTIVE state
PROCESS(process_light, "light");       // declare r-timer for light sensor
AUTOSTART_PROCESSES(&process_rtimer);  // start r-timer-IMU on system boot

/* IMU */
static struct rtimer timer_rtimer;                          // create a task
static rtimer_clock_t timeout_rtimer = RTIMER_SECOND / 20;  // 20 Hz
static bool moved = false;                                  // movement flag
static int moveCount = 0;                                   // counter for detecting significant movement

/* Buzzer */
int buzzerFrequency = 3951;

/* Light Sensor */
static struct rtimer light_timer;                         // create a task
static rtimer_clock_t timeout_light = RTIMER_SECOND / 4;  // 4Hz
static int prevValue = 0;                                 // var to hold prev Lux value
static bool lightChange =
    false;                  // flag var to indicate significant light change
static int count_exec = 0;  // to account for prevValue which is not derived on
                            // first time of execution
static int value = 0;
static int diff = 0;

static void init_mpu_reading(void);
static void get_mpu_reading(int arr[]);

static void init_opt_reading(void);
static void get_light_reading();

/* IMU callback */
void do_rtimer_timeout(struct rtimer *timer, void *ptr) {
    if (!moved) {
        /* rtimer period 50ms = 20Hz*/
        int sensor[6];
        // gx, gy, gz, ax, ay, az
        // read the IMU sensor every 0.05s i.e. 20Hz, 20 Samples per second
        rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0,
                   do_rtimer_timeout, NULL);
        get_mpu_reading(sensor);
        if (sensor[0] < GYRO_X || sensor[1] > GYRO_Y || sensor[2] > GYRO_Z ||
            sensor[3] > ACC_X || sensor[4] < ACC_Y || sensor[5] > ACC_Z) {
            moveCount += 1;
            if (moveCount > 10) {
                moved = true;
                moveCount = 0;
                printf("Detected Significant motion\n");
            }
        }
    }
}

/* Light callback */
void do_light(struct rtimer *timer, void *ptr) {
    if (!lightChange) {
        /* Re-arm rtimer. Starting up the sensor takes around 125ms */
        rtimer_set(&light_timer, RTIMER_NOW() + timeout_light, 0, do_light,
                   NULL);
        get_light_reading();
    }
}

static void get_mpu_reading(int arr[]) {
    int value;
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_X);
    arr[0] = value;
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Y);
    arr[1] = value;
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Z);
    arr[2] = value;
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_X);
    arr[3] = value;
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Y);
    arr[4] = value;
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Z);
    arr[5] = value;
}

static void get_light_reading() {
    value = opt_3001_sensor.value(0);
    if (value != CC26XX_SENSOR_READING_ERROR) {
        // printf("OPT: Light=%d.%02d lux\n", value / 100, value % 100);
    } else {
        printf("OPT: Light Sensor's Warming Up\n\n");
    }
    init_opt_reading();
}

static void init_opt_reading(void) { SENSORS_ACTIVATE(opt_3001_sensor); }

static void init_mpu_reading(void) {
    mpu_9250_sensor.configure(SENSORS_ACTIVE, MPU_9250_SENSOR_TYPE_ALL);
}

// IMU
PROCESS_THREAD(process_rtimer, ev, data) {
    PROCESS_BEGIN();
    static struct etimer E;
    init_mpu_reading();
    if (process_is_running(&process_etimer)) {
        buzzer_stop();
        process_exit(&process_etimer);
    }
    if (process_is_running(&process_light)) {
        process_exit(&process_light);
    }

    printf("You have 1.5 seconds.Place the sensor tag on a steady table NOW!\n");
    etimer_set(&E, 1.5 * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
    printf("IDLE state... Ready for any movement!\n");
    // process_event_t movement_detected_event = process_alloc_event();
    while (1) {
        if (moved) {
            moved = false;
            process_start(&process_light, NULL);
            process_start(&process_etimer, NULL);
            printf("Going to ACTIVE state\n");
            // process_post(&process_etimer, movement_detected_event, NULL);
            break;
        } else {
            rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0,
                       do_rtimer_timeout, NULL);
            // PROCESS_YIELD();
        }
    }
    // printf("Out of loop\n");
    PROCESS_YIELD();
    // process_start(&process_etimer, NULL);
    // printf("Exited the process\r\n");
    PROCESS_END();
}

// Buzzer
PROCESS_THREAD(process_etimer, ev, data) {
    static struct etimer timer_etimer;
    PROCESS_BEGIN();
    if (process_is_running(&process_rtimer)) {
        process_exit(&process_rtimer);
    }
    buzzer_init();
    while (1) {
        if (lightChange) {
            buzzer_stop();
            break;
        }
        // PROCESS_WAIT_EVENT_UNTIL(ev == movement_detected_event);
        buzzer_start(buzzerFrequency);
        etimer_set(&timer_etimer, 3 * CLOCK_SECOND);
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
        buzzer_stop();
        etimer_set(&timer_etimer, 3 * CLOCK_SECOND);
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
    }
    PROCESS_YIELD();
    PROCESS_END();
}

// Light sensor
PROCESS_THREAD(process_light, ev, data) {
    PROCESS_BEGIN();
    if (process_is_running(&process_rtimer)) {
        process_exit(&process_rtimer);
    }
    init_opt_reading();
    static struct etimer et;
    while (1) {
        if (lightChange) {
            lightChange = false;
            printf("Light change detected\n");
            count_exec = 0;
            prevValue = 0;
            value = 0;
            process_exit(&process_etimer);
            printf("Going to IDLE state\n");
            break;
        } else {
            etimer_set(&et, CLOCK_SECOND);
            PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
            rtimer_set(&light_timer, RTIMER_NOW() + timeout_light, 0, do_light,
                       NULL);
            if (count_exec > 0) {
                diff = abs(value - prevValue);
                printf("diff: %d\n", diff);
                if (diff > 300) {
                    lightChange = true;
                }
            }
            prevValue = value;
            count_exec += 1;
        }
    }
    process_start(&process_rtimer, NULL);
    PROCESS_YIELD();
    PROCESS_END();
}
