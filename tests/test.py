import minimalmodbus

instr = minimalmodbus.Instrument("COM3", 2)
instr.address = 2
instr.serial.baudrate = 38400
instr.serial.timeout = 0.5
instr.debug = 1

print(instr.read_register(0x00, functioncode=4))

instr.write_registers(0, [0x065b, 0x4f66]);

instr.write_bit(0, 0)