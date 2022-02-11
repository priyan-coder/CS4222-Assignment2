# CS4222-Assignment2
> Value of CLOCK_SECOND is 128. 
###### 128 clock ticks per second for etimer. 
> Value of RTIMER_SECOND is 65536. 
###### 65536 clock ticks per second for rtimer. 

## Compile Instructions
------------------
```bash
# generate bin file
make TARGET=srf06-cc26xx BOARD=sensortag/cc2650 buzz.bin CPU_FAMILY=cc26xx
``` 
#### Load buzz.bin into UniFlash

#### cd into `contiki/tools/sky`

```bash 
ls /dev/tty* | grep usb
```

```bash
# Obtain XXXXX from prev cmd
./serialdump-macos -b115200 /dev/tty.usbmodemXXXXX
```

#### Press the reset button on the sensortag

## Usage Instructions
------------------

1) Place the sensor tag on a steady table.
    - Assuming you have already run the compile instructions, you will be greeted with "IDLE state... Ready for any movement!" on the terminal. 
2) Pick it up or move erratically.
    - If a significant movement has been detected, "Going to ACTIVE state" will be displayed on the terminal. 
    - Buzzer will buzz in 3 sec intervals, unless a significant light change of > 300 lux is detected. 
3) The diff will be printed on the terminal. This corresponds to the difference between light intensity readings detected. To achieve significant light change, aim for > 300 diff.
    - Upon significant light change detection, the "Light change detected" notification will be displayed on the terminal. 
    - Thereafter you have 2.2 seconds to place the sensor tag on a steady table to ensure that there is not back to back active state transition caused by erratic movements, which would keep the buzzer buzzing. 
4) Place the sensor tag on a steady table in the given 2.2sec window, upon light change detection.
    -  System will be back to IDLE state.
5) You can terminate the system using Ctrl + C. 


