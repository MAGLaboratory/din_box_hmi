#!/usr/bin/python3

import minimalmodbus
import datetime, time, signal
from progress.bar import IncrementalBar

p_exit = 0

def signal_handler(sig, frame):
    global p_exit
    p_exit = 1

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

# settings
testing_count = 100000
progress = 4
pnt_time = True
pnt_except = True
sleep_wait = 0.0

timeout_list = []
#timeout_list = [0.10, 0.11, 0.12]

for i in range(3):
    timeout_list.append(i * 0.001 + 0.017)

for target_timeout in timeout_list:
    succ = 0
    fail = 0
    c_co = 0
    cons = 0
    errors = ""
    e_counter = 0
    
    instr = minimalmodbus.Instrument("COM3", 2)
    instr.serial.baudrate = 38400
    instr.serial.timeout = target_timeout
    print("Timeout: " + str(instr.serial.timeout))
    if sleep_wait > 0.0:
        print("Sleep Wait: " + str(sleep_wait))
    if progress == 1 or progress == 4:
        bar = IncrementalBar('Testing', max = testing_count, suffix='%(percent)d%% [%(elapsed_td)s / %(eta)d / %(eta_td)s]')

    if progress == 4:
        print("\033[s")
        
    # while (not p_exit):
    if pnt_time:
        start_time = datetime.datetime.now()
        p_i = 0
    for i in range(testing_count):
        try:
            dummy = instr.read_registers(0x00, 1)
            c_co = 0
            succ += 1
            if progress == 2:
                p_i += 1
                print(".", end = '', flush=True)
            if progress == 1:
                bar.next()
            if progress == 4:
                bar.next()
                print("", end = "\033[u")
        except IOError as err:
            if progress >= 2:
                p_i += 1
                print("x", end = '' if progress == 4 else "\033[s", flush=True)
            if progress == 1:
                bar.next()
            if progress == 4:
                bar.next()
                print("", end = "\033[u")
            if e_counter < 10:
                errors += str(err)
                errors += "\n"
                e_counter += 1
            c_co += 1
            if c_co > cons:
                cons = c_co
            fail += 1
        if progress >= 2 and (p_i % 80 == 79):
            print("", end="\033[nK")
        if sleep_wait > 0.0:
            time.sleep(sleep_wait)
        if p_exit:
            break
    
    print()
    if pnt_time:
        print("Execution time: {}".format(datetime.datetime.now() - start_time))
    if progress == 1:
        bar.finish()
    print("Success: " + str(succ), ", Failure: " + str(fail), end='')
    print(", Consecutive Failures: " + str(cons))
    if pnt_except:
        print("Exceptions encountered:")
        print(errors)
    if p_exit:
        break
    
