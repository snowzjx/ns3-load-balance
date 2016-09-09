#ifndef NS3_IPV4_XPATH_TAG
#define NS3_IPV4_XPATH_TAG

#include "ns3/tag.h"

namespace ns3 {

class Ipv4XPathTag: public Tag
{
public:
    Ipv4XPathTag ();

    static TypeId GetTypeId (void);

    uint32_t GetPathId (void);

    void SetPathId (uint32_t pathId);

    virtual TypeId GetInstanceTypeId (void) const;

    virtual uint32_t GetSerializedSize (void) const;

    virtual void Serialize (TagBuffer i) const;

    virtual void Deserialize (TagBuffer i);

    virtual void Print (std::ostream &os) const;

private:
    uint32_t m_pathId;
};

}

#endif
