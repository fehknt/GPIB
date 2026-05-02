#include "appfile.h"
#include <gtest/gtest.h>

TEST(AppFile, KVStoreBasic) {
    KVSTORE<KVAL> db;
    
    db.addstr("test_key", NULL, "test_value");
    ASSERT_STREQ(db.str("test_key"), "test_value");
    
    db.update_value("test_key", "new_value");
    ASSERT_STREQ(db.str("test_key"), "new_value");
}

TEST(AppFile, KVStoreTypes) {
    KVSTORE<KVAL> db;
    
    S32 val_num = 123;
    db.addnum("num_key", &val_num);
    ASSERT_EQ(db.num("num_key"), 123);
    
    db.update_value("num_key", "456");
    ASSERT_EQ(db.num("num_key"), 456);
    ASSERT_EQ(val_num, 456); // Backing variable should be updated
    
    DOUBLE val_dbl = 1.234;
    db.adddbl("dbl_key", &val_dbl);
    ASSERT_NEAR(db.dbl("dbl_key"), 1.234, 0.0001);
    
    db.update_value("dbl_key", "5.678");
    ASSERT_NEAR(db.dbl("dbl_key"), 5.678, 0.0001);
    ASSERT_NEAR(val_dbl, 5.678, 0.0001);
}

TEST(AppFile, KVStoreFileIO) {
    KVSTORE<KVAL> db;
    db.addstr("key1", NULL, "val1");
    db.addnum("key2", 123);
    
    const char* filename = "test_config.ini";
    ASSERT_TRUE(db.write(filename));
    
    KVSTORE<KVAL> db2;
    db2.addstr("key1", NULL, "");
    db2.addnum("key2", 0);
    
    ASSERT_TRUE(db2.read(filename));
    ASSERT_STREQ(db2.str("key1"), "val1");
    ASSERT_EQ(db2.num("key2"), 123);
    
    _unlink(filename);
}

TEST(AppFile, TempFn) {
    {
        TEMPFN tmp(".txt");
        ASSERT_TRUE(tmp.status());
        ASSERT_TRUE(strstr(tmp.name, ".txt") != NULL);
        
        FILE* f = fopen(tmp.name, "w");
        ASSERT_TRUE(f != NULL);
        fprintf(f, "test");
        fclose(f);
        
        // File should exist
        f = fopen(tmp.name, "r");
        ASSERT_TRUE(f != NULL);
        fclose(f);
    }
    // After destructor, file should be gone (unless keep was true)
    // Wait a bit just in case? No, it should be synchronous.
}
