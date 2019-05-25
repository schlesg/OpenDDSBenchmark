/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>

#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>
#include "DataReaderListener.h"
#include <dds/DCPS/SubscriberImpl.h>

#include "dds/DCPS/StaticIncludes.h"
#include <iostream>
#ifdef ACE_AS_STATIC_LIBS
#ifndef OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#endif
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "MessengerTypeSupportImpl.h"

//#include "Writer.h"
#include "Args.h"

#ifdef OPENDDS_SECURITY
const char auth_ca_file[] = "file:../../security/certs/identity/identity_ca_cert.pem";
const char perm_ca_file[] = "file:../../security/certs/permissions/permissions_ca_cert.pem";
const char id_cert_file[] = "file:../../security/certs/identity/test_participant_01_cert.pem";
const char id_key_file[] = "file:../../security/certs/identity/test_participant_01_private_key.pem";
const char governance_file[] = "file:./governance_signed.p7s";
const char permissions_file[] = "file:./permissions_1_signed.p7s";

const char DDSSEC_PROP_IDENTITY_CA[] = "dds.sec.auth.identity_ca";
const char DDSSEC_PROP_IDENTITY_CERT[] = "dds.sec.auth.identity_certificate";
const char DDSSEC_PROP_IDENTITY_PRIVKEY[] = "dds.sec.auth.private_key";
const char DDSSEC_PROP_PERM_CA[] = "dds.sec.access.permissions_ca";
const char DDSSEC_PROP_PERM_GOV_DOC[] = "dds.sec.access.governance";
const char DDSSEC_PROP_PERM_DOC[] = "dds.sec.access.permissions";
#endif

bool reliable = false;
bool wait_for_acks = false;

bool dw_reliable()
{
  OpenDDS::DCPS::TransportConfig_rch gc = TheTransportRegistry->global_config();
  return !(gc->instances_[0]->transport_type_ == "udp");
}

void append(DDS::PropertySeq &props, const char *name, const char *value)
{
  const DDS::Property_t prop = {name, value, false /*propagate*/};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var participant;

  try
  {

    std::cout << "Starting publisher" << std::endl;
    {
      // Initialize DomainParticipantFactory
      dpf = TheParticipantFactoryWithArgs(argc, argv);

      std::cout << "Starting publisher with " << argc << " args" << std::endl;
      int error;
      if ((error = parse_args(argc, argv)) != 0) //TBD
      {
        return error;
      }

      DDS::DomainParticipantQos part_qos;
      dpf->get_default_participant_qos(part_qos);

#if defined(OPENDDS_SECURITY)
      if (TheServiceParticipant->get_security())
      {
        DDS::PropertySeq &props = part_qos.property.value;
        append(props, DDSSEC_PROP_IDENTITY_CA, auth_ca_file);
        append(props, DDSSEC_PROP_IDENTITY_CERT, id_cert_file);
        append(props, DDSSEC_PROP_IDENTITY_PRIVKEY, id_key_file);
        append(props, DDSSEC_PROP_PERM_CA, perm_ca_file);
        append(props, DDSSEC_PROP_PERM_GOV_DOC, governance_file);
        append(props, DDSSEC_PROP_PERM_DOC, permissions_file);
      }
#endif
      // Create DomainParticipant
      participant = dpf->create_participant(4,
                                            part_qos,
                                            DDS::DomainParticipantListener::_nil(),
                                            OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(participant.in()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                              ACE_TEXT(" ERROR: create_participant failed!\n")),
                         -1);
      }

      // Register TypeSupport (Messenger::Message)
      Messenger::MessageTypeSupport_var mts =
          new Messenger::MessageTypeSupportImpl();

      if (mts->register_type(participant.in(), "") != DDS::RETCODE_OK)
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                              ACE_TEXT(" ERROR: register_type failed!\n")),
                         -1);
      }

      // Create Topic
      CORBA::String_var type_name = mts->get_type_name();
      DDS::Topic_var topic =
          participant->create_topic("Ping",
                                    type_name.in(),
                                    TOPIC_QOS_DEFAULT,
                                    DDS::TopicListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(topic.in()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                              ACE_TEXT(" ERROR: create_topic failed!\n")),
                         -1);
      }

      // Create Publisher
      DDS::Publisher_var pub =
          participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                        DDS::PublisherListener::_nil(),
                                        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(pub.in()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                              ACE_TEXT(" ERROR: create_publisher failed!\n")),
                         -1);
      }

      DDS::DataWriterQos qos;
      pub->get_default_datawriter_qos(qos);
      if (dw_reliable())
      {
        std::cout << "Reliable DataWriter" << std::endl;
        qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
        qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      }

      // Create DataWriter
      DDS::DataWriter_var dw =
          pub->create_datawriter(topic.in(),
                                 qos,
                                 DDS::DataWriterListener::_nil(),
                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(dw.in()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                              ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                         -1);
      }

      Messenger::MessageDataWriter_var message_dw = Messenger::MessageDataWriter::_narrow(dw.in());

      if (CORBA::is_nil(message_dw.in()))
      {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                       ACE_TEXT(" ERROR: _narrow failed!\n")));
        ACE_OS::exit(-1);
      }

      //Creating subscriber side (pong)

      // Create Topic
      DDS::Topic_var pongTopic =
          participant->create_topic("Pong",
                                    type_name.in(),
                                    TOPIC_QOS_DEFAULT,
                                    DDS::TopicListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(pongTopic.in()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                              ACE_TEXT(" ERROR: create_topic() failed!\n")),
                         -1);
      }

      // Create Subscriber
      DDS::Subscriber_var sub =
          participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                         DDS::SubscriberListener::_nil(),
                                         OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(sub.in()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                              ACE_TEXT(" ERROR: create_subscriber() failed!\n")),
                         -1);
      }

      // Create DataReader
      DataReaderListenerImpl *const listener_servant = new DataReaderListenerImpl;
      DDS::DataReaderListener_var listener(listener_servant);

      DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos(dr_qos);
      if (DataReaderListenerImpl::is_reliable())
      {
        std::cout << "Reliable DataReader" << std::endl;
        dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      }

      DDS::DataReader_var reader =
          sub->create_datareader(pongTopic.in(),
                                 dr_qos,
                                 listener.in(),
                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(reader.in()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                              ACE_TEXT(" ERROR: create_datareader() failed!\n")),
                         -1);
      }

      //Main flow
      Messenger::Message message;
      message.subject_id = 99;

      DDS::InstanceHandle_t handle = message_dw->register_instance(message);

      message.from = "Comic Book Guy";
      message.subject = "Review";
      message.text = "Worst. Movie. Ever.";
      message.count = 0;

      const int num_messages = 40; //TBD by Config

      for (int i = 0; i < num_messages; i++)
      {
        DDS::ReturnCode_t error;
        do
        {
          std::cout << "Writing..." << std::endl;
          error = message_dw->write(message, handle);
          sleep(1);
        } while (error == DDS::RETCODE_TIMEOUT);

        if (error != DDS::RETCODE_OK)
        {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("%N:%l: svc()")
                         ACE_TEXT(" ERROR: write returned %d!\n"),
                     error));
        }

        message.count++;
      }

      std::cout << "Writer finished " << std::endl;
      //writer->end();
      // let any missed multicast/rtps messages get re-delivered
      std::cout << "Writer wait small time" << std::endl;
      ACE_Time_Value small_time(0, 250000);
      ACE_OS::sleep(small_time);

      std::cerr << "deleting DW" << std::endl;
      //delete writer;
    }
    // Clean-up!
    std::cerr << "deleting contained entities" << std::endl;
    participant->delete_contained_entities();
    std::cerr << "deleting participant" << std::endl;
    dpf->delete_participant(participant.in());
    std::cerr << "shutdown" << std::endl;
    TheServiceParticipant->shutdown();
  }
  catch (const CORBA::Exception &e)
  {
    e._tao_print_exception("Exception caught in main():");
    ACE_OS::exit(-1);
  }

  return 0;
}
