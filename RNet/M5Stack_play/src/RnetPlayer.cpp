#include "RnetPlayer.hpp"

RnetPlayer::RnetPlayer(MCP_CAN *can):m_can(can),m_nextAnswerId(-1){

}
RnetPlayer::~RnetPlayer(){
    
}

/* @brief determine next timestamp for a msg
   */
bool RnetPlayer::setDefAndTS(const RnetDefinition &def, uint32_t ts){
    m_def = def;
    m_startTS = ts;
    m_nextAnswerId = 0;
    if(m_def.answerLength() > 0){
        m_nextTS = m_def.getAnswer(0).tstamp;
        return true;
    }
    return false;
}

/* @brief send msgs on the Rnet bus
   */
bool RnetPlayer::play(){
    /* if time between two msgs is more than the calculated next timestamp */
    if(m_nextAnswerId >= 0 && (uint32_t)(micros() - m_startTS) >= m_nextTS){
        RnetDefinition::rnet_msg_t nextDef = m_def.getAnswer(m_nextAnswerId);
        m_can->sendMsgBuf(nextDef.canId, nextDef.len, nextDef.buf);
        m_nextAnswerId++;
        if(m_nextAnswerId < m_def.answerLength()){
            m_nextTS = m_def.getAnswer(m_nextAnswerId).tstamp;
        } else {
            m_nextAnswerId = -1;
        }
    }
    return (m_nextAnswerId>=0);
}
