/*****************************************************************************

  Licensed to Accellera Systems Initiative Inc. (Accellera) under one or
  more contributor license agreements.  See the NOTICE file distributed
  with this work for additional information regarding copyright ownership.
  Accellera licenses this file to you under the Apache License, Version 2.0
  (the "License"); you may not use this file except in compliance with the
  License.  You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied.  See the License for the specific language governing
permissions and limitations under the License.

 *****************************************************************************/

/*****************************************************************************

  i2c.h : i2c Model class definition

  Original Author: Rajashekar G S, SiliconPatterns, 2026

 *****************************************************************************/

/*****************************************************************************

  MODIFICATION LOG - modifiers, enter your name, affiliation, date and
  changes you are making here.

  Name, Affiliation, Date:
  Description of Modification:

 *****************************************************************************/

#ifndef __i2c_h
#define __i2c_h

#include <systemc.h>

#include "simple_bus_types.h"
#include "simple_bus_slave_if.h"

#include "i2c_types.h"

class i2c
: public simple_bus_slave_if
	, public sc_module
{
	public:
		SC_HAS_PROCESS(i2c);
		// constructor
		i2c(sc_module_name name_ , unsigned int start_address , unsigned int end_address) : sc_module(name_) , m_start_address(start_address) , m_end_address(end_address) {
			sc_assert(m_start_address <= m_end_address);
			sc_assert((m_end_address-m_start_address+1)%4 == 0);
			i2c_CR1 = 0x0;
			i2c_CR2 = 0x0;
			i2c_OAR1 = 0x0;
			i2c_OAR2 = 0x0;
			i2c_DR = 0x0;
			i2c_SR1 = 0x0;
			i2c_SR2 = 0x0;
			i2c_CCR = 0x0;
			i2c_TRISE = 0x2;
			m_addressingMode = ADDR7;
			m_slaveTransmitOrReceiver = TRANSMIT;
			m_secondAddressAck = false;
			m_sdaAckAssert = false;
			m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, 0x0, false};
			m_AckRelatedOrNot = NOT_ACK_RELATED;
			m_slaveHeaderOrResponsePhase = HEADER;
			m_masterOrSlaveMode = SLAVE_MODE;
			m_startCondition = false;
			m_masterHeaderOrResponsePhase = HEADER; 
			m_masterTransmitOrReceiver = TRANSMIT; 
			m_ackCount = 0;
			m_SR1ReadDone = false;
			m_dataRegisterYetToRead = false;
			m_ongoingTransmit = false;
			m_DRwritten = false;
			m_peWasEnabled = false;
			m_toggleEvent = false;
			m_errorInterruptMethod = false;
			m_add10Read = false;

			//m_first10BitHeaderAcked =false;
			//m_waitingForSecond10BitByte =false;
			//m_add10ReadDone = false;
			

			//SC_THREAD( interruptThread );
			
			SC_METHOD(interruptErrorMethod);
			dont_initialize();
			sensitive << m_interruptErrorEvent << m_errorMethodEvent;

			SC_METHOD(interruptThread);
			dont_initialize();
			sensitive << m_interruptRequestEvent << m_toggleRequestEvent;

			SC_METHOD( sdaInputChangeCB );
			dont_initialize();
			sensitive << sda_i;

			SC_METHOD( slaveAddressAckEventCB);
			dont_initialize();
			sensitive << m_slaveAddressAckEvent << m_slaveTenBitAddressAckEvent;

			SC_METHOD( masterResponsePhase );
			dont_initialize();
			sensitive << m_masterResponsePhaseStartEvent;

			SC_METHOD( slaveResponsePhase );
			dont_initialize();
			sensitive << m_slaveResponsePhaseStartEvent;

			SC_METHOD( transmitDataEventCB );
			dont_initialize();
			sensitive << m_transmitDataEvent;

			SC_METHOD( sdaOutPortDriveCB );
			dont_initialize();
			sensitive << m_sdaAckDeassertEvent << m_sdaOutPortDriveEvent;

			SC_METHOD( updateDataRegEventCB );
			dont_initialize();
			sensitive << m_updateDataRegEvent;
		}

		// destructor
		~i2c();

		// direct Slave Interface
		bool direct_read(int *data, unsigned int address);
		bool direct_write(int *data, unsigned int address);

		// Slave Interface
		simple_bus_status read(int *data, unsigned int address);
		simple_bus_status write(int *data, unsigned int address);

		unsigned int start_address() const;
		unsigned int end_address() const;

		sc_in< sc_time > clockPeriod_i;
		sc_out< bool > it_event_o;
		sc_out< bool > it_error_o;

		sc_in< i2cDataTlm > sda_i;
		sc_in< i2cSclTlm > scl_i;
		sc_out< i2cDataTlm > sda_o;
		sc_out< i2cSclTlm > scl_o;

	private:
		int i2c_CR1;
		int i2c_CR2;
		int i2c_OAR1;
		int i2c_OAR2;
		int i2c_DR;
		int i2c_SR1;
		int i2c_SR2;
		int i2c_CCR;
		int i2c_TRISE;
		unsigned int m_start_address;
		unsigned int m_end_address;
		enum addr m_addressingMode; 
		enum transmitOrReceiver m_slaveTransmitOrReceiver; 
		bool m_secondAddressAck; 
		bool m_sdaAckAssert;
		i2cDataTlm m_sendingTlm;
		enum ackRelated m_AckRelatedOrNot; 
		enum headerOrResponse m_slaveHeaderOrResponsePhase; 
		enum masterOrSlave m_masterOrSlaveMode;
		bool m_startCondition; 
		enum headerOrResponse m_masterHeaderOrResponsePhase; 
		enum transmitOrReceiver m_masterTransmitOrReceiver; 
		unsigned int m_ackCount;
		bool m_SR1ReadDone; 
		bool m_dataRegisterYetToRead;
		bool m_ongoingTransmit;
		bool m_DRwritten;
		bool m_dataAvailableForTransmit;
		bool m_peWasEnabled;
		bool m_toggleEvent;
		bool m_errorInterruptMethod;
		bool m_add10Read;
		//bool m_waitingForSecond10BitByte;
		//bool m_add10ReadDone;
		//bool m_first10BitHeaderAcked;

		sc_event m_slaveAddressAckEvent;
		sc_event m_slaveTenBitAddressAckEvent;
		sc_event m_slaveResponsePhaseStartEvent;
		sc_event m_sdaAckDeassertEvent;
		sc_event m_transmitDataEvent;
		sc_event m_updateDataRegEvent; 
		sc_event m_sdaOutPortDriveEvent;
		sc_event m_masterResponsePhaseStartEvent;

		sc_event  m_interruptRequestEvent;
		sc_event  m_interruptErrorEvent;
		sc_event m_toggleRequestEvent;
		sc_event m_errorMethodEvent;
		

		unsigned int getOwnAddress();
		void sdaInputChangeCB();

		void slaveAddressAckEventCB();
		void slaveResponsePhase();
		void sdaOutPortDriveCB(); 
		void transmitDataEventCB();
		void updateDataRegEventCB();
		void masterResponsePhase();
		void checkEventInterrupt();// for interrupt enable checking
		void checkErrorInterrupt();// for error interrupt

		void interruptThread();
		void interruptErrorMethod();
}; // end class i2c

inline bool i2c::direct_read(int *data, unsigned int address)
{
	return (read(data, address) == SIMPLE_BUS_OK);
}

inline bool i2c::direct_write(int *data, unsigned int address)
{
	return (write(data, address) == SIMPLE_BUS_OK);
}

inline  i2c::~i2c()
{
	//  if (MEM) delete [] MEM;
	//  MEM = (int *)0;
}

inline unsigned int i2c::start_address() const
{
	return m_start_address;
}

inline unsigned int i2c::end_address() const
{
	return m_end_address;
}

#endif
