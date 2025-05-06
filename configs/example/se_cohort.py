from m5.objects import *
# from gem5.runtime import get_runtime_isa
from gem5.utils.override import override_mem_mode
from gem5.simulate.simulator import run

# Define system
system = System()
system.clk_domain = SrcClockDomain(clock="1GHz", voltage_domain=VoltageDomain())
system.mem_mode = 'timing'  # Required for memory system simulation

# Set up memory
system.mem_ranges = [AddrRange("512MB")]

# Set up CPU
system.cpu = TimingSimpleCPU()

# Custom memory system: CohortEngine
# Replace this with your actual C++ object name if different
system.membus = SystemXBar()
system.cohort_engine = CohortEngine()
system.cpu.icache_port = system.membus.cpu_side_ports
system.cpu.dcache_port = system.membus.cpu_side_ports

# Connect cohort engine to membus (assuming CohortEngine has a mem_side and cpu_side)
system.cohort_engine.cpu_side = system.membus.mem_side_ports
system.cohort_engine.mem_side = system.membus.cpu_side_ports

# Memory controller
system.mem_ctrl = DDR3_1600_8x8()
system.mem_ctrl.range = system.mem_ranges[0]
system.mem_ctrl.port = system.cohort_engine.mem_side

# Set up process
binary = "test_cohort.elf"  # Your test binary
process = Process()
process.cmd = [binary]
system.cpu.workload = process
system.cpu.createThreads()

# Root object
root = Root(full_system=False, system=system)
override_mem_mode(system)
run()
