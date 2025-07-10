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
#include "fake_rtc.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

// Logging removed for SF2000 compatibility

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace gambatte
{

// Static constants
const char* FakeRtc::SAVE_FILENAME = "gambatte_rtc.dat";

// Global instance
FakeRtc* g_fake_rtc = NULL;
bool g_fake_rtc_initialized = false;

FakeRtc::FakeRtc()
    : persistent_bump_minutes_(0)
    , one_off_bump_minutes_(0)
    , previous_persistent_bump_(0)
    , last_save_time_(0)
{
    memset(&state_, 0, sizeof(state_));
    state_.enabled = true;
    state_.last_real_time = static_cast<uint32_t>(time(NULL));
}

FakeRtc::~FakeRtc()
{
    if (state_.enabled && state_.needs_save)
    {
        save();
    }
}

void FakeRtc::init()
{
    if (!state_.enabled)
        return;
        
    uint32_t time_after_load, time_after_persistent, time_after_oneoff;
    
    // Load existing data first
    load();
    time_after_load = state_.total_minutes;
    
    // Apply persistent bump after loading
    apply_persistent_bump();
    time_after_persistent = state_.total_minutes;
    
    // Apply one-off bump after loading
    apply_one_off_bump();
    time_after_oneoff = state_.total_minutes;
    
    // Update timing
    state_.last_real_time = get_current_unix_time();
    last_save_time_ = state_.last_real_time;
}

void FakeRtc::update()
{
    if (!state_.enabled)
        return;
        
    update_elapsed_time();
    
    // Save periodically
    if (should_save())
    {
        save();
    }
}

void FakeRtc::save()
{
    if (!state_.enabled)
        return;
        
    save_to_file();
    state_.needs_save = false;
    last_save_time_ = get_current_unix_time();
}

void FakeRtc::load()
{
    if (!state_.enabled)
        return;
        
    load_from_file();
}

void FakeRtc::bump_time(int bump_minutes)
{
    if (!state_.enabled)
        return;
        
    uint32_t before = state_.total_minutes;
    
    // Add the bump to total minutes
    if (bump_minutes > 0)
    {
        state_.total_minutes += static_cast<uint32_t>(bump_minutes);
    }
    else if (bump_minutes < 0)
    {
        uint32_t abs_bump = static_cast<uint32_t>(-bump_minutes);
        if (abs_bump > state_.total_minutes)
        {
            state_.total_minutes = 0;
        }
        else
        {
            state_.total_minutes -= abs_bump;
        }
    }
    
    state_.needs_save = true;
}

void FakeRtc::get_time(struct tm* time_out)
{
    if (!time_out)
        return;
        
    if (!state_.enabled)
    {
        // Fall back to system time
        time_t current_time = time(NULL);
        *time_out = *localtime(&current_time);
        return;
    }
    
    *time_out = minutes_to_tm(state_.total_minutes);
}

void FakeRtc::reset_one_off_bump()
{
    if (!state_.enabled)
        return;
        
    // Reset the one-off bump parameter in .opt file
    find_and_modify_opt_file("gambatte_fake_rtc_one_off_bump_minutes", "0");
}

void FakeRtc::set_enabled(bool enabled)
{
    if (state_.enabled && !enabled && state_.needs_save)
    {
        save();
    }
    state_.enabled = enabled;
}

bool FakeRtc::is_enabled() const
{
    return state_.enabled;
}

void FakeRtc::set_save_dir(const std::string& save_dir)
{
    save_dir_ = save_dir;
}

void FakeRtc::set_persistent_bump(int minutes)
{
    persistent_bump_minutes_ = minutes;
}

void FakeRtc::set_one_off_bump(int minutes)
{
    one_off_bump_minutes_ = minutes;
}

void FakeRtc::apply_persistent_bump()
{
    if (!state_.enabled)
        return;
        
    // For persistent bump, we apply the full bump value on first run
    // or apply the difference if it changed
    int diff = persistent_bump_minutes_ - previous_persistent_bump_;
    if (diff != 0)
    {
        uint32_t before = state_.total_minutes;
        bump_time(diff);
        previous_persistent_bump_ = persistent_bump_minutes_;
    }
}

void FakeRtc::apply_one_off_bump()
{
    if (!state_.enabled)
        return;
        
    if (one_off_bump_minutes_ != 0)
    {
        bump_time(one_off_bump_minutes_);
        reset_one_off_bump();
        one_off_bump_minutes_ = 0;
    }
}

uint64_t FakeRtc::get_base_time() const
{
    if (!state_.enabled)
    {
        uint64_t sys_time = std::time(NULL);
        return sys_time;
    }
        
    // Convert our fake time to a unix timestamp
    uint64_t fake_time = EPOCH_TIMESTAMP + (static_cast<uint64_t>(state_.total_minutes) * 60);
    return fake_time;
}

void FakeRtc::set_base_time(uint64_t base_time)
{
    // Convert base time to our fake RTC format
    if (!state_.enabled)
        return;
        
    uint64_t current_time = get_current_unix_time();
    uint64_t fake_time = current_time - base_time + current_time;
    
    if (fake_time >= EPOCH_TIMESTAMP)
    {
        state_.total_minutes = static_cast<uint32_t>((fake_time - EPOCH_TIMESTAMP) / 60);
        state_.needs_save = true;
    }
}

void FakeRtc::save_state(void* state_data) const
{
    if (state_data)
    {
        memcpy(state_data, &state_, sizeof(state_));
    }
}

void FakeRtc::load_state(const void* state_data)
{
    if (state_data)
    {
        memcpy(&state_, state_data, sizeof(state_));
    }
}

size_t FakeRtc::state_size() const
{
    return sizeof(state_);
}

void FakeRtc::update_elapsed_time()
{
    uint32_t current_time = get_current_unix_time();
    uint32_t elapsed_seconds = current_time - state_.last_real_time;
    
    if (elapsed_seconds >= 60)
    {
        uint32_t elapsed_minutes = elapsed_seconds / 60;
        state_.total_minutes += elapsed_minutes;
        state_.last_real_time = current_time - (elapsed_seconds % 60);
        state_.needs_save = true;
    }
}

void FakeRtc::save_to_file()
{
    if (save_dir_.empty())
    {
        return;
    }
        
    std::string filepath = save_dir_ + "/" + SAVE_FILENAME;
    
    std::ofstream file(filepath.c_str(), std::ios::binary);
    if (file.is_open())
    {
        file.write(reinterpret_cast<const char*>(&state_.total_minutes), sizeof(state_.total_minutes));
        file.close();
    }
}

void FakeRtc::load_from_file()
{
    if (save_dir_.empty())
    {
        return;
    }
        
    std::string filepath = save_dir_ + "/" + SAVE_FILENAME;
    
    std::ifstream file(filepath.c_str(), std::ios::binary);
    if (file.is_open())
    {
        uint32_t loaded_minutes = 0;
        file.read(reinterpret_cast<char*>(&loaded_minutes), sizeof(loaded_minutes));
        if (file.gcount() == sizeof(loaded_minutes))
        {
            state_.total_minutes = loaded_minutes;
        }
        file.close();
    }
    else
    {
        // If no save file exists, start with a reasonable time
        // Set to approximately 2 years after epoch (Jan 1, 2002)  
        // This gives games a reasonable-looking RTC time
        state_.total_minutes = 2 * 365 * 24 * 60; // 2 years in minutes
        state_.needs_save = true;
    }
}

bool FakeRtc::find_and_modify_opt_file(const std::string& option_name, const std::string& new_value)
{
    std::string opt_file_path = get_opt_file_path();
    if (opt_file_path.empty())
        return false;
        
    // Read the entire file
    std::ifstream input_file(opt_file_path.c_str());
    if (!input_file.is_open())
        return false;
        
    std::string file_content;
    std::string line;
    bool found = false;
    
    while (std::getline(input_file, line))
    {
        if (line.find(option_name + " = ") == 0)
        {
            file_content += option_name + " = \"" + new_value + "\"\n";
            found = true;
        }
        else
        {
            file_content += line + "\n";
        }
    }
    input_file.close();
    
    if (!found)
        return false;
        
    // Write back to file
    std::ofstream output_file(opt_file_path.c_str());
    if (output_file.is_open())
    {
        output_file << file_content;
        output_file.close();
        return true;
    }
    
    return false;
}

std::string FakeRtc::get_opt_file_path() const
{
    if (save_dir_.empty())
        return "";
        
    // Try multiple possible locations
    std::string possible_paths[2];
    possible_paths[0] = save_dir_ + "/gambatte.opt";
    possible_paths[1] = save_dir_ + "/../configs/gambatte/gambatte.opt";
    
    for (int i = 0; i < 2; i++)
    {
        std::ifstream file(possible_paths[i].c_str());
        if (file.is_open())
        {
            file.close();
            return possible_paths[i];
        }
    }
    
    return "";
}

uint32_t FakeRtc::get_current_unix_time() const
{
    return static_cast<uint32_t>(time(NULL));
}

struct tm FakeRtc::minutes_to_tm(uint32_t total_minutes) const
{
    struct tm result;
    memset(&result, 0, sizeof(result));
    
    // Convert total minutes to seconds since epoch
    uint64_t total_seconds = EPOCH_TIMESTAMP + (static_cast<uint64_t>(total_minutes) * 60);
    
    // Convert to tm structure
    time_t time_val = static_cast<time_t>(total_seconds);
    struct tm* tm_ptr = gmtime(&time_val);
    
    if (tm_ptr)
    {
        result = *tm_ptr;
    }
    
    return result;
}

bool FakeRtc::should_save() const
{
    if (!state_.needs_save)
        return false;
        
    uint32_t current_time = get_current_unix_time();
    return (current_time - last_save_time_) >= SAVE_INTERVAL;
}

// C-style interface implementation
extern "C" {

void fake_rtc_init()
{
    if (!g_fake_rtc)
    {
        g_fake_rtc = new FakeRtc();
    }
    
    if (!g_fake_rtc_initialized)
    {
        g_fake_rtc->init();
        g_fake_rtc_initialized = true;
    }
}

void fake_rtc_update()
{
    if (g_fake_rtc)
    {
        g_fake_rtc->update();
    }
}

void fake_rtc_save()
{
    if (g_fake_rtc)
    {
        g_fake_rtc->save();
    }
}

void fake_rtc_load()
{
    if (g_fake_rtc)
    {
        g_fake_rtc->load();
    }
}

void fake_rtc_bump_time(int bump_minutes)
{
    if (g_fake_rtc)
    {
        g_fake_rtc->bump_time(bump_minutes);
    }
}

void fake_rtc_get_time(struct tm* time_out)
{
    if (g_fake_rtc)
    {
        g_fake_rtc->get_time(time_out);
    }
}

void fake_rtc_reset_one_off_bump()
{
    if (g_fake_rtc)
    {
        g_fake_rtc->reset_one_off_bump();
    }
}

void fake_rtc_set_enabled(bool enabled)
{
    if (!g_fake_rtc)
    {
        g_fake_rtc = new FakeRtc();
    }
    g_fake_rtc->set_enabled(enabled);
}

bool fake_rtc_is_enabled()
{
    if (g_fake_rtc)
    {
        return g_fake_rtc->is_enabled();
    }
    return false;
}

void fake_rtc_set_save_dir(const char* save_dir)
{
    if (!g_fake_rtc)
    {
        g_fake_rtc = new FakeRtc();
    }
    if (save_dir)
    {
        g_fake_rtc->set_save_dir(std::string(save_dir));
    }
}

void fake_rtc_set_persistent_bump(int minutes)
{
    if (!g_fake_rtc)
    {
        g_fake_rtc = new FakeRtc();
    }
    g_fake_rtc->set_persistent_bump(minutes);
}

void fake_rtc_set_one_off_bump(int minutes)
{
    if (!g_fake_rtc)
    {
        g_fake_rtc = new FakeRtc();
    }
    g_fake_rtc->set_one_off_bump(minutes);
}

uint64_t fake_rtc_get_base_time()
{
    if (g_fake_rtc)
    {
        return g_fake_rtc->get_base_time();
    }
    return 0;
}

void fake_rtc_set_base_time(uint64_t base_time)
{
    if (g_fake_rtc)
    {
        g_fake_rtc->set_base_time(base_time);
    }
}

} // extern "C"

} // namespace gambatte