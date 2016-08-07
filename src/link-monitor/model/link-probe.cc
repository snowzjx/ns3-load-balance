/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "link-probe.h"

#include "link-monitor.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LinkProbe");

NS_OBJECT_ENSURE_REGISTERED (LinkProbe);

TypeId
LinkProbe::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LinkProbe")
        .SetParent<Object> ()
        .SetGroupName ("LinkProbe");

  return tid;
}

LinkProbe::LinkProbe (Ptr<LinkMonitor> linkMonitor)
{
  linkMonitor->AddLinkProbe (this);
}

std::map<uint32_t, std::vector<struct LinkProbe::LinkStats> >
LinkProbe::GetLinkStats (void)
{
  return m_stats;
}

void
LinkProbe::SetProbeName (std::string name)
{
  m_probeName = name;
}

std::string
LinkProbe::GetProbeName (void)
{
  return m_probeName;
}

}

