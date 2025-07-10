/***************************************************************************
 *   Copyright (C) 2024 by Claude Code                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License version 2 for more details.                *
 ***************************************************************************/
#ifndef FAKE_RTC_H
#define FAKE_RTC_H

#include <stdint.h>
#include <ctime>
#include <string>

namespace gambatte
{

typedef struct {
    uint32_t total_minutes;        // Total minutes since epoch (Jan 1, 2000 00:00)
    uint32_t last_real_time;       // Last real time check for auto-increment (seconds)
    bool enabled;                  // Whether fake RTC is enabled
    bool needs_save;               // Flag to save data periodically
} fake_rtc_state_type;

class FakeRtc
{
public:
    FakeRtc();
    ~FakeRtc();

    // Core functions
    void init();
    void update();
    void save();
    void load();
    void bump_time(int bump_minutes);
    void get_time(struct tm* time_out);
    void reset_one_off_bump();

    // Configuration
    void set_enabled(bool enabled);
    bool is_enabled() const;
    void set_save_dir(const std::string& save_dir);
    
    // Time bump handling
    void set_persistent_bump(int minutes);
    void set_one_off_bump(int minutes);
    void apply_persistent_bump();
    void apply_one_off_bump();

    // Integration with existing RTC
    uint64_t get_base_time() const;
    void set_base_time(uint64_t base_time);
    
    // Save state support
    void save_state(void* state_data) const;
    void load_state(const void* state_data);
    size_t state_size() const;

private:
    fake_rtc_state_type state_;
    std::string save_dir_;
    int persistent_bump_minutes_;
    int one_off_bump_minutes_;
    int previous_persistent_bump_;
    uint32_t last_save_time_;
    
    // Constants
    static const uint32_t EPOCH_TIMESTAMP = 946684800;  // Jan 1, 2000 00:00:00 UTC
    static const uint32_t SAVE_INTERVAL = 300;          // 5 minutes in seconds
    static const char* SAVE_FILENAME;
    
    // Helper functions
    void update_elapsed_time();
    void save_to_file();
    void load_from_file();
    bool find_and_modify_opt_file(const std::string& option_name, const std::string& new_value);
    std::string get_opt_file_path() const;
    uint32_t get_current_unix_time() const;
    struct tm minutes_to_tm(uint32_t total_minutes) const;
    bool should_save() const;
};

// Global instance for integration
extern FakeRtc* g_fake_rtc;

// C-style interface for easier integration
extern "C" {
    void fake_rtc_init();
    void fake_rtc_update();
    void fake_rtc_save();
    void fake_rtc_load();
    void fake_rtc_bump_time(int bump_minutes);
    void fake_rtc_get_time(struct tm* time_out);
    void fake_rtc_reset_one_off_bump();
    void fake_rtc_set_enabled(bool enabled);
    bool fake_rtc_is_enabled();
    void fake_rtc_set_save_dir(const char* save_dir);
    void fake_rtc_set_persistent_bump(int minutes);
    void fake_rtc_set_one_off_bump(int minutes);
    uint64_t fake_rtc_get_base_time();
    void fake_rtc_set_base_time(uint64_t base_time);
}

}

#endif // FAKE_RTC_H