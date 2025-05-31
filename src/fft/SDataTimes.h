#pragma once
struct s_data_times
{
    void reset()
    {
        is_data = false;
        is_time = false;
    }
    bool is_data = false;
    bool is_time = false;
};
