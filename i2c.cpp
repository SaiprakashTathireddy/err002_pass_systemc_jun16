#include "i2c.h"
#include "i2c_reg.h"

static int thirdAckCount=0;
static int headerOccurence=0;
static int startBitCount=0;
static bool checkingOccurenceOf10Bit = false;
static bool restartEnable=false;
static int DrOccurence=0;
static int add10address =0;
void i2c::slaveAddressAckEventCB()
{
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << endl;

	i2c_SR1 |= 0x2; // setting ADDR field after successful address match of the slave
	checkEventInterrupt();// calling the function after setting the ADDR field
	m_AckRelatedOrNot = ACK_RELATED;
	m_sdaAckDeassertEvent.notify();


	// delay for 1 clock period to account for ack bit
	if( m_addressingMode == ADDR10 )
	{
		if(thirdAckCount >= 1)
		//if((restartEnable && thirdAckCount >= 2) || (!restartEnable && thirdAckCount >= 1))
		{
			thirdAckCount++;
			m_slaveResponsePhaseStartEvent.notify( clockPeriod_i.read() );
		}
		else
		{
			thirdAckCount++;
		}
	}
	else
	{
		m_slaveResponsePhaseStartEvent.notify( clockPeriod_i.read() );
	}
}

void i2c::updateDataRegEventCB()
{
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << endl;
	i2cDataTlm receivedDataTlm = sda_i.read();

	if(m_dataRegisterYetToRead == true)
	{
		i2c_SR1 |= SR1_OVR; // setting OVR bit
		checkErrorInterrupt();
		i2c_SR1 |= 0x4; // if the DR register is not yet to read when the new data is ready for DR register, BTF is set
		checkEventInterrupt();//calling the function after setting the BTF field
		/*// Force NACK on overrun — bypass CR1_ACK check
		m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false};
		m_sendingTlm.ackOrNack = NACK;
		m_AckRelatedOrNot = NOT_ACK_RELATED;
		m_sdaOutPortDriveEvent.notify();
		return; // don't overwrite DR or set RxNE for the overrun byte*/
	}

	i2c_DR = receivedDataTlm.data;
	i2c_SR1 |= 0x40;//setting of RXNE
	checkEventInterrupt();//calling the function after setting the RXNE field
	cout << "i2c_DR " << hex << i2c_DR << endl;

	m_dataRegisterYetToRead = true;
	

	m_AckRelatedOrNot = ACK_RELATED;
	m_sdaOutPortDriveEvent.notify();
}

void i2c::sdaOutPortDriveCB()
{
	if ((i2c_CR1 & 0x1) == 0)
    {
        return;
    }
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << endl;

	if(m_AckRelatedOrNot == ACK_RELATED)
	{
		m_ackCount++;
		
		
		m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false};
		if ((m_slaveHeaderOrResponsePhase == HEADER) || (i2c_CR1 & 0x400))
        {
            m_sendingTlm.ackOrNack = ACK;
        }
        else
        {
            // CR1_ACK=0 in RESPONSE phase → NACK this data byte
            m_sendingTlm.ackOrNack = NACK;
        }
		m_sendingTlm.ackCount = m_ackCount;

		sda_o.write( m_sendingTlm );

	}
	else
	{
		sda_o.write( m_sendingTlm );
	}
}

void i2c::transmitDataEventCB()
{
	
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << endl;
	
	m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, false, false}; 
	m_sendingTlm.data = i2c_DR;
	i2c_DR =0x0;
	m_AckRelatedOrNot = NOT_ACK_RELATED;
	m_sdaOutPortDriveEvent.notify(); // since sda_o can only be driven from the function already handling the ack (both assertion & deassertion), we use the NOT_ACK_RELATED value to distinguish and drive the data immediately (hence no time delay)

	m_ongoingTransmit = true;
	//i2c_SR1.B.TXE=1; // DR became empty after moving data for transmission so setting TXE
	i2c_SR1 |= 0x80;
	checkEventInterrupt();//calling the function after setting the TXE field
}


void i2c::slaveResponsePhase()
{
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << endl;

	m_SR1ReadDone = false;

	m_slaveHeaderOrResponsePhase = RESPONSE;

	/*if(m_slaveTransmitOrReceiver == TRANSMIT)
	{
		cout << "starting Slave transmit" << endl;
		//sda_o.write();
	}
	else
	{
		cout << "starting Slave receive" << endl;
		//sda_i.read();
	}*/

	if(m_slaveTransmitOrReceiver == TRANSMIT)
	{
		cout << "starting Slave transmit" << endl;
		if(m_DRwritten == false)
		{
			// DR not yet written — raise TxE so firmware writes DR
			i2c_SR1 |= 0x80; // TxE
			checkEventInterrupt();
		}
		else
		{
			m_DRwritten = false;
			transmitDataEventCB();
		}
	}
	else
	{
		cout << "starting Slave receive" << endl;
	}
}

void i2c::masterResponsePhase()
{
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << endl;

	m_SR1ReadDone = false;

	m_masterHeaderOrResponsePhase = RESPONSE;

	if(m_masterTransmitOrReceiver == TRANSMIT)
	{
		cout << "starting master transmit" << endl;
		if(m_DRwritten == false)
		{
			/*i2c_SR1 |= 0x4; // BTF field is set because DR register is not yet written when we are ready to send
			checkEventInterrupt();//calling the function after setting the BTF field*/

			// DR not yet written — assert TxE so firmware knows to fill DR.
			// BTF must not fire here; it fires only after the byte is transmitted
			// and DR is still empty (second-level stall).
			i2c_SR1 |= 0x80; // TxE: DR empty, ready for next byte
			checkEventInterrupt();
		}
		else
		{
			m_DRwritten = false;
			transmitDataEventCB();
		}
	}
	else
	{
		cout << "starting master receive" << endl;
		//sda_i.read();
	}
}

void i2c::sdaInputChangeCB()
{
	cout << this->name() << " " << sc_time_stamp() << " " << __PRETTY_FUNCTION__ << endl;
	static unsigned int previousAddr10Bit=0;
	i2cDataTlm receivedDataTlm = sda_i.read();
	
	cout << receivedDataTlm << endl;
	if ((m_masterOrSlaveMode == MASTER_MODE) &&
		(receivedDataTlm.start == true) &&
		(receivedDataTlm.ackCount == 0) && // not our own TLM echoed back
		(m_startCondition == false))	   // we are past our own START
	{
		i2c_SR1 |= SR1_ARLO;
		i2c_SR2 &= ~SR2_MSL; // hardware switches back to slave mode
		m_masterOrSlaveMode = SLAVE_MODE;
		m_masterHeaderOrResponsePhase = HEADER;
		checkErrorInterrupt();
		return;
	}

	if(receivedDataTlm.data && m_slaveHeaderOrResponsePhase== HEADER && m_addressingMode == ADDR7)
	{
		//slaveResponsePhase();
		slaveAddressAckEventCB();
	}
	if(receivedDataTlm.repeatedStart)
        {
                  m_slaveHeaderOrResponsePhase = HEADER; 
                  m_masterHeaderOrResponsePhase = HEADER;
        }
        else if( receivedDataTlm.repeatedStart == false && receivedDataTlm.start == false)
        {
                  //m_masterHeaderOrResponsePhase = RESPONSE;
                  m_slaveHeaderOrResponsePhase = RESPONSE; 
        }
	if(m_masterOrSlaveMode == SLAVE_MODE)
	{
		if ((m_slaveHeaderOrResponsePhase == RESPONSE) &&
			(receivedDataTlm.start == true) &&
			(receivedDataTlm.repeatedStart == false)) // plain START, not repeated-START
		{
			i2c_SR1 |= SR1_BERR;
			checkErrorInterrupt();
			// Do NOT return — the slave still needs to reset its state below
		}

		if( m_slaveHeaderOrResponsePhase == HEADER )
		{ 
			if( receivedDataTlm.start == true )
			{
			
				if(		((receivedDataTlm.addr7OrAddr10 == ADDR7) && (m_addressingMode == ADDR7))  )
				{
					if(receivedDataTlm.address == 0x0 && (i2c_CR1 & 0x40))
					{ 
						cout<<"General-call address received ,ENGC enabled"<<endl;
						m_slaveTransmitOrReceiver=RECEIVE;
						i2c_SR2 |= 0x10;//setting GENCALL
						m_slaveTenBitAddressAckEvent.notify( (1+7+1) * clockPeriod_i.read() );
					}
					else if( receivedDataTlm.address == getOwnAddress())
					{
						cout << "Slave address from OAR1 register 0x" << hex << getOwnAddress() << " matches to the received address on SDA line 0x" << hex << receivedDataTlm.address << endl;
						m_slaveTransmitOrReceiver = (receivedDataTlm.readOrWrite == READ)? TRANSMIT: RECEIVE;
						m_slaveAddressAckEvent.notify( (1+7+1) * clockPeriod_i.read() );
						if(m_addressingMode == ADDR10)
						{
							m_slaveTenBitAddressAckEvent.notify( (1+7+1+1+8) * clockPeriod_i.read() );
						}
					}
					// Check OAR2 secondary address when ENDUAL (bit 0) is set
					//addr1 testcase
					else if ((i2c_OAR2 & 0x1) && (receivedDataTlm.address == ((i2c_OAR2 >> 1) & 0x7F)))
					{
						cout << "Slave address from OAR2 register 0x" << hex << ((i2c_OAR2 >> 1) & 0x7F) << " matches to the received address on SDA line 0x" << hex << receivedDataTlm.address << endl;
						i2c_SR2 |= 0x80; // DUALF=1: matched via OAR2
						m_slaveTransmitOrReceiver = (receivedDataTlm.readOrWrite == READ) ? TRANSMIT : RECEIVE;
						m_slaveAddressAckEvent.notify((1 + 7 + 1) * clockPeriod_i.read());
					}
					else
					{
						cout << "Slave address from OAR1 register 0x" << hex << getOwnAddress() << " DOES NOT match to the received address on SDA line 0x" << hex << receivedDataTlm.address << endl;
						// Send NACK via sdaOutPortDriveCB so we don't drive sda_o from two processes
						m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false};
						m_sendingTlm.ackOrNack = NACK;
						m_AckRelatedOrNot = NOT_ACK_RELATED;
						m_sdaOutPortDriveEvent.notify();
					}
				}
                else if(((receivedDataTlm.addr7OrAddr10 == ADDR10) && (m_addressingMode == ADDR10)))
                {  
					//cout << "inside 10bit****************" <<endl;           
					//cout <<  ((receivedDataTlm.address & ~(0xF9))>>1) << "------"<< ((getOwnAddress() & ~(0xf0ff))>>8) <<endl;
					if(previousAddr10Bit == 0)
					{
						if (receivedDataTlm.repeatedStart)
						{
							// High bits already verified in phase 1 — directly ACK as TRANSMIT
							m_slaveTransmitOrReceiver = TRANSMIT;
							m_slaveAddressAckEvent.notify((1 + 7 + 1) * clockPeriod_i.read());
						}
						else if(headerOccurence ==0)
						{
							if( ((receivedDataTlm.address & ~(0xF9))>>1) == ((getOwnAddress() & ~(0xf0ff))>>8))
							{
						/*      cout << ((receivedDataTlm.address & ~(0xF9))>>1) << ((getOwnAddress() & ~(0xf0ff))>>8) << endl;
																receivedDataTlm.address = ((receivedDataTlm.address & ~(0xF9) >> 1) << 8);
					*/          previousAddr10Bit = (((receivedDataTlm.address & ~(0xF9))>>1) << 8);
									
								cout << " First Part of 10bit OAR1 SLAVE ADDRESS is matched " << hex << "0x" << receivedDataTlm.address << endl;
								m_slaveTransmitOrReceiver = (receivedDataTlm.readOrWrite == READ)? TRANSMIT: RECEIVE;
								headerOccurence++;
								m_slaveAddressAckEvent.notify( (1+7+1) * clockPeriod_i.read() );

															
								}
								else
								{
									cout << "Slave Address not Matched"<<endl;
									m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false};
									m_sendingTlm.ackOrNack = NACK;
									m_AckRelatedOrNot = NOT_ACK_RELATED;
									m_sdaOutPortDriveEvent.notify();
								}
							}
							else
							{
								headerOccurence = 0;
								cout << "ASSUMING THAT THE ADDRESSES ARE SAME " << endl;
								m_slaveTransmitOrReceiver = (receivedDataTlm.readOrWrite == READ) ? TRANSMIT : RECEIVE;
								m_slaveAddressAckEvent.notify((1 + 7 + 1) * clockPeriod_i.read());
							}
						
					}

					
					
					else{

						if(receivedDataTlm.address == (getOwnAddress() & 0xff))
						{
							receivedDataTlm.address = previousAddr10Bit | receivedDataTlm.address;
							previousAddr10Bit=0;
							cout << "Slave address from OAR1 register 0x" << hex << getOwnAddress() << "******************MATCH to the received address on SDA line 0x" << hex << receivedDataTlm.address << endl;
							i2c_SR1 |=SR1_ADDR;
							cout<<"************setting ADDR**************"<<endl;
							checkEventInterrupt();
							m_slaveAddressAckEvent.notify((1 + 7 + 1) * clockPeriod_i.read());
						}
						else
						{
							cout << "Slave address from OAR1 register 0x" << hex << getOwnAddress() << "******************DOES NOT ******************MATCH to the received address on SDA line 0x" << hex << receivedDataTlm.address << endl;
							previousAddr10Bit = 0;
							m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false};
							m_sendingTlm.ackOrNack = NACK;
							m_AckRelatedOrNot = NOT_ACK_RELATED;
							m_sdaOutPortDriveEvent.notify();
						}
					}
					}
				else
				{
					//cout <<"sai*********************"<<endl;
				}
			}
			else
			{
				
			}
		}
		else
		{
			if(receivedDataTlm.stop == true)
			{
				cout << "Received stop" << endl;
				i2c_SR1 |=0x10;
				checkEventInterrupt();//calling the function after setting the STOPF field
				i2c_SR2 &= ~(0x10);//clearing GENCALL bit
				//added this part
				/*
				m_secondAddressAck = false;*/
				m_slaveHeaderOrResponsePhase = HEADER;
				startBitCount=0;
				checkingOccurenceOf10Bit = false;
				headerOccurence = 0;
				thirdAckCount = 0;
			}
			else
			{
				if(m_slaveTransmitOrReceiver == TRANSMIT)
				{
					cout << "Expecting ack in response phase : ";
					cout << receivedDataTlm.ackOrNack << endl; 
					if(receivedDataTlm.ackOrNack == ACK)
					{
//						m_transmitDataEvent.notify( clockPeriod_i.read() );
						m_ongoingTransmit = false;

						if(m_DRwritten == false)
						{
							i2c_SR1 |= 0x4; // BTF field is set because DR register is not yet written when we are ready to send
							checkEventInterrupt();//calling the function after setting the BTF field
						}
						else
						{
							m_DRwritten = false;
							transmitDataEventCB();
						}
					}
					else
					{
						if(m_sendingTlm.addr7OrAddr10 == ADDR10 && checkingOccurenceOf10Bit == true)
						{
							//i2c_SR1 |= SR1_ADD10;
							//checkEventInterrupt();
						}
						i2c_SR1 |= 0x400;//setting AF SR1 when ack failure occurs 
						checkErrorInterrupt();
					}
				}
				else
				{
					cout << "Received data is " << hex << receivedDataTlm.data << endl;
					m_updateDataRegEvent.notify( 8 * clockPeriod_i.read() );
				}
			}
		}
	}
	else
	{
		if( m_masterHeaderOrResponsePhase == HEADER )
		{
			if(receivedDataTlm.ackOrNack == ACK)
			{
				/*i2c_SR1 |= 0x2; // setting the ADDR field after successful ack
				checkEventInterrupt();//calling the function after setting ADDR field*/

				if (m_sendingTlm.addr7OrAddr10 == ADDR7)
				{
					// Normal 7-bit address or second phase of 10-bit address
					i2c_SR1 |= SR1_ADDR;
					checkEventInterrupt();
				}
				if(m_masterTransmitOrReceiver == TRANSMIT)
				{
					i2c_SR2 |= 0x4;//setting TRA
				}
				else{
					i2c_SR2 &= ~(0x4);//clearing TRA
				}
				if (m_sendingTlm.addr7OrAddr10 == ADDR7)
				{
					m_masterResponsePhaseStartEvent.notify( clockPeriod_i.read() );
				}
				else
				{
					//cout<<"..........................."<<endl;
					cout << m_sendingTlm.ackCount <<"*************************"<< endl;
					cout << receivedDataTlm << endl;

					if (receivedDataTlm.ackCount == 2)
					{
						// 10-bit addressing, 1st address phase: ADDR set after ACK of 2nd address byte
						i2c_SR1 |= SR1_ADDR;
						checkEventInterrupt();
					}

					if (receivedDataTlm.ackCount == 3)
					{
						cout << "**********************SUCCESS******************"<<endl;
						restartEnable = false;
						checkingOccurenceOf10Bit = false;
						i2c_SR1 |=SR1_ADDR;
						checkEventInterrupt();
						m_startCondition=false;
						m_masterResponsePhaseStartEvent.notify(clockPeriod_i.read());
					}
					/*if (receivedDataTlm.ackCount == 3)
					{
						cout << "**********************SUCCESS******************" << endl;
						// Repeated-start read header fully matched — fire SR1_ADDR (not ADD10)
						i2c_SR1 &= ~SR1_ADD10;
						i2c_SR1 |= SR1_ADDR;
						checkEventInterrupt();
						m_startCondition = false;
						m_masterResponsePhaseStartEvent.notify(clockPeriod_i.read());
					}*/
					else
					{
						//cout << "*******************FAILURE***********************"<<endl;
					}
				}
				
			}
			else
			{
				cout<<"ssssssssssssssssssssssssssssssssssssssssssss"<<endl;
				i2c_SR1 |= 0x400;//setting AF SR1 when ack failure occurs 
				checkErrorInterrupt();
			}
		}
	
		else
		{
			if( m_masterTransmitOrReceiver == TRANSMIT )
			{
				if(receivedDataTlm.ackOrNack == ACK)
				{
					cout << "Master in Response phase, received an ack " << endl;
					m_ongoingTransmit = false;

					if(m_DRwritten == false)
					{
						i2c_SR1 |= 0x4; // BTF field is set because DR register is not yet written when we are ready to send
						i2c_SR1 |= 0x80;//TxE
						checkEventInterrupt();//calling the function after setting the BTF field
					}
					else
					{
						m_DRwritten = false;
						transmitDataEventCB();
					}
				}
				else{
					i2c_SR1 |= 0x400; // SR1_AF
					checkErrorInterrupt();
				}
			}
			else
			{
				cout << "Received data is " << hex << receivedDataTlm.data << endl;
				m_updateDataRegEvent.notify( 8 * clockPeriod_i.read() );
			}
		}
	}

}

void i2c::checkEventInterrupt()
{
    bool eventInterrupt = false;

    if (i2c_CR2 & (1 << 9))// ITEVTEN is set (from i2c_init)
    {
        if (i2c_SR1 & (SR1_SB)) eventInterrupt = true; // SB
        if (i2c_SR1 & (SR1_ADDR)) eventInterrupt = true; // ADDR
        if (i2c_SR1 & (SR1_BTF)) eventInterrupt = true; // BTF
        if (i2c_SR1 & (SR1_ADD10)) eventInterrupt = true; // ADD10
        if (i2c_SR1 & (SR1_STOPF)) eventInterrupt = true; // STOPF

        if (i2c_CR2 & (1 << 10))// ITBUFEN → NOT SET, this block skipped
        {
            if (i2c_SR1 & (SR1_RXNE)) eventInterrupt = true; // RXNE
            if (i2c_SR1 & (SR1_TXE)) eventInterrupt = true; // TXE
        }
    }

    if (eventInterrupt)
        m_interruptRequestEvent.notify();
}

void i2c::interruptThread()
{
   /* while (true)
    {
        wait(m_interruptRequestEvent);
        it_event_o.write(false);
        wait(SC_ZERO_TIME);
        it_event_o.write(true);
    }*/
   if(m_toggleEvent)
   {
	it_event_o.write(true);
	m_toggleEvent =false;

   }
   else{
	m_toggleEvent = true;
	it_event_o.write(false);
	m_toggleRequestEvent.notify(SC_ZERO_TIME);

   }

}

void i2c::checkErrorInterrupt()
{
    bool errorInterrupt = false;

    if (i2c_CR2 & (CR2_ITERREN))
    {
        if (i2c_SR1 & (SR1_BERR)) errorInterrupt = true; // BERR
        if (i2c_SR1 & (SR1_ARLO)) errorInterrupt = true; // ARLO
        if (i2c_SR1 & (SR1_AF))   errorInterrupt = true; // AF
        if (i2c_SR1 & (SR1_OVR))  errorInterrupt = true; // OVR
        if (i2c_SR1 & (SR1_PECERR)) errorInterrupt = true; // PECERR
		if (i2c_SR1 & (SR1_TIMEOUT)) errorInterrupt = true; // TIMEOUT
        if (i2c_SR1 & (SR1_SMBALERT)) errorInterrupt = true; // SMBALERT

		
    }

    if (errorInterrupt)
        m_interruptErrorEvent.notify();
}

void i2c::interruptErrorMethod()
{
    if(m_errorInterruptMethod)
   {
	it_error_o.write(true);
	m_errorInterruptMethod =false;

   }
   else{
	m_errorInterruptMethod = true;
	it_error_o.write(false);
	m_errorMethodEvent.notify(SC_ZERO_TIME);

   }
}




inline unsigned int i2c::getOwnAddress()
{
	if(m_addressingMode == ADDR7)
	{
		return ( (i2c_OAR1 >> 1) & 0x7F );
	}
	else
	{
		return (i2c_OAR1 & 0x3FF);
	}
}


simple_bus_status i2c::read(int *data
		, unsigned int address)
{
	//  *data = MEM[(address - m_start_address)/4];
	int offset = address - m_start_address;
	switch( offset )
	{
		case 0x0: *data = i2c_CR1;
				  break;

		case 0x4: *data = i2c_CR2;
				  break;

		case 0x8: *data = i2c_OAR1;
				  break;

		case 0xc: *data = i2c_OAR2;
				  break;

		case 0x10: *data = i2c_DR;
				   m_dataRegisterYetToRead = false; // data register read is happening hence flag should be set to false
				   i2c_SR1 &= ~0x40; //clearing of RXNE

				   if( ((m_masterOrSlaveMode == MASTER_MODE) && (m_masterHeaderOrResponsePhase == RESPONSE) && (m_masterTransmitOrReceiver == RECEIVE)) ||
					((m_masterOrSlaveMode == SLAVE_MODE) && (m_slaveHeaderOrResponsePhase == RESPONSE) && (m_slaveTransmitOrReceiver == RECEIVE)) )
				   {
					   if( m_SR1ReadDone )
					   {
						   i2c_SR1 &= ~0x4; // BTF field is cleared because of DR read after SR1 read
					   }
				   }
				   break;

		case 0x14: *data = i2c_SR1;
					if( 
						(i2c_SR1 & 0x1) ||
						(i2c_SR1 & 0x2) ||
						(i2c_SR1 & 0x4) )
					{
						m_SR1ReadDone = true;
					}
				   break;

		case 0x18: *data = i2c_SR2;
					if( m_SR1ReadDone )
					{
						i2c_SR1 &= ~SR1_ADDR; // ADDR field is cleared because of SR2 read after SR1 read
						cout<<"*****************clearing ADDR***************"<<endl;

						m_SR1ReadDone = false;//sb bit related added
					}
				   break;

		case 0x1c: *data = i2c_CCR;
				   break;

		case 0x20: *data = i2c_TRISE;
				   break;

		default:
				   printf("Received address 0x%x does not match the starting address of any of the registers\n", address);
				   break;
	}
	return SIMPLE_BUS_OK;
}

simple_bus_status i2c::write(int *data
		, unsigned int address)
{
	//  MEM[(address - m_start_address)/4] = *data;
	unsigned int receivedOAR1;
    static bool addr_7or10 = ADDR7; 
	int offset = address - m_start_address;
	//static bool checkingOccurenceOf10Bit = false;
    //static int startBitCount=0;
	switch( offset )
	{
		case 0x0: i2c_CR1 = *data;
				  cout << this->name() << " " << sc_time_stamp() << " CR1 register is being written with value 0x" << hex << i2c_CR1 << endl;

				  if ((i2c_CR1 & 0x1) == 0)
				  {
					 /* //i2c_SR1 &= ~0x1;
					  i2c_SR1 = 0;
					  i2c_SR2 = 0;

					  m_masterHeaderOrResponsePhase = HEADER;
					  m_slaveHeaderOrResponsePhase = HEADER;
					  m_ackCount = 0;

					  m_startCondition = false;
					  return SIMPLE_BUS_OK;*/

					 if ((i2c_CR1 & 0x100) && m_peWasEnabled)
					 {
						 i2c_CR1 |= 0x1; // PE was on before, restore it
					 }
					 else
					 {
						 i2c_SR1 = 0;
						 i2c_SR2 = 0;
						 m_masterHeaderOrResponsePhase = HEADER;
						 m_slaveHeaderOrResponsePhase = HEADER;
						 m_ackCount = 0;
						 m_startCondition = false;

						 // Reset statics so next i2c_start() doesn't see leftover state
						 startBitCount = 0;
						 restartEnable = false;
						 DrOccurence = 0;
						 thirdAckCount = 0;
						 checkingOccurenceOf10Bit = false;
						 add10address = 0;
					//	 addr_7or10 = ADDR7;
						headerOccurence = 0;

						 m_peWasEnabled = false;
						 return SIMPLE_BUS_OK;
					 }
				  }

				  
				  if((i2c_CR1 >> 15) & 0x1)
				  {
					i2c_CR1 =0x0;
				  }



				  //checks whether the PE bit is cleared
				  // PE = 0 -> clear SB bit
				  

				  if(m_SR1ReadDone == true && m_masterOrSlaveMode == SLAVE_MODE)
				  {
					i2c_SR1 &= ~0x10;//clearing stopf of sr1
				  }
				  
				  if(i2c_CR1 & 0x100 && (i2c_CR1 & 0x1))
				  {
					  startBitCount++;
					  m_masterOrSlaveMode = MASTER_MODE;
					  m_startCondition = true;
					  m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false};
					  m_sendingTlm.start = true;
					  if(startBitCount==2)
					  {
						m_sendingTlm.repeatedStart=true;
						//cout <<"*******************REPEATED START**********************" << endl;
						restartEnable=true;
					  }
					  m_AckRelatedOrNot = NOT_ACK_RELATED;
					  //m_sdaOutPortDriveEvent.notify();

					  i2c_SR2 |= 0x3; // set MSL | BUSY when master generates START
					  i2c_SR1 = (0x1 << 0); //SB field of SR1 register is set on start sent on sda_o line
					  checkEventInterrupt(); //calling the function after setting the SB field

					  //i2c_SR1.B.TXE = 0; //Clear TXE on START condition
					  i2c_CR1 &= ~0x100; 
					  
				  }
				  else if(i2c_CR1 & 0x200)
				  {
					  if( (m_masterOrSlaveMode == MASTER_MODE) && (m_masterHeaderOrResponsePhase == RESPONSE) )
					  {
						  m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false};
						  m_sendingTlm.stop = true;
						  m_AckRelatedOrNot = NOT_ACK_RELATED;
						  m_sdaOutPortDriveEvent.notify();

						 // i2c_SR1.B.TXE = 0; //Clear TXE on STOP condition
						 i2c_CR1 &= ~0x200;
					  }
				  }
				  else
				  {
					  m_masterOrSlaveMode = SLAVE_MODE;
				  }

				  if((i2c_CR1 & 0X1) == 0)
				  {
					i2c_SR1 =0x0;
					i2c_SR2 = 0x0;
				  }
				  m_peWasEnabled = (i2c_CR1 & 0x1) ? true : false;
				  break;

		case 0x4: i2c_CR2 = *data;
				  	i2c_CR2 &= ~(0xFFFFE0C0);//only keeps CR2_RW_MASK
				  break;

		case 0x8: receivedOAR1 = (*data) & 0xFFFF;
				  receivedOAR1 &= ~(0x7C00);
				  if( receivedOAR1 & (1<<15)) 
				  {
					  // it means 10 bit addressing
					  m_addressingMode = ADDR10;
					  addr_7or10=ADDR10;
				  }
				  else
				  {
					  // it means 7 bit addressing
					  m_addressingMode = ADDR7;
					  addr_7or10 = ADDR7;
					  //receivedOAR1 = (receivedOAR1 & 0x7F) << 1;

					  
				  }
				  i2c_OAR1 =receivedOAR1;
				  //i2c_OAR1 = *data;
				  break;

		case 0xc: i2c_OAR2 = *data;
				  i2c_OAR2 &= ~(0XFFFFFF00);
				  break;

		case 0x10:

				if ((i2c_CR1 & 0x1) == 0)
				{
					break;
				}
				i2c_DR = *data;
				   //i2c_SR1.B.TXE=0;  //Clearing TXE when  writes to DR
				   i2c_SR1 &= ~0x80; //Clearing TXE when  writes to DR
				   i2c_SR1 &= ~0x40; //clearing of RXNE

				   

				 //  DrOccurence++;
				 if (m_masterOrSlaveMode == MASTER_MODE) DrOccurence++;
				   cout << this->name() << " " << sc_time_stamp() << " DR register is being written with value 0x" << hex << i2c_DR << endl;
				  
				    if ((restartEnable == false) && (DrOccurence == 3) && (addr_7or10 == ADDR10))
					{
						m_AckRelatedOrNot=NOT_ACK_RELATED;
						m_startCondition=false;
						m_sendingTlm.start = false;
						//i2c::masterResponsePhase();
						m_masterResponsePhaseStartEvent.notify( clockPeriod_i.read());
					}
					if( (m_masterOrSlaveMode == MASTER_MODE) && (m_startCondition == true))
					{
						m_sendingTlm = {false, 0x0, ADDR7, READ, NACK, m_ackCount, 0x0, false, false};
						if(m_startCondition){
							m_sendingTlm.start = true;
							m_sendingTlm.repeatedStart = restartEnable;   //  added
							m_sdaOutPortDriveEvent.notify();
						}
						

						if(addr_7or10 == ADDR10)
                        {
						    m_sendingTlm.addr7OrAddr10 = ADDR10;
							if (checkingOccurenceOf10Bit == false)
							{
								checkingOccurenceOf10Bit=true;
								m_sendingTlm.address = (i2c_DR | 0xf0);
								//cout << m_sendingTlm.address<<"******************__________"<<endl;
						    	m_sendingTlm.readOrWrite = (i2c_DR & 0x1)? READ: WRITE;
								if((i2c_DR && 0x1)==0)
								{
									i2c_SR2 |=0x4;//setting TRA of sr2
								}
								m_masterTransmitOrReceiver=(i2c_DR & 0x1)? RECEIVE: TRANSMIT;
								if(add10address == 0)
								{
									add10address++;
									i2c_SR1 |= SR1_ADD10;
									cout<<"********************setting add10*************************"<<endl;
									checkEventInterrupt();
								}
								
							}
							else{
								checkingOccurenceOf10Bit=false;
								m_sendingTlm.address=(i2c_DR);
							}
                            
                        }
                        else
						{
                            m_sendingTlm.addr7OrAddr10 = ADDR7;
						    m_sendingTlm.address = i2c_DR >> 1;
						    m_sendingTlm.readOrWrite = (i2c_DR & 0x1)? READ: WRITE;
						    m_masterTransmitOrReceiver = (i2c_DR & 0x1)? RECEIVE: TRANSMIT;
							m_startCondition=false;
                        }

						m_AckRelatedOrNot = NOT_ACK_RELATED;
						m_sdaOutPortDriveEvent.notify();

				//		m_startCondition = false;

						if(m_SR1ReadDone)
						{
							if (i2c_SR1 & SR1_ADD10)
							{
								i2c_SR1 &= ~SR1_ADD10;//clearing ADD10 after SR1 read followed by write in DR
								cout<<"*****************clearing ADD10****************"<<endl;
							}
							i2c_SR1 &= ~0x1;
							m_SR1ReadDone = false; // reset sequence tracker
						}
					}
					else if((m_masterOrSlaveMode == MASTER_MODE) && (m_masterHeaderOrResponsePhase == HEADER) && (m_startCondition == false))
					{
						m_DRwritten = true;
					}
					else if( (m_masterOrSlaveMode == MASTER_MODE) && (m_masterHeaderOrResponsePhase == RESPONSE) && (m_masterTransmitOrReceiver == TRANSMIT))
					{
						if(m_ongoingTransmit == true)
						{
							m_DRwritten = true;
						}
						else
						{
							transmitDataEventCB();
						}

						if(m_SR1ReadDone)
						{
							i2c_SR1 &= ~0x4; // BTF field cleared on write to DR following a read of SR1

							m_SR1ReadDone = false;//sb bit related added
						}
					}
					else if( (m_masterOrSlaveMode == SLAVE_MODE) && (m_slaveHeaderOrResponsePhase == RESPONSE) && (m_slaveTransmitOrReceiver == TRANSMIT))
					{
						if(m_ongoingTransmit == true)
						{
							m_DRwritten = true;
						}
						else
						{
							transmitDataEventCB();
						}

						if(m_SR1ReadDone)
						{
							i2c_SR1 &= ~0x4; // BTF field cleared on write to DR following a read of SR1

							m_SR1ReadDone = false;//sb bit related added
						}
					}
					else if ((m_masterOrSlaveMode == SLAVE_MODE) && (m_slaveHeaderOrResponsePhase == HEADER))
					{
						m_DRwritten =true;
					}
					
					
					
				   break;

		case 0x14: i2c_SR1 &= ( (unsigned int)(*data) | ~SR1_ERR_MASK );
				   break;

		case 0x18: i2c_SR2 = *data;
				   break;

		case 0x1c: i2c_CCR = *data;
					i2c_CCR &= ~(0XFFFF3000);
				   break;

		case 0x20: i2c_TRISE = *data;
					i2c_TRISE &= ~(0XFFFFFFC0);
				   break;

		default:
				   printf(" Enough\n ");
				   break;
	}

	return SIMPLE_BUS_OK;
}
