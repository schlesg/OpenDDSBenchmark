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
  std::cout << "Transport is " << (reliable_ ? "" : "UN-") << "RELIABLE" << std::endl;
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
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_liveliness_changed()\n")));
}

void DataReaderListenerImpl::on_subscription_matched(
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_subscription_matched()\n")));
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
  //++num_reads_;

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
    // std::cout << "SampleInfo.sample_rank = " << si.sample_rank << std::endl;
    // std::cout << "SampleInfo.instance_state = " << si.instance_state << std::endl;

    if (si.valid_data)
    {
      // if (!counts_.insert(message.count).second)
      // {
      //   std::cout << "ERROR: Repeat ";
      //   valid_ = false;
      // }

      // std::cout << "Message: subject    = " << message.subject.in() << std::endl
      //           << "         subject_id = " << message.subject_id << std::endl
      //           << "         from       = " << message.from.in() << std::endl
      //           << "         count      = " << message.count << std::endl
      //           << "         text       = " << message.text.in() << std::endl;

      std::cout << "Pong #" << message.count << std::endl;
      retcode = (*m_dataWriter)->write(message, DDS::HANDLE_NIL);
      //sleep(1);

      if (retcode != DDS::RETCODE_OK)
      {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                       ACE_TEXT(" ERROR: write returned %d!\n"),
                   retcode));
      }
    }
    // Non-Valid sample
    // else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE)
    // {
    //   ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is disposed\n")));
    // }
    // else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
    // {
    //   ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is unregistered\n")));
    // }
    // else
    // {
    //   ACE_ERROR((LM_ERROR,
    //              ACE_TEXT("%N:%l: on_data_available()")
    //                  ACE_TEXT(" ERROR: unknown instance state: %d\n"),
    //              si.instance_state));
    // }
  }
  else
  {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("%N:%l: on_data_available()")
                   ACE_TEXT(" ERROR: unexpected status: %d\n"),
               retcode));
  }
}

