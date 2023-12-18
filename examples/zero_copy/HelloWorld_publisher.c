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
#include "wh_sm/wh_sm_history.h"
#include "rh_sm/rh_sm_history.h"
#include "disc_dpse/disc_dpse_dpsediscovery.h"
#include "netio_zcopy/posixNotifMechanism.h"

#include "HelloWorld.h"
#include "HelloWorldSupport.h"
#include "HelloWorldPlugin.h"
#include "HelloWorldApplication.h"

static void
HelloWorldPublisher_on_publication_matched(
        void *listener_data,
        DDS_DataWriter *writer,
        const struct DDS_PublicationMatchedStatus *status)
{
    /* listener_data and writer parameters not used in this example */
    (void)listener_data;
    (void)writer;

    if (status->current_count_change > 0)
    {
        printf("INFO: Matched a subscriber\n");
    }
    else if (status->current_count_change < 0)
    {
        printf("INFO: Unmatched a subscriber\n");
    }
}

static int
publisher_main_w_args(
        DDS_DomainId_t domain_id,
        char *peer,
        long int time_between_writes,
        long int count)
{
    DDS_Publisher *publisher;
    DDS_DataWriter *datawriter;
    HelloWorldDataWriter *hw_datawriter;
    struct DDS_DataWriterQos dw_qos = DDS_DataWriterQos_INITIALIZER;
    DDS_ReturnCode_t retcode;
    HelloWorld *sample = NULL;
    struct Application *application = NULL;
    struct DDS_DataWriterListener dw_listener = DDS_DataWriterListener_INITIALIZER;
    struct DDS_SubscriptionBuiltinTopicData rem_subscription_data =
            DDS_SubscriptionBuiltinTopicData_INITIALIZER;

    application = Application_create(
            "publisher",
            "subscriber",
            0,
            domain_id,
            peer,
            time_between_writes,
            count);

    if (application == NULL)
    {
        printf("ERROR: Failed Application create\n");
        goto done;
    }

    publisher = DDS_DomainParticipant_create_publisher(
            application->participant,
            &DDS_PUBLISHER_QOS_DEFAULT,
            NULL,
            DDS_STATUS_MASK_NONE);
    if (publisher == NULL)
    {
        printf("ERROR: publisher == NULL\n");
        goto done;
    }

    dw_qos.reliability.kind = DDS_BEST_EFFORT_RELIABILITY_QOS;

    dw_qos.resource_limits.max_samples_per_instance = 100;
    dw_qos.resource_limits.max_instances = 2;
    dw_qos.resource_limits.max_samples = dw_qos.resource_limits.max_instances *
                                         dw_qos.resource_limits.max_samples_per_instance;
    dw_qos.history.depth = 100;

    dw_qos.protocol.rtps_reliable_writer.heartbeat_period.sec = 0;
    dw_qos.protocol.rtps_reliable_writer.heartbeat_period.nanosec = 250000000;

    dw_qos.durability.kind = DDS_TRANSIENT_LOCAL_DURABILITY_QOS;
    dw_qos.protocol.rtps_object_id = 100;
   
    dw_listener.on_publication_matched = HelloWorldPublisher_on_publication_matched;
    datawriter = DDS_Publisher_create_datawriter(
            publisher,
            application->topic,
            &dw_qos,
            &dw_listener,
            DDS_PUBLICATION_MATCHED_STATUS);
    if (datawriter == NULL)
    {
        printf("ERROR: datawriter == NULL\n");
        goto done;
    }

    /* Assert Remote Subscriptions */
    rem_subscription_data.topic_name = DDS_String_dup(application->topic_name);
    rem_subscription_data.type_name = DDS_String_dup(application->type_name);
    rem_subscription_data.reliability.kind = dw_qos.reliability.kind;
    rem_subscription_data.durability.kind = dw_qos.durability.kind;
    rem_subscription_data.key.value[DDS_BUILTIN_TOPIC_KEY_OBJECT_ID] = 101;
    
    if (DDS_RETCODE_OK !=
        DPSE_RemoteSubscription_assert(
                application->participant,
                "subscriber",
                &rem_subscription_data,
                HelloWorld_get_key_kind(HelloWorldTypePlugin_get(), NULL)))
    {
        printf("ERROR: Failed to assert remote subscription\n");
        goto done;
    }

    retcode = Application_enable(application);
    if (retcode != DDS_RETCODE_OK)
    {
        printf("ERROR: Failed to enable application\n");
        goto done;
    }

    printf("INFO: Construction done, press <return> to go ahead...\n");
    getchar();

    hw_datawriter = HelloWorldDataWriter_narrow(datawriter);
    for (int i = 0; i < count; ++i)
    {
        retcode = HelloWorldDataWriter_get_loan(hw_datawriter, &sample);
        if (retcode != DDS_RETCODE_OK)
        {
            printf("ERROR: Failed to loan sample\n");
        }
        /* two instances will be created, id = 0 and id = 1 */
        sample->id = i % 2;
        sample->data[0] = 100 + i;
        printf("INFO: Writing sample %d\n", i);
        retcode = HelloWorldDataWriter_write(hw_datawriter, sample, &DDS_HANDLE_NIL);
        if (retcode != DDS_RETCODE_OK)
        {
            printf("ERROR: Failed to write to sample\n");
        }
        application_sleep(application->period, 0);
    }
done:
    return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
    DDS_DomainId_t domain_id = 0;
    char *peer = NULL;
    long int time_between_writes = 1;
    long int count = 100;
  
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-domain"))
        {
            ++i;
            if (i == argc)
            {
                printf("ERROR: -domain <domain_id>\n");
                return EXIT_FAILURE;
            }
            /* The value of a DDS Domain ID is held in a signed 32-bit integer, 
             * and the range of valid values is in fact only in the hundreds.           
             * For that reason, this conversion is safe.
             */
            long l = strtol(argv[i], NULL, 0);
            if (l <= INT_MAX) 
            {
                domain_id = (DDS_DomainId_t)l;
            }
            else 
            { 
                return EXIT_FAILURE;
            }          
        }
        else if (!strcmp(argv[i], "-peer"))
        {
            ++i;
            if (i == argc)
            {
                printf("ERROR -peer <address>\n");
                return EXIT_FAILURE;
            }
            peer = argv[i];
        }
        else if (!strcmp(argv[i], "-period"))
        {
            ++i;
            if (i == argc)
            {
                printf("ERROR: -period <time between writes>\n");
                return EXIT_FAILURE;
            }
            time_between_writes = strtol(argv[i], NULL, 0);
        }
        else if (!strcmp(argv[i], "-count"))
        {
            ++i;
            if (i == argc)
            {
                printf("ERROR: -count <count>\n");
                return EXIT_FAILURE;
            }
            count = strtol(argv[i], NULL, 0);
        }
        else if (!strcmp(argv[i], "-h"))
        {
            Application_help(argv[0]);
            return EXIT_SUCCESS;
        }
        else
        {
            printf("ERROR: Unknown option: %s\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    return publisher_main_w_args(domain_id, peer, time_between_writes, count);
}
