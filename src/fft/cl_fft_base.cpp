#include "cl_fft_base.h"




// ReSharper disable once CppPossiblyUninitializedMember
my_fft::cl_fft_base::cl_fft_base()
{
// 1. Получаем платформу
    cl_uint num_platforms = 0;
    err_ = clGetPlatformIDs(0, nullptr, &num_platforms);
    if (err_ != CL_SUCCESS || num_platforms == 0) throw std::runtime_error("Failed to find any OpenCL platform");

    std::vector<cl_platform_id> platforms(num_platforms);
    err_ = clGetPlatformIDs(num_platforms, platforms.data(), nullptr);
    if (err_ != CL_SUCCESS)  throw std::runtime_error("Failed to get OpenCL platforms");
    platform_ = platforms[0]; // Используем первую платформу

    // 2. Получаем устройство GPU
    cl_uint num_devices = 0;
    err_ = clGetDeviceIDs(platform_, CL_DEVICE_TYPE_GPU, 0, nullptr, &num_devices);
    if (err_ != CL_SUCCESS || num_devices == 0)  throw std::runtime_error("Failed to find any GPU devices"); 

    std::vector<cl_device_id> devices(num_devices);
    err_ = clGetDeviceIDs(platform_, CL_DEVICE_TYPE_GPU, num_devices, devices.data(), nullptr);
    if (err_ != CL_SUCCESS)  throw std::runtime_error("Failed to get GPU devices"); 
    device_ = devices[0]; // Используем первое устройство

    // 3. Создаём контекст
    context_ = clCreateContext(nullptr, 1, &device_, nullptr, nullptr, &err_);
    if (err_ != CL_SUCCESS)  throw std::runtime_error("Failed to create OpenCL context"); 

    // 4. Создаём очередь команд с профилированием
    constexpr cl_queue_properties props[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};

    queue_ = clCreateCommandQueueWithProperties(context_, device_, props, &err_);
    if (err_ != CL_SUCCESS) throw std::runtime_error("Failed to create command queue");

    // 5. Инициализация clFFT
    clfftSetupData fft_setup;
    err_ = clfftInitSetupData(&fft_setup);
    if (err_ != CL_SUCCESS) throw std::runtime_error("clFFT setup init failed");

    err_ = clfftSetup(&fft_setup);
    if (err_ != CL_SUCCESS) throw std::runtime_error("clFFT setup failed");
    

    // 6. Инициализируем map с лямбда-функциями
    typeActions_ = {
        { std::type_index(typeid(calc_time_opencl)), [this]() { s_data_times_.is_data = false; s_data_times_.is_time = true; } },
        { std::type_index(typeid(data_fft)),        [this]() { s_data_times_.is_data = true;  s_data_times_.is_time = false; } },
        { std::type_index(typeid(fft_data_time)),   [this]() { s_data_times_.is_data = true;  s_data_times_.is_time = true;  } }
    };
}


void my_fft::cl_fft_base::calculate(std::unique_ptr<std::any> data, std::any& t, size_t n, size_t m)
{
    process_type(t.type());
    system_clock::time_point t_start = system_clock::now();
    n_ = n;  m_ = m;
    std::cerr << "number of points " << n_ << "\n"; // кол-во точек

    // 5. Создаем буферы для данных (комплексные числа: float2)
    // Реальная и мнимая части
    const auto& vec_ref = std::any_cast<const v_fft&>(*data);
    auto input_data = const_cast<v_fft*>(&vec_ref); // если нужен неконстант

    //    cl_mem input_buffer_ = clCreateBuffer(context_, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float2) * inputData.size(), inputData.data(), &err_);
    input_buffer_ = clCreateBuffer(context_, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float2) * n_*m_, input_data->data(), &err_);
    cl_mem outputBuffer = clCreateBuffer(context_, CL_MEM_READ_WRITE, sizeof(cl_float2) * n_ * m_, nullptr, &err_);

    if (err_ != CL_SUCCESS)
    {
        clReleaseCommandQueue(queue_);
        clReleaseContext(context_);
        throw std::runtime_error("Error creating buffers: "+ std::to_string(err_));  // Ошибка создания буферов
    }

    // 6. Инициализация clFFT
    clfftSetupData fftSetup;
    err_ = clfftInitSetupData(&fftSetup);
    if (err_ != CL_SUCCESS) throw std::runtime_error("Initialization error clFFT: " + std::to_string(err_));    //  Ошибка инициализации

    err_ = clfftSetup(nullptr);
    if (err_ != CL_SUCCESS) throw std::runtime_error("Installation error clFFT: " + std::to_string(err_));      // Ошибка установк

    if(m_ > 1 )
    {
        err_ = clEnqueueWriteBuffer(queue_, input_buffer_, CL_TRUE, 0, sizeof(cl_float2) * m_ * n_,
            input_data->data(), 0, nullptr, nullptr);
        if (err_ != CL_SUCCESS) throw std::runtime_error("Failed to write buffer" + std::to_string(err_));
    }

    // 7. Создаем план FFT
    clfftPlanHandle plan;
    err_ = clfftCreateDefaultPlan(&plan, context_, CLFFT_1D, &n_);
    if (err_ != CL_SUCCESS)  throw std::runtime_error("Error creating plan FFT: " + std::to_string(err_));     // Ошибка создания плана

    if (m_ == 1)
    {
        // Настраиваем план: комплексный ввод и вывод, прямое преобразование
        err_ = clfftSetPlanPrecision(plan, CLFFT_SINGLE);
        err_ |= clfftSetLayout(plan, CLFFT_COMPLEX_INTERLEAVED, CLFFT_COMPLEX_INTERLEAVED);
        err_ |= clfftSetResultLocation(plan, CLFFT_OUTOFPLACE);
        if (err_ != CL_SUCCESS)   throw std::runtime_error("Error setting up plan FFT: " + std::to_string(err_));     // Ошибка настройки плана
    } else
    {
        err_ = clfftSetPlanBatchSize(plan, m_);
        err_ |= clfftSetPlanPrecision(plan, CLFFT_SINGLE);
        err_ |= clfftSetLayout(plan, CLFFT_COMPLEX_INTERLEAVED, CLFFT_COMPLEX_INTERLEAVED);
        err_ |= clfftSetResultLocation(plan, CLFFT_INPLACE);
        if (err_ != CL_SUCCESS) throw std::runtime_error("Failed to set plan parameters" + std::to_string(err_));
    }

    // Запекаем план
    err_ = clfftBakePlan(plan, 1, &queue_, nullptr, nullptr);
    if (err_ != CL_SUCCESS)   throw std::runtime_error("Error baking plan FFT: " + std::to_string(err_));     // Ошибка запекания плана

    // 8. Выполняем прямое FFT с профилированием
    cl_event event;
    err_ = clfftEnqueueTransform(plan, CLFFT_FORWARD, 1, &queue_, 0, nullptr, &event,
        &input_buffer_, &outputBuffer, nullptr);
    if (err_ != CL_SUCCESS)  throw std::runtime_error("Launch (run) error FFT: " + std::to_string(err_));     // Ошибка запуска

    clFinish(queue_);
    system_clock::time_point t_end = system_clock::now();
    time_opencl_ = calc_Time_OpenCl(event);
    time_opencl_.set_time_average(time_average_value(&t_start, &t_end));

    if (s_data_times_.is_time && (!s_data_times_.is_data))
    {
        t = time_opencl_;
        return;
    }
    process_fft_result(t);
    return;
}

void my_fft::cl_fft_base::process_type(const std::type_info& type) {
    if (const auto it = typeActions_.find(std::type_index(type)); it != typeActions_.end()) 
        it->second(); // Вызов соответствующей функции
    else
        throw std::runtime_error("Error return Class or Struct. ");
}

calc_time_opencl my_fft::cl_fft_base::calc_Time_OpenCl(cl_event event)
{
    cl_ulong start = 0, end = 0;
    cl_ulong queued = 0, submit = 0;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(start), &start, nullptr);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(end), &end, nullptr);

    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &queued, nullptr);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &submit, nullptr);

    double queueTime = (submit - queued) * 1e-3;    // Время в очереди (мkс)
    double submitTime = (start - submit) * 1e-3;   // Время ожидания перед запуском (мkс)

    printf("Queue time (mks) %f (mks)\n", queueTime);                     // Время в очереди (мkс)
    printf("Waiting time before launch (mks): %f (mks)\n", submitTime);      // Время ожидания перед запуском (мkс)

    double elapsedMs = (end - start) * 1e-3;
    std::cout << "Direct execution time FFT: " << elapsedMs << " (mks)\n";     // Время выполнения прямого

    return { queueTime, submitTime, elapsedMs };
}

void my_fft::cl_fft_base::process_fft_result(std::any& t)
{
    // 9. Считываем результат
    std::vector<cl_float2> outputData(n_ * m_);
    err_ = clEnqueueReadBuffer(queue_, input_buffer_, CL_TRUE, 0, sizeof(cl_float2) * n_ * m_,
        outputData.data(), 0, nullptr, nullptr);
    if (err_ != CL_SUCCESS)
        throw std::runtime_error("Buffer read error: " + std::to_string(err_));


    if (m_ == 1) {
        auto amplitudes = std::make_shared<std::vector<float>>(n_);
#pragma omp parallel for
        for (size_t n = 0; n < n_; ++n) {
            float real = outputData[n].x;
            float imag = outputData[n].y;
            (*amplitudes)[n] = std::sqrt(real * real + imag * imag);
        }

        if (s_data_times_.is_time) {
            fft_data_time _fft_data_time;
            _fft_data_time.calc_time_opencl_ = time_opencl_;
            _fft_data_time.data_fft_ = data_fft();
            _fft_data_time.data_fft_.data_one_am = std::move(amplitudes);
            t = _fft_data_time;
            return;
        }
        else {
            data_fft data_fft_;
            data_fft_.data_one_am = std::move(amplitudes);
            t = data_fft_;
            return;
        }
    }
    else {
        using v_fft_many_am = std::vector<std::vector<float>>;
        auto amplitudes = std::make_shared<v_fft_many_am>(m_, std::vector<float>(n_));
#pragma omp parallel for
        for (size_t m = 0; m < m_; ++m) {
            for (size_t n = 0; n < n_; ++n) {
                float real = outputData[m * n_ + n].x;
                float imag = outputData[m * n_ + n].y;
                (*amplitudes)[m][n] = std::sqrt(real * real + imag * imag);
            }
        }

        if (s_data_times_.is_time) {
            fft_data_time _fft_data_time;
            _fft_data_time.calc_time_opencl_ = time_opencl_;
            _fft_data_time.data_fft_ = data_fft();
            _fft_data_time.data_fft_.data_many_am = std::move(amplitudes);
            t = _fft_data_time;
            return;
        }
        else {
            data_fft data_fft_;
            data_fft_.data_many_am = std::move(amplitudes);
            t = data_fft_;
        }
    }
}

my_fft::cl_fft_base::~cl_fft_base()
{
    // clear params OpenCL 
    clReleaseMemObject(input_buffer_);
    clReleaseCommandQueue(queue_);
    clReleaseContext(context_);
}

string my_fft::cl_fft_base::time_average_value(system_clock::time_point* t_start, system_clock::time_point* t_end)
{
    //system_clock::time_point tstart = system_clock::now();
    //std::this_thread::sleep_for(milliseconds(1000));
    //system_clock::time_point tend = system_clock::now();

    system_clock::time_point tstart = *t_start;
    system_clock::time_point tend = *t_end;

    const auto duration = tend - tstart;
    const auto half_duration = duration / 2;
    const system_clock::time_point tmed = tstart + half_duration;

    const std::time_t tmed_time_t = system_clock::to_time_t(tmed);

    std::tm tm;
    localtime_s(&tm, &tmed_time_t);  // ���������� ������ localtime

    auto ms = duration_cast<milliseconds>(tmed.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    std::string datetime_str = oss.str();

    //    std::cout << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    //    std::cout << '.' << std::setfill('0') << std::setw(3) << ms.count() << std::endl;

    return datetime_str;
}


void my_fft::cl_fft_base::print_data_test(data_fft data)
{
}

void my_fft::cl_fft_base::print_data_test(fft_data_time data)
{
}

void my_fft::cl_fft_base::print_data_test(calc_time_opencl times)
{
}

