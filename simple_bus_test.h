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
 
  simple_bus_test.h : The test bench.
 
  Original Author: Ric Hilderink, Synopsys, Inc., 2001-10-11
 
 *****************************************************************************/
 
/*****************************************************************************
 
  MODIFICATION LOG - modifiers, enter your name, affiliation, date and
  changes you are making here.
 
      Name, Affiliation, Date:
  Description of Modification:
 
 *****************************************************************************/

#ifndef __simple_bus_test_h
#define __simple_bus_test_h

#include <systemc.h>

#include "simple_bus_master_non_blocking.h"
#include "simple_bus.h"
#include "i2c.h"
#include "simple_bus_arbiter.h"

SC_MODULE(simple_bus_test)
{
  // channels
  sc_clock C1;

  // module instances
  simple_bus_master_non_blocking *master_nb;
  simple_bus                     *bus;
  i2c            *i2c0;
  i2c            *i2c1;
  simple_bus_arbiter             *arbiter;

  sc_signal< sc_time > i2c_busClkPeriod;
  sc_signal< bool > i2c0_interrupt_event;
  sc_signal< bool > i2c0_interrupt_error;
  sc_signal< i2cDataTlm > i2c0sdaOutToi2c1sdaIn;
  sc_signal< i2cDataTlm > i2c1sdaOutToi2c0sdaIn;
  sc_signal< i2cSclTlm > i2c0_sclIn;
  sc_signal< i2cSclTlm > i2c0_sclOut;
  sc_signal< i2cSclTlm > i2c1_sclIn;
  sc_signal< i2cSclTlm > i2c1_sclOut;

  sc_signal< bool > i2c1_interrupt_event;
  sc_signal< bool > i2c1_interrupt_error;

  // constructor
  SC_CTOR(simple_bus_test)
    : C1("C1")
  {
    // create instances
    master_nb = new simple_bus_master_non_blocking("master_nb", 3, 0x40005400, false, 20);
    i2c0 = new i2c("i2c0", 0x40005400, 0x400057ff);
    i2c1 = new i2c("i2c1", 0x40005800, 0x40005Bff);
    bus = new simple_bus("bus", true);
    arbiter = new simple_bus_arbiter("arbiter");

    i2c_busClkPeriod.write(sc_time(10,SC_NS));

    // connect instances
    bus->clock(C1);
    master_nb->clock(C1);
    master_nb->bus_port(*bus);
    bus->arbiter_port(*arbiter);
    bus->slave_port(*i2c0);
    bus->slave_port(*i2c1);

	i2c0->clockPeriod_i( i2c_busClkPeriod );
	i2c0->it_event_o( i2c0_interrupt_event );
	i2c0->it_error_o( i2c0_interrupt_error );
	i2c0->sda_i( i2c1sdaOutToi2c0sdaIn );
	i2c0->scl_i( i2c0_sclIn);
	i2c0->sda_o( i2c0sdaOutToi2c1sdaIn );
	i2c0->scl_o( i2c0_sclOut );

	i2c1->clockPeriod_i( i2c_busClkPeriod );
	i2c1->it_event_o( i2c1_interrupt_event );
	i2c1->it_error_o( i2c1_interrupt_error );
	i2c1->sda_i( i2c0sdaOutToi2c1sdaIn );
	i2c1->scl_i( i2c1_sclIn );
	i2c1->sda_o( i2c1sdaOutToi2c0sdaIn );
	i2c1->scl_o( i2c1_sclOut );

/*	SC_METHOD( sdaOutChanged );
	dont_initialize();
	sensitive << i2c0sdaOutToi2c1sdaIn;
*/
	SC_THREAD( driveSdaInput );
	//this is also added
	//SC_THREAD( verifySR1_SB );
	
  }

  void sdaOutChanged()
  {
  	i2cDataTlm dataReceived = i2c0sdaOutToi2c1sdaIn.read();
  	if( dataReceived.start == true)
	{
		cout << sc_time_stamp() << " Start condition received" << endl;
	}
  	if( dataReceived.ackOrNack == ACK )
	{
		cout << "Ack received" << endl;
	}
	/*else
	{
		cout << "No ack" << endl;
	}*/
//	cout << "Data received into Testbench is 0x" << hex << dataReceived.data << endl;
  }


	void driveSdaInput()
	{/*
		wait(15, SC_NS);

		struct i2cDataTlm sdaValue;
		sdaValue.start = true;
		sdaValue.address = 0x7C;
		sdaValue.addr7OrAddr10 = ADDR10;
		sdaValue.readOrWrite = WRITE;
		sdaValue.ackOrNack = NACK; // redundant only following address later might have role to play based on direction of ack
		sdaValue.data = 0x0;
		sdaValue.stop = false;

		i2c0sdaOutToi2c1sdaIn.write( sdaValue );

		wait( 190, SC_NS );
		sdaValue.start = false;
		sdaValue.address = 0x0;
		sdaValue.addr7OrAddr10 = ADDR7;
		sdaValue.readOrWrite = READ;
		sdaValue.ackOrNack = NACK;
		sdaValue.stop = false;

		sdaValue.data = 0xF3;

		i2c0sdaOutToi2c1sdaIn.write( sdaValue ); // 205 ns

		wait( 90, SC_NS );
		sdaValue.start = false;
		sdaValue.address = 0x0;
		sdaValue.addr7OrAddr10 = ADDR7;
		sdaValue.readOrWrite = READ;
		sdaValue.ackOrNack = NACK;
		sdaValue.data = 0x0;

		sdaValue.stop = true;

		i2c0sdaOutToi2c1sdaIn.write( sdaValue ); // 295 ns
		*/
/*
		wait( 270, SC_NS );
		sdaValue.start = false;
		sdaValue.address = 0x0;
		sdaValue.addr7OrAddr10 = ADDR7;
		sdaValue.readOrWrite = READ;
		sdaValue.data = 0x0;
		sdaValue.stop = false;

		sdaValue.ackOrNack = ACK;

		i2c0sdaOutToi2c1sdaIn.write( sdaValue ); // 285 ns

		wait( 90, SC_NS );
		sdaValue.start = false;
		sdaValue.address = 0x0;
		sdaValue.addr7OrAddr10 = ADDR7;
		sdaValue.readOrWrite = READ;
		sdaValue.data = 0x0;
		sdaValue.stop = false;

		sdaValue.ackOrNack = NACK;

		i2c0sdaOutToi2c1sdaIn.write( sdaValue ); //375 ns

		wait( 10, SC_NS );
		sdaValue.start = false;
		sdaValue.address = 0x0;
		sdaValue.addr7OrAddr10 = ADDR7;
		sdaValue.readOrWrite = READ;
		sdaValue.ackOrNack = NACK;
		sdaValue.data = 0x0;

		sdaValue.stop = true;

		i2c0sdaOutToi2c1sdaIn.write( sdaValue ); // 385 ns
*/
	}

// SR1 SB bit verification
	void verifySR1_SB()
	{
		int data;

		wait(20, SC_NS);

		cout << "\nSB BIT VERIFICATION\n"
			 << endl;
		
		// TESTCASE 1 : Generate START and verify SB = 1
		

		data = 0x101; // START bit+PE
		i2c0->write(&data, 0x40005400 + 0x0);

		i2c0->read(&data, 0x40005400 + 0x14);

		if (data & 0x1)
		{
			cout << "TEST1 PASS : SB bit SET" << endl;
		}
		else
		{
			cout << "TEST1 FAIL : SB bit NOT set" << endl;
		}

		wait(20, SC_NS);
		cout<< sc_time_stamp()<<endl;

		
		// TESTCASE 2 : Read SR1 + Write DR -> SB clears
		

		// Read SR1 first
		i2c0->read(&data, 0x40005400 + 0x14);

		// Write DR
		data = 0x50;
		i2c0->write(&data, 0x40005400 + 0x10);

		// Read SR1 again
		i2c0->read(&data, 0x40005400 + 0x14);

		if ((data & 0x1) == 0)
		{
			cout << "TEST2 PASS : SB bit CLEARED" << endl;
		}
		else
		{
			cout << "TEST2 FAIL : SB bit NOT cleared" << endl;
		}

		wait(20, SC_NS);

		
		// TESTCASE 3 : DR write alone should NOT clear SB
		

		// Generate START again
		data = 0x101;
		i2c0->write(&data, 0x40005400 + 0x0);

		// Direct DR write
		data = 0x60;
		i2c0->write(&data, 0x40005400 + 0x10);

		// Read SR1
		i2c0->read(&data, 0x40005400 + 0x14);

		if (data & 0x1)
		{
			cout << "TEST3 PASS : SB still SET" << endl;
		}
		else
		{
			cout << "TEST3 FAIL : SB cleared incorrectly" << endl;
		}

		wait(20, SC_NS);

		
		// TESTCASE 4 : PE=0 clears SB
		

		data = 0x0;
		i2c0->write(&data, 0x40005400 + 0x0);

		i2c0->read(&data, 0x40005400 + 0x14);

		if ((data & 0x1) == 0)
		{
			cout << "TEST4 PASS : SB cleared by PE=0" << endl;
		}
		else
		{
			cout << "TEST4 FAIL : SB not cleared by PE=0" << endl;
		}

		cout << "\nEND OF SB VERIFICATION \n"
			 << endl;
	}
  // destructor
  ~simple_bus_test()
  {
    if (master_nb) {delete master_nb; master_nb = 0;}
    if (bus) {delete bus; bus = 0;}
    if (i2c0) {delete i2c0; i2c0 = 0;}
    if (arbiter) {delete arbiter; arbiter = 0;}
  }

}; // end class simple_bus_test

#endif
