#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ipv4-conga.h"
#include "ipv4-conga-tag.h"
#include "ns3/flow-id-tag.h"

#include <limits>
#include <time.h>
#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4Conga");

NS_OBJECT_ENSURE_REGISTERED (Ipv4Conga);

TypeId
Ipv4Conga::GetTypeId (void)
{
  static TypeId tid = TypeId("ns3::Ipv4Conga")
      .SetParent<Object>()
      .SetGroupName ("Internet")
      .AddConstructor<Ipv4Conga> ();

  return tid;
}

Ipv4Conga::Ipv4Conga ():
    m_isLeaf (false),
    m_leafId (0),
    m_X (0)
{
  NS_LOG_FUNCTION (this);
  srand((unsigned)time(NULL));
}

Ipv4Conga::~Ipv4Conga ()
{
  NS_LOG_FUNCTION (this);
  std::map<uint32_t, Flowlet *>::iterator itr = m_flowletTable.begin ();
  for ( ; itr != m_flowletTable.end (); ++itr )
  {
    delete (itr->second);
  }

  std::map<uint32_t, std::vector<FeedbackInfo*> >::iterator fbItr = m_congaFromLeafTable.begin ();
  for ( ; fbItr != m_congaFromLeafTable.end (); ++fbItr )
  {
    std::vector<FeedbackInfo *>::iterator fbItemItr = (fbItr->second).begin ();
    for ( ; fbItemItr != (fbItr->second).end (); ++fbItemItr )
    {
      if ((*fbItemItr) != NULL)
      {
        delete (*fbItemItr);
      }
    }
  }
}

void
Ipv4Conga::SetLeafId (uint32_t leafId)
{
  m_isLeaf = true;
  m_leafId = leafId;
}

void
Ipv4Conga::SetFlowletTimeout (double timeout)
{
  m_flowletTimeout = timeout;
}

void
Ipv4Conga::AddAddressToLeafIdMap (Ipv4Address addr, uint32_t leafId)
{
  m_ipLeafIdMap[addr] = leafId;
}

bool
Ipv4Conga::ProcessPacket (Ptr<Packet> packet, Ipv4Header ipv4Header, uint32_t &path, uint32_t availPath)
{
  // Packet arrival time
  double now = Simulator::Now ().GetSeconds ();

  // Extract the flow id
  uint32_t flowId = 0;
  FlowIdTag flowIdTag;
  bool flowIdFound = packet->PeekPacketTag(flowIdTag);
  if (!flowIdFound)
  {
    NS_LOG_ERROR ("Ipv4Conga::ProcessPacket Cannot find the flow id");
    return false;
  }
  flowId = flowIdTag.GetFlowId ();

  // First, check if this switch if leaf switch
  if (m_isLeaf)
  {
    // If the switch is leaf switch, two possible situations
    // 1. The sender is connected to this leaf switch
    // 2. The receiver is connected to this leaf switch
    // We can distinguish it by checking whether the packet has CongaTag
    Ipv4CongaTag ipv4CongaTag;
    bool found = packet->PeekPacketTag(ipv4CongaTag);

    if (!found)
    {
      // When sending a new packet
      // Build an empty Conga header (as the packet tag)
      // Determine the path and fill the header fields

      // Determine the dest switch leaf id
      std::map<Ipv4Address, uint32_t>::iterator itr = m_ipLeafIdMap.find(ipv4Header.GetDestination ());
      if (itr == m_ipLeafIdMap.end ())
      {
        NS_LOG_ERROR ("Ipv4Conga::ProcessPacket Cannot find leaf switch id");
        return false;
      }
      uint32_t destLeafId = itr->second;

      // TODO Check piggyback information
      std::map<uint32_t, std::vector<FeedbackInfo*> >::iterator fbItr =
          m_congaFromLeafTable.find (destLeafId);

      uint8_t fbLbTag = 0;
      uint8_t fbMetric = 0;

      // Piggyback the last changed one
      if (fbItr != m_congaFromLeafTable.end ())
      {
        uint32_t maxChange = 0;
        for (uint32_t pathId = 0; pathId < (fbItr->second).size (); pathId ++)
        {
            if ((fbItr->second)[pathId]->change > maxChange)
          {
            fbLbTag = pathId;
            fbMetric = (fbItr->second)[pathId]->ce;
            maxChange = (fbItr->second)[pathId]->change;
          }
        }
        // Update the Conga From Leaf Table
        (fbItr->second)[fbLbTag]->change = 0;
      }

      // Path determination logic:
      // Firstly, check the flowlet table to see whether there is existing flowlet
      // If not hit, determine the path based on the congestion degree of the link

      // Flowlet table look up
      struct Flowlet *flowlet = NULL;

      // If the flowlet table entry is valid, return the path
      std::map<uint32_t, struct Flowlet *>::iterator flowletItr = m_flowletTable.find (flowId);
      if (flowletItr != m_flowletTable.end ())
      {
        flowlet = flowletItr->second;
        if (flowlet != NULL && // Impossible in normal cases
            now - flowlet->activeTime <= m_flowletTimeout)
        {
          // Do not forget to update the flowlet active time
          flowlet->activeTime = now;

          // Return the path information used for routing routine to select the path
          path = flowlet->pathId;

          // Construct Conga Header for the packet
          ipv4CongaTag.SetLbTag (path);
          ipv4CongaTag.SetCe (0);

          // TODO Piggyback the feedback information
          ipv4CongaTag.SetFbLbTag (fbLbTag);
          ipv4CongaTag.SetFbMetric (fbMetric);
          packet->AddPacketTag(ipv4CongaTag);

          return true;
        }
      }

      // Not hit. Determine the path

      // 1. Select path congestion information based on dest leaf switch id
      std::map<uint32_t, std::vector<double> >::iterator pathInfoItr = m_congaToLeafTable.find (destLeafId);

      std::vector<double> pathCongestions;

      // Initialize Conga To Leaf Table if no entry is found
      if (pathInfoItr == m_congaToLeafTable.end ())
      {
        for (uint32_t pathId = 0; pathId < availPath; pathId ++)
        {
          pathCongestions[pathId] = 0.0f;
        }
        m_congaToLeafTable[destLeafId] = pathCongestions;
      }
      else
      {
        pathCongestions = pathInfoItr->second;
      }

      // 2. Prepare the candidate path
      double minPathCongestion = (std::numeric_limits<double>::max)();
      std::vector<uint32_t> pathCandidates;
      for (uint32_t pathId = 0; pathId < pathCongestions.size(); pathId ++)
      {
        if (pathCongestions[pathId] < minPathCongestion)
        {
          // Strictly better path
          minPathCongestion = pathCongestions[pathId];
          pathCandidates.clear();
          pathCandidates.push_back(pathId);
        }
        if (pathCongestions[pathId] == minPathCongestion)
        {
          // Equally good path
          pathCandidates.push_back(pathId);
        }
      }

      // 3. Select one path from all those candidate paths
      if (flowlet != NULL &&
            std::find(pathCandidates.begin (), pathCandidates.end (), flowlet->pathId) != pathCandidates.end ())
      {
        // Prefer the path cached in flowlet table
        path = flowlet->pathId;
        // Activate the flowlet entry again
        flowlet->activeTime = now;
      }
      else
      {
        // If there are no cached paths, we randomly choose a good path
        path = rand() % availPath;
        if (flowlet == NULL)
        {
          struct Flowlet *newFlowlet = new Flowlet;
          newFlowlet->pathId = path;
          newFlowlet->activeTime = now;
          m_flowletTable[flowId] = newFlowlet;
        }
        else
        {
          flowlet->pathId = path;
          flowlet->activeTime = now;
        }
      }

      // 4. Construct Conga Header for the packet
      ipv4CongaTag.SetLbTag (path);
      ipv4CongaTag.SetCe (0);

      // TODO Piggyback the feedback information
      ipv4CongaTag.SetFbLbTag (fbLbTag);
      ipv4CongaTag.SetFbMetric (fbMetric);
      packet->AddPacketTag(ipv4CongaTag);

      return true;
    }
    else
    {
      // Forwarding the packet to dest

      // Determine the source switch leaf id
      std::map<Ipv4Address, uint32_t>::iterator itr = m_ipLeafIdMap.find(ipv4Header.GetSource ());
      if (itr == m_ipLeafIdMap.end ())
      {
        NS_LOG_ERROR ("Ipv4Conga::ProcessPacket Cannot find leaf switch id");
        return false;
      }
      uint32_t sourceLeafId = itr->second;

      // 1. Update the CongaFromLeafTable
      std::map<uint32_t, std::vector<FeedbackInfo*> >::iterator fromLeafItr = m_congaFromLeafTable.find (sourceLeafId);
      if (fromLeafItr == m_congaFromLeafTable.end () ||
            (fromLeafItr->second)[ipv4CongaTag.GetLbTag ()] == NULL) // No entry
      {
        FeedbackInfo *feedbackInfo = new FeedbackInfo ();
        feedbackInfo->ce = ipv4CongaTag.GetCe ();
        feedbackInfo->change = 1;
        (fromLeafItr->second)[ipv4CongaTag.GetLbTag ()] = feedbackInfo;
      }
      else
      {
        (fromLeafItr->second)[ipv4CongaTag.GetLbTag ()]->ce = ipv4CongaTag.GetCe ();
        (fromLeafItr->second)[ipv4CongaTag.GetLbTag ()]->change++;
      }

      // 2. Update the CongaToLeafTable
      std::map<uint32_t, std::vector<double> >::iterator toLeafItr = m_congaToLeafTable.find(sourceLeafId);
      if (toLeafItr != m_congaToLeafTable.end ())
      {
        (toLeafItr->second)[ipv4CongaTag.GetFbLbTag()] = ipv4CongaTag.GetFbMetric ();
      }
    }

    // Not necessary
    // Remove the Conga Header
    packet->RemovePacketTag (ipv4CongaTag);

    // Pick path using standard ECMP
    path = flowId % availPath;

    return true;
  }
  else
  {
    // If the switch is not leaf swith, DRE algorithm works here
    //
    return true;
  }
}

}
