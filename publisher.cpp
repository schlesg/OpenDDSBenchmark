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
//#include "DataReaderListener.h"
#include <dds/DCPS/SubscriberImpl.h>

#include "dds/DCPS/StaticIncludes.h"
#include <iostream>
#include "string"
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/chrono.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/thread.hpp>

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
using namespace std;

namespace po = boost::program_options;

class Config
{
public:
  static std::string pubTopic;
  static std::string subTopic;
  static std::string publisherUniqueName;
  static std::string transportConfig;
  static uint64_t subCount;
  static uint64_t updateRate;
  static uint64_t payloadSize;
  static int64_t roundtripCount;
};

std::string Config::pubTopic;
std::string Config::subTopic;
std::string Config::publisherUniqueName;
std::string Config::transportConfig;
uint64_t Config::updateRate;
uint64_t Config::subCount;
uint64_t Config::payloadSize;
int64_t Config::roundtripCount;

void append(DDS::PropertySeq &props, const char *name, const char *value)
{
  const DDS::Property_t prop = {name, value, false /*propagate*/};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

int main(int argc, char *argv[])
{
#pragma region Init
  DDS::ReturnCode_t retcode;
  po::options_description description("Options");
  description.add_options()
  ("help", "produce help message. Execution example - ./DDSInitiator --PubTopic pingTopic --SubTopic pongTopic -msgLength 1000")
  ("TransportConfig", po::value(&Config::transportConfig)->default_value("rtps_disc.ini"), "TransportConfig e.g. rtps_disc.ini, shmem.ini Note that SHMEM requires running the DCPSInfoRepo via '$DDS_ROOT/bin/DCPSInfoRepo'")
  ("PubTopic", po::value(&Config::pubTopic)->default_value("pingTopic"), "Publish to Topic (Ping)")
  ("SubTopic", po::value(&Config::subTopic)->default_value("pongTopic"), "Subscribe to Topic (Pong) ")
  ("msgLength", po::value(&Config::payloadSize)->default_value(100), "Message Length (bytes)")
  ("roundtripCount", po::value(&Config::roundtripCount)->default_value(1000), "ping-pong intervals (0 == Infinite, only for general load purposes)")
  ("subCount", po::value(&Config::subCount)->default_value(1), "number of subscribers to wait for a ping")
  ("pubName", po::value(&Config::publisherUniqueName)->default_value("Initiator"), "publisher name. Name 'Dummy' will cause the echoer to not echo messages")
  ("updateRate", po::value(&Config::updateRate)->default_value(0), "update rate (for general load purposes only)");
  po::variables_map vm;

  try
  {
    po::store(po::parse_command_line(argc, argv, description), vm);
    po::notify(vm);
  }
  catch (const po::error &e)
  {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  if (vm.count("help"))
  {
    std::cout << description << "\n";
    return EXIT_SUCCESS;
  }

  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var participant;

  try
  {

    std::cout << "Starting Initiator" << std::endl;
    {
      int argsNum = 3;
      char *param_list[] = {(char *)".", (char *)"-DCPSConfigFile", (char *)Config::transportConfig.c_str()};

      // Initialize DomainParticipantFactory
      dpf = TheParticipantFactoryWithArgs(argsNum, param_list);
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
#pragma endregion

#pragma region PubSide
      //Creating publisher side (pong)
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
          participant->create_topic(Config::pubTopic.c_str(),
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
      qos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;

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

      //Message initialization
      Messenger::Message message;
      message.subject_id = 99;

      DDS::InstanceHandle_t handle = message_dw->register_instance(message);
      DDS::ReturnCode_t status;
      message.from = Config::publisherUniqueName.c_str();
      message.subject = "";
      std::string s(Config::payloadSize, '*');
      message.text = s.c_str();
      message.count = 0;

#pragma endregion
#pragma region SubSide

      //Creating subscriber side (pong)
      // Create Topic
      DDS::Topic_var pongTopic =
          participant->create_topic(Config::subTopic.c_str(),
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
      DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos(dr_qos);
      dr_qos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;

      DDS::DataReader_var reader =
          sub->create_datareader(pongTopic.in(),
                                 dr_qos,
                                 DDS::DataReaderListener::_nil(),
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

      Messenger::Message receivedMessage;
      DDS::SampleInfo si;
#pragma endregion

      sleep(2); 
      cout << "Running scenario ..." << endl;
      boost::chrono::high_resolution_clock::time_point start = boost::chrono::high_resolution_clock::now();
      for (int i = 1; i != Config::roundtripCount; i++)
      {
        //std::cout << "Writing #" << message.count << std::endl;
        retcode = message_dw->write(message, handle);

        if (retcode != DDS::RETCODE_OK)
        {
          ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: svc()") ACE_TEXT(" ERROR: write returned %d!\n"), retcode));
          return 1;
        }

        if (Config::updateRate != 0) //only for general load puropses
        {
          usleep(1000 * (1000 / Config::updateRate));
        }

        else
        {
          for (size_t jj = 0; jj < Config::subCount; ++jj)
          {
            do
            {
              status = message_dr->take_next_sample(receivedMessage, si);
            } while (status == DDS::RETCODE_NO_DATA);
          }
        }

        message.count++;
      }

      boost::chrono::high_resolution_clock::time_point end = boost::chrono::high_resolution_clock::now();
      boost::chrono::microseconds diff = boost::chrono::duration_cast<boost::chrono::microseconds>(end - start);
      cout << "Average latency = " << diff.count() / Config::roundtripCount << " microseconds with roundtrip count of " << Config::roundtripCount << endl;

      std::cout << "Shutting down..." << std::endl;
    }
    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());
    TheServiceParticipant->shutdown();
  }
  catch (const CORBA::Exception &e)
  {
    e._tao_print_exception("Exception caught in main():");
    ACE_OS::exit(-1);
  }

  return 0;
}
