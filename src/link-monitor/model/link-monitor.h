/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef LINK_MONITOR_H
#define LINK_MONITOR_H

#include "ns3/object.h"
#include "ns3/nstime.h"
#include "link-probe.h"

#include <vector>
#include <string>

namespace ns3 {

class LinkMonitor : public Object
{
public:

  static TypeId GetTypeId (void);

  static std::string DefaultFormat (struct LinkProbe::LinkStats stat);

  LinkMonitor ();

  void AddLinkProbe (Ptr<LinkProbe> probe);

  void Start (Time startTime);

  void Stop (Time stopTime);

  void OutputToFile (std::string filename, std::string (*formatFunc)(struct LinkProbe::LinkStats));

private:

  void DoStart (void);

  void DoStop (void);

  std::vector<Ptr<LinkProbe> > m_linkProbes;

};

}

#endif /* LINK_MONITOR_H */

