#pragma once
#include <string>

struct calc_time_opencl
{
    // ����������� � ���������� time_average
    calc_time_opencl(std::string time_average, double queue_time, double submit_time, double elapsed_time)
        : time_average(std::move(time_average)), queue_time(queue_time), submit_time(submit_time), elapsed_time(elapsed_time) {
    }

    // ������������ ����������� ��� time_average � �������� �������� � ������ �������
    calc_time_opencl(double queue_time, double submit_time, double elapsed_time) : calc_time_opencl("", queue_time, submit_time, elapsed_time) {}

    // ����� ��������� time_average � ������������� ������� ��� ����������� � �����������
    void set_time_average(std::string new_time_average, size_t n, size_t m)
    {
        this->n = n;
        this->m = m;
        time_average = std::move(new_time_average);
    }
    calc_time_opencl() = default;

    std::string time_average;
    double queue_time = 0.;
    double submit_time = 0.;
    double elapsed_time = 0.;
    size_t n;      // point FFT
    size_t m;      // beam

};
