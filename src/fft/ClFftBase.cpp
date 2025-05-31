#include "ClFftBase.h"

namespace std
{
    class type_index;
}

my_fft::ClFftBase::ClFftBase()
{
/*
    // 1. Получаем платформу
    err_ = clGetPlatformIDs(1, &platform_, nullptr);
    if (err_ != CL_SUCCESS) throw std::runtime_error("Failed to get platform");

    err_ = clGetDeviceIDs(platform_, CL_DEVICE_TYPE_GPU, 1, &device_, nullptr);
    if (err_ != CL_SUCCESS) throw std::runtime_error("Failed to get device");

    context_ = clCreateContext(nullptr, 1, &device_, nullptr, nullptr, &err_);
    if (err_ != CL_SUCCESS) throw std::runtime_error("Failed to create context");

    constexpr cl_queue_properties props[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, CL_QUEUE_ON_DEVICE, 0 };

    queue_ = clCreateCommandQueueWithProperties(context_, device_, props, &err_);
    if (err_ != CL_SUCCESS) throw std::runtime_error("Failed to create queue");

    // 2. Получаем устройство GPU
    clfftSetupData fft_setup;
    err_ = clfftInitSetupData(&fft_setup);
    if (err_ != CL_SUCCESS) throw std::runtime_error("clFFT setup init failed");

    err_ = clfftSetup(&fft_setup);
    if (err_ != CL_SUCCESS) throw std::runtime_error("clFFT setup failed");

    typeActions_ = {
            { std::type_index(typeid(calc_time_opencl)), [this]() { s_data_times_.is_data = false;  s_data_times_.is_time = true;  } },
            { std::type_index(typeid(data_fft)),       [this]() { s_data_times_.is_data = true;  s_data_times_.is_time = false;  } },
            { std::type_index(typeid(fft_data_time)),  [this]() { s_data_times_.is_data = true;  s_data_times_.is_time = true; } }
    };
*/
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
        constexpr cl_queue_properties props[] = {
            CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE,
            0
        };

        queue_ = clCreateCommandQueueWithProperties(context_, device_, props, &err_);
        if (err_ != CL_SUCCESS) {
            throw std::runtime_error("Failed to create command queue");
        }

        // 5. Инициализация clFFT
        clfftSetupData fft_setup;
        err_ = clfftInitSetupData(&fft_setup);
        if (err_ != CL_SUCCESS) {
            throw std::runtime_error("clFFT setup init failed");
        }

        err_ = clfftSetup(&fft_setup);
        if (err_ != CL_SUCCESS) {
            throw std::runtime_error("clFFT setup failed");
        }

        // 6. Инициализируем map с лямбда-функциями
        typeActions_ = {
            { std::type_index(typeid(calc_time_opencl)), [this]() { s_data_times_.is_data = false; s_data_times_.is_time = true; } },
            { std::type_index(typeid(data_fft)),        [this]() { s_data_times_.is_data = true;  s_data_times_.is_time = false; } },
            { std::type_index(typeid(fft_data_time)),   [this]() { s_data_times_.is_data = true;  s_data_times_.is_time = true;  } }
        };
    }


void my_fft::ClFftBase::Calculate(std::unique_ptr<std::any> data, std::any& t, size_t n, size_t m)
{
    // Размер FFT
//    const size_t N = 1 << 10;
    n_ = n;
    m_ = m;

    std::cerr << "number of points " << n_ << "\n"; // кол-во точек

    // 5. Создаем буферы для данных (комплексные числа: float2)
    std::vector<cl_float2>* inputData;  // Реальная и мнимая части
    //    outputData = std::vector<cl_float2>( N);
    // Заполняем входные данные (например, синусоидальный сигнал)

    //const float freq = 5 * 2 * pi / N;
    //for (size_t i = 0; i < N; ++i) {
    //    inputData[i].x = std::sin(freq * i);
    //    inputData[i].y = 0.0f;
    //}

//    if (data.type() == typeid(calc_Time_OpenCl)) { int iii1 = 1; }

//    if (data->type() == typeid(v_fft)) {
    const v_fft& vec_ref = std::any_cast<const v_fft&>(*data);
    inputData = const_cast<v_fft*>(&vec_ref); // если нужен неконстант

    //std::vector<cl_float2> inputData1 = data
    

//    cl_mem inputBuffer = clCreateBuffer(context_, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float2) * inputData.size(), inputData.data(), &err_);
    cl_mem inputBuffer = clCreateBuffer(context_, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float2) * n_*m_, inputData->data(), &err_);
    cl_mem outputBuffer = clCreateBuffer(context_, CL_MEM_READ_WRITE, sizeof(cl_float2) * n_ * m_, nullptr, &err_);

    if (err_ != CL_SUCCESS)
    {
        std::cerr << "Error creating buffers: " << err_ << "\n"; // Ошибка создания буферов
        clReleaseCommandQueue(queue_);
        clReleaseContext(context_);
        return;
    }

    // 6. Инициализация clFFT
    clfftSetupData fftSetup;
    //    err = clfftInitSetupData(nullptr);
    err_ = clfftInitSetupData(&fftSetup);
    if (err_ != CL_SUCCESS)
    {
        std::cerr << "Initialization error clFFT\n";    //  Ошибка инициализации
        return;
    }
    err_ = clfftSetup(nullptr);
    if (err_ != CL_SUCCESS)
    {
        std::cerr << "Installation error clFFT\n";        //  Ошибка установк
        return;
    }

    if(m_ > 1 )
    {
        err_ = clEnqueueWriteBuffer(queue_, inputBuffer, CL_TRUE, 0, sizeof(cl_float2) * m_ * n_,
            inputData->data(), 0, nullptr, nullptr);
        if (err_ != CL_SUCCESS) throw std::runtime_error("Failed to write buffer");
    }

    // 7. Создаем план FFT
    clfftPlanHandle plan;
    err_ = clfftCreateDefaultPlan(&plan, context_, CLFFT_1D, &n_);
    if (err_ != CL_SUCCESS) {
        std::cerr << "Error creating plan FFT\n";         // Ошибка создания плана
        return;
    }
    if (m_ == 1)
    {
        // Настраиваем план: комплексный ввод и вывод, прямое преобразование
        err_ = clfftSetPlanPrecision(plan, CLFFT_SINGLE);
        err_ |= clfftSetLayout(plan, CLFFT_COMPLEX_INTERLEAVED, CLFFT_COMPLEX_INTERLEAVED);
        err_ |= clfftSetResultLocation(plan, CLFFT_OUTOFPLACE);
        if (err_ != CL_SUCCESS) {
            std::cerr << "Error setting up plan FFT\n";        //  Ошибка настройки плана
            return;
        }
    } else
    {
        err_ = clfftSetPlanBatchSize(plan, m_);
        err_ |= clfftSetPlanPrecision(plan, CLFFT_SINGLE);
        err_ |= clfftSetLayout(plan, CLFFT_COMPLEX_INTERLEAVED, CLFFT_COMPLEX_INTERLEAVED);
        err_ |= clfftSetResultLocation(plan, CLFFT_INPLACE);
        if (err_ != CL_SUCCESS) throw std::runtime_error("Failed to set plan parameters");
    }

    // Запекаем план
    err_ = clfftBakePlan(plan, 1, &queue_, nullptr, nullptr);
    if (err_ != CL_SUCCESS) {
        std::cerr << "Error baking plan FFT\n";        // Ошибка запекания плана
        return ;
    }

    // 8. Выполняем прямое FFT с профилированием
    cl_event event;
    err_ = clfftEnqueueTransform(plan, CLFFT_FORWARD, 1, &queue_, 0, nullptr, &event,
        &inputBuffer, &outputBuffer, nullptr);
    if (err_ != CL_SUCCESS) {
        std::cerr << "Launch (run) error FFT\n";        // Ошибка запуска
        return;
    }

    clFinish(queue_);

    // Получаем время выполнения
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

    // 9. Считываем результат
    std::vector<cl_float2> outputData = v_fft(n_*m_);

    err_ = clEnqueueReadBuffer(queue_, inputBuffer, CL_TRUE, 0, sizeof(cl_float2) * n_ * m_,
        outputData.data(), 0, nullptr, nullptr);
    if (err_ != CL_SUCCESS) {
        std::cerr << "Buffer read error \n";  //  Ошибка чтения буфера
        return ;
    }

    if(m_==1)
    {
#pragma omp parallel for
        std::vector<float> amplitudes(n_);
        for (size_t n = 0; n < n_; ++n) {
            float real = outputData[m * n_ + n].x;
            float imag = outputData[m * n_ + n].y;
            amplitudes[n] = std::sqrt(real * real + imag * imag);
        }
    } else
    {
#pragma omp parallel for
        std::vector<std::vector<float>> amplitudes(m_, std::vector<float>(n_));
        for (size_t m = 0; m < m_; ++m) {
            for (size_t n = 0; n < n_; ++n) {
                const float real = outputData[m * n_ + n].x;
                const float imag = outputData[m * n_ + n].y;
                amplitudes[m][n] = std::sqrt(real * real + imag * imag);
            }
        }
    }

    int kkk = 1;




/*
    //std::unique_ptr<std::vector<cl_float2>> data_ = std::make_unique<std::vector<cl_float2>>(N);

//    std::vector<cl_float2>* inputData = new std::vector<cl_float2>(N);
    // Заполнение сигнала, например, синусоидой
    v_fft data =  v_fft (N); // Обычный вектор, не unique_ptr

    // 2. Заполняем вектор
    const float freq = 5 * 2 * pi / N;
    for (size_t i = 0; i < N; ++i) {
        (data)[i].x = std::sin(freq * i);
        (data)[i].y = 0.0f;
    }
    input_data_one_ =  v_fft(n_);
    n_ = n;
    m_ = m;
//    data_ = std::move(e_data);

    if (m_ < 1)
    {
        throw std::runtime_error("Failed to bake M < 1 ");
    }
    else if (m_ == 1)
    {
        //try 
        //{
        //    input_data_one_ = std::any_cast<const v_fft>(*std::move(data));
        //}
        //catch (const std::bad_any_cast& e) {
        //    throw std::runtime_error(std::string("Bad any_cast: ") + e.what());
        //}

//        input_data_one_ = new v_fft(n_);
        buffer_ = clCreateBuffer(context_, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float2) * n_,  data.data() , &err_); // input_data_one_->data()

        if (err_ != CL_SUCCESS) throw std::runtime_error("Failed to create buffer");

        err_ = clEnqueueWriteBuffer(queue_, buffer_, CL_TRUE, 0, sizeof(cl_float2) * n_, input_data_one_.data(),
            0, nullptr, nullptr);
        if (err_ != CL_SUCCESS) throw std::runtime_error("Failed to write buffer");

    }
    else
    {
        //v_fft_many vec_many_ ;// = std::any_cast<const v_fft_many>(*data_);
        //try
        //{
        //    // Создаём shared_ptr на копию вектора
        //    vec_many_ = std::any_cast<const v_fft_many>(std::any_cast<const v_fft_many&>(*data));
        //    // Теперь d_vec_one хранит копию данных из any
        //}
        //catch (const std::bad_any_cast& e) {
        //    throw std::runtime_error(std::string("Bad any_cast: ") + e.what());
        //}

    }
*/
}

void my_fft::ClFftBase::process_type(const std::type_info& type) {
    if (const auto it = typeActions_.find(std::type_index(type)); it != typeActions_.end()) 
        it->second(); // Вызов соответствующей функции
    else
        throw std::runtime_error("Error return Class or Struct. ");
}

calc_time_opencl my_fft::ClFftBase::calc_Time_OpenCl(cl_event event)
{
    cl_ulong start = 0, end = 0;
    cl_ulong queued = 0, submit = 0;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(start), &start, nullptr);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(end), &end, nullptr);

    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &queued, nullptr);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &submit, nullptr);

    const double queueTime = (submit - queued) * 1e-3;    // ����� � ������� (���)
    const double submitTime = (start - submit) * 1e-3;   // ����� �������� ����� �������� (���)
    double elapsedMs = (end - start) * 1e-3;

    printf(" - FFT count N = %zu point \n", n_);
    printf("Time waite in queue (mks): %f mks\n", queueTime);
    printf("Time waite load in queue (mks): %f mks\n", submitTime);
    std::cout << "Time calc FFT mks: " << elapsedMs << " ���\n";

    return { queueTime, submitTime, elapsedMs };
}

void my_fft::ClFftBase::read_data(std::any& t)
{
    process_type(t.type());
    system_clock::time_point t_start = system_clock::now();
///*
//    if (m_ == 1)
//    {
//        // 8. Выполняем прямое FFT с профилированием
//        err_ = clfftEnqueueTransform(plan_, CLFFT_FORWARD, 1, &queue_, 0, nullptr, &event,
//            &inputBuffer, &outputBuffer, nullptr);
//        if (err_ != CL_SUCCESS) throw std::runtime_error("Failed to enqueue FFT");
//
//        clFinish(queue_);
//
//        // Получаем время выполнения
//        //v_fft vec_one_ = v_fft(n_);
//        err_ = clEnqueueReadBuffer(queue_, buffer_, CL_TRUE, 0, sizeof(cl_float2) * n_, input_data_one_.data(),
//            0, nullptr, nullptr);
//        if (err_ != CL_SUCCESS) throw std::runtime_error("Failed to read buffer");
//    }
//    else
//    {
//
//    }
//*/
//    // 8. Выполняем прямое FFT с профилированием
//    cl_event event;
//    err_ = clfftEnqueueTransform(plan_, CLFFT_FORWARD, 1, &queue_, 0, nullptr, &event,
//        &inputBuffer, &outputBuffer, nullptr);
//    if (err_ != CL_SUCCESS) { std::cerr << "Ошибка запуска FFT\n"; return; }
//
//    clFinish(queue_);
//    // �������� ����� ����������
//    calc_time_opencl  time_opencl;
//
//    // 9. Считываем результат
//    if (m_ == 1)
//    {
////        v_fft vec_one_ = v_fft(n_);
//        err_ = clEnqueueReadBuffer(queue_, outputBuffer, CL_TRUE, 0, sizeof(float) * outputData.size(),
//            outputData.data(), 0, nullptr, nullptr);
//        if (err_ != CL_SUCCESS) {
//            std::cerr << "������ ������ ������\n";
//            return;
//        }
//        system_clock::time_point t_end = system_clock::now();
//
//        time_opencl = calc_Time_OpenCl(event);
//        time_opencl.set_time_average(time_average_value(&t_start, &t_end));
//
//        if (s_data_times_.is_time && (!s_data_times_.is_data))
//        {
//            t = time_opencl;
//            return; 
//        }
//
//        if (s_data_times_.is_data && (!s_data_times_.is_time))
//        {
//            data_fft data_fft_ = data_fft();
//            if(m_ == 1)
//            {
//                data_fft_.data_one = std::make_shared<v_fft>(input_data_one_);
//                t = data_fft_;
//                return;
//            } else
//            {
//                //            std::unique_ptr<v_fft> data_one = nullptr;         std::unique_ptr<v_fft_many> data_many = nullptr;
//                return;
//            }
//        }
//
//        if (s_data_times_.is_data && s_data_times_.is_time)
//        {
//            fft_data_time _fft_data_time = fft_data_time();
//            _fft_data_time.calc_time_opencl_ = time_opencl;
//            _fft_data_time.data_fft_ = data_fft();
//            _fft_data_time.data_fft_.data_one = std::make_shared<v_fft>(input_data_one_);
//
//            t = _fft_data_time;
//            return;
//        }
//    }
}


my_fft::ClFftBase::~ClFftBase()
{
    // 9. ����������� OpenCL �������
    clReleaseMemObject(buffer_);
    clReleaseCommandQueue(queue_);
    clReleaseContext(context_);

}

string my_fft::ClFftBase::time_average_value(system_clock::time_point* t_start, system_clock::time_point* t_end)
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


void my_fft::ClFftBase::print_data_test(data_fft data)
{
}

void my_fft::ClFftBase::print_data_test(fft_data_time data)
{
}

void my_fft::ClFftBase::print_data_test(calc_time_opencl times)
{
}

