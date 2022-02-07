#include <stdio.h>
#include "contiki.h"
#include "sys/rtimer.h"
#include "board-peripherals.h"
#include <stdint.h>

/* Thresholds for picking up*/
int ACC_X = 20;  // >
int ACC_Y = -11; // <
int ACC_Z = -90; // >

int GYRO_X = -300; // <
int GYRO_Y = 800;  // >
int GYRO_Z = 1000; // >

PROCESS(process_rtimer, "RTimer");                         // declare a process
AUTOSTART_PROCESSES(&process_rtimer);                      // start process on system boot
static int counter_rtimer;                                 // our own counter
static struct rtimer timer_rtimer;                         // create a rtimer
static rtimer_clock_t timeout_rtimer = RTIMER_SECOND / 20; // 20 Hz

static void init_mpu_reading(void);
static void get_mpu_reading(int arr[]);

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

void do_rtimer_timeout(struct rtimer *timer, void *ptr)
{
    /* rtimer period 50ms = 20Hz*/
    clock_time_t t;
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
        printf("DETECTED SIGNIFICANT MOVEMENT");
        return;
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

PROCESS_THREAD(process_rtimer, ev, data)
{
    PROCESS_BEGIN();
    init_mpu_reading();

    while (1)
    {
        rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);
        // Here we call the function once and then it will continue to call itself again inside its implementation
        // based on the delay specified
        PROCESS_YIELD(); // yield the current process
    }

    PROCESS_END();
}
