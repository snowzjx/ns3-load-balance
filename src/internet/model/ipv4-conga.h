#ifndef IPV4_CONGA_H
#define IPV4_CONGA_H

#include <map>
#include <vector>
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ipv4-address.h"

namespace ns3
{

struct PathCongestion {
  uint32_t pathId;
  uint8_t ce;
};

class Ipv4Conga : public Object
{

public:
  static TypeId GetTypeId (void);

  Ipv4Conga ();
  ~Ipv4Conga ();

  void SetLeafId (uint32_t leafId);
  void AddAddressToLeafIdMap (Ipv4Address addr, uint32_t leafId);

  uint32_t ProcessPacket (Ptr<Packet> packet);

private:
  // Used to determine whether this switch is leaf switch
  bool m_isLeaf;
  uint32_t m_leafId;

  // Ip and leaf switch map,
  // used to determine the which leaf swithc the packet would go through
  std::map<Ipv4Address, uint32_t> m_ipLeafIdMap;

  // Congestion to Leaf Table
  std::map<uint32_t, std::vector<struct PathCongestion> > m_congToLeafTable;

  // Congestion From Leaf Table
  std::map<uint32_t, std::vector<struct PathCongestion> > m_congFromLeafTable;

  uint8_t m_X;
};

}

#endif
