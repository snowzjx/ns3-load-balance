#include "tcp-pause-buffer.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("TcpPauseBuffer");

NS_OBJECT_ENSURE_REGISTERED (TcpPauseBuffer);

TypeId
TcpPauseBuffer::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::TcpPauseBuffer")
        .SetParent<Object> ()
        .SetGroupName ("Internet")
        .AddConstructor<TcpPauseBuffer> ();

    return tid;

}

TcpPauseBuffer::TcpPauseBuffer ()
{
    NS_LOG_FUNCTION (this);
}

TcpPauseBuffer::~TcpPauseBuffer ()
{
    NS_LOG_FUNCTION (this);
}

struct TcpPauseItem
TcpPauseBuffer::GetBufferedItem (void)
{
    struct TcpPauseItem item = m_pauseItems.front ();
    m_pauseItems.pop_front ();
    return item;
}

bool
TcpPauseBuffer::HasBufferedItem (void)
{
    return m_pauseItems.empty ();
}

void
TcpPauseBuffer::BufferItem (Ptr<Packet> p, TcpHeader header)
{
    struct TcpPauseItem pauseItem;
    pauseItem.packet = p;
    pauseItem.header = header;
    m_pauseItems.push_back (pauseItem);
}

}
