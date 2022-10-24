#pragma once
#include "RnetDefinition.hpp"


class RnetDictionnary {

protected:
    std::vector<RnetDefinition> m_dict;
    bool _triggerAlreadyExists(const RnetDefinition::rnet_msg_t &msg);
    RnetDefinition::rnet_msg_t m_possibleTrigger;
    bool n_newTrigger;
    bool m_triggerOk;
    bool isHeartBeat(const RnetDefinition::rnet_msg_t &msg);
    bool isE(const RnetDefinition::rnet_msg_t &msg);
    bool is9EC(const RnetDefinition::rnet_msg_t &msg);
    bool isC(const RnetDefinition::rnet_msg_t &msg);
    RnetDefinition::rnet_msg_t  get9ECTrigger(const RnetDefinition::rnet_msg_t &msg);

public:
    RnetDictionnary();
    ~RnetDictionnary();
    int addTrigger(const RnetDefinition::rnet_msg_t &msg);
    bool addAnswer(int triggerId, const RnetDefinition::rnet_msg_t &msg);
    int getTriggerId(const RnetDefinition::rnet_msg_t &msg);
    size_t getAnswerLength(int triggerId);
    RnetDefinition::rnet_msg_t getAnswer(int triggerId, int answerId);
    RnetDefinition getDefinition(int triggerId) const {return m_dict[triggerId];}
    bool addPossibleTrigger(const RnetDefinition::rnet_msg_t &msg);
    bool addAnswerToLastTrigger(const RnetDefinition::rnet_msg_t &msg);
    bool isJoystickMsg(const RnetDefinition::rnet_msg_t &msg);
    void print();
    void printToFile(File &f);
    void writeToFile(File &f);
    bool readFromFile(File &f);
    size_t getLength() const { return m_dict.size();}
    void setDone(int id){m_dict[id].setDone();}
    void resetDone();
};