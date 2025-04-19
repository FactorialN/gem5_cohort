from m5.objects.AbstractMemory import *
from m5.params import *


class CohortEngine(AbstractMemory):
    type = "CohortEngine"
    cxx_header = "mem/cohort_engine.hh"
    cxx_class = "gem5::memory::CohortEngine"

    port = ResponsePort("This port sends responses and receives requests")
    latency = Param.Latency("30ns", "Request to response latency")
    latency_var = Param.Latency("0ns", "Request to response latency variance")
    # The memory bandwidth limit default is set to 12.8GiB/s which is
    # representative of a x64 DDR3-1600 channel.
    bandwidth = Param.MemoryBandwidth(
        "12.8GiB/s", "Combined read and write bandwidth"
    )

    def controller(self):
        # cohort engine doesn't use a MemCtrl
        return self
