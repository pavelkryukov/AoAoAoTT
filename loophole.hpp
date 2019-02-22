// Based on public domain code authored by Alexandr Poltavsky
// https://github.com/alexpolt/luple

#ifndef AO_AO_AO_TT_MAGIC
#define AO_AO_AO_TT_MAGIC

#include <cstddef>
#include <utility>

namespace aoaoaott {

namespace type_list_ns
{
    template<typename... TT> struct type_list {
        static constexpr size_t size = sizeof...(TT);
        static constexpr size_t sizeof_total = (sizeof(TT) + ...);
        using Indices = std::make_index_sequence<size>;
    };
    
    template<> struct type_list<> {
        static constexpr size_t size = 0;
        static constexpr size_t sizeof_total = 0;
        using Indices = std::make_index_sequence<0>;
    };

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
 
    template<typename T, size_t ... Ns> type_list<tlist_get_t<T, Ns>...> trim_type_list_n(std::index_sequence<Ns...>);
    template<typename T, size_t N> using trim_type_list = decltype(trim_type_list_n<T>(std::make_index_sequence<N>{}));
} // namespace type_list_ns

namespace loophole_ns
{
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
        using type = type_list_ns::type_list< decltype(loophole(tag<T, NN>{}))... >;
    };

    template<typename T>
    using as_type_list =
        typename loophole_type_list<T, std::make_integer_sequence<int, fields_number<T>(0)>>::type;
} // namespace loophole_ns

} // namespace aoaoaott

#endif // AO_AO_AO_TT_MAGIC
