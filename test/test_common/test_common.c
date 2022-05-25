#include <unity.h>

#include <string.h>

#include "exposure-notification.h"

#define TEST_ASSERT_EQUAL_KEY(k1, k2) TEST_ASSERT_EQUAL(0, memcmp(k1.b, k2.b, sizeof(k1.b)))
#define TEST_ASSERT_NOT_EQUAL_KEY(k1, k2) TEST_ASSERT_NOT_EQUAL(0, memcmp(k1.b, k2.b, sizeof(k1.b)))


void test_init(void) {
    int ret = en_init(NULL);   
    TEST_ASSERT_EQUAL(0, ret);
}

void test_get_interval_number(void) {
    TEST_ASSERT_EQUAL(0, en_get_interval_number(0));       
    TEST_ASSERT_EQUAL(0, en_get_interval_number(EN_INTERVAL_LENGTH-1));       
    TEST_ASSERT_EQUAL(1, en_get_interval_number(EN_INTERVAL_LENGTH));
    TEST_ASSERT_EQUAL(42, en_get_interval_number(EN_INTERVAL_LENGTH*42));       
}

void test_get_interval_number_at_period_start(void) {
    TEST_ASSERT_EQUAL(0, en_get_interval_number_at_period_start(0));       
    TEST_ASSERT_EQUAL(0, en_get_interval_number_at_period_start(EN_INTERVAL_LENGTH-1));       
    TEST_ASSERT_EQUAL(0, en_get_interval_number_at_period_start(EN_INTERVAL_LENGTH));
    TEST_ASSERT_EQUAL(0, en_get_interval_number_at_period_start(EN_INTERVAL_LENGTH*42));       
    TEST_ASSERT_EQUAL(EN_TEK_ROLLING_PERIOD, en_get_interval_number_at_period_start(EN_INTERVAL_LENGTH*EN_TEK_ROLLING_PERIOD));
    TEST_ASSERT_EQUAL(EN_TEK_ROLLING_PERIOD, en_get_interval_number_at_period_start(EN_INTERVAL_LENGTH*EN_TEK_ROLLING_PERIOD+1));
}

void test_generate_period_key(void) {
    ENPeriodKey p1, p2;
    en_generate_period_key(&p1);
    en_generate_period_key(&p2);
    // yes there is a minimal chance that both are the same ;)
    TEST_ASSERT_NOT_EQUAL_KEY(p1, p2);
}

void test_get_period_identifier_key(void) {

    ENPeriodKey pk;
    en_generate_period_key(&pk);

    ENPeriodIdentifierKey k1, k2;
    en_derive_period_identifier_key(&k1, &pk);
    en_derive_period_identifier_key(&k2, &pk);

    // This time both should be the same key as the depent on the period key
    TEST_ASSERT_EQUAL_KEY(k1, k2);
    
    // BUT: they should be different than the period key itself
    TEST_ASSERT_NOT_EQUAL_KEY(k1, pk);
}


void test_get_interval_identifier(void) {

    ENPeriodKey pk;
    en_generate_period_key(&pk);

    ENIntervalNumber iv = en_get_interval_number(0);

    ENIntervalIdentifier id1, id2;
    en_derive_interval_identifier(&id1, &pk, iv);
    en_derive_interval_identifier(&id2, &pk, iv);

    // identities should be the same for the same intervalnumber and period key

    TEST_ASSERT_EQUAL_KEY(id1, id2);

    iv = en_get_interval_number(EN_INTERVAL_LENGTH);
    en_derive_interval_identifier(&id2, &pk, iv);

    // BUT: they should be different when we are using another interval number
    TEST_ASSERT_NOT_EQUAL_KEY(id1, id2);

    // They should also be different for another period key but the the same interval number
    en_generate_period_key(&pk);
    en_derive_interval_identifier(&id1, &pk, iv);

    en_generate_period_key(&pk);
    en_derive_interval_identifier(&id2, &pk, iv);

    TEST_ASSERT_NOT_EQUAL_KEY(id1, id2);
}

void test_get_period_metadata_encryption_key(void) {

    ENPeriodKey pk;
    en_generate_period_key(&pk);
    ENPeriodMetadataEncryptionKey k1, k2;
    en_derive_period_metadata_encryption_key(&k1, &pk);
    en_derive_period_metadata_encryption_key(&k2, &pk);

    // This time both should be the same key as the depent on the period key
    TEST_ASSERT_EQUAL_KEY(k1, k2);
    
    // BUT: they should be different than the period key itself
    TEST_ASSERT_NOT_EQUAL_KEY(k1, pk);

    // and they should also be different than the periodIdentifierKey
    ENPeriodIdentifierKey pik;
    en_derive_period_identifier_key(&pik, &pk);
    TEST_ASSERT_NOT_EQUAL_KEY(k1, pik);
}


void test_en_encrypt_interval_metadata(void) {

    ENPeriodKey pk;
    en_generate_period_key(&pk);
    ENPeriodMetadataEncryptionKey k1;
    en_derive_period_metadata_encryption_key(&k1, &pk);

    ENIntervalNumber iv = en_get_interval_number(0);
    ENIntervalIdentifier id;
    en_derive_interval_identifier(&id, &pk, iv);

    unsigned char testData[42] = "This is some test data :)";
    unsigned char encryptedData[42] = "";

    en_encrypt_interval_metadata(&k1, &id, testData, encryptedData, sizeof(testData));

    // the original data should not be changed
    TEST_ASSERT_EQUAL_CHAR_ARRAY("This is some test data :)", testData, sizeof("This is some test data :)")-1);

    // but the encrypted one should be changed
    TEST_ASSERT_NOT_EQUAL(0, memcmp(testData, encryptedData, sizeof(testData)));
    
    unsigned char decryptedData[42] = "";
    en_decrypt_interval_metadata(&k1, &id, encryptedData, decryptedData, sizeof(testData));
    
    // the decrypted metadata should be the same as the testData
    TEST_ASSERT_EQUAL_CHAR_ARRAY(testData, decryptedData, sizeof(testData));
}

void test_identifier_generation_performance(void) {

    int runs = 1000000;

    for(int i = 0; i < runs; i++) {
        ENPeriodKey pk;
        en_generate_period_key(&pk);
        ENPeriodIdentifierKey ik;
        en_derive_period_identifier_key(&ik, &pk);

        for(int iv = 0; iv < EN_TEK_ROLLING_PERIOD; iv++) {
            ENIntervalNumber intervalNumber = en_get_interval_number(iv);
            ENIntervalIdentifier id;
            en_derive_interval_identifier(&id, &ik, intervalNumber);
        }
    }
}

void test_generate_and_derive_period_keys(void) {
    // Test multiple generation
    ENPeriodKey periodKey;
    ENPeriodIdentifierKey periodIdentifierKey;
    ENPeriodMetadataEncryptionKey periodMetadaEncryptionKey;
    for(int i = 0; i < 10; i++) {
        en_generate_and_derive_period_keys(&periodKey, &periodIdentifierKey, &periodMetadaEncryptionKey);
    }
}


void test_against_fixtures(void) {
    // First define base values
    ENIntervalNumber intervalNumber = 2642976;
    ENPeriodKey periodKey = {.b = {0x75, 0xc7, 0x34, 0xc6, 0xdd, 0x1a, 0x78, 0x2d, 0xe7, 0xa9, 0x65, 0xda, 0x5e, 0xb9, 0x31, 0x25}};
    unsigned char metadata[4] = {0x40, 0x08, 0x00, 0x00};

    // define the expected values
    ENPeriodIdentifierKey expectedPIK = {.b = {0x18, 0x5a, 0xd9, 0x1d, 0xb6, 0x9e, 0xc7, 0xdd, 0x04, 0x89, 0x60, 0xf1, 0xf3, 0xba, 0x61, 0x75}};
    ENPeriodMetadataEncryptionKey expectedPMEK = {.b = {0xd5, 0x7c, 0x46, 0xaf, 0x7a, 0x1d, 0x83, 0x96, 0x5b, 0x9b, 0xed, 0x8b, 0xd1, 0x52, 0x93, 0x6a}};

    ENIntervalIdentifier expectedIntervalIdentifier = {.b = {0x8b, 0xe6, 0xcd, 0x37, 0x1c, 0x5c, 0x89, 0x16, 0x04, 0xbf, 0xbe, 0x49, 0xdf, 0x84, 0x50, 0x96}};
    unsigned char expectedEncryptedMetadata[4] = {0x72, 0x03, 0x38, 0x74};


    ENPeriodIdentifierKey pik;
    en_derive_period_identifier_key(&pik, &periodKey);
    TEST_ASSERT_EQUAL_KEY(expectedPIK, pik);

    ENPeriodMetadataEncryptionKey pmek;
    en_derive_period_metadata_encryption_key(&pmek, &periodKey);
    TEST_ASSERT_EQUAL_KEY(expectedPMEK, pmek);

    ENIntervalIdentifier intervalIdentifier;
    en_derive_interval_identifier(&intervalIdentifier, &pik, intervalNumber);
    TEST_ASSERT_EQUAL_KEY(expectedIntervalIdentifier, intervalIdentifier);


    unsigned char encryptedMetadata[sizeof(metadata)] = {0};
    en_encrypt_interval_metadata(&pmek, &intervalIdentifier, metadata, encryptedMetadata, sizeof(metadata));
    TEST_ASSERT_EQUAL_CHAR_ARRAY(expectedEncryptedMetadata, encryptedMetadata, sizeof(expectedEncryptedMetadata));
}


void test_exposure_notification() {
    UNITY_BEGIN();
    RUN_TEST(test_init);
    RUN_TEST(test_get_interval_number);
    RUN_TEST(test_get_interval_number_at_period_start);
    RUN_TEST(test_generate_period_key);
    RUN_TEST(test_get_period_identifier_key);
    RUN_TEST(test_get_interval_identifier);
    RUN_TEST(test_get_period_metadata_encryption_key);
    RUN_TEST(test_en_encrypt_interval_metadata);
    RUN_TEST(test_generate_and_derive_period_keys);
    RUN_TEST(test_against_fixtures);
    //RUN_TEST(test_identifier_generation_performance);
    UNITY_END();
}




int main(int argc, char **argv) {
    test_exposure_notification();
    return 0;
}