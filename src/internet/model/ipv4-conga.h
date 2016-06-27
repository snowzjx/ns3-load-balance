#ifndef IPV4_CONGA_H
#define IPV4_CONGA_H

#include <map>
#include <vector>
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"

namespace ns3
{

struct Flowlet {
  uint32_t pathId;
  /* bool isValid; */
  double activeTime;
};

struct FeedbackInfo {
  uint8_t ce;
  uint32_t change;
};

class Ipv4Conga : public Object
{

public:
  static TypeId GetTypeId (void);

  Ipv4Conga ();
  ~Ipv4Conga ();

  void SetLeafId (uint32_t leafId);

  void SetFlowletTimeout (double timeout);

  void AddAddressToLeafIdMap (Ipv4Address addr, uint32_t leafId);

  /**
   * \brief Conga packet processing routine, each packet should go through this routine
   * \param packet The packet to process
   * \param ipv4Header The ipv4 header
   * \param path The path id, the path id should < availPath
   * \param availPath The number of all available paths
   * \return True if the routine is corrected executed, the path should be referred for the correct path.
   */
  bool ProcessPacket (Ptr<Packet> packet, Ipv4Header ipv4Header, uint32_t &path, uint32_t availPath);

private:
  // Used to determine whether this switch is leaf switch
  bool m_isLeaf;
  uint32_t m_leafId;

  // Ip and leaf switch map,
  // used to determine the which leaf swithc the packet would go through
  std::map<Ipv4Address, uint32_t> m_ipLeafIdMap;

  // Congestion to Leaf Table
  std::map<uint32_t, std::vector<double> > m_congaToLeafTable;

  // Congestion From Leaf Table
  std::map<uint32_t, std::vector<FeedbackInfo *> > m_congaFromLeafTable;

  // Flowlet Table
  std::map<uint32_t, Flowlet *> m_flowletTable;

  // Parameters
  // DRE
  uint8_t m_X;

  // Flowlet Timeout
  double m_flowletTimeout;
};

}

#endif
