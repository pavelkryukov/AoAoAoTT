/*
 * Copyright (c) 2019 Pavel I. Kryukov
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

#include "magic.hpp"

#include <boost/iterator/iterator_facade.hpp>

namespace aoaoaott {

template<typename T>
class Traits
{
    static_assert(std::is_trivially_copyable<T>::value, "AoAoAoTT supports only trivially copyable types");
    static_assert(sizeof(T) == loophole_ns::sizeof_type_elements<T>, "AoAoAoTT does not support types with padding bytes");
protected:
    static const constexpr size_t members_count = loophole_ns::as_type_list<T>::size;
    using Indices = std::make_index_sequence<members_count>;
};

template<typename T, typename Container>
class BaseFacade
{
public:
    T aggregate() const noexcept { return base->aggregate(this->get_index()); }

    template<auto ptr, typename = std::enable_if_t<std::is_member_function_pointer_v<decltype(ptr)>>, typename ... Args>
    auto method(Args&& ... args) const // noexcept?
    {
        object_mover om(this);
        return (om.object.*ptr)(std::forward<Args>(args)...);
    }

    template<typename R, typename ... Args>
    auto operator->*(R (T::* fun)(Args ...)) const noexcept
    {
        return [this, fun](Args&& ... args) {
            object_mover om(this);
            return (om.object.*fun)(std::forward<Args>(args)...);
        };
    }

    template<typename R, typename ... Args>
    auto operator->*(R (T::* fun)(Args ...) const) const noexcept
    {
        return [this, fun](Args&& ... args) {
            object_mover om(this);
            return (om.object.*fun)(std::forward<Args>(args)...);
        };
    }

private:
    const Container* base;
    size_t index;
    struct object_mover
    {
        const BaseFacade* const Facade;
        T object;
        explicit object_mover(const BaseFacade* Facade) : Facade(Facade), object(Facade->aggregate()) { }
        ~object_mover() { Facade->get_base()->dissipate(std::move(object), Facade->get_index()); }
    };

protected:
    constexpr BaseFacade(const Container* base, size_t index) : base(base), index(index) { }
    constexpr auto get_index() const noexcept { return index; }

    constexpr const auto* get_base() const noexcept { return base; }
    void inc_index() noexcept { ++index; }
    void dec_index() noexcept { --index; }
    void advance_index(ptrdiff_t n) noexcept { index += n; }
};

template<typename T, typename Container>
class ConstFacade : public BaseFacade<T, Container>
{
public:
    ConstFacade( const Container* b, size_t index) : BaseFacade<T, Container>(b, index) { }

    using BaseFacade<T, Container>::operator->*;

    template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
    const auto& get() const noexcept { return this->get_base()->get_element(ptr, this->get_index()); }

    template<typename R>
    const R& operator->*(R T::* field) const noexcept
    {
        return this->get_base()->get_element(field, this->get_index());
    }
};

template<typename T, typename Container>
class Facade : public BaseFacade<T, Container>
{
public:
    Facade( Container* b, size_t index) : BaseFacade<T, Container>(b, index) { }

    template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
    auto& get() const noexcept { return this->get_base()->get_element(ptr, this->get_index()); }

    using BaseFacade<T, Container>::operator->*;

    template<typename R>
    R& operator->*(R T::* field) const noexcept { return this->get_base()->get_element(field, this->get_index()); }

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

template<typename T, typename ContainerT>
class AoSRandomAccessContainer : Traits<T>
{
    friend class BaseFacade<T, AoSRandomAccessContainer>;
    friend class Facade<T, AoSRandomAccessContainer>;
    friend class ConstFacade<T, AoSRandomAccessContainer>;

public:
    auto size() const noexcept { return storage.size(); }
    bool empty() const noexcept { return storage.empty(); }

protected:
    T aggregate(size_t index) const noexcept { return storage[index]; }
    void dissipate(const T& rhs, size_t index) const noexcept  { storage[index] = rhs; }
    void dissipate(T&& rhs, size_t index) const noexcept { storage[index] = std::move(rhs); }

    void replicate(const T& value, size_t start, size_t end)
    {
        for (size_t i = start; i < end; ++i)
            storage[i] = value;
    }

    template<typename R>
    constexpr R& get_element(R T::* member, size_t index) const noexcept
    {
        return storage[index].*member;
    }

    mutable ContainerT storage;
};

template<typename T, typename ContainerT>
class SoARandomAccessContainer : Traits<T>
{
    using Indices = typename Traits<T>::Indices;
    friend class BaseFacade<T, SoARandomAccessContainer>;
    friend class Facade<T, SoARandomAccessContainer>;
    friend class ConstFacade<T, SoARandomAccessContainer>;

public:
    auto size() const noexcept { return std::get<0>(storage).size(); }
    bool empty() const noexcept { return std::get<0>(storage).empty(); }
    
protected:
    T aggregate(size_t index) const noexcept { return aggregate(index, Indices{}); }
    void dissipate(const T& rhs, size_t index) const noexcept  { dissipate(rhs, index, Indices{}); }
    void dissipate(T&& rhs, size_t index) const noexcept { dissipate(std::move(rhs), index, Indices{}); }
    void replicate(const T& value, size_t start, size_t end) { replicate(value, start, end, Indices{}); }

    mutable ContainerT storage;

private:

    template<typename R, size_t I>
    R* get_data_ptr_impl(size_t index) const
    {
        if constexpr (I == 0)
            return nullptr;
        else if constexpr(!std::is_same_v<typename std::tuple_element_t<I - 1, decltype(this->storage)>::value_type, R>)
            return get_data_ptr_impl<R, I - 1>(index);
        else if (index != I - 1)
            return get_data_ptr_impl<R, I - 1>(index);
        else
            return std::get<I - 1>(this->storage).data();
    }

    template<typename R>
    R* get_data_ptr(size_t index) const { return get_data_ptr_impl<R, Traits<T>::members_count>(index); }

    template<typename R>
    constexpr R& get_element(R T::* member, size_t index) const noexcept
    {
        return get_data_ptr<std::remove_cv_t<R>>(member_offset_helpers::get_member_id(member))[index];
    }

    template<size_t N>
    constexpr auto& get_element(size_t index) const noexcept { return std::get<N>(this->storage)[index]; }

    template<size_t N, typename R>
    void replicate(const R& value, size_t start, size_t end)
    {
        // It sounds ridiculous, but there is only one for-loop in the entire header
        // That should be optimized with vectors or 'rep stos'
        for (size_t i = start; i < end; ++i)
            get_element<N>(i) = value;
    }

    template<size_t ... N>
    void dissipate(const T& src, size_t index, std::index_sequence<N...>)
        const noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
    {
        std::tie(get_element<N>(index)...) = std::tie(member_offset_helpers::get_nth_member<T, N>(src)...);
    }

    template<size_t ... N>
    T aggregate(size_t index, std::index_sequence<N...>)
        const noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
    {
        T result{};
        std::tie(member_offset_helpers::get_nth_member<T, N>(result)...) = std::tie(get_element<N>(index)...);
        return result;
    }

    template<size_t ... N>
    void replicate(const T& src, size_t start, size_t end, std::index_sequence<N...>)
        noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
    {
        // Use comma operator to replicate the `replicate` function
        ((void)replicate<N>(member_offset_helpers::get_nth_member<T, N>(src), start, end), ...);
    }
};

template<typename T, typename BaseContainer>
class RandomAccessContainer : public BaseContainer
{
    using MyBaseFacade  = BaseFacade<T, BaseContainer>;
    using MyFacade      = Facade<T, BaseContainer>;
    using MyConstFacade = ConstFacade<T, BaseContainer>;
public:
    auto operator[](size_t index) noexcept { return MyFacade{ this, index}; }
    auto operator[](size_t index) const noexcept { return MyConstFacade{ this, index}; }

    auto at(size_t index) { check_index(index); return operator[](index); }
    auto at(size_t index) const { check_index(index); return operator[](index); }

    class const_iterator : MyConstFacade,
        public boost::iterator_facade<const_iterator, MyConstFacade const, std::random_access_iterator_tag, const MyConstFacade&>
    {
        friend class RandomAccessContainer;
        friend class boost::iterator_core_access;

        const_iterator(const BaseContainer* base, size_t index) : MyConstFacade(base, index) { }

        const auto& dereference() const noexcept { return *this; }
        bool equal(const const_iterator& rhs) const noexcept { return this->get_index() == rhs.get_index(); }
        void increment() noexcept { this->inc_index(); }
        void decrement() noexcept { this->dec_index(); }
        void advance(size_t n) noexcept { this->advance_index( n); }
        ptrdiff_t distance_to(const const_iterator& rhs) const noexcept { return rhs.get_index() - this->get_index(); }
    };

    class iterator : MyFacade,
        public boost::iterator_facade<iterator, MyFacade, std::random_access_iterator_tag, const MyFacade&>
    {
        friend class RandomAccessContainer;
        friend class boost::iterator_core_access;

        iterator(BaseContainer* base, size_t index) : MyFacade(base, index) { }

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

template<typename T, typename Allocator = std::allocator<T>>
class AoSVector : public RandomAccessContainer<T, AoSRandomAccessContainer<T, std::vector<T, Allocator>>>
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

template<typename T, size_t N>
class AoSArray : public RandomAccessContainer<T, AoSRandomAccessContainer<T, std::array<T, N>>>
{
public:
    AoSArray() = default;
    explicit AoSArray(const T& value) { fill(value); }
    void fill(const T& value) { this->storage.fill(value); }
};

template<typename T>
class SoAVector : public RandomAccessContainer<T, SoARandomAccessContainer<T, containers::loophole_tuple_of_vectors<T>>>
{
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

template<typename T, size_t N>
class SoAArray : public RandomAccessContainer<T, SoARandomAccessContainer<T, containers::loophole_tuple_of_arrays<N, T>>>
{
public:
    SoAArray()
    {
        if constexpr(!std::is_trivially_constructible_v<T>)
            fill(T());
    }

    SoAArray(const T& value) { fill(value); }
    void fill(const T& value) { this->replicate( value, 0, N); }
};

} // namespace aoaoaott

#endif
