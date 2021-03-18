// Threadsafe blocking queue
#pragma once

#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <iostream>

template <typename T>
class threadsafe_queue{
    public:
        threadsafe_queue() = default;

        threadsafe_queue(const threadsafe_queue& other){
            std::lock_guard lg(mtx);
            data = other.data;
        }

        threadsafe_queue(threadsafe_queue&& other){
           std::lock_guard lg(other.mtx);
           data = std::move(other.data);
        }

        threadsafe_queue& operator=(const threadsafe_queue& other){
            if(this == &other) return *this;
            std::scoped_lock(mtx, other.mtx);
            data = other.data;
            return *this;
        }

        threadsafe_queue& operator=(threadsafe_queue&& other){
            if(this == &other) return *this;
            std::scoped_lock(mtx, other.mtx);
            data = std::move(other.data);
            return *this;
        }

        void push(T val){
            std::lock_guard lg(mtx);
            data.push(val);
            condition.notify_one();
        }

        void waitAndPop(T& val){
            std::unique_lock ul(mtx);
            condition.wait(ul,
                [this]{return !data.empty();}
            );
            val = data.front();
            data.pop();
        }

        bool tryPop(T& val){
            std::unique_lock ul(mtx, std::defer_lock);
            if(ul.try_lock() && !data.empty()){
                val = data.front();
                data.pop();
                return true;
            }
            return false;
        }

        bool empty() const{
            std::lock_guard lg(mtx);
            return data.empty();
        }

        std::shared_ptr<T> tryPop(){
            std::unique_lock ul(mtx, std::defer_lock);
            if(ul.try_lock() && !data.empty()){
                std::shared_ptr<T> val{std::make_shared<T>(data.front())};
                data.pop();
                return val;
            }
            return std::shared_ptr<T>();
        }

        std::shared_ptr<T> waitAndPop(){
            std::unique_lock ul(mtx);
            condition.wait(ul,
                [this]{return !data.empty();}
            );
            std::shared_ptr<T> val{std::make_shared<T>(data.front())};
            data.pop();
            return val;
        }

    private:
        mutable std::mutex mtx;
        std::queue<T> data;
        std::condition_variable condition;
};