#!/usr/bin/python3

import minimalmodbus
import datetime, time, signal
from progress.bar import IncrementalBar
from pynput.keyboard import Key, Controller

p_exit = 0

def signal_handler(sig, frame):
    global p_exit
    p_exit = 1

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

keyboard = Controller()

# settings
testing_count = 2000000
progress = 4  
pnt_time = True
pnt_except = True
error_sleep_wait = 0.001
sleep_wait = 0.0
click_key = Key.f9
clicked = 1 # set to 1 to disable

timeout_list = []
#timeout_list = [0.10, 0.11, 0.12]

for i in range(3):
    timeout_list.append(i * 0.001 + 0.016)

for target_timeout in timeout_list:
    succ = 0
    fail = 0
    c_co = 1 # for resetting the first bit output
    cons = 0
    errors = ""
    e_counter = 0
    
    instr = minimalmodbus.Instrument("COM3", 2)
    instr.serial.baudrate = 38400
    instr.serial.timeout = target_timeout
    instr.clear_buffers_before_each_transaction = False
    print("Timeout: " + str(instr.serial.timeout))
    if sleep_wait > 0.0:
        print("Sleep Wait: " + str(sleep_wait))
    if progress == 1 or progress >= 4:
        bar = IncrementalBar('Testing', max = testing_count, suffix='%(percent)d%% [%(elapsed_td)s / %(eta)d / %(eta_td)s]')

    if progress >= 4:
        print("")
        
    # while (not p_exit):
    if pnt_time:
        start_time = datetime.datetime.now()
        p_i = -1
    for i in range(testing_count):
        try:
            if c_co == 0:
                dummy = instr.read_registers(0x00, 1)
            else:
                instr.write_bit(0, i > 0)
            c_co = 0
            succ += 1
            if progress == 2 or progress == 5:
                p_i += 1
                print("." if progress == 2 else f"\033[F\033[{p_i%80+1}G.\n", end = '', flush=True)
            if progress == 1 or progress >= 4:
                bar.next()
        except IOError as err:
            if progress >= 2:
                p_i += 1
                print("x" if progress == 2 else f"\033[F\033[{p_i%80+1}Gx\n", end = '', flush=True)
            if progress == 1 or progress >= 2:
                bar.next()
            if clicked == 0:
                keyboard.press(click_key)
                keyboard.release(click_key)
                clicked = 1
            if e_counter < 10:
                errors += str(err)
                errors += "\n"
                e_counter += 1
            instr.serial.flush() # sometimes the buffer gets screwy
            if error_sleep_wait > 0.0:
                time.sleep(error_sleep_wait)
            c_co += 1
            if c_co > cons:
                cons = c_co
            fail += 1
        if (progress >= 2) and (p_i > 0) and (p_i % 80 == 79):
            print("\n" if progress < 4 else "\033[1G\033[K\n", end='', flush=True)
        if sleep_wait > 0.0:
            time.sleep(sleep_wait)
        if p_exit:
            break
    
    print()
    if pnt_time:
        print("Execution time: {}".format(datetime.datetime.now() - start_time))
    if progress == 1 or progress >= 4:
        bar.finish()
    print("Success: " + str(succ), ", Failure: " + str(fail), end='')
    print(", Consecutive Failures: " + str(cons))
    if pnt_except:
        print("Exceptions encountered:")
        print(errors)
    if p_exit:
        break
    
