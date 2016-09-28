#ifndef TCP_PAUSE_BUFFER_H
#define TCP_PAUSE_BUFFER_H

#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/tcp-header.h"

#include <deque>

namespace ns3 {

struct TcpPauseItem
{
    Ptr<Packet> packet;
    TcpHeader header;
};

class TcpPauseBuffer : public Object
{
public:
    static TypeId GetTypeId (void);

    TcpPauseBuffer ();
    ~TcpPauseBuffer ();

    struct TcpPauseItem GetBufferedItem (void);
    bool HasBufferedItem (void);
    void BufferItem (Ptr<Packet> p, TcpHeader header);

private:
    std::deque<struct TcpPauseItem> m_pauseItems;
};

}

#endif
