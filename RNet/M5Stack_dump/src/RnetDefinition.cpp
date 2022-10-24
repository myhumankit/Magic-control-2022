#include "RnetDefinition.hpp"


RnetDefinition::RnetDefinition(){

}

RnetDefinition::RnetDefinition(const RnetDefinition &c): m_trigger(c.m_trigger), m_answers(c.m_answers), m_done(false){

}

RnetDefinition::~RnetDefinition(){

}

/* @brief determine if a received message is a trigger
   @param rnet_msg_t : Rnet message
   */
bool RnetDefinition::isTrigger(const RnetDefinition::rnet_msg_t &msg){
    if(m_trigger.canId != msg.canId) return false;
    if((m_trigger.len & 0x0F) != (msg.len  & 0x0F)) return false;
    if(!(m_trigger.len & 0x80)){
        for(int i = 0 ; i < (msg.len & 0x0F) ; i++){
            if(m_trigger.buf[i] != msg.buf[i]) return false;
        }
    }
    return true;
}

/* @brief print trigger and Answers into dict file (format .txt)
   */
void RnetDefinition::printToFile(File &f) {
    //print Trigger Info
    f.printf("[%9d][%x]", m_trigger.tstamp, m_trigger.canId);
    if(m_trigger.len & 0x0F)
        f.printf(":");
    for(int i = 0 ; i < (m_trigger.len & 0x0F) ; i++){
        f.printf("%02x", m_trigger.buf[i]);
    }
    f.printf("\n");

    //print Answers Info
    for (std::vector<RnetDefinition::rnet_msg_t>::iterator it = m_answers.begin() ; it != m_answers.end(); ++it){
        f.printf("\t[%9d][%x]", it->tstamp, it->canId);
        if(it->len & 0x0F)
            f.printf(":");
        for(int i = 0 ; i < (it->len & 0x0F) ; i++){
            f.printf("%02x", it->buf[i]);
        }
        f.printf("\n");
    }

}

/* @brief write trigger and Answers infos on serial (debug)
   */
void RnetDefinition::print() {
    //print Trigger Info
    Serial.printf("[%9d][%x]", m_trigger.tstamp, m_trigger.canId);
    if(m_trigger.len)
        Serial.printf(":");
    for(int i = 0 ; i < (m_trigger.len & 0x0F)  ; i++){
        Serial.printf("%02x", m_trigger.buf[i]);
    }
    Serial.printf("\n");

    //print Answers Info
    for (std::vector<RnetDefinition::rnet_msg_t>::iterator it = m_answers.begin() ; it != m_answers.end(); ++it){
        Serial.printf("\t[%9d][%x]:", it->tstamp, it->canId);
        if(m_trigger.len)
            Serial.printf(":");
        for(int i = 0 ; i < (it->len & 0x0F)  ; i++){
            Serial.printf("%02x", it->buf[i]);
        }
        Serial.printf("\n");
    }

}

/* @brief write trigger and Answers into dict file (format .bin)
   */
void RnetDefinition::writeToFile(File &f){
    f.write((uint8_t*)(&m_trigger),sizeof(RnetDefinition::rnet_msg_t));
    uint32_t answersSize = m_answers.size();
    f.write((uint8_t*)(&answersSize), sizeof(uint32_t));
    for (int i = 0 ; i < answersSize ; i++){
        f.write((uint8_t*)(&(m_answers[i])), sizeof(RnetDefinition::rnet_msg_t));
    }
}

/* @brief read trigger and Answers from dict file (format .bin)
   */
bool RnetDefinition::readFromFile(File &f){
    if(f.read((uint8_t*)(&m_trigger),sizeof(RnetDefinition::rnet_msg_t)) != sizeof(RnetDefinition::rnet_msg_t)) return false;
    uint32_t answersSize = 0;
    if(f.read((uint8_t*)(&answersSize), sizeof(uint32_t)) != sizeof(uint32_t)) return false;
    if(answersSize>0){
        m_answers.reserve(answersSize);
        for(int i = 0 ; i < answersSize ; i++){
            rnet_msg_t ans;
            if(f.read((uint8_t*)(&ans), sizeof(RnetDefinition::rnet_msg_t)) != sizeof(RnetDefinition::rnet_msg_t)) {
                return false;
            } else {
                m_answers.push_back(ans);
            }
        }
    }
    return true;
}