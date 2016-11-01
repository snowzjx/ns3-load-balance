/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_CLOVE_H
#define IPV4_CLOVE_H

#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/ipv4-address.h"

#include <vector>
#include <map>
#include <utility>

#define CLOVE_RUNMODE_EDGE_FLOWLET 0
#define CLOVE_RUNMODE_ECN 1
#define CLOVE_RUNMODE_INT 2

namespace ns3 {

struct CloveFlowlet {
    Time lastSeen;
    uint32_t path;
};

class Ipv4Clove : public Object {

public:
    Ipv4Clove ();
    Ipv4Clove (const Ipv4Clove&);

    static TypeId GetTypeId (void);

    void AddAddressWithTor (Ipv4Address address, uint32_t torId);
    void AddAvailPath (uint32_t destTor, uint32_t path);

    uint32_t GetPath (uint32_t flowId, Ipv4Address saddr, Ipv4Address daddr);

    void FlowRecv (uint32_t path, Ipv4Address daddr, bool withECN);

    bool FindTorId (Ipv4Address daddr, uint32_t &torId);

private:
    uint32_t CalPath (uint32_t destTor);

    Time m_flowletTimeout;
    uint32_t m_runMode;

    std::map<uint32_t, std::vector<uint32_t> > m_availablePath;
    std::map<Ipv4Address, uint32_t> m_ipTorMap;
    std::map<uint32_t, CloveFlowlet> m_flowletMap;

    // Clove ECN
    Time m_halfRTT;
    bool m_disToUncongestedPath;
    std::map<std::pair<uint32_t, uint32_t>, double> m_pathWeight;
    std::map<std::pair<uint32_t, uint32_t>, Time> m_pathECNSeen;
};

}

#endif /* IPV4_CLOVE_H */

