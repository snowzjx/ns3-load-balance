#ifndef TCP_TLB_TAG_H
#define TCP_TLB_TAG_H

#include "ns3/tag.h"
#include "ns3/nstime.h"

namespace ns3 {

class TcpTLBTag : public Tag
{
public:

    static TypeId GetTypeId (void);

    TcpTLBTag ();

    uint32_t GetPath (void) const;

    void SetPath (uint32_t path);

    Time GetTime (void) const;

    void SetTime (Time time);

    virtual TypeId GetInstanceTypeId (void) const;

    virtual uint32_t GetSerializedSize (void) const;

    virtual void Serialize (TagBuffer i) const;

    virtual void Deserialize (TagBuffer i);

    virtual void Print (std::ostream &os) const;

private:
    uint32_t m_path;

    Time m_time;
};

}

#endif
