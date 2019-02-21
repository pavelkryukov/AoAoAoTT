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

#ifndef AO_AO_AO_TT_MAGIC
#define AO_AO_AO_TT_MAGIC

#include <array>
#include <cassert>
#include <tuple>
#include <vector>

namespace ao_ao_ao_tt {

namespace loophole_ns {
    // Based on public domain code authored by Alexandr Poltavsky
    // https://github.com/alexpolt/luple
    template<typename... TT> struct type_list { static constexpr size_t size = sizeof...(TT); };
    template<typename T, int N, int M = 0> struct tlist_get;
    template<int N, int M, typename T, typename... TT> struct tlist_get< type_list<T, TT...>, N, M > {
        static_assert(N < (int) sizeof...(TT)+1 + M, "type index out of bounds");
        using type = std::conditional_t< N == M, T, typename tlist_get< type_list<TT...>, N, M + 1 >::type >;
    };

    template<int N, int M> struct tlist_get< type_list<>, N, M > { using type = void; };
    template<int N> struct tlist_get< type_list<>, N, 0 > {};
    template<typename T, int N> using tlist_get_t = typename tlist_get<T, N>::type;

    template<typename T, typename U, int N = 0> struct tlist_get_n;
    template<typename U, int N, typename T, typename... TT> struct tlist_get_n< type_list<T, TT...>, U, N > {
        static const int value = std::is_same< T, U >::value ? N : tlist_get_n< type_list<TT...>, U, N + 1 >::value;
    };
    template<typename U, int N> struct tlist_get_n< type_list<>, U, N > {
        static const int value = -1;
    };

    /*
        tag<T,N> generates friend declarations and helps with overload resolution.
        There are two types: one with the auto return type, which is the way we read types later.
        The second one is used in the detection of instantiations without which we'd get multiple
        definitions.
    */
    template<typename T, int N>
    struct tag {
        friend auto loophole(tag<T,N>);
        constexpr friend int cloophole(tag<T,N>);
    };


    /*
        The definitions of friend functions.
    */
    template<typename T, typename U, int N, bool B>
    struct fn_def {
        friend auto loophole(tag<T,N>) { return U{}; }
        constexpr friend int cloophole(tag<T,N>) { return 0; }
    };

    /*
        This specialization is to avoid multiple definition errors.
    */
    template<typename T, typename U, int N>
    struct fn_def<T, U, N, true> {};

    /*
        This has a templated conversion operator which in turn triggers instantiations.
        Important point, using sizeof seems to be more reliable. Also default template
        arguments are "cached" (I think). To fix that I provide a U template parameter to
        the ins functions which do the detection using constexpr friend functions and SFINAE.
    */
    template<typename T, int N>
    struct c_op {
        template<typename U, int M> static auto ins(...) -> int;
        template<typename U, int M, int = cloophole(tag<T,M>{}) > static auto ins(int) -> char;

        template<typename U, int = sizeof(fn_def<T, U, N, sizeof(ins<U, N>(0)) == sizeof(char)>)>
        operator U();
    };

    /*
        Here we detect the data type field number. The byproduct is instantiations.
        Uses list initialization. Won't work for types with user provided constructors.
        In C++17 there is std::is_aggregate which can be added later.
    */

    template<typename T, int... NN>
    constexpr int fields_number(...) { return sizeof...(NN)-1; }

    template<typename T, int... NN>
    constexpr auto fields_number(int) -> decltype(T{ c_op<T,NN>{}... }, 0) {
        return fields_number<T, NN..., sizeof...(NN)>(0);
    }

    /*
        This is a helper to turn a data structure into a type list.
        Usage is: loophole_ns::as_type_list< data_t >
        I keep dependency on luple (a lightweight tuple of my design) because it's handy
        to turn a structure into luple (tuple). luple has the advantage of more stable layout
        across compilers and we can reinterpret_cast between the data structure and luple.
        More details are in the luple.h header.
    */

    template<typename T, typename U>
    struct loophole_type_list;

    template<typename T, int... NN>
    struct loophole_type_list< T, std::integer_sequence<int, NN...> > {
        using type = type_list< decltype(loophole(tag<T, NN>{}))... >;
    };

    template<typename T>
    using as_type_list =
        typename loophole_type_list<T, std::make_integer_sequence<int, fields_number<T>(0)>>::type;

    template<typename... TT>
    static constexpr const size_t sizeof_type_list_n = (sizeof(TT) + ...);

    template<typename ... TT>
    static constexpr size_t sizeof_type_list(type_list<TT...>)
    {
        return sizeof_type_list_n<TT...>;
    }

    template<typename T>
    static constexpr const size_t sizeof_type_elements = sizeof_type_list(as_type_list<T>());
} // namespace loophole_ns

namespace member_offset_helpers
{
    // This namespace contains core magic by Pavel Kryukov
    // I'm pretty sure 95% of that code is UB
    //
    template<typename R, typename T>
    static constexpr std::ptrdiff_t member_offset(R T::* member) noexcept
    {
        return reinterpret_cast<std::ptrdiff_t>(&(reinterpret_cast<T const volatile*>(NULL)->*member));
    }

    template<typename T, size_t N> using NthMemberType = loophole_ns::tlist_get_t<loophole_ns::as_type_list<T>, N>;

    template<typename T, size_t N> constexpr std::ptrdiff_t nth_member_offset = sizeof(NthMemberType<T, N - 1>) + nth_member_offset<T, N - 1>;
    template<typename T> constexpr std::ptrdiff_t nth_member_offset<T, 0> = 0;

    template<typename T, size_t N> static const auto& get_nth_member(const T& value)
    {
        return *reinterpret_cast<const NthMemberType<T, N>*>(reinterpret_cast<const char*>(&value) + nth_member_offset<T, N>);
    }

    template<typename T, size_t N> static auto& get_nth_member(T& value)
    {
        return *reinterpret_cast<NthMemberType<T, N>*>(reinterpret_cast<char*>(&value) + nth_member_offset<T, N>);
    }

    template<size_t N, typename R, typename T>
    constexpr bool is_nth_member(R T::* member) noexcept
    {
        return member_offset(member) == nth_member_offset<T, N>;
    }

    template<size_t N, typename R, typename T>
    constexpr size_t check_nth_member(R T::* member) noexcept
    {
        if (is_nth_member<N>(member))
            return N;
        else if constexpr (N > 0)
            return check_nth_member<N-1>(member);
        else
            return -1;
    }

    template<typename R, typename T>
    constexpr size_t get_member_id(R T::* member) noexcept
    {
        return check_nth_member<loophole_ns::as_type_list<T>::size - 1>(member);
    }
} // namespace member_offset_helpers


namespace containers {
    using namespace loophole_ns;

    template<typename ... TT> using tuple_of_vectors = std::tuple<std::vector<std::remove_cv_t<TT>>...>;
    template<typename ... TT> tuple_of_vectors<TT...> tuple_of_vectors_generator(type_list<TT...>);
    template<typename T> using loophole_tuple_of_vectors = decltype(tuple_of_vectors_generator(as_type_list<T>()));

    template<size_t N, typename ... TT> using tuple_of_arrays = std::tuple<std::array<std::remove_cv_t<TT>, N>...>;
    template<size_t N, typename ... TT> tuple_of_arrays<N, TT...> tuple_of_arrays_generator(type_list<TT...>);
    template<size_t N, typename T> using loophole_tuple_of_arrays = decltype(tuple_of_arrays_generator<N>(as_type_list<T>()));

    template<typename R, typename Tuple, size_t I>
    R* get_data_ptr_impl(Tuple& tuple, size_t index)
    {
        if constexpr (I == 0)
            return nullptr;
        else if constexpr(!std::is_same_v<typename std::tuple_element_t<I - 1, Tuple>::value_type, R>)
            return get_data_ptr_impl<R, Tuple, I - 1>(tuple, index);
        else if (index != I - 1)
            return get_data_ptr_impl<R, Tuple, I - 1>(tuple, index);
        else
            return std::get<I - 1>(tuple).data();
    }

    template<typename R, typename Tuple>
    R* get_data_ptr(Tuple& tuple, size_t index)
    {
        return get_data_ptr_impl<R, Tuple, std::tuple_size<Tuple>::value>(tuple, index);
    }
} // containers

namespace visitor {
    template <size_t I, typename T, typename F>
    static void visit_all_impl(T& tup, F fun)
    {
        fun(std::get<I>(tup));
        if constexpr (I > 0)
            visit_all_impl<I - 1>(tup, fun);
    }
    
    template <size_t I, typename T, typename F>
    static void visit_impl(T& tup, size_t idx, F fun)
    {
        if (idx == I)
            fun(std::get<I>(tup));
        else if constexpr (I > 0)
            visit_impl<I - 1>(tup, idx, fun);
    }

    template <typename F, typename... Ts>
    void visit_at(std::tuple<Ts...> const& tup, size_t idx, F fun)
    {
        visit_impl<sizeof...(Ts) - 1>(tup, idx, fun);
    }

    template <typename F, typename... Ts>
    void visit_at(std::tuple<Ts...>& tup, size_t idx, F fun)
    {
        visit_impl<sizeof...(Ts) - 1>(tup, idx, fun);
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

} // namespace ao_ao_ao_tt

#endif // AO_AO_AO_TT_MAGIC
