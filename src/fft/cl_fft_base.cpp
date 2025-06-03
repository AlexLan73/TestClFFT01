#include "cl_fft_base.h"

#include <ranges>


// ReSharper disable once CppPossiblyUninitializedMember
wrapper::cl_fft_base::cl_fft_base()
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

    // Инициализация handlers_ в конструкторе
    if (handlers_.empty()) {
        handlers_.emplace(typeid(calc_time_opencl), [this](std::any& t) { return_calc_time_opencl(t); });
        handlers_.emplace(typeid(data_fft), [this](std::any& t) { return_data_fft(t); });
        handlers_.emplace(typeid(fft_data_time), [this](std::any& t) { return_fft_data_time(t); });
    }
}



//void wrapper::cl_fft_base::calculate(std::unique_ptr<std::any> data, std::any& t, size_t n, size_t m)
void wrapper::cl_fft_base::calculate(std::shared_ptr<v_fft> data, std::any& t, size_t n, size_t m)
{
    process_type(t.type());
    system_clock::time_point t_start = system_clock::now();
//    n_ = 1 << 10;     m_ = 1;
    n_ = n;  m_ = m;
    if (is_print_ == true)
    {
        std::cerr << "count beam " << m_ << "\n";       // кол-во лучей
        std::cerr << "number of points " << n_ << "\n"; // кол-во точек
    }

    std::shared_ptr<v_fft> input_data_ = std::move(data);

    buffer_ = clCreateBuffer(context_, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float2) * n_ * m_,
        input_data_->data(), &err_);

    outputBuffer = clCreateBuffer(context_, CL_MEM_READ_WRITE, sizeof(cl_float2) * n_ * m_, nullptr, &err_);

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
        err_ = clEnqueueWriteBuffer(queue_, buffer_, CL_TRUE, 0, sizeof(cl_float2) * m_ * n_,
            input_data_->data(), 0, nullptr, nullptr);
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
    cl_mem inputBuffers[] = { buffer_ };
    err_ = clfftEnqueueTransform(plan, CLFFT_FORWARD, 1, &queue_, 0, nullptr, &event,
        &buffer_, inputBuffers, nullptr);
    if (err_ != CL_SUCCESS)  throw std::runtime_error("Launch (run) error FFT: " + std::to_string(err_));     // Ошибка запуска

    clFinish(queue_);
    system_clock::time_point t_end = system_clock::now();
    time_opencl_ = calc_Time_OpenCl(event);
    time_opencl_.set_time_average(time_average_value(&t_start, &t_end), n_, m_);
    
    std::type_index current_type = t.type();

    // Если тип не совпадает с последним, выбираем новый обработчик
    if (current_type != last_type_) {
        freturn_data_ = getHandlerForType(current_type);
        last_type_ = current_type;
    }

    // Вызываем обработчик
    if (freturn_data_) {
        freturn_data_(t);
    }
    else {
        // По умолчанию вызываем обработчик для calc_time_opencl
        return_calc_time_opencl(t);
    }
    return;
}

void wrapper::cl_fft_base::set_params(std::unordered_map<std::string, std::any> dict)
{
    std::vector<std::string> keys;
    keys.reserve(dict.size()); // для эффективности

    for (const auto& key : dict | views::keys) 
        keys.push_back(key);
    // Выводим ключи
    for (const auto& key : keys) 
    {
        if (key == "print")
        {
            try {
                is_print_ = std::any_cast<bool>(dict["print"]);
                //double d = std::any_cast<double>(dict["double_value"]);
                //std::string s = std::any_cast<std::string>(dict["string_value"]);
            }
            catch (const std::bad_any_cast& e) {
                is_print_ = false;
            }
        }
        else if (key == "other_key")
        {
        }
    }

}

void wrapper::cl_fft_base::process_type(const std::type_info& type) {
    if (const auto it = typeActions_.find(std::type_index(type)); it != typeActions_.end()) 
        it->second(); // Вызов соответствующей функции
    else
        throw std::runtime_error("Error return Class or Struct. ");
}

calc_time_opencl wrapper::cl_fft_base::calc_Time_OpenCl(cl_event event)
{
    cl_ulong start = 0, end = 0;
    cl_ulong queued = 0, submit = 0;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(start), &start, nullptr);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(end), &end, nullptr);

    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &queued, nullptr);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &submit, nullptr);

    double queueTime = (submit - queued) * 1e-3;    // Время в очереди (мkс)
    double submitTime = (start - submit) * 1e-3;   // Время ожидания перед запуском (мkс)
    double elapsedMs = (end - start) * 1e-3;

    if (is_print_ == true)
    {
        printf("Queue time (mks) %f (mks)\n", queueTime);                     // Время в очереди (мkс)
        printf("Waiting time before launch (mks): %f (mks)\n", submitTime);      // Время ожидания перед запуском (мkс)
        std::cout << "Direct execution time FFT: " << elapsedMs << " (mks)\n";     // Время выполнения прямого
    }

    return { queueTime, submitTime, elapsedMs };
}

wrapper::cl_fft_base::~cl_fft_base()
{
    // clear params OpenCL 
    clReleaseMemObject(buffer_);
    clReleaseCommandQueue(queue_);
    clReleaseContext(context_);
}

string wrapper::cl_fft_base::time_average_value(const system_clock::time_point* t_start, const system_clock::time_point* t_end)
{
    //std::this_thread::sleep_for(milliseconds(1000));

    const system_clock::time_point start_ = *t_start;
    system_clock::time_point end_ = *t_end;

    const auto duration = end_ - start_;
    const auto half_duration = duration / 2;
    const system_clock::time_point med_ = start_ + half_duration;

    const std::time_t med_time_t = system_clock::to_time_t(med_);

    std::tm tm;
    localtime_s(&tm, &med_time_t);  // ���������� ������ localtime

    auto ms = duration_cast<milliseconds>(med_.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    std::string datetime_str = oss.str();

    //    std::cout << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    //    std::cout << '.' << std::setfill('0') << std::setw(3) << ms.count() << std::endl;

    return datetime_str;
}

// считываем название метода
wrapper::cl_fft_base::FuncType wrapper::cl_fft_base::getHandlerForType(const std::type_index& type) {
    if (const auto it = handlers_.find(type); it != handlers_.end())  return it->second;
    return nullptr;         // Нет обработчика для данного типа
}

// Примеры реализаций обработчиков
void wrapper::cl_fft_base::return_calc_time_opencl(std::any& t) {    // Реализация обработки calc_time_opencl
    t = time_opencl_;
}

// Реализация обработки data_fft
void wrapper::cl_fft_base::return_data_fft(std::any& t){
    t = convert_data();
}

void wrapper::cl_fft_base::return_fft_data_time(std::any& t) {
    // Реализация обработки fft_data_time
    fft_data_time _fft_data_time;
    _fft_data_time.calc_time_opencl_ = time_opencl_;
    _fft_data_time.data_fft_ = convert_data();
    t = _fft_data_time;
}

data_fft wrapper::cl_fft_base::convert_data()
{
    data_fft data_fft_;
    // 9. Считываем результат
    std::vector<cl_float2> outputData(n_ * m_);
    err_ = clEnqueueReadBuffer(queue_, buffer_, CL_TRUE, 0, sizeof(cl_float2) * n_ * m_,
        outputData.data(), 0, nullptr, nullptr);
    if (err_ != CL_SUCCESS)
        throw std::runtime_error("Buffer read error: " + std::to_string(err_));

    if (m_ == 1) 
    {
        auto amplitudes = std::make_shared<std::vector<float>>(n_);
#pragma omp parallel for
        for (size_t n = 0; n < n_; ++n) {
            const float real = outputData[n].x;
            const float image = outputData[n].y;
            (*amplitudes)[n] = std::sqrt(real * real + image * image);
        }
        data_fft_.data_one_am = std::move(amplitudes);
        return data_fft_;
    }
    else {
        using v_fft_many_am = std::vector<std::vector<float>>;  // NOLINT(clang-diagnostic-shadow)
        auto amplitudes = std::make_shared<v_fft_many_am>(m_, std::vector<float>(n_));
#pragma omp parallel for
        for (size_t m = 0; m < m_; ++m) {
            for (size_t n = 0; n < n_; ++n) {
                const float real = outputData[m * n_ + n].x;
                const float image = outputData[m * n_ + n].y;
                (*amplitudes)[m][n] = std::sqrt(real * real + image * image);
            }
        }
        data_fft_.data_many_am = std::move(amplitudes);
        return data_fft_;
    }
    return data_fft_;
}



