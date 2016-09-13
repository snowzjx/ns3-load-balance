#ifndef TLB_PATH_INFO_H
#define TLB_PATH_INFO_H

#include "ns3/nstime.h"

namespace ns3 {

class TLBPathInfo
{
public:
  uint32_t pathId;
  uint32_t size;
  uint32_t ecnSize;
  Time minRtt;
  bool isRetransmission;
  bool isTimeout;
  bool isProbingTimeout;
  uint32_t flowCounter;
};

}

#endif
