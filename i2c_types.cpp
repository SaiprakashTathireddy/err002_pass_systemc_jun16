#include "i2c_types.h"


bool operator==(const i2cDataTlm& lhs, const i2cDataTlm& rhs)
{
	return	lhs.start == rhs.start && 
			lhs.address == rhs.address &&
			lhs.addr7OrAddr10 == rhs.addr7OrAddr10 &&
			lhs.readOrWrite == rhs.readOrWrite &&
			lhs.ackOrNack == rhs.ackOrNack &&
			lhs.ackCount == rhs.ackCount &&
			lhs.stop == rhs.stop &&
			lhs.repeatedStart == rhs.repeatedStart &&
			lhs.data == rhs.data;


}


std::ostream& operator<<(std::ostream& os, const i2cDataTlm& obj)
{
	os	<< " { start : " << obj.start
		<< " address: " << obj.address
		<< " addr7 or addr10 " << obj.addr7OrAddr10
		<< " read or write " << obj.readOrWrite
		<< " ack or nack " << obj.ackOrNack
		<< " ack count " << obj.ackCount
		<< " repeated start: " << obj.repeatedStart
		<< " data: " << obj.data
		<< " stop: " << obj.stop
		<< " } ";
	return os;
}



void sc_trace(sc_core::sc_trace_file* tf, const i2cDataTlm& obj, const std::string& name)
{
	sc_trace(tf, obj.start, name + ".start");
	sc_trace(tf, obj.address, name + ".address");
	sc_trace(tf, obj.addr7OrAddr10, name + ".addr7OrAddr10");
	sc_trace(tf, obj.readOrWrite, name + ".readOrWrite");
	sc_trace(tf, obj.ackOrNack, name + ".ackOrNack");
	sc_trace(tf, obj.ackCount, name + ".ackCount");
	sc_trace(tf, obj.repeatedStart, name + ".repeatedStart");
	sc_trace(tf, obj.data, name + ".data");
	sc_trace(tf, obj.stop, name + ".stop");
}


bool operator==(const i2cSclTlm& lhs, const i2cSclTlm& rhs)
{
	return	lhs.period == rhs.period && 
			lhs.idleOrToggling == rhs.idleOrToggling;
}

std::ostream& operator<<(std::ostream& os, const i2cSclTlm& obj)
{
	os	<< " { period: " << obj.period
		<< " idleOrToggling: " << obj.idleOrToggling
		<< " } ";
	return os;
}

void sc_trace(sc_core::sc_trace_file* tf, const i2cSclTlm& obj, const std::string& name)
{
	sc_trace(tf, obj.period, name + ".period");
	sc_trace(tf, obj.idleOrToggling, name + ".idleOrToggling");
}