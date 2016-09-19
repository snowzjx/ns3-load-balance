#ifndef IPV4_TLB_PROBING_TAG_H
#define IPV4_TLB_PROBING_TAG_H

#include "ns3/tag.h"
#include "ns3/nstime.h"
#include "ns3/ipv4-address.h"

namespace ns3 {

class Ipv4TLBProbingTag : public Tag
{
public:

    static TypeId GetTypeId (void);

    Ipv4TLBProbingTag ();

    uint16_t GetId (void) const;

    void SetId (uint16_t id);

    uint16_t GetPath (void) const;

    void SetPath (uint16_t path);

    Ipv4Address GetProbeAddres (void) const;

    void SetProbeAddress (Ipv4Address address);

    uint8_t GetIsReply (void) const;

    void SetIsReply (uint8_t isReply);

    Time GetTime (void) const;

    void SetTime (Time time);

    uint8_t GetIsCE (void) const;

    void SetIsCE (uint8_t ce);

    uint8_t GetIsBroadcast (void) const;

    void SetIsBroadcast (uint8_t isBroadcast);

    virtual TypeId GetInstanceTypeId (void) const;

    virtual uint32_t GetSerializedSize (void) const;

    virtual void Serialize (TagBuffer i) const;

    virtual void Deserialize (TagBuffer i);

    virtual void Print (std::ostream &os) const;

private:
    uint16_t m_id;
    uint16_t m_path;
    Ipv4Address m_probeAddress;
    uint8_t  m_isReply;     // 0 for false and 1 for true
    Time     m_time;
    uint8_t  m_isCE;        // 0 for not CE and 1 for CE
    uint8_t  m_isBroadcast; // 0 for not broadcast and 1 for broadcast
};

}
#endif
