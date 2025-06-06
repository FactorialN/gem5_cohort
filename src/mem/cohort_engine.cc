/*
 * Copyright (c) 2010-2013, 2015 ARM Limited
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

#include "mem/cohort_engine.hh"

#include "base/random.hh"
#include "base/trace.hh"
#include "debug/Drain.hh"
#include "sim/system.hh"
#include "mem/packet_access.hh" 


namespace gem5
{

namespace memory
{

CohortEngine::CohortEngine(const CohortEngineParams &p) :
    ClockedObject(p),
    latency(p.latency),
    latency_var(p.latency_var), bandwidth(p.bandwidth), isBusy(false),
    retryReq(false),
    releaseEvent([this]{ release(); }, name()),
    req_port(name() + ".req_port", *this),
    tickEvent([this]{ tick();}, name()),
    requestorId(0)
    //system(p.system)
{
    
}

Port &
CohortEngine::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "req_port") {
        return req_port;
    }else {
        return ClockedObject::getPort(if_name, idx);
    }
}

bool
CohortEngine::readFromMemory(Addr addr, void *buf, int size)
{
    Request::Flags flags;
    auto req = std::make_shared<Request>(addr, size, flags, requestorId);
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    pkt->dataStatic(buf);  // This sets where the read value goes

    // Functional access directly calls target memory
    req_port.sendFunctional(pkt);

    delete pkt;
    return true;
}



Tick CohortEngine::tick()
{
    uint64_t data = 0;
    //PortProxy memProxy(req_port, system->getCacheLineSize());
    //memProxy.readBlob(queueBaseAddr, &data, sizeof(data));
    //DPRINTF(Drain, "Read value: 0x%016lx\n", data);
    // this 

    // req_port -> fixed address in memory -> check address
    // read state
    uint64_t inhead, intail;
    readFromMemory(queueBaseAddr, &inhead, sizeof(inhead));
    readFromMemory(queueBaseAddr+8, &intail, sizeof(intail));

    if (inhead < intail) {
        // queue is not empty

        // Compute pop address
        Addr dataAddr = (inhead-vqueueBaseAddr)+queueBaseAddr;

        uint64_t value;
        readFromMemory(dataAddr, &value, sizeof(value));

        std::cout << "[Cohort Consumer] Tick:" << curTick() <<" Popped value: 0x" << std::hex << value << " " << dataAddr << " " << inhead << " " << intail << std::endl;
        processEntry(value);
        // Advance head
        inhead+=8;
        writeAddr(queueBaseAddr, inhead);
    }


    schedule(tickEvent, curTick() + pollingInterval);

    // Only tick once for now
    return MaxTick;
}


void
CohortEngine::init()
{
    ClockedObject::init();

    std::cout << "CohortEngine::init() called at tick " << curTick() << std::endl;

    // Create a memory request to QUEUE_ADDR
    queueBaseAddr = 0x85000;
    vqueueBaseAddr = 0x10000000;
    outqueueBaseAddr = 0x86000;
    voutqueueBaseAddr = 0x11000000;
    acclBaseAddr = 0x87000;

    pendingRequests.clear();
}

bool
CohortEngine::readAddr(Addr addr) {
    PacketPtr pkt = buildReadRequest(addr, queueEntrySize);

    if (!pendingRequests.empty()) {
        // Can't send now, queue it
        pendingRequests.push_back(pkt);
        return false;
    }

    if (!req_port.sendTimingReq(pkt)) {
        pendingRequests.push_back(pkt);  // Track this failed request
        return false;
    }
    return true;
}

bool
CohortEngine::writeAddr(Addr addr, uint64_t data) {
    RequestPtr req = std::make_shared<Request>(
        addr,                        // Physical address to write to
        sizeof(data),                     // Size of the write
        Request::UNCACHEABLE, requestorId
    );

    // 2. Allocate a write packet
    PacketPtr pkt = new Packet(req, MemCmd::WriteReq);
    uint64_t *data_copy = new uint64_t(data);
    pkt->dataStatic(data_copy); // You can use dataDynamic if you want to allocate internally

    // 3. Send the request
    if (!pendingRequests.empty()) {
        // Can't send now, queue it
        pendingRequests.push_back(pkt);
        return false;
    }

    if (!req_port.sendTimingReq(pkt)) {
        pendingRequests.push_back(pkt);  // Track this failed request
        return false;
    }
    return true;
}

void
CohortEngine::startup()
{
    // Now it's safe to schedule memory access
    ClockedObject::startup();
    //requestorId = SimObject::getRequestorId(this);
    std::cout << "[Cohort] Starting Cohort Engine "<< std::endl;
    std::cout << "[Cohort] Current RequestorID for is "<< requestorId << std::endl;

    schedule(tickEvent, curTick() + 1000);
    readAddr(queueBaseAddr);
}



bool
CohortEngine::recvTimingReq(PacketPtr pkt)
{
    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    panic_if(!(pkt->isRead() || pkt->isWrite()),
             "Should only see read and writes at memory controller, "
             "saw %s to %#llx\n", pkt->cmdString(), pkt->getAddr());
    Addr addr = pkt->getAddr();

    std::cout << "Trying to access memory at: " << addr << std::endl;
    if (addr == 0x10000000 && pkt->isRead()) {
        // Fabricate a response
        uint64_t result = 2333; // or anything useful
        std::memcpy(pkt->getPtr<uint8_t>(), &result, sizeof(result));
        pkt->makeResponse();
        return true;
    }

    // we should not get a new request after committing to retry the
    // current one, but unfortunately the CPU violates this rule, so
    // simply ignore it for now
    if (retryReq)
        return false;

    // if we are busy with a read or write, remember that we have to
    // retry
    if (isBusy) {
        retryReq = true;
        return false;
    }

    // technically the packet only reaches us after the header delay,
    // and since this is a memory controller we also need to
    // deserialise the payload before performing any write operation
    Tick receive_delay = pkt->headerDelay + pkt->payloadDelay;
    pkt->headerDelay = pkt->payloadDelay = 0;

    // update the release time according to the bandwidth limit, and
    // do so with respect to the time it takes to finish this request
    // rather than long term as it is the short term data rate that is
    // limited for any real memory

    // calculate an appropriate tick to release to not exceed
    // the bandwidth limit
    Tick duration = pkt->getSize() * bandwidth;

    // only consider ourselves busy if there is any need to wait
    // to avoid extra events being scheduled for (infinitely) fast
    // memories
    if (duration != 0) {
        schedule(releaseEvent, curTick() + duration);
        isBusy = true;
    }

    // go ahead and deal with the packet and put the response in the
    // queue if there is one
    bool needsResponse = pkt->needsResponse();

    return true;
}

AddrRange
CohortEngine::getAddrRange() const
{
    // Return a fake, unused, non-overlapping range (e.g., very high)
    return AddrRange(queueBaseAddr, queueBaseAddr+0x10000000);
}

void
CohortEngine::processEntry(uint64_t val){
    
    uint64_t outhead, outtail, acc;
    //readFromMemory(outqueueBaseAddr, &outhead, sizeof(outhead));
    readFromMemory(outqueueBaseAddr+8, &outtail, sizeof(outtail));
    readFromMemory(acclBaseAddr, &acc, sizeof(acc));
    Addr dataAddr = outtail-voutqueueBaseAddr+outqueueBaseAddr ;

    // Advance tail
    std::cout << "[Accelerator] Tick:" << curTick() <<" Processing Acclerator " << acc << " with value " << val << std::endl;
    val+=acc;
    outtail+=8;
    std::cout << "[Cohort Producer] Tick:" << curTick() <<" Pushed value: 0x" << std::hex << val << " " << dataAddr  << " " << outtail << std::endl;
    writeAddr(dataAddr, val);
    writeAddr(outqueueBaseAddr + 8, outtail);

}

bool
CohortEngine::recvTimingResp(PacketPtr pkt)
{
    uint64_t val = pkt->getLE<uint64_t>();

    delete pkt;
    return true;
}

void
CohortEngine::release()
{
    assert(isBusy);
    isBusy = false;
    if (retryReq) {
        retryReq = false;
        //res_port.sendRetryReq();
    }
}


Tick
CohortEngine::getLatency() const
{
    return latency +
        (latency_var ? rng->random<Tick>(0, latency_var) : 0);
}

void
CohortEngine::recvRespRetry()
{

}


DrainState
CohortEngine::drain()
{
    return DrainState::Drained;
}


CohortEngine::RequestQueuePort::RequestQueuePort(
    const std::string &name, CohortEngine &owner)
    : RequestPort(name), owner(owner)
{ }

bool
CohortEngine::RequestQueuePort::recvTimingResp(PacketPtr pkt)
{
    return owner.recvTimingResp(pkt);
}

void
CohortEngine::RequestQueuePort::recvReqRetry()
{
    owner.recvReqRetry();
}



PacketPtr
CohortEngine::buildReadRequest(Addr addr, unsigned size)
{
    RequestPtr req = std::make_shared<Request>(
        addr, size, Request::UNCACHEABLE, requestorId);

    req->setContext(0);  // Optional
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    pkt->allocate();
    return pkt;
}



void
CohortEngine::recvReqRetry()
{
    while (!pendingRequests.empty()) {
        PacketPtr pkt = pendingRequests.front();

        if (req_port.sendTimingReq(pkt)) {
            pendingRequests.pop_front();  // Sent successfully
        } else {
            // Stop trying once a send fails
            break;
        }
    }
}



} // namespace memory
} // namespace gem5
