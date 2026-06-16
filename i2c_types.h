#include <iostream>
#include <systemc.h>

enum addr {
	ADDR7,
	ADDR10
};

enum rw {
	READ,
	WRITE
};

enum transmitOrReceiver {
	TRANSMIT,
	RECEIVE
}; 

enum ack {
	NACK,
	ACK
};

enum ackRelated {
	NOT_ACK_RELATED,
	ACK_RELATED
};

enum headerOrResponse {
	HEADER,
	RESPONSE
};

enum masterOrSlave {
	SLAVE_MODE,
	MASTER_MODE
};

enum sclStatus {
	IDLE,
	TOGGLING
};

struct i2cDataTlm
{								// default values
	bool start;					// false
	unsigned int address;		// 0x0
	enum addr addr7OrAddr10;	// ADDR7
	enum rw readOrWrite;		// READ
	enum ack ackOrNack;			// NACK
	unsigned int ackCount;		// m_ackCount
	unsigned int data;			// 0x0
	bool stop;					// false
	bool repeatedStart;         //false
};
struct i2cSclTlm
{
	sc_time period;
	enum sclStatus idleOrToggling;
};

bool operator==(const i2cDataTlm& lhs, const i2cDataTlm& rhs);

std::ostream& operator<<(std::ostream& os, const i2cDataTlm& obj);

void sc_trace(sc_core::sc_trace_file* tf, const i2cDataTlm& obj, const std::string& name);


bool operator==(const i2cSclTlm& lhs, const i2cSclTlm& rhs);

std::ostream& operator<<(std::ostream& os, const i2cSclTlm& obj);

void sc_trace(sc_core::sc_trace_file* tf, const i2cSclTlm& obj, const std::string& name);