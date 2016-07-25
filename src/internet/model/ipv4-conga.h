#ifndef IPV4_CONGA_H
#define IPV4_CONGA_H

#include <map>
#include <vector>
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"

namespace ns3
{

struct Flowlet {
  uint32_t pathId;
  /* bool isValid; */
  Time activeTime;
};

struct FeedbackInfo {
  uint32_t ce;
  bool change;
};

class Ipv4Conga : public Object
{

public:
  static TypeId GetTypeId (void);

  Ipv4Conga ();
  ~Ipv4Conga ();

  void SetLeafId (uint32_t leafId);

  void SetAlpha (double alpha);

  void SetTDre (Time time);

  void SetFlowletTimeout (Time timeout);

  void AddAddressToLeafIdMap (Ipv4Address addr, uint32_t leafId);

  /**
   * \brief Conga packet processing routine, each packet should go through this routine
   * \param packet The packet to process
   * \param ipv4Header The ipv4 header
   * \param path The path id, the path id should < availPath
   * \param availPath The number of all available paths
   * \return True if the routine is corrected executed, the path should be referred for the correct path.
   */
  bool ProcessPacket (Ptr<Packet> packet, const Ipv4Header &ipv4Header, uint32_t &path, uint32_t availPath);

  // Dev use
  void InsertCongaToLeafTable (uint32_t destLeafId, std::vector<double> table);

  void EnableEcmpMode ();

private:
  // Variables
  // Used to maintain the round robin
  unsigned long m_feedbackIndex;

  // Parameters

  // Used to determine whether this switch is leaf switch
  bool m_isLeaf;
  uint32_t m_leafId;

  // DRE algorithm related parameters
  Time m_tdre;
  double m_alpha;

  // Flowlet Timeout
  Time m_flowletTimeout;

  // Ip and leaf switch map,
  // used to determine the which leaf switch the packet would go through
  std::map<Ipv4Address, uint32_t> m_ipLeafIdMap;

  // Congestion To Leaf Table
  std::map<uint32_t, std::vector<double> > m_congaToLeafTable;

  // Congestion From Leaf Table
  std::map<uint32_t, std::map<uint32_t, FeedbackInfo> > m_congaFromLeafTable;

  // Flowlet Table
  std::map<uint32_t, Flowlet *> m_flowletTable;

  // Parameters
  // DRE
  std::map<uint32_t, uint32_t> m_XMap;

  EventId m_dreEvent;

  // Dev use
  bool m_ecmpMode;

  // DRE algorithm
  void DreEvent();

  void PrintCongaToLeafTable ();
  void PrintCongaFromLeafTable ();
  void PrintFlowletTable ();
};

}

#endif
