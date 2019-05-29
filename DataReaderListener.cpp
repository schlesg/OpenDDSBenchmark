/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/Service_Participant.h>

#include "Args.h"
#include "DataReaderListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"

#include <iostream>

DataReaderListenerImpl::DataReaderListenerImpl(Messenger::MessageDataWriter_var *dataWriter)
    : num_reads_(0), valid_(true), reliable_(is_reliable())
{
  m_dataWriter = dataWriter;
  //std::cout << "Transport is " << (reliable_ ? "" : "UN-") << "RELIABLE" << std::endl;
}

DataReaderListenerImpl::DataReaderListenerImpl()
    : num_reads_(0), valid_(true), reliable_(is_reliable())
{
  //std::cout << "Transport is " << (reliable_ ? "" : "UN-") << "RELIABLE" << std::endl;
}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
}

bool DataReaderListenerImpl::is_reliable()
{
  OpenDDS::DCPS::TransportConfig_rch gc = TheTransportRegistry->global_config();
  return !(gc->instances_[0]->transport_type_ == "udp");
}

void DataReaderListenerImpl::on_requested_deadline_missed(
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_deadline_missed()\n")));
}

void DataReaderListenerImpl::on_requested_incompatible_qos(
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_incompatible_qos()\n")));
}

void DataReaderListenerImpl::on_liveliness_changed(
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
  //ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_liveliness_changed()\n")));
}

void DataReaderListenerImpl::on_subscription_matched(
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus &stat)
{
  //ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_subscription_matched on topic ")));
  if (stat.current_count_change > 0)
  {
    std::cout << "on_subscription_matched on topic " << reader->get_topicdescription()->get_name() << std::endl;
  }
}

void DataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_rejected()\n")));
}

void DataReaderListenerImpl::on_sample_lost(
    DDS::DataReader_ptr,
    const DDS::SampleLostStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_lost()\n")));
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  Messenger::MessageDataReader_var message_dr =
      Messenger::MessageDataReader::_narrow(reader);

  if (CORBA::is_nil(message_dr.in()))
  {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("%N:%l: on_data_available()")
                   ACE_TEXT(" ERROR: _narrow failed!\n")));
    ACE_OS::exit(-1);
  }

  DDS::ReturnCode_t retcode = message_dr->take_next_sample(message, si);

  if (retcode == DDS::RETCODE_OK)
  {
    if (si.valid_data)
    {
      //std::cout << "message #" << message.count << std::endl;
      if (strcmp(message.from, "Dummy") == 0)
      {
        //std::cout << "Dummy message #" << message.count << std::endl;
        return; //skipping echo case dummy publisher for load purposes
      }
      //std::cout << "message #" << message.count << std::endl;
      retcode = (*m_dataWriter)->write(message, DDS::HANDLE_NIL);
      //std::cout << "Pong #" << message.text << std::endl;
      if (retcode != DDS::RETCODE_OK)
      {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                       ACE_TEXT(" ERROR: write returned %d!\n"),
                   retcode));
      }
    }
    // Non-Valid sample
    else
    {
      //ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is disposed\n")));
    }
  }
  else
  {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("%N:%l: on_data_available()")
                   ACE_TEXT(" ERROR: unexpected status: %d\n"),
               retcode));
  }
}
