#pragma once
#include <inttypes.h>
#include <vector>
#include "FS.h"

class RnetDefinition {
public:
    typedef struct{
    unsigned long tstamp;
    unsigned long canId;
    uint8_t len;
    uint8_t direction;
    uint8_t buf[8];
    } rnet_msg_t;

protected:
    rnet_msg_t m_trigger;
    std::vector<rnet_msg_t> m_answers;
    bool m_done;

public:
    RnetDefinition();
    ~RnetDefinition();

    RnetDefinition(const RnetDefinition &);
    
    inline rnet_msg_t getTrigger() const { return m_trigger;}
    inline void setTrigger(const rnet_msg_t &msg){m_trigger = msg;}
    bool isTrigger(const rnet_msg_t &msg);
    inline uint32_t getTriggerTimeStamp()const{return m_trigger.tstamp;}

    inline rnet_msg_t getAnswer(int id) const {return m_answers[id];}
    inline void addAnswer(const rnet_msg_t &msg){m_answers.push_back(msg);}
    inline size_t answerLength() const {return m_answers.size();}
    inline bool isDone() const { return m_done;}
    inline void setDone(){m_done = true;}
    inline void resetDone(){m_done = false;}

    void printToFile(File &f);
    void print();
    void writeToFile(File &f);
    bool readFromFile(File &f);
};
