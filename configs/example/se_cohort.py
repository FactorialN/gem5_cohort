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
system.cohort = CohortEngine(clk_domain=system.clk_domain)
system.cohort.queueBaseAddr = 0x10000000
#system.cohort.res_port = system.membus.mem_side_ports
print("Available ports on cohort:", system.cohort._ports)

system.cohort.req_port = system.membus.cpu_side_ports


# SE mode workload
binary_path = '/596/gem5_cohort/test_cohort.elf'
system.workload = SEWorkload.init_compatible(binary_path)

process = Process(pid=100)
process.cmd = [binary_path]
process.executable = binary_path
system.cpu.workload = process
system.cpu.createThreads()

system.cpu.createInterruptController()
system.system_port = system.membus.cpu_side_ports

root = Root(full_system=False, system=system)
m5.instantiate()

#system.cohort.requestor_id = system.getRequestorId(system.cohort, "cohort_engine")

print("Beginning simulation with Cohort Engine")
exit_event = m5.simulate()
print('Exiting @ tick %i because %s' % (m5.curTick(), exit_event.getCause()))
