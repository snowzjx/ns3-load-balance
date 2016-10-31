/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef TCP_CLOVE_TAG_H
#define TCP_CLOVE_TAG_H

#include "ns3/tag.h"

namespace ns3 {

class TcpCloveTag : public Tag
{
public:

    static TypeId GetTypeId (void);

    TcpCloveTag ();

    uint32_t GetPath (void) const;

    void SetPath (uint32_t path);

    virtual TypeId GetInstanceTypeId (void) const;

    virtual uint32_t GetSerializedSize (void) const;

    virtual void Serialize (TagBuffer i) const;

    virtual void Deserialize (TagBuffer i);

    virtual void Print (std::ostream &os) const;

private:

    uint32_t m_path;

};

}

#endif
