/*
 * (c) 2014-2023 Copyright, Real-Time Innovations, Inc.  All rights reserved.
 *
 * RTI grants Licensee a license to use, modify, compile, and create derivative
 * works of the Software.  Licensee has the right to distribute object form
 * only for use with RTI products.  The Software is provided "as is", with no
 * warranty of any type, including any warranty for fitness for any purpose.
 * RTI is under no obligation to maintain or support the Software.  RTI shall
 * not be liable for any incidental or consequential damages arising out of the
 * use or inability to use the software.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rti_me_c.h"
#include "disc_dpse/disc_dpse_dpsediscovery.h"
#include "HelloWorldApplication.h"
#include "HelloWorldPlugin.h"
#include "HelloWorldSupport.h"
#include "wh_sm/wh_sm_history.h"
#include "rh_sm/rh_sm_history.h"
#include "netio_zcopy/netio_zcopy.h"

/* PSL implementations of Notif mechanism and UDP */
#include "netio/netio_mynotif.h"
#include "netio/netio_psl_udp.h"

const char* const MY_UDP_NAME = "_udp";

/* forward declarations (for gcc -Wmissing-declarations)*/
void
on_inconsistent_topic(
        void *listener_data,
        DDS_Topic *topic,
        const struct DDS_InconsistentTopicStatus *status);
/* end - forward declarations */

void
on_inconsistent_topic(
        void *listener_data,
        DDS_Topic *topic,
        const struct DDS_InconsistentTopicStatus *status)
{
    /* listener data and topic parameters not used in this example */
    (void)listener_data;
    (void)topic;

    printf("INCONSISTENT TOPIC: %d\n", status->total_count_change);
}

void application_sleep(time_t sleep_seconds, long sleep_nanoseconds)
{
    struct timespec remain, next;
    int rval;
    next.tv_sec = sleep_seconds;
    next.tv_nsec = sleep_nanoseconds;

    do {
        rval = nanosleep(&next, &remain);
        if ((rval == -1) && (errno == EINTR))
        {
            next = remain;
        }
    } while ((rval == -1) && (errno == EINTR));
}

void
Application_help(char *appname)
{
    printf("%s [options]\n", appname);
    printf("options:\n");
    printf("-h                 - This text\n");
    printf("-domain <id>       - DomainId (default: 0)\n");
    printf("-peer <address>    - peer address (default: 127.0.0.1)\n");
    printf("-count <count>     - count (default 100)\n");
    printf("-period <sec>        - time interval in seconds\n");
    printf("\tpublisher: time between writes (default 1s)\n");
    printf("\tsubscriber: time to run before exit (default 60s)\n");
    printf("\n");
}

struct Application *
Application_create(
        const char *local_participant_name,
        const char *remote_participant_name,
        DDS_Long local_participant_id,
        DDS_DomainId_t domain_id,
        char *peer,
        long int period,
        long int count)
{
    DDS_ReturnCode_t retcode;
    DDS_Boolean success = DDS_BOOLEAN_FALSE;

    DDS_DomainParticipantFactory *dp_factory = NULL;
    struct DDS_DomainParticipantFactoryQos dpf_qos =
            DDS_DomainParticipantFactoryQos_INITIALIZER;
    struct DDS_DomainParticipantQos dp_qos = DDS_DomainParticipantQos_INITIALIZER;
    RT_Registry_T *registry = NULL;

    /* UDP and Zero Copy related */
    UDPv4_TransportProperty_T *udp_property = NULL;
    struct ZCOPY_NotifInterfaceFactoryProperty notif_prop =
            ZCOPY_NotifInterfaceFactoryProperty_INITIALIZER;
    struct ZCOPY_NotifMechanismProperty notif_mech_prop =
            ZCOPY_NotifMechanismProperty_INITIALIZER;

    /* discovery related */
    struct DPSE_DiscoveryPluginProperty discovery_plugin_properties =
            DPSE_DiscoveryPluginProperty_INITIALIZER;
    struct DDS_Duration_t my_lease = {5,0};
    struct DDS_Duration_t my_assert_period = {2,0};
    discovery_plugin_properties.participant_liveliness_lease_duration = my_lease;
    discovery_plugin_properties.participant_liveliness_assert_period = my_assert_period;

    /* application specific */
    struct Application *application = NULL;

    application = (struct Application *)malloc(sizeof(struct Application));
    if (application == NULL)
    {
        printf("ERROR: Failed to malloc for application\n");
        goto done;
    }

    application->period = period;
    application->count = count;

    dp_factory = DDS_DomainParticipantFactory_get_instance();
    registry = DDS_DomainParticipantFactory_get_registry(dp_factory);

    if (!RT_Registry_register(
                registry,
                DDSHST_READER_DEFAULT_HISTORY_NAME,
                RHSM_HistoryFactory_get_interface(),
                NULL,
                NULL))
    {
        printf("ERROR: Failed to register rh\n");
        goto done;
    }

    udp_property = UDPv4_TransportProperty_new();

    /* This function takes the following arguments:
     * Param 1 is the UDP property
     * Param 2 is the IP address of the interface in host order
     * Param 3 is the Netmask of the interface
     * Param 4 is the name of the interface
     * Param 5 are flags. The following flags are supported (use OR for multiple):
     *      UDP_INTERFACE_INTERFACE_UP_FLAG - Interface is up
     *      UDP_INTERFACE_INTERFACE_MULTICAST_FLAG - Interface supports multicast
     */
    if (!UDPv4_InterfaceTable_add_entry(
                udp_property,
                0x7f000001,
                0xff000000,
                "lo",
                UDP_INTERFACE_INTERFACE_UP_FLAG |
                UDP_INTERFACE_INTERFACE_MULTICAST_FLAG))
    {
        printf("ERROR: Failed to add interface\n");
    }

    if (!UDPv4_Interface_register(registry, MY_UDP_NAME, udp_property))
    {
        printf("ERROR: Failed to register udp\n");
        goto done;
    }

    if (!NDDS_Transport_ZeroCopy_initialize(registry, NULL, NULL))
    {
        printf("ERROR: Failed to initialize zero copy\n");
        goto done;
    }

    notif_mech_prop.intf_addr = 0;
    notif_prop.user_property = &notif_mech_prop;
    notif_prop.max_samples_per_notif = 1;
    if (!ZCOPY_NotifMechanism_register(registry, NETIO_DEFAULT_NOTIF_NAME, &notif_prop))
    {
        printf("ERROR: Failed to register notif\n");
        goto done;
    }

    DDS_DomainParticipantFactory_get_qos(dp_factory, &dpf_qos);
    dpf_qos.entity_factory.autoenable_created_entities = DDS_BOOLEAN_FALSE;
    DDS_DomainParticipantFactory_set_qos(dp_factory, &dpf_qos);

    if (peer == NULL)
    {
        peer = "127.0.0.1"; /* default is loopback */
    }

    if (!RT_Registry_register(
                registry,
                "dpse",
                DPSE_DiscoveryFactory_get_interface(),
                &discovery_plugin_properties._parent,
                NULL))
    {
        printf("ERROR: Failed to register dpse\n");
        goto done;
    }

    if (!RT_ComponentFactoryId_set_name(&dp_qos.discovery.discovery.name, "dpse"))
    {
        printf("ERROR: Failed to set discovery plugin name\n");
        goto done;
    }

    if (!DDS_StringSeq_set_maximum(&dp_qos.transports.enabled_transports, 2))
    {
        printf("ERROR: Failed to set transports.enabled_transports maximum\n");
        goto done;
    }
    if (!DDS_StringSeq_set_length(&dp_qos.transports.enabled_transports, 2))
    {
        printf("ERROR: Failed to set transports.enabled_transports length\n");
        goto done;
    }
    /* UDP and Notification are enabled*/
    *DDS_StringSeq_get_reference(&dp_qos.transports.enabled_transports, 0) =
            DDS_String_dup("notif");
    *DDS_StringSeq_get_reference(&dp_qos.transports.enabled_transports, 1) =
            DDS_String_dup(MY_UDP_NAME);

    /* Discovery takes place over UDP */
    DDS_StringSeq_set_maximum(&dp_qos.discovery.enabled_transports, 1);
    DDS_StringSeq_set_length(&dp_qos.discovery.enabled_transports, 1);
    *DDS_StringSeq_get_reference(&dp_qos.discovery.enabled_transports, 0) =
            DDS_String_dup("_udp://");

    /* User data uses Notification Transport */
    DDS_StringSeq_set_maximum(&dp_qos.user_traffic.enabled_transports, 1);
    DDS_StringSeq_set_length(&dp_qos.user_traffic.enabled_transports, 1);
    *DDS_StringSeq_get_reference(&dp_qos.user_traffic.enabled_transports, 0) =
            DDS_String_dup("notif://");

    DDS_StringSeq_set_maximum(&dp_qos.discovery.initial_peers, 1);
    DDS_StringSeq_set_length(&dp_qos.discovery.initial_peers, 1);
    *DDS_StringSeq_get_reference(&dp_qos.discovery.initial_peers, 0) =
            DDS_String_dup(peer);

    /* if there are more remote or local endpoints, you need to increase these limits */
    dp_qos.resource_limits.max_destination_ports = 32;
    dp_qos.resource_limits.max_receive_ports = 32;
    dp_qos.resource_limits.local_topic_allocation = 1;
    dp_qos.resource_limits.local_type_allocation = 1;
    dp_qos.resource_limits.local_reader_allocation = 8;
    dp_qos.resource_limits.local_writer_allocation = 8;
    dp_qos.resource_limits.remote_participant_allocation = 1;
    dp_qos.resource_limits.remote_reader_allocation = 8;
    dp_qos.resource_limits.remote_writer_allocation = 8;

    /* Must manually assign participant_id */
    dp_qos.protocol.participant_id = local_participant_id;
    strcpy(dp_qos.participant_name.name, local_participant_name);

    application->participant = DDS_DomainParticipantFactory_create_participant(
            dp_factory,
            domain_id,
            &dp_qos,
            NULL,
            DDS_STATUS_MASK_NONE);

    if (application->participant == NULL)
    {
        printf("ERROR: Failed to create participant\n");
        goto done;
    }

    strncpy(application->type_name, HelloWorldTypeSupport_get_type_name(),
            strlen(HelloWorldTypeSupport_get_type_name()));
    retcode = HelloWorldTypeSupport_register_type(
            application->participant,
            application->type_name);
    if (retcode != DDS_RETCODE_OK)
    {
        printf("ERROR: Failed to register type: %s\n", "test_type");
        goto done;
    }

    sprintf(application->topic_name, "Example HelloWorld");

    struct DDS_TopicListener listener = DDS_TopicListener_INITIALIZER;
    listener.on_inconsistent_topic = on_inconsistent_topic;

    application->topic = DDS_DomainParticipant_create_topic(
            application->participant,
            application->topic_name,
            application->type_name,
            &DDS_TOPIC_QOS_DEFAULT,
            &listener,
            DDS_INCONSISTENT_TOPIC_STATUS);

    if (application->topic == NULL)
    {
        printf("ERROR: topic == NULL\n");
        goto done;
    }

    retcode = DPSE_RemoteParticipant_assert(
            application->participant,
            remote_participant_name);
    if (retcode != DDS_RETCODE_OK)
    {
        printf("ERROR: Failed to assert remote participant\n");
        goto done;
    }

    success = DDS_BOOLEAN_TRUE;

done:

    if (!success)
    {
        if (udp_property != NULL)
        {
            free(udp_property);
        }
        free(application);
        application = NULL;
    }

    return application;
}

DDS_ReturnCode_t
Application_enable(struct Application *application)
{
    DDS_Entity *entity;
    DDS_ReturnCode_t retcode;

    entity = DDS_DomainParticipant_as_entity(application->participant);

    retcode = DDS_Entity_enable(entity);
    if (retcode != DDS_RETCODE_OK)
    {
        printf("ERROR: Failed to enable entity\n");
    }

    return retcode;
}
