/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef CONGESTION_PROBING_TAG_H
#define CONGESTION_PROBING_TAG_H

#include "ns3/tag.h"
#include "ns3/nstime.h"

namespace ns3 {

class CongestionProbingTag : public Tag
{
public:

    static TypeId GetTypeId (void);

    CongestionProbingTag ();

    uint32_t GetId (void) const;

    void SetId (uint32_t id);

    uint8_t GetIsReply (void) const;

    void SetIsReply (uint8_t isReply);

    Time GetTime (void) const;

    void SetTime (Time time);

    uint8_t GetIsCE (void) const;

    void SetIsCE (uint8_t ce);

    virtual TypeId GetInstanceTypeId (void) const;

    virtual uint32_t GetSerializedSize (void) const;

    virtual void Serialize (TagBuffer i) const;

    virtual void Deserialize (TagBuffer i);

    virtual void Print (std::ostream &os) const;

private:
    uint32_t m_id;
    uint8_t  m_isReply;     // 0 for false and 1 for true
    Time     m_time;
    uint8_t  m_isCE;        // 0 for not CE and 1 for CE
};

}

#endif
