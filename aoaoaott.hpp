/*
 * Copyright (c) 2019-2021 Pavel I. Kryukov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef AO_AO_AO_TT
#define AO_AO_AO_TT

#include <boost/pfr.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/reverse_iterator.hpp>

#include <array>
#include <cassert>
#include <tuple>
#include <vector>

namespace aoaoaott {
  
namespace visitor {
    template <size_t I, typename T, typename F>
    static void visit_all_impl(T& tup, F fun)
    {
        fun(std::get<I>(tup));
        if constexpr (I > 0)
            visit_all_impl<I - 1>(tup, fun);
    }

    template <typename F, typename... Ts>
    void visit_all(std::tuple<Ts...> const& tup, F fun)
    {
        visit_all_impl<sizeof...(Ts) - 1>(tup, fun);
    }

    template <typename F, typename... Ts>
    void visit_all(std::tuple<Ts...>& tup, F fun)
    {
        visit_all_impl<sizeof...(Ts) - 1>(tup, fun);
    }
} // visitor

template<typename T>
class Traits
{
protected:
    static_assert(!std::is_empty<T>::value, "AoAoAoTT does not support empty structures");
    static_assert(std::is_trivially_copyable<T>::value, "AoAoAoTT supports only trivially copyable structures");
    static_assert(std::is_standard_layout<T>::value, "AoAoAoTT supports only standard layout structures");
};

template<typename Container>
class BaseFacade
{
protected:
    using T = typename Container::value_type;
public:
    auto aggregate() const noexcept { return base->aggregate(this->get_index()); }
    operator T() const noexcept { return aggregate(); }

    template<auto ptr, typename = std::enable_if_t<std::is_member_function_pointer_v<decltype(ptr)>>, typename ... Args>
    auto method(Args&& ... args) const // noexcept?
    {
        object_mover om(this);
        return (om.object.*ptr)(std::forward<Args>(args)...);
    }

    template<typename R, typename ... Args>
    constexpr auto operator->*(R (T::* fun)(Args ...)) const noexcept
    {
        return [this, fun](Args&& ... args) {
            object_mover om(this);
            return (om.object.*fun)(std::forward<Args>(args)...);
        };
    }

    template<typename R, typename ... Args>
    constexpr auto operator->*(R (T::* fun)(Args ...) const) const noexcept
    {
        return [this, fun](Args&& ... args) {
            object_mover om(this);
            return (om.object.*fun)(std::forward<Args>(args)...);
        };
    }

protected:
    constexpr BaseFacade(const Container* base, size_t index) : base(base), index(index) { }
    constexpr auto get_index() const noexcept { return index; }

    constexpr const auto* get_base() const noexcept { return base; }
    void inc_index() noexcept { ++index; }
    void dec_index() noexcept { --index; }
    void advance_index(ptrdiff_t n) noexcept { index += n; }

private:
    const Container* base;
    size_t index;
    struct object_mover
    {
        const BaseFacade* const facade;
        T object;
        explicit object_mover(const BaseFacade* facade) : facade(facade), object(facade->aggregate()) { }
        ~object_mover() { facade->get_base()->dissipate(std::move(object), facade->get_index()); }
    };
};

template<typename Container>
class ConstFacade : public BaseFacade<Container>
{
    using typename BaseFacade<Container>::T;
public:
    constexpr ConstFacade( const Container* b, size_t index) : BaseFacade<Container>(b, index) { }

    using BaseFacade<Container>::operator->*;

    template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
    constexpr const auto& get() const noexcept { return this->get_base()->get_member(ptr, this->get_index()); }

    template<typename R>
    constexpr const R& operator->*(R T::* field) const noexcept
    {
        return this->get_base()->get_member(field, this->get_index());
    }
};

template<typename Container>
class Facade : public BaseFacade<Container>
{
    using typename BaseFacade<Container>::T;
public:
    constexpr Facade( Container* b, size_t index) : BaseFacade<Container>(b, index) { }

    template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
    constexpr auto& get() const noexcept { return this->get_base()->get_member(ptr, this->get_index()); }

    using BaseFacade<Container>::operator->*;

    template<typename R>
    constexpr R& operator->*(R T::* field) const noexcept { return this->get_base()->get_member(field, this->get_index()); }

    void operator=(const T& rhs) const noexcept
    {
        static_assert(std::is_copy_assignable_v<T>, "Object cannot be assigned because its copy assignment operator is implicitly deleted");
        this->get_base()->dissipate(rhs, this->get_index());
    }

    void operator=(T&& rhs) const noexcept
    {
        static_assert(std::is_move_assignable_v<T>, "Object cannot be assigned because its move assignment operator is implicitly deleted");
        this->get_base()->dissipate(std::move(rhs), this->get_index());
    }
};

template<typename T, template <typename> class Container>
class AoSRandomAccessContainer : Traits<T>
{
    friend class BaseFacade<AoSRandomAccessContainer>;
    friend class Facade<AoSRandomAccessContainer>;
    friend class ConstFacade<AoSRandomAccessContainer>;

public:
    using value_type = T;
    auto size() const noexcept { return storage.size(); }
    bool empty() const noexcept { return storage.empty(); }

protected:
    T aggregate(size_t index) const noexcept { return storage[index]; }
    void dissipate(const T& rhs, size_t index) const noexcept { storage[index] = rhs; }
    void dissipate(T&& rhs, size_t index) const noexcept { storage[index] = std::move(rhs); }

    void replicate(const T& value, size_t start, size_t end)
    {
        for (size_t i = start; i < end; ++i)
            storage[i] = value;
    }

    template<typename R>
    constexpr R& get_member(R T::* member, size_t index) const noexcept
    {
        return storage[index].*member;
    }

    mutable Container<T> storage;
};

template<typename T, template <typename> class Container>
class SoARandomAccessContainer : Traits<T>
{
    template<typename... TT> struct type_list {};

    template<typename U> struct loophole_type_list;
    template<int... NN> struct loophole_type_list<std::integer_sequence<int, NN...>>
    {
        using type = type_list< std::remove_cv_t<boost::pfr::tuple_element_t<NN, T>>... >;
    };

    static const constexpr size_t tuple_size = boost::pfr::tuple_size_v<T>;
    using AsTypeList = typename loophole_type_list<std::make_integer_sequence<int, tuple_size>>::type;
    using Indices = std::make_index_sequence<tuple_size>;

    template<typename ... TT>
    static constexpr std::tuple<Container<TT>...> tupilzer(type_list<TT...>);

    using Storage = decltype(tupilzer(AsTypeList{}));

    template<typename ... TT>
    static constexpr size_t sizeof_list(type_list<TT...>)
    {
        size_t result = 0;
        for (auto e : { sizeof(TT) ... })
            result += e;
        return result;
    }

    template<typename ... TT>
    static constexpr bool check_bool(type_list<TT...>)
    {
	for (auto e : { std::is_same_v<TT, bool>... })
	    if (e)
	        return true;
	return false;
    }

    static_assert(sizeof(T) == sizeof_list(AsTypeList{}), "AoAoAoTT does not support structures with padding bytes");

    friend class BaseFacade<SoARandomAccessContainer>;
    friend class Facade<SoARandomAccessContainer>;
    friend class ConstFacade<SoARandomAccessContainer>;

public:
    using value_type = T;
    auto size() const noexcept { return std::get<0>(storage).size(); }
    bool empty() const noexcept { return std::get<0>(storage).empty(); }

protected:
    static constexpr bool has_bool() { return check_bool(AsTypeList{}); }

    T aggregate(size_t index) const noexcept { return aggregate(index, Indices{}); }
    void dissipate(const T& rhs, size_t index) const noexcept  { dissipate(rhs, index, Indices{}); }
    void dissipate(T&& rhs, size_t index) const noexcept { dissipate(std::move(rhs), index, Indices{}); }
    void replicate(const T& value, size_t start, size_t end) { replicate(value, start, end, Indices{}); }

    template<typename R>
    constexpr R& get_member(R T::* member, size_t index) const noexcept
    {
        return get_container(member)[index];
    }

    mutable Storage storage;

private:
    template<size_t ... N>
    void dissipate(const T& src, size_t index, std::index_sequence<N...>)
        const noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
    {
        std::tie(std::get<N>(storage)[index]...) = boost::pfr::structure_tie(src);
    }

    template<size_t ... N>
    T aggregate(size_t index, std::index_sequence<N...>)
        const noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
    {
        T result{};
        boost::pfr::structure_tie(result) = std::tie(std::get<N>(storage)[index]...);
        return result;
    }

    template<size_t ... N>
    void replicate(const T& src, size_t start, size_t end, std::index_sequence<N...>)
        noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
    {
        // Use comma operator to replicate the `replicate` function
        ((void)replicate_member<N>(src, start, end), ...);
    }

    template<size_t N>
    void replicate_member(const T& src, size_t start, size_t end)
    {
        const auto& value = boost::pfr::get<N>(src);
        for (size_t i = start; i < end; ++i)
            std::get<N>(storage)[i] = value;
    }

    template<typename R>
    auto& get_container(R T::* member) const noexcept
    {
        return *get_container_impl<tuple_size>(member);
    }

    template<size_t I, typename R>
    constexpr Container<std::remove_cv_t<R>>* get_container_impl(R T::* member) const noexcept
    {
        (void)member;
        if constexpr (I == 0)
            return nullptr;
        else if constexpr(!std::is_same_v<std::remove_cv_t<boost::pfr::tuple_element_t<I - 1, T>>, std::remove_cv_t<R>>)
            return get_container_impl<I - 1>(member);
        else if (member_to_index(member) == I - 1)
            return &std::get<I - 1>(storage);
        else
            return get_container_impl<I - 1>(member);
    }

    struct DelayConstruct {
        static const inline T value{};  // construct in runtime.  
    };

    // Taken from https://github.com/boostorg/pfr/issues/60 by Fuyutsubaki
    template<typename R>
    constexpr size_t member_to_index(R T::* member) const noexcept
    {
        const auto &t = DelayConstruct::value;
        return std::apply([&](const auto&... e) {
            size_t idx = 0;
            for (auto b : { static_cast<const void*>(&e) ... }) {
                if (b == &(t.*member))
                    return idx;
                idx += 1;
            }
            return tuple_size;
        }, boost::pfr::structure_tie(t));
    }
};

template<typename BaseContainer>
class RandomAccessContainer : public BaseContainer
{
public:
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = Facade<BaseContainer>;
    using const_reference = ConstFacade<BaseContainer>;

    constexpr auto operator[](size_t index) noexcept { return reference{ this, index}; }
    constexpr auto operator[](size_t index) const noexcept { return const_reference{ this, index}; }

    auto at(size_t index) { check_index(index); return operator[](index); }
    auto at(size_t index) const { check_index(index); return operator[](index); }

    class const_iterator : const_reference,
        public boost::iterator_facade<const_iterator, const_reference const, std::random_access_iterator_tag, const const_reference&>
    {
        friend class RandomAccessContainer;
        friend class boost::iterator_core_access;

        const_iterator(const BaseContainer* base, size_t index) : const_reference(base, index) { }

        const auto& dereference() const noexcept { return *this; }
        bool equal(const const_iterator& rhs) const noexcept { return this->get_index() == rhs.get_index(); }
        void increment() noexcept { this->inc_index(); }
        void decrement() noexcept { this->dec_index(); }
        void advance(size_t n) noexcept { this->advance_index( n); }
        ptrdiff_t distance_to(const const_iterator& rhs) const noexcept { return rhs.get_index() - this->get_index(); }
    };

    class iterator : reference,
        public boost::iterator_facade<iterator, reference, std::random_access_iterator_tag, const reference&>
    {
        friend class RandomAccessContainer;
        friend class boost::iterator_core_access;

        iterator(BaseContainer* base, size_t index) : reference(base, index) { }

        const auto& dereference() const noexcept { return *this; }
        bool equal(const iterator& rhs) const noexcept { return this->get_index() == rhs.get_index(); }
        void increment() noexcept { this->inc_index(); }
        void decrement() noexcept { this->dec_index(); }
        void advance(size_t n) noexcept { this->advance_index( n); }
        ptrdiff_t distance_to(const iterator& rhs) const noexcept { return rhs.get_index() - this->get_index(); }
    };

    auto cbegin() const noexcept { return const_iterator{ this, 0}; }
    auto cend() const noexcept { return const_iterator{ this, this->size()}; }
    auto begin() const noexcept { return cbegin(); }
    auto end() const noexcept { return cend(); }
    auto begin() noexcept { return iterator{ this, 0}; }
    auto end() noexcept { return iterator{ this, this->size()}; }

    using reverse_iterator = boost::reverse_iterator<iterator>;
    using const_reverse_iterator = boost::reverse_iterator<const_iterator>;

    auto crbegin() const noexcept { return const_reverse_iterator(cend()); }
    auto crend() const noexcept { return const_reverse_iterator(cbegin()); }
    auto rbegin() const noexcept { return crbegin(); }
    auto rend() const noexcept { return crend(); }
    auto rbegin() noexcept { return reverse_iterator(cend()); }
    auto rend() noexcept { return reverse_iterator(begin()); } 

    const auto front() const { return *begin(); }
    auto front() { return *begin(); }

    const auto back() const { auto tmp = end(); --tmp; return *tmp; }
    auto back() { auto tmp = end(); --tmp; return *tmp; }

private:
    void check_index(size_t index) const
    {
        if (index >= this->size())
            throw std::out_of_range("SoA container is out of range");
    }
};

template<typename T, size_t N, typename Base>
class BaseArray : public Base
{
public:
    BaseArray() = default;
    void fill(const T& value) { this->replicate( value, 0, N); }
};

template<size_t N>
struct ArrayBinder
{
    template<typename T> using type = std::array<T, N>;
};

template<typename T, size_t N>
using AoSArray = BaseArray<T, N, RandomAccessContainer<AoSRandomAccessContainer<T, ArrayBinder<N>::template type>>>;

template<typename T, size_t N>
using SoAArray = BaseArray<T, N, RandomAccessContainer<SoARandomAccessContainer<T, ArrayBinder<N>::template type>>>;

template<template <typename> typename Allocator>
struct VectorBinder
{
    template<typename T> using type = std::vector<T, Allocator<T>>;
};

template<typename T, template <typename> typename Allocator = std::allocator>
class AoSVector : public RandomAccessContainer<AoSRandomAccessContainer<T, VectorBinder<Allocator>::template type>>
{
public:
    AoSVector() { }
    explicit AoSVector(size_t size) { resize(size); }
    AoSVector(size_t size, const T& value) { resize(size, value); }

    void resize(size_t size) { this->storage.resize(size); }
    void resize(size_t size, const T& value) { this->storage.resize(size, value); }

    void reserve(size_t size) { this->storage.reserve(size); }
    auto capacity() const noexcept { return this->storage.capacity(); }
    void shrink_to_fit() { this->storage.shrink_to_fit(); }

    void push_back(const T& value) { this->storage.push_back(value); }
    void push_back(T&& value) { this->storage.push_back(std::move(value)); }

    void assign(size_t count, const T& value) { this->storage.assign(count, value); }
};

template<typename T, template <typename> typename Allocator = std::allocator>
class SoAVector : public RandomAccessContainer<SoARandomAccessContainer<T, VectorBinder<Allocator>::template type>>
{
    using Base = RandomAccessContainer<SoARandomAccessContainer<T, VectorBinder<Allocator>::template type>>;
    static_assert(!Base::has_bool(), "AoAoAoTT does not support vectors with Booleans");
public:
    SoAVector() : SoAVector(0) { }
    explicit SoAVector(size_t s) { resize(s); }
    SoAVector(size_t s, const T& value) { resize(s, value); }

    void resize(size_t s)
    {
        if constexpr(std::is_trivially_constructible_v<T>)
            resize_memory(s);
        else
            resize(s, T());
    }

    void resize(size_t s, const T& value)
    {
        size_t old_size = this->size();
        resize_memory(s);
        this->replicate( value, old_size, s);
    }

    auto capacity() const noexcept { return std::get<0>(this->storage).capacity(); }
    void reserve(size_t s) { visitor::visit_all(this->storage, [s](auto& v){ v.reserve(s); }); }
    void shrink_to_fit()   { visitor::visit_all(this->storage, [](auto& v){ v.shrink_to_fit(); }); }

    void assign(size_t s, const T& value)
    {
        if (s > this->size())
            resize(s, value);
        else
            this->replicate( value, 0, s);
    }

    void push_back(const T& value)
    {
        auto s = this->size();
        resize_memory(s + 1);
        this->dissipate(value, s);
    }

    void push_back(T&& value)
    {
        auto s = this->size();
        resize_memory(s + 1);
        this->dissipate(std::move(value), s);
    }
private:
    void resize_memory(size_t s) { visitor::visit_all(this->storage, [s](auto& v){ v.resize(s); }); }
};

} // namespace aoaoaott

#endif
