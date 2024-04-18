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
HelloWorldSubscriber_on_data_available(void *listener_data, DDS_DataReader *reader)
{
    HelloWorldDataReader *hw_reader = HelloWorldDataReader_narrow(reader);
    DDS_ReturnCode_t retcode;
    struct DDS_SampleInfo *sample_info = NULL;
    HelloWorld *sample = NULL;
    struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
    struct HelloWorldSeq sample_seq = DDS_SEQUENCE_INITIALIZER;
    const DDS_Long TAKE_MAX_SAMPLES = 32;

    (void)listener_data;

    printf("INFO: Sample received\n");
    DDS_Long samples_taken = 0;

    do
    {
        retcode = HelloWorldDataReader_take(
                hw_reader,
                &sample_seq,
                &info_seq,
                TAKE_MAX_SAMPLES,
                DDS_ANY_SAMPLE_STATE,
                DDS_ANY_VIEW_STATE,
                DDS_ANY_INSTANCE_STATE);

        if (retcode == DDS_RETCODE_NO_DATA)
        {
            /* If this is the first time through the loop and there are no
             * Samples available, then an INFO message is written to the
             * console. However, we may pass through this loop multiple times
             * if there are more than TAKE_MAX_SAMPLES Samples available-- don't
             * output a message in the case that we are simply done reading a
             * larger batch of Samples.
             */
            if (samples_taken == 0)
            {
                printf("INFO: No data available\n");;
            }
            continue;
        }
        else if (retcode != DDS_RETCODE_OK)
        {
            printf("ERROR: take() failed with retcode %d\n", retcode);
            continue;
        }

        /* Print each valid sample taken */
        for (int i = 0; i < HelloWorldSeq_get_length(&sample_seq); ++i)
        {
            sample_info = DDS_SampleInfoSeq_get_reference(&info_seq, i);

            if (sample_info->valid_data)
            {
                sample = HelloWorldSeq_get_reference(&sample_seq, i);
                printf("\tid : %d data[0]: %d, instance_state is 0x%02x\n",
                        sample->id,
                        sample->data[0],
                        sample_info->instance_state);
            }
            else
            {
                printf("INFO: Sample received\n\tINVALID DATA, "
                        "instance_state is 0x%02x\n",
                        sample_info->instance_state);
            }

            samples_taken++;
        }
        HelloWorldDataReader_return_loan(hw_reader, &sample_seq, &info_seq);
    } while (DDS_RETCODE_OK == retcode);
}

static void
HelloWorldSubscriber_on_subscription_matched(
        void *listener_data,
        DDS_DataReader *reader,
        const struct DDS_SubscriptionMatchedStatus *status)
{
    (void)listener_data;
    (void)reader;

    if (status->current_count_change > 0)
    {
        printf("INFO: Matched a publisher\n");
    }
    else if (status->current_count_change < 0)
    {
        printf("INFO: Unmatched a publisher\n");
    }
}

static int
subscriber_main_w_args(
        DDS_DomainId_t domain_id,
        char *peer,
        long int time_to_run,
        long int count)
{
    DDS_Subscriber *subscriber;
    DDS_DataReader *datareader;
    struct DDS_DataReaderQos dr_qos = DDS_DataReaderQos_INITIALIZER;
    DDS_ReturnCode_t retcode;
    struct DDS_DataReaderListener dr_listener = DDS_DataReaderListener_INITIALIZER;
    struct DDS_PublicationBuiltinTopicData rem_publication_data =
            DDS_PublicationBuiltinTopicData_INITIALIZER;
    struct Application *application;

    application = Application_create(
            "subscriber",
            "publisher",
            1,
            domain_id,
            peer,
            time_to_run,
            count);

    if (application == NULL)
    {
        return EXIT_FAILURE;
    }

    subscriber = DDS_DomainParticipant_create_subscriber(
            application->participant,
            &DDS_SUBSCRIBER_QOS_DEFAULT,
            NULL,
            DDS_STATUS_MASK_NONE);
    if (subscriber == NULL)
    {
        printf("ERROR: subscriber == NULL\n");
        goto done;
    }

    dr_listener.on_data_available = HelloWorldSubscriber_on_data_available;
    dr_listener.on_subscription_matched = HelloWorldSubscriber_on_subscription_matched;

    dr_qos.resource_limits.max_instances = 2;
    dr_qos.resource_limits.max_samples_per_instance = 100;
    dr_qos.resource_limits.max_samples = dr_qos.resource_limits.max_instances *
                                         dr_qos.resource_limits.max_samples_per_instance;
    /* if there are more remote writers, you need to increase these limits */
    dr_qos.reader_resource_limits.max_remote_writers = 10;
    dr_qos.reader_resource_limits.max_remote_writers_per_instance = 10;
    dr_qos.history.depth = 100;

    dr_qos.reliability.kind = DDS_BEST_EFFORT_RELIABILITY_QOS;
    dr_qos.durability.kind = DDS_TRANSIENT_LOCAL_DURABILITY_QOS;

    /* DW1 on notif:// with object_id 101 */
    dr_qos.protocol.rtps_object_id = 101;
    datareader = DDS_Subscriber_create_datareader(
            subscriber,
            DDS_Topic_as_topicdescription(application->topic),
            &dr_qos,
            &dr_listener,
            DDS_DATA_AVAILABLE_STATUS | DDS_SUBSCRIPTION_MATCHED_STATUS);
    if (datareader == NULL)
    {
        printf("ERROR: datareader == NULL\n");
        goto done;
    }

    /*Assert Remote Publications */
    printf("INFO: Asserting remote publisher \n");
    rem_publication_data.key.value[DDS_BUILTIN_TOPIC_KEY_OBJECT_ID] = 100;
    rem_publication_data.topic_name = DDS_String_dup(application->topic_name);
    rem_publication_data.type_name = DDS_String_dup(application->type_name);
    rem_publication_data.reliability.kind = dr_qos.reliability.kind;
    rem_publication_data.durability.kind = dr_qos.durability.kind;

    if (DDS_RETCODE_OK !=
        DPSE_RemotePublication_assert(
                application->participant,
                "publisher",
                &rem_publication_data,
                HelloWorld_get_key_kind(HelloWorldTypePlugin_get(), NULL)))
    {
        printf("ERROR: Failed to assert remote publication\n");
        goto done;
    }

    retcode = Application_enable(application);
    if (retcode != DDS_RETCODE_OK)
    {
        printf("ERROR: Failed to enable application\n");
        goto done;
    }

    if (application->period != 0)
    {
        printf("INFO: Running for %u seconds, press Ctrl-C to exit\n", (unsigned int)application->period);
		application_sleep(application->period, 0);
        printf("INFO: Subscriber done, closing down\n");
    }
    else
    {
        printf("INFO: Running until <return> has been pressed...\n");
        getchar();
    }

done:
    return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
    DDS_DomainId_t domain_id = 0;
    char *peer = NULL;
    long int time_to_run = 60;
    long int count = 0;

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
                printf("ERROR: -peer <address>\n");
                return EXIT_FAILURE;
            }
            peer = argv[i];
        }
        else if (!strcmp(argv[i], "-period"))
        {
            ++i;
            if (i == argc)
            {
                printf("ERROR: -period <time_to_run>\n");
                return EXIT_FAILURE;
            }
            time_to_run = strtol(argv[i], NULL, 0);
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
            printf("INFO: <count> ignored in subscriber application\n");
        }
        else if (!strcmp(argv[i], "-h"))
        {
            Application_help(argv[0]);
            return EXIT_SUCCESS;
        }
        else
        {
            printf("ERROR: unknown option: %s", argv[i]);
            return EXIT_FAILURE;
        }
    }

    return subscriber_main_w_args(domain_id, peer, time_to_run, count);
}
