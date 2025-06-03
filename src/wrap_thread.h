#pragma once

#include <any>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <iostream>
#include <atomic>
#include <ranges>

#include <boost/thread.hpp>
#include <boost/utility.hpp> // для boost::noncopyable
#include <utility>

class wrap_thread : private boost::noncopyable 
{
public:
    explicit wrap_thread(const int id)
    {
        id_ = id;
        is_repeat_ = false;
        count_pause_ = 500;
    }

    // Конструктор, принимающий функцию и аргументы
    template <typename Fn, typename... Args>
    explicit wrap_thread(const int id, Fn&& f, Args&&... args)
        : worker_thread_(std::forward<Fn>(f), std::forward<Args>(args)...)
    {
        id_ = id;
        is_repeat_ = false;
        count_pause_ = 500;
    }
    virtual ~wrap_thread();
    virtual void start();
    virtual void stop();
    virtual void pause();
    virtual void resume();
    virtual void thread_func();
    virtual void repeat_continue(bool b){}

    void thread_join()
    {
        worker_thread_.join();
    }

    boost::thread& get() {
        return worker_thread_;
    }
    bool is_thread() const
    {
        return  worker_thread_.joinable();
    }
    
    virtual void set_params(std::unordered_map<std::string, std::any> dict);

    virtual bool logic_while();
    virtual void my_func(){}
    int id_ = -1;


protected:

    boost::thread worker_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> paused_{ false };
    boost::mutex pause_mutex_;
    boost::condition_variable pause_cond_;
    std::atomic<bool> is_repeat_;
    std::atomic<int> count_pause_;

//    boost::thread thread_;
};


inline wrap_thread::~wrap_thread()
{
    wrap_thread::stop();
}

bool inline wrap_thread::logic_while()
{
//    std::cout << " fun ID logic "<< id_ << "  "<< is_repeat_ <<"\n" << std::endl;

    return true;
}

inline void wrap_thread::start()
{
    if (running_) return; // уже запущен
    running_ = true;
    worker_thread_ = boost::thread(&wrap_thread::thread_func, this);
};

inline void wrap_thread::stop()
{
    if (!running_) return;
    is_repeat_ = false;
    running_ = false;
    worker_thread_.interrupt(); // прервать поток
    if (worker_thread_.joinable())
        worker_thread_.join();
}

inline void wrap_thread::pause()
{
    paused_ = true;
}

inline void wrap_thread::resume()
{
    paused_ = false;
    pause_cond_.notify_one();
}

inline void wrap_thread::thread_func()
{
    try {
        while (is_repeat_? wrap_thread::logic_while(): logic_while())
        {
//            std::cout << " --- ID logic " << id_ <<"  "<< is_repeat_ << "\n" << std::endl;
            std::cout << "Working... " << id_ << std::endl;
            my_func();
            //// Проверка на паузу
            //boost::unique_lock<boost::mutex> lock(pause_mutex_);
            //while (paused_) {
            //    pause_cond_.wait(lock);
            //    std::cout << "!!!!!! Working... next PAUSE " << std::endl;

            //}
            //lock.unlock();

            // Основная работа потока

            // Спать 500 мс, с возможностью прерывания
            boost::this_thread::sleep_for(boost::chrono::milliseconds(count_pause_));
        }
        int uuu = 1;
    }
    catch (boost::thread_interrupted&) {
        std::cout << "Thread interrupted, exiting..." << std::endl;
    }
}


inline void wrap_thread::set_params(std::unordered_map<std::string, std::any> dict)
{
    std::vector<std::string> keys;
    keys.reserve(dict.size()); // для эффективности

    for (const auto& key : dict | std::views::keys)
        keys.push_back(key);
    // Выводим ключи
    for (const auto& key : keys)
    {
        if (key == "is_repeat")
        {
            try { is_repeat_ = std::any_cast<bool>(dict["is_repeat"]); }
            catch (const std::bad_any_cast& e) { is_repeat_ = false; }
        }
        else if (key == "count_pause")
        {
            try { count_pause_ = std::any_cast<int>(dict["count_pause"]); }
            catch (const std::bad_any_cast& e) { count_pause_ = 500;  }
        }
    }

}


///////////////////////////////////////
/*  !! пример!!!

class wrap_thread : private boost::noncopyable
{
    virtual bool logic_while();
    virtual void my_func(){};
    virtual void thread_func();  функция которая вызывается в пооке

    std::atomic<bool> is_repeat_;
}
bool inline wrap_thread::logic_while()
{
    return is_repeat_;
}
inline void wrap_thread::thread_func()
{
    try {
        while (wrap_thread::logic_while())
        {
            wrap_thread::my_func();
        }
    }
    catch (boost::thread_interrupted&) {
        std::cout << "Thread interrupted, exiting..." << std::endl;
    }
}

class test_th_a: public wrap_thread
{
public:
    test_th_a(int id, int n): wrap_thread(id)
    {
        i = 0;
        this->n = n;
        is_ = true;
    }

    bool logic_while() override
    {
        return i < n;
    }
    void my_func() override
    {
        i++;
        printf("thread id: %zo \n", i);
        boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
    }
    int i = 0;
    int n;
    bool is_ = true;
};

из inline void wrap_thread::thread_func()
не вызывются пеоеопределенные функции
      logic_while()
       my_func();






 #include <boost/thread.hpp>
#include <boost/utility.hpp> // для boost::noncopyable
#include <utility>

class wrap_thread : private boost::noncopyable {
public:
    wrap_thread() {}

    // Конструктор, принимающий функцию и аргументы
    template <typename Fn, typename... Args>
    explicit wrap_thread(Fn&& f, Args&&... args)
        : thread_(std::forward<Fn>(f), std::forward<Args>(args)...) {}

    // Запуск потока с функцией (если нужно)
    template <typename Fn, typename... Args>
    void start(Fn&& f, Args&&... args) {
        thread_ = boost::thread(std::forward<Fn>(f), std::forward<Args>(args)...);
    }

    // Проверка, запущен ли поток
    bool joinable() const {
        return thread_.joinable();
    }

    // Ожидание завершения потока
    void join() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    // Прерывание потока (если поддерживается)
    void interrupt() {
        thread_.interrupt();
    }

    // Получить ссылку на внутренний поток (если нужно)
    boost::thread& get() {
        return thread_;
    }

private:
    boost::thread thread_;
};

#include <iostream>
#include <boost/thread.hpp>

void worker(int id) {
    std::cout << "Поток " << id << " запущен\n";
    boost::this_thread::sleep_for(boost::chrono::seconds(2));
    std::cout << "Поток " << id << " завершён\n";
}

int main() {
    wrap_thread t([]{ worker(1); });
    if (t.joinable()) {
        t.join();
    }
    return 0;
}


 
 */