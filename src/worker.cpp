#include "../include/worker.hpp"



Worker::Worker (int id,TaskQueue& queue_,PriorityTaskQueue& priority_queue_): queue_(queue_),priority_queue_(priority_queue_),id_(id)
{
    thread_ =std::thread(&Worker::run,this);
}

Worker::~Worker()
{
    if(thread_.joinable())
    {
        thread_.join();
    }
}



void Worker::run()
{
    while(true)
    {   
        while (!should_stop_.load())
        {
            auto task =priority_queue_.try_pop();
            if(!task)
                break;

            busy_ = true;
            (*task)();
            busy_ =false;
        }
        if(should_stop_.load())
        {
            break;
        }
        auto task =queue_.pop(&should_stop_);
        if(!task || should_stop_.load())
        {
            break;
        }    
        
        busy_ = true;
        (*task)();
        busy_ = false;

    }
}

bool Worker::is_busy() const
{
    return busy_;
}

int Worker::id() const
{
    return id_;
}