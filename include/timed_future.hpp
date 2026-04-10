#pragma once

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