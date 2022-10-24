#include "RnetDictionnary.hpp"
    
RnetDictionnary::RnetDictionnary():n_newTrigger(true),m_triggerOk(false){

}
RnetDictionnary::~RnetDictionnary(){

}

/* @brief add new trigger into m_dict (RnetDefinition vector)
   @return dictionnary size to ckeck if trigger addition was done without issue
   */
int RnetDictionnary::addTrigger(const RnetDefinition::rnet_msg_t &msg){

    RnetDefinition newDef;
    newDef.setTrigger(msg);
    m_dict.push_back(newDef);
    return m_dict.size() - 1;
}

/* @brief add new answer related to a trigger into m_dict (RnetDefinition vector)
   */
bool RnetDictionnary::addAnswer(int triggerId, const RnetDefinition::rnet_msg_t &msg){
    /* if no trigger in the dict, answer not added */
    if(triggerId >= m_dict.size()) 
        return false;

    RnetDefinition::rnet_msg_t ans = msg;
    ans.tstamp -= m_dict[triggerId].getTriggerTimeStamp();
    m_dict[triggerId].addAnswer(ans);
    return true;
}

/* @brief browse the dictionnary and return trigger id
   */
int RnetDictionnary::getTriggerId(const RnetDefinition::rnet_msg_t &msg){
    bool foundTrigger = false;
    bool foundPossibleTrigger = false;
    int cpt = 0;
    int lastPossibleId = 0;
    int triggerId = 0;
    for (std::vector<RnetDefinition>::iterator it = m_dict.begin() ; it != m_dict.end(); ++it, cpt ++){
        if(it->isTrigger(msg)){
            if(!(it->isDone())){
                foundTrigger = true;
                triggerId = cpt;
                break;
            } else {
                foundPossibleTrigger = true;
                lastPossibleId = cpt;
            }
        }
    }
    if(foundTrigger) 
        return triggerId;
    if(foundPossibleTrigger)
        return lastPossibleId;
    return -1;
}

/* @brief get answer length
   @param: trigger id related to the answer
   */
size_t RnetDictionnary::getAnswerLength(int triggerId){
    return m_dict[triggerId].answerLength();
}

/* @brief get answer from the dictionnary 
   @param: trigger id and answer id
   */
RnetDefinition::rnet_msg_t RnetDictionnary::getAnswer(int triggerId, int answerId){
    return m_dict[triggerId].getAnswer(answerId);
}

/* @brief determine if possible trigger 
   */
bool RnetDictionnary::addPossibleTrigger(const RnetDefinition::rnet_msg_t &msg){
    /* eliminate known messages that are definitely not trigger (heartbeat, 0x0E or joystick msgs) */
    if(isHeartBeat(msg) || isE(msg) || isJoystickMsg(msg) || isC(msg)) 
        return false;
    m_possibleTrigger = msg;
    n_newTrigger = true;
    return true;
}

/* @brief add answer related to a trigger into the dictionnary  
   */
bool RnetDictionnary::addAnswerToLastTrigger(const RnetDefinition::rnet_msg_t &msg){
    /* eliminate known messages that are definitely not trigger (heartbeat, 0x0E or joystick msgs) */    
    if(isHeartBeat(msg) || isE(msg) || isJoystickMsg(msg) || isC(msg)) 
        return false;

    if(is9EC(msg) && is9EC(m_possibleTrigger)){
        addTrigger(get9ECTrigger(msg));
        m_triggerOk = true;
    } else {
        /* if possible trigger has not already been added to the dictionnary */
        if(n_newTrigger){
            n_newTrigger = false;
            if(addTrigger(m_possibleTrigger) < 0){
                m_triggerOk = false;
                return false;
            }
            m_triggerOk = true;
        }
    }
    /* if trigger ok, then add the related answer */
    if(m_triggerOk)
        return addAnswer(m_dict.size()-1, msg);
    else
        return false;
}

/* @brief determine if a msg is a heartbeat message  
   */
bool RnetDictionnary::isHeartBeat(const RnetDefinition::rnet_msg_t &msg){
    if(msg.canId == 0x83c30f0f && msg.len == 7){
        bool ok = true;
        for(int i = 0 ; i < 7  && ok ; i++){
            ok = (msg.buf[i] == 0x87);
        }
        return ok;
    }
    return false;
}

/* @brief determine if a msg is a 0x0E message  
   */
bool RnetDictionnary::isE(const RnetDefinition::rnet_msg_t &msg){
    if(msg.canId == 0xE && msg.len == 8)
        return true;
    else
        return false;
}

bool RnetDictionnary::isC(const RnetDefinition::rnet_msg_t &msg){
    if(msg.canId == 0xC)
        return true;
    else
        return false;
}

/* @brief determine if a msg is a joystick message  
   */
bool RnetDictionnary::isJoystickMsg(const RnetDefinition::rnet_msg_t &msg){
    if(((msg.canId & 0xFFFFF0FF) == 0x82000000) && msg.len == 2)
        return true;
    else
        return false;
}

bool RnetDictionnary::is9EC(const RnetDefinition::rnet_msg_t &msg){
    if(((msg.canId & 0xFFF00000) == 0x9EC00000) && ((msg.canId & 0x000F0000) != 0))
        return true;
    else
        return false;
}

RnetDefinition::rnet_msg_t RnetDictionnary::get9ECTrigger(const RnetDefinition::rnet_msg_t &msg){
    RnetDefinition::rnet_msg_t ret;
    ret.canId = msg.canId - 0x00010000;
    ret.len = msg.len | 0x80;
    ret.tstamp = msg.tstamp - 1000;
    return ret;
}

/* @brief browse the dictionnary vector and print to txt dictionnary file
   */
void RnetDictionnary::printToFile(File &f){
    for (std::vector<RnetDefinition>::iterator it = m_dict.begin() ; it != m_dict.end(); ++it){
        it->printToFile(f);
    }
}
/* @brief print on serial (debug)
   */
void RnetDictionnary::print(){
    for (std::vector<RnetDefinition>::iterator it = m_dict.begin() ; it != m_dict.end(); ++it){
        it->print();
    }
}

/* @brief browse the dictionnary vector and write to bin dictionnary file
   */
void RnetDictionnary::writeToFile(File &f){
    uint32_t dictSize = m_dict.size();
    f.write((uint8_t*)(&dictSize), sizeof(uint32_t));
    for (std::vector<RnetDefinition>::iterator it = m_dict.begin() ; it != m_dict.end(); ++it){
        it->writeToFile(f);
    }
}

/* @brief read bin dictionnary file
   */
bool RnetDictionnary::readFromFile(File &f){
    uint32_t dictSize = 0;
    if(f.read((uint8_t*)(&dictSize), sizeof(uint32_t)) != sizeof(uint32_t)) return false;
    if(dictSize > 0){
        m_dict.reserve(dictSize);
        bool ok = true;
        for(int i = 0 ; i < dictSize ; i++){
            RnetDefinition def;
            ok = def.readFromFile(f);
            if(ok)
                m_dict.push_back(def);
        }
        return ok;
    }
    return false;
}


void RnetDictionnary::resetDone(){
    for (std::vector<RnetDefinition>::iterator it = m_dict.begin() ; it != m_dict.end(); ++it){
        it->resetDone();
    }
}