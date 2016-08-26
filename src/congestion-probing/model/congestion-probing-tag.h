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

    uint32_t GetV (void) const;

    void SetV (uint32_t v);

    uint32_t GetIsReply (void) const;

    void SetIsReply (uint32_t isReply);

    Time GetSendTime (void) const;

    void SetSendTime (Time time);

    uint32_t GetIsCE (void) const;

    void SetIsCE (uint32_t ce);

    virtual TypeId GetInstanceTypeId (void) const;

    virtual uint32_t GetSerializedSize (void) const;

    virtual void Serialize (TagBuffer i) const;

    virtual void Deserialize (TagBuffer i);

    virtual void Print (std::ostream &os) const;

private:
    uint32_t m_V;
    uint32_t m_isReply;     // 0 for false and 1 for true
    Time     m_sendTime;
    uint32_t m_isCE;        // 0 for not CE and 1 for CE
};

}

#endif
