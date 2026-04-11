#pragma once

#include <atomic>
#include <memory>

namespace cortex
{
    class CancellationToken
    {
        private:
            std::shared_ptr<std::atomic<bool>> cancelled_;
        
        public:

            CancellationToken()
                :cancelled_(std::make_shared<std::atomic<bool>>(false))
                        {}
            
            void cancel() noexcept
            {
                cancelled_->store(true,std::memory_order_release);
            }
            
            bool is_cancelled() const noexcept
            {
                return cancelled_->load(std::memory_order_acquire);
            }
    };
}