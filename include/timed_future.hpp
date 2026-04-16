#pragma once

#include <thread>
#include <type_traits>
#include <future>
#include <chrono>
#include <optional>
#include <utility>


namespace cortex
{
    template<typename T>

    class TimedFuture
    {
        private:
            std::future<T> future_;
        
        public:
            TimedFuture() = default;
            explicit TimedFuture(std::future<T> f): future_(std::move(f))
                {}
                
            TimedFuture(TimedFuture&&) noexcept = default;
            TimedFuture& operator=(TimedFuture&&) noexcept = default;
            
            TimedFuture(const TimedFuture&) = delete;
            TimedFuture& operator=(const TimedFuture&) = delete;


            T get()
            {
                if (!future_.valid())
                {
                    throw std::future_error(std::future_errc::no_state);
                }
                return future_.get();
            }

            template<typename Rep,typename Period>
            std::optional<T> get_with_timeout(
                std::chrono::duration<Rep,Period> timeout)
            {
                if (!future_.valid())
                {
                    throw std::future_error(std::future_errc::no_state);
                }

                auto status = future_.wait_for(timeout);
                if (status == std::future_status::ready)
                {
                    return future_.get();
                }

                return std::nullopt;
            }  
            
                  
             template<typename F>
    auto then(F&& func) 
        -> TimedFuture<std::invoke_result_t<F, T>> 
    {
        using NextType = std::invoke_result_t<F, T>;
        auto promise = std::make_shared<std::promise<NextType>>();
        std::future<NextType> next_future = promise->get_future();

        
        auto current_future = std::move(future_);

        
        std::thread([curr = std::move(current_future), 
                     f = std::forward<F>(func), 
                     prom = std::move(promise)]() mutable 
                     {
            try 
            {
                if constexpr (std::is_void_v<T>) {
                    curr.get(); // Wait for void task
                    if constexpr (std::is_void_v<NextType>) 
                    {
                        f(); prom->set_value();
                    } 
                    else 
                    {
                        prom->set_value(f());
                    }
                } 
                else 
                {
                    auto val = curr.get(); 
                    if constexpr (std::is_void_v<NextType>) 
                    {
                        f(std::move(val)); prom->set_value();
                    } 
                    else 
                    {
                        prom->set_value(f(std::move(val)));
                    }
                }
            } 
            catch (...) 
            {
                prom->set_exception(std::current_exception());
            }
        }).detach();

        return TimedFuture<NextType>(std::move(next_future));
    }

            bool is_ready() const
            {
                if(!future_.valid())
                    return false;

                return future_.wait_for(std::chrono::seconds(0))
                    == std::future_status::ready;    
            }

            bool valid() const noexcept
            {
                return future_.valid();
            }

            explicit operator bool() const noexcept
            {
                return valid();
            }
                   

    };
}