#ifndef NS3_IPV4_ECN_TAG
#define NS3_IPV4_ECN_TAG

#include "ns3/tag.h"
#include "ipv4-header.h"

namespace ns3 {

class Ipv4EcnTag : public Tag
{
public:
  Ipv4EcnTag ();

  void SetEcn (Ipv4Header::EcnType ecn);

  Ipv4Header::EcnType GetEcn (void) const;

  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;

  virtual uint32_t GetSerializedSize (void) const;

  virtual void Serialize (TagBuffer i) const;

  virtual void Deserialize (TagBuffer i);

  virtual void Print (std::ostream &os) const;

private:
  uint8_t m_ipv4Ecn;
};

}

#endif
