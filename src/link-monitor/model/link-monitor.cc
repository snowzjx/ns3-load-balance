/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "link-monitor.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <fstream>
#include <sstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LinkMonitor");

NS_OBJECT_ENSURE_REGISTERED (LinkMonitor);

TypeId
LinkMonitor::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LinkMonitor")
            .SetParent<Object> ()
            .SetGroupName ("LinkMonitor")
            .AddConstructor<LinkMonitor> ();

  return tid;
}

LinkMonitor::LinkMonitor ()
{
  NS_LOG_FUNCTION (this);
}

void
LinkMonitor::AddLinkProbe (Ptr<LinkProbe> probe)
{
  m_linkProbes.push_back (probe);
}

void
LinkMonitor::Start (Time startTime)
{
  Simulator::Schedule (startTime, &LinkMonitor::DoStart, this);
}

void
LinkMonitor::Stop (Time stopTime)
{
  Simulator::Schedule (stopTime, &LinkMonitor::DoStop, this);
}

void
LinkMonitor::DoStart (void)
{
  std::vector<Ptr<LinkProbe> >::iterator itr = m_linkProbes.begin ();
  for ( ; itr != m_linkProbes.end (); ++itr)
  {
    (*itr)->Start ();
  }
}

void
LinkMonitor::DoStop (void)
{
  std::vector<Ptr<LinkProbe> >::iterator itr = m_linkProbes.begin ();
  for ( ; itr != m_linkProbes.end (); ++itr)
  {
    (*itr)->Stop ();
  }
}

void
LinkMonitor::OutputToFile (std::string filename, std::string (*formatFunc)(struct LinkProbe::LinkStats))
{
  std::ofstream os (filename.c_str (), std::ios::out|std::ios::binary);

  std::vector<Ptr<LinkProbe> >::iterator itr = m_linkProbes.begin ();

  for ( ; itr != m_linkProbes.end (); ++itr)
  {
    Ptr<LinkProbe> linkProbe = *itr;
    std::map<uint32_t, std::vector<struct LinkProbe::LinkStats> > stats = linkProbe->GetLinkStats ();
    os << linkProbe->GetProbeName () << ": (contain: " << stats.size () << " ports)" << std::endl;
    std::map<uint32_t, std::vector<struct LinkProbe::LinkStats> >::iterator portItr = stats.begin ();
    for ( ; portItr != stats.end (); ++portItr)
    {
      os << "\tPort: " << portItr->first << " (contain " << (portItr->second).size ()  << " entries)"<< std::endl;
      os << "\t\t";
      std::vector<struct LinkProbe::LinkStats>::iterator timeItr = (portItr->second).begin ();
      for ( ; timeItr != (portItr->second).end (); ++timeItr)
      {
        os << formatFunc (*timeItr) << "\t";
      }
      os << std::endl;
    }
    os << std::endl;
  }

  os.close ();
}

std::string
LinkMonitor::DefaultFormat (struct LinkProbe::LinkStats stat)
{
  std::ostringstream oss;
  oss << stat.txLinkUtility << "/"
      << stat.packetsInQueue << "/"
      << stat.bytesInQueue << "/"
      << stat.packetsInQueueDisc << "/"
      << stat.bytesInQueueDisc;
  return oss.str ();
}

}

