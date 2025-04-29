from m5.objects import System
from m5.objects import CohortEngine
from m5.objects import TimingSimpleCPU
from m5.objects import Root
from m5.objects import AddrRange
from m5.objects import SrcClockDomain, VoltageDomain
from m5.objects import MemCtrl, DDR3_1600_8x8
from m5.objects import SystemXBar

import m5
from m5.util import convert

system = System()

# Setup basic clock and voltage domains
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

# Setup memory
system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('512MB')]

# CPU
system.cpu = TimingSimpleCPU()

# Memory Bus
system.membus = SystemXBar()

# Connect CPU to membus
system.cpu.icache_port = system.membus.cpu_side_ports
system.cpu.dcache_port = system.membus.cpu_side_ports

# Add Memory Controller
system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports

# 🛠️ Insert your Cohort Engine
cohort = CohortEngine()
cohort.queueBaseAddr = 0x80001000
cohort.mem_port = system.membus.mem_side_ports
system.cohort = cohort

# Interrupt controller for SE mode (not full system)
system.cpu.createInterruptController()

# System port
system.system_port = system.membus.cpu_side_ports

# Root
root = Root(full_system=False, system=system)

# Instantiate
m5.instantiate()

# Run
print("Beginning simulation with Cohort Engine")
exit_event = m5.simulate()
print('Exiting @ tick %i because %s' % (m5.curTick(), exit_event.getCause()))
