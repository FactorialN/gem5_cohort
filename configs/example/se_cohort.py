from m5.objects import *
import m5

system = System()

system.clk_domain = SrcClockDomain(clock='1GHz', voltage_domain=VoltageDomain())
system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('512MB')]

system.cpu = TimingSimpleCPU()
system.membus = SystemXBar()

system.cpu.icache_port = system.membus.cpu_side_ports
system.cpu.dcache_port = system.membus.cpu_side_ports

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports

# CohortEngine setup
system.cohort = CohortEngine(system=system, clk_domain=system.clk_domain)
system.cohort.queueBaseAddr = 0x90000000
system.cohort.cpu_side = system.membus.mem_side_ports
system.cohort.mem_side = system.membus.cpu_side_ports

# SE mode workload
process = Process()
process.cmd = ['test_cohort.elf']
system.cpu.workload = process
system.cpu.createThreads()

system.cpu.createInterruptController()
system.system_port = system.membus.cpu_side_ports

root = Root(full_system=False, system=system)
m5.instantiate()

print("Beginning simulation with Cohort Engine")
exit_event = m5.simulate()
print('Exiting @ tick %i because %s' % (m5.curTick(), exit_event.getCause()))
