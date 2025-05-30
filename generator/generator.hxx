#pragma once

#include "common.hxx"

#include <variant>


template <typename _ValueType>
struct generator
{
    using value_type = _ValueType;

    struct promise_type
    {
        static constexpr std::size_t Empty = 0;
        static constexpr std::size_t Value = 1;
        static constexpr std::size_t Exception = 2;

        using handle = std::coroutine_handle<promise_type>;

        std::variant<std::monostate, value_type, std::exception_ptr> value;

        generator get_return_object()
        {
            Verbose("generator::promise_type::get_return_object()");

            return generator(std::coroutine_handle<promise_type>::from_promise(*this));
        }

        std::suspend_always initial_suspend() noexcept
        {
            Verbose("generator::promise_type::initial_suspend()");

            return {};
        }

        std::suspend_always final_suspend() noexcept
        {
            Verbose("generator::promise_type::final_suspend()");

            return {};
        }

        void return_void()
        {
            Verbose("generator::promise_type::return_void()");

            value.template emplace<Empty>();
        }

        std::suspend_always yield_value(value_type&& v)
        {
            Verbose("generator::promise_type::yield_value()");

            value.template emplace<Value>(std::move(v));
            return {};
        }

        void unhandled_exception()
        {
            Verbose("generator::promise_type::unhandled_exception()");

            value.template emplace<Exception>(std::current_exception());
        }
    };

    using handle = std::coroutine_handle<promise_type>;

    ~generator()
    {
        Verbose("generator::~generator()");
        
        if (handle_)
            handle_.destroy();
    }

    generator()
        : handle_(nullptr)
    {
        Verbose("generator::generator()");
    }

    generator(handle h)
        : handle_(h)
    {
        Verbose("generator::generator({})", fmt::ptr(h.address()));
    }

    generator(const generator&) = delete;
    generator& operator=(const generator&) = delete;

    generator(generator&& o) noexcept
        : generator()
    {
        Verbose("generator::generator(generator&&)");
        swap(o);
    }

    generator& operator=(generator&& o) noexcept
    {
        Verbose("generator::operator=(generator&&)");
        generator tmp(std::move(o));
        swap(tmp);
        return *this;
    }

    void swap(generator& o) noexcept
    {
        using std::swap;
        swap(handle_, o.handle_);
    }

    value_type* next()
    {
        Verbose("generator::next()");
        
        if (handle_.done())
            return nullptr;

        handle_.resume();

        if (auto err = std::get_if<promise_type::Exception>(&handle_.promise().value); err != nullptr)
        {
            std::rethrow_exception(*err);
        }

        return std::get_if<value_type>(&handle_.promise().value);
    }

    struct iterator
    {
        using difference_type = std::ptrdiff_t;
        using value_type = _ValueType;
        using reference_type = std::add_lvalue_reference_t<value_type>;

        [[nodiscard]] bool operator==(const iterator& other) const noexcept
        {
            if (owner_ != other.owner_)
                return false;

            if (!owner_)
                return true; // both are at end()

            return false;
        }

        iterator& operator++()
        {
            if (!owner_)
                return *this;

            value_ = owner_->next();
            fetched_ = true;
            if (!value_)
                owner_ = nullptr; // at end()

            return *this;
        }

        reference_type operator*() const
        {
            assert(owner_);

            if (!fetched_)
            {
                value_ = owner_->next();
                fetched_ = true;
            }

            assert(value_);
            return *value_;
        }

    private:
        friend struct generator;

        constexpr explicit iterator(generator* owner = nullptr) noexcept
            : owner_(owner)
        {
        }

        mutable generator* owner_;
        mutable value_type* value_ = nullptr;
        mutable bool fetched_ = false;
    };

    iterator begin() noexcept
    {
        return iterator{ this };
    }

    constexpr iterator end() noexcept
    {
        return iterator{ nullptr };
    }

private:
    handle handle_;
};


template <typename _Container>
auto make_generator_from(_Container&& container) -> generator<std::decay_t<typename _Container::value_type>>
{
    VerboseBlock("make_generator_from()");

    for (auto& value : container)
    {
        VerboseBlock("co_yield [{}]", value);

        co_yield std::move(value); // same as co_await generator::promise_type::yield_value(std::move(value))
    }

    Verbose("end of source");
    co_return;
}



