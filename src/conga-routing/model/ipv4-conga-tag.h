#ifndef NS3_IPV4_CONGA_TAG
#define NS3_IPV4_CONGA_TAG

#include "ns3/tag.h"

namespace ns3 {

class Ipv4CongaTag: public Tag
{
public:
    Ipv4CongaTag ();

    static TypeId GetTypeId (void);

    void SetLbTag (uint32_t lbTag);

    uint32_t GetLbTag (void) const;

    void SetCe (uint32_t ce);

    uint32_t GetCe (void) const;

    void SetFbLbTag (uint32_t fbLbTag);

    uint32_t GetFbLbTag (void) const;

    void SetFbMetric (uint32_t fbMetric);

    uint32_t GetFbMetric (void) const;

    virtual TypeId GetInstanceTypeId (void) const;

    virtual uint32_t GetSerializedSize (void) const;

    virtual void Serialize (TagBuffer i) const;

    virtual void Deserialize (TagBuffer i);

    virtual void Print (std::ostream &os) const;

private:
    uint32_t m_lbTag;
    uint32_t m_ce;
    uint32_t m_fbLbTag;
    uint32_t m_fbMetric;
};

}

#endif
