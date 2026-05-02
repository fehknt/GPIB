#include <gtest/gtest.h>
#include "recorder.h"
#include <time.h>
#include <stdio.h>
#include <windows.h>

class RecorderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temp filename
        char temp_path[MAX_PATH];
        GetTempPathA(MAX_PATH, temp_path);
        GetTempFileNameA(temp_path, "rec", 0, filename);
    }

    void TearDown() override {
        // Delete the temp file
        remove(filename);
    }

    char filename[MAX_PATH];
};

TEST_F(RecorderTest, BasicWriteRead) {
    RECORDER recorder;
    S32 input_width = 100;
    DOUBLE freq_start = 100.0;
    DOUBLE freq_end = 200.0;
    SINGLE min_dBm = -100.0;
    SINGLE max_dBm = 0.0;
    S32 n_amplitude_levels = 10;
    S32 dB_per_division = 10;
    SINGLE initial_top_cursor_dBm = -10.0;
    SINGLE initial_bottom_cursor_dBm = -90.0;

    // Open for writing
    ASSERT_TRUE(recorder.open_writable_file(filename, input_width, freq_start, freq_end, 
                                          min_dBm, max_dBm, n_amplitude_levels, dB_per_division,
                                          initial_top_cursor_dBm, initial_bottom_cursor_dBm, NULL, 0));

    // Write some data
    SINGLE data[100];
    for (int i = 0; i < 100; i++) data[i] = (SINGLE)i;

    recorder.set_arrayed_points(data);
    S32 t = (S32)time(NULL);
    ASSERT_TRUE(recorder.write_record(t, 30.0, -97.0, 150.0, 100.0, 200.0, (C8*)"Test Record"));

    recorder.close_writable_file();

    // Open for reading
    S32 read_file_version = 0;
    DOUBLE read_freq_start = 0;
    DOUBLE read_freq_end = 0;
    SINGLE read_min_dBm = 0;
    SINGLE max_dBm_read = 0;
    S32 read_n_amplitude_levels = 0;
    S32 read_dB_per_division = 0;
    SINGLE read_top_cursor_dBm = 0;
    SINGLE read_bottom_cursor_dBm = 0;

    S32 read_record_width = recorder.open_readable_file(filename, &read_file_version, &read_freq_start, &read_freq_end,
                                                      &read_min_dBm, &max_dBm_read, &read_n_amplitude_levels, &read_dB_per_division,
                                                      &read_top_cursor_dBm, &read_bottom_cursor_dBm, NULL, 0);

    ASSERT_EQ(read_record_width, input_width);
    EXPECT_NEAR(read_freq_start, freq_start, 0.001);
    EXPECT_NEAR(read_freq_end, freq_end, 0.001);

    ASSERT_EQ(recorder.n_readable_records(), 1);

    SINGLE read_data[100];
    DOUBLE read_lat, read_lon, read_alt, read_start, read_stop;
    C8 read_caption[128];
    S32 read_t = recorder.read_record(read_data, 0, &read_lat, &read_lon, &read_alt, &read_start, &read_stop, read_caption, sizeof(read_caption));

    EXPECT_EQ(read_t, t);
    EXPECT_NEAR(read_lat, 30.0, 0.001);
    EXPECT_NEAR(read_lon, -97.0, 0.001);
    EXPECT_NEAR(read_alt, 150.0, 0.001);
    EXPECT_STREQ(read_caption, "Test Record");

    for (int i = 0; i < 100; i++) {
        EXPECT_NEAR(read_data[i], (SINGLE)i, 0.001);
    }

    recorder.close_readable_file();
}
