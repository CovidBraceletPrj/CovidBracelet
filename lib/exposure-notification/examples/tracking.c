#include "exposure-notification.h"
#include "time.h"


int main(int argc, char **argv) {

    // first init everything
    // TODO: More randomization, we init with NULL to let en init mbdet entropy for us
    en_init(NULL);

    // get the currentTime
    time_t currentTime = time(NULL);

    // The periodKey changes each EN_TEK_ROLLING_PERIOD intervals
    ENIntervalNumber periodInterval = en_get_interval_number_at_period_start(currentTime);

    ENPeriodKey periodKey;
    ENPeriodIdentifierKey periodIdentifierKey;
    ENPeriodMetadataEncryptionKey periodMetadaEncryptionKey;

    // setup period keys at the beginning
    // in theory you could let them generate automatically setting periodInterval to 0
    en_generate_and_derive_period_keys(&periodKey, &periodIdentifierKey, &periodMetadaEncryptionKey);

    char metadata[128] = "";

    while(1) {
        // we check the current time to know if we actually need to regenerate anything
        currentTime = time(NULL);
        ENIntervalNumber currentInterval = en_get_interval_number(currentTime);

        // we check if we need to generate new keys
        if (currentInterval - periodInterval >= EN_TEK_ROLLING_PERIOD) {
            periodInterval = en_get_interval_number_at_period_start(currentTime);
            en_generate_and_derive_period_keys(&periodKey, &periodIdentifierKey, &periodMetadaEncryptionKey);
            // TODO: Store new periodKey with periodInterval
        }

        // we now generate the new interval identifier and re-encrypt the metadata
        ENIntervalIdentifier intervalIdentifier;
        en_derive_interval_identifier(&intervalIdentifier, &periodIdentifierKey, currentInterval);

        char encryptedMetadata[sizeof(metadata)];
        en_encrypt_interval_metadata(&periodMetadaEncryptionKey, &intervalIdentifier, metadata, encryptedMetadata, sizeof(metadata));

        // broadcast intervalIdentifier plus encryptedMetada according to specs
        // TODO: receive packets and store them
        // repeat for 10-20 minutes
    }

    return 0;
}