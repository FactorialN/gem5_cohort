/*
 * Copyright (c) 2012-2013 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2001-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * CohortEngine declaration
 */

#ifndef __MEM_COHORT_ENGINE_HH__
#define __MEM_COHORT_ENGINE_HH__

#include <list>

#include "base/random.hh"
#include "mem/abstract_mem.hh"
#include "mem/port.hh"
#include "params/CohortEngine.hh"
#include "mem/port_proxy.hh"
#include "sim/system.hh"

namespace gem5
{

namespace memory
{

/**
 * The simple memory is a basic single-ported memory controller with
 * a configurable throughput and latency.
 *
 * @sa  \ref gem5MemorySystem "gem5 Memory System"
 */
class CohortEngine : public ClockedObject
{

  private:

    /**
     * A deferred packet stores a packet along with its scheduled
     * transmission time
     */
    class DeferredPacket
    {

      public:

        const Tick tick;
        const PacketPtr pkt;

        DeferredPacket(PacketPtr _pkt, Tick _tick) : tick(_tick), pkt(_pkt)
        { }
    };

    /**
     * Latency from that a request is accepted until the response is
     * ready to be sent.
     */
    const Tick latency;

    /**
     * Fudge factor added to the latency.
     */
    const Tick latency_var;


    /**
     * Bandwidth in ticks per byte. The regulation affects the
     * acceptance rate of requests and the queueing takes place after
     * the regulation.
     */
    const double bandwidth;

    /**
     * Track the state of the memory as either idle or busy, no need
     * for an enum with only two states.
     */
    bool isBusy;

    /**
     * Remember if we have to retry an outstanding request that
     * arrived while we were busy.
     */
    bool retryReq;


    mutable Random::RandomPtr rng = Random::genRandom();

    /**
     * Release the memory after being busy and send a retry if a
     * request was rejected in the meanwhile.
     */
    void release();

    EventFunctionWrapper releaseEvent;


    /**
     * Detemine the latency.
     *
     * @return the latency seen by the current packet
     */
    Tick getLatency() const;

    /**
     * The port used to read from the shared memory queue
     */
    class RequestQueuePort : public RequestPort
    {
      private:
        CohortEngine &owner;

      public:
        RequestQueuePort(const std::string &name, CohortEngine &owner);

        // implement the two required methods
        bool recvTimingResp(PacketPtr pkt) override;
        void recvReqRetry() override;
    };

    RequestQueuePort req_port;

    /** Request id for all generated traffic */
    RequestorID requestorId;

    PacketPtr retryPkt = nullptr;

    // queue parameters
    Addr queueBaseAddr = 0;
    unsigned queueEntrySize = sizeof(uint64_t);
    unsigned queueLength = 64;
    unsigned headIndex = 0;
    EventFunctionWrapper pollEvent;
    Tick pollingInterval = 10000;

    //System *system;
    //Params<Unsigned> cache_line_size;

    EventFunctionWrapper tickEvent;


  public:

    CohortEngine(const CohortEngineParams &p);

    DrainState drain() override;
    void init() override;
    void startup() override;

    Port &getPort(const std::string &if_name, PortID idx = InvalidPortID) override;


  protected:
    bool recvTimingReq(PacketPtr pkt);
    bool recvTimingResp(PacketPtr pkt);
    void recvRespRetry();
    void recvReqRetry();  // For memory request retry handling
    PacketPtr buildReadRequest(Addr addr, unsigned size);
    void pollQueue();
    AddrRange getAddrRange() const;
    void processEntry(uint64_t val);
    Tick tick();
    bool readAddr(Addr addr);
    bool writeAddr(Addr addr, uint64_t data);
    bool readFromMemory(Addr addr, void *buf, int size);
};

} // namespace memory
} // namespace gem5

#endif //__MEM_COHORT_ENGINE_HH__
