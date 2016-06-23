#ifndef NS3_IPV4_CONGA_TAG
#define NS3_IPV4_CONGA_TAG

#include "ns3/tag.h"

namespace ns3 {

class Ipv4CongaTag: public Tag
{
public:
    Ipv4CongaTag ();

    static TypeId GetTypeId (void);

    void SetLbTag (uint8_t lbTag);

    uint8_t GetLbTag (void) const;

    void SetCe (uint8_t ce);

    uint8_t GetCe (void) const;

    void SetFbLbTag (uint8_t fbLbTag);

    uint8_t GetFbLbTag (void) const;

    void SetFbMetric (uint8_t fbMetric);

    uint8_t GetFbMetric (void) const;

    virtual TypeId GetInstanceTypeId (void) const;

    virtual uint32_t GetSerializedSize (void) const;

    virtual void Serialize (TagBuffer i) const;

    virtual void Deserialize (TagBuffer i);

    virtual void Print (std::ostream &os) const;

private:
    uint8_t m_lbTag;
    uint8_t m_ce;
    uint8_t m_fbLbTag;
    uint8_t m_fbMetric;
};

}

#endif
