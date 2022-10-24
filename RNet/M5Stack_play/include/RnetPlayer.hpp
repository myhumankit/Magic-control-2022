#pragma once
#include "RnetDefinition.hpp"
#include <mcp_can.h>
#include <inttypes.h>


class RnetPlayer {
protected:
    uint32_t m_startTS;
    RnetDefinition m_def;
    int32_t m_nextAnswerId;
    uint32_t m_nextTS;
    MCP_CAN *m_can;

public:
    RnetPlayer(MCP_CAN *can);
    ~RnetPlayer();
    bool setDefAndTS(const RnetDefinition &def, uint32_t ts);
    bool play();
    inline bool isFinished() const {return m_nextAnswerId < 0;}
};
