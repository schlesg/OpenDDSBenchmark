/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#ifndef OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#endif
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>

#include <dds/DCPS/PublisherImpl.h>

#include "DataReaderListener.h"
#include "MessengerTypeSupportImpl.h"
#include "Args.h"

#ifdef OPENDDS_SECURITY
const char auth_ca_file[] = "file:../../security/certs/identity/identity_ca_cert.pem";
const char perm_ca_file[] = "file:../../security/certs/permissions/permissions_ca_cert.pem";
const char id_cert_file[] = "file:../../security/certs/identity/test_participant_02_cert.pem";
const char id_key_file[] = "file:../../security/certs/identity/test_participant_02_private_key.pem";
const char governance_file[] = "file:./governance_signed.p7s";
const char permissions_file[] = "file:./permissions_2_signed.p7s";

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
  int status = 0;
  try
  {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);

    int error;
    if ((error = parse_args(argc, argv)) != 0)
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
    DDS::DomainParticipant_var participant =
        dpf->create_participant(4,
                                part_qos,
                                DDS::DomainParticipantListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in()))
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                            ACE_TEXT(" ERROR: create_participant() failed!\n")),
                       -1);
    }

    // Register Type (Messenger::Message)
    Messenger::MessageTypeSupport_var ts =
        new Messenger::MessageTypeSupportImpl();

    if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                            ACE_TEXT(" ERROR: register_type() failed!\n")),
                       -1);
    }

    //Create Publishing side (Pong)
    // Create Topic
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var pongTopic =
        participant->create_topic("Pong",
                                  type_name.in(),
                                  TOPIC_QOS_DEFAULT,
                                  DDS::TopicListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(pongTopic.in()))
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
    qos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;

    // Create DataWriter
    DDS::DataWriter_var dw =
        pub->create_datawriter(pongTopic.in(),
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

    //Create Subscribing side (ping)
    // Create Topic
    DDS::Topic_var topic =
        participant->create_topic("Ping",
                                  type_name.in(),
                                  TOPIC_QOS_DEFAULT,
                                  DDS::TopicListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in()))
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
    DataReaderListenerImpl *const listener_servant = new DataReaderListenerImpl(&message_dw);
    DDS::DataReaderListener_var listener(listener_servant);

    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    dr_qos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;

    DDS::DataReader_var reader =
        sub->create_datareader(topic.in(),
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

    Messenger::MessageDataReader_var message_dr =
        Messenger::MessageDataReader::_narrow(reader);

    if (CORBA::is_nil(message_dr.in()))
    {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: on_data_available()")
                     ACE_TEXT(" ERROR: _narrow failed!\n")));
      ACE_OS::exit(-1);
    }

    while (true)
    {
      //sleep(1);
    }

    //status = listener_servant->is_valid() ? 0 : -1;

    //ws->detach_condition(condition);

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());
    TheServiceParticipant->shutdown();
  }
  catch (const CORBA::Exception &e)
  {
    e._tao_print_exception("Exception caught in main():");
    status = -1;
  }

  return status;
}
