// Based on public domain code authored by Alexandr Poltavsky
// https://github.com/alexpolt/luple

#ifndef AO_AO_AO_TT_TYPE_LIST
#define AO_AO_AO_TT_TYPE_LIST

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

} // namespace aoaoaott

#endif // AO_AO_AO_TT_TYPE_LIST
