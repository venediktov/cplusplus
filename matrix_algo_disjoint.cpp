
/*
*  Рассмотрим матрицу целых чисел размера N x M, в которой каждое число описывает
*  высоту рельефа воображаемой поверхности. Опустим данную поверхность в воду так глубоко,
*  чтобы покрыть водой поверхность полностью, т.е. включая ее наивысшую точку.
*  Затем вынем поверхность целиком из воды. Часть воды стечет через границы поверхности вниз,
*  часть останется внутри границ поверхности.
*  Ваша задача реализовать эффективное решение позволяющее вычислить все лужи,
*  оставшиеся на поверхности. Программа должна компилироваться и работать под Windows и Unix.
*  Задание должно быть реализовано на С++.
*  Пожалуйста, вместе с программой пришлите словесное описание реализованного решения,
*  оценку сложности работы алгоритма и набор тестов, подтверждающих корректность работы программы.
*
* Using disjoint sets to implement water puddles algorithm
* iterating 2 times over the entire matrix algorithm complexity estimated at O(2N) => O(N)
* first iteration :
* connenct potential puddle cells of a matrix into unions see disjoint_sets
* second iteration :
* if has the same parent then belongs to same puddle , counter only incremented firs time
* edge case for outflow on the edge is handled by extra check of edge of the matrix during second iteration
* 
*  
*
*/

#include <string>
#include <array>
#include <iostream>
#include <boost/lexical_cast.hpp>
//#include <boost/pending/disjoint_sets.hpp>
#include <stdexcept>
#include <memory>
#include <vector>
#include <algorithm>
#include <numeric>

#define assert_(e) ((void) ((e) ? 0 : my_assert (#e, __FILE__, __LINE__)))
#define my_assert( e, file, line ) ( throw std::runtime_error(\
   std::string("file:")+boost::lexical_cast<std::string>(line)+": failed assertion "+e))


//primitive implementation of disjoint sets without path compression
template<typename std::size_t MN>
struct disjoint_sets {

    std::array<int,MN> rank;
    std::array<int,MN> parent;

    disjoint_sets() : rank{}, parent{} {
        std::iota(std::begin(parent), std::end(parent), 0);
    }

    
    int find_set(int x) {
        if (parent[x] != x) {
            return find_set(parent[x]);
        }
        return x;
    }

    void union_set(int x, int y) {
        int x_root = find_set(x);
        int y_root = find_set(y);

        if (x_root == y_root) {
            return;
        }

        if (rank[x_root] < rank[y_root]) {
            parent[x_root] = y_root;
        }
        else if (rank[y_root] < rank[x_root]) {
            parent[y_root] = x_root;
        }
        else {
            parent[y_root] = x_root;
            rank[x_root] = rank[x_root] + 1;
        }
    }
};

// M x N => ROW x COL
template<typename T, std::size_t M, std::size_t N>
struct matrix {
    template <typename TT, std::size_t ROW, std::size_t COL>
    using matrix_impl = std::array<std::array<TT, COL>, ROW>;
    matrix_impl<T, M, N> impl_;
    disjoint_sets<M*N> dis_;
    std::array<T,M*N> frequency_;

    matrix(const matrix_impl<T, M, N> &impl) : impl_{ impl }, dis_{}, frequency_{} {}

    std::size_t count_encircled_chunks()
    {
        std::size_t total_count{};
        //join sets
        for (int m = 0; m < M; ++m) {
            for (int n = 0; n < N; ++n) {
                T value = impl_.at(m).at(n);
                if (m + 1 < M && impl_.at(m + 1).at(n) == value) {
                    dis_.union_set(m*N + n, (m + 1)*N + n);
                }
                if (m - 1 >= 0 && impl_.at(m - 1).at(n) == value) {
                    dis_.union_set(m*N + n, (m - 1)*N + n);
                }
                if (n + 1 < N && impl_.at(m).at(n + 1) == value) {
                    dis_.union_set(m*N + n, m*N + (n + 1));
                }
                if (n - 1 >= 0 && impl_.at(m).at(n - 1) == value) {
                    dis_.union_set(m*N + n, m*N + (n - 1));
                }

            }
        }

  
        //do the actual counting 
        for (int m = 1; m < M - 1; ++m) {
            for (int n = 1; n < N - 1; ++n) {
                T value = impl_.at(m).at(n);
                if (impl_.at(m + 1).at(n) < value) continue;
                if (impl_.at(m - 1).at(n) < value) continue;
                if (impl_.at(m).at(n + 1) < value) continue;
                if (impl_.at(m).at(n - 1) < value) continue;

                int x = dis_.find_set(m * N + n);

                if (frequency_[x] == 0) {
                    total_count++;
                    frequency_[x]++;
                }
                else {
                    frequency_[x]++;
                }

                if ((m + 1 == M - 1 && impl_.at(m + 1).at(n) <= value) ||
                    (m - 1 == 0 && impl_.at(m - 1).at(n) <= value) ||
                    (n + 1 == N && impl_.at(m).at(n + 1) <= value) ||
                    (n - 1 == 0 && impl_.at(m).at(n - 1) <= value)
                    ) { --total_count; }

            }
        }


        return  total_count;
    }

  
};


int main(int argc, char** argv) {


    std::array<std::array<int, 5>, 5>  puddles_5x5_3_3 = 
    {{
        { 0,12,0,0,18 },
        { 21,0,15,28,0 },
        { 0,27,0, 0,30 },
        { 40,0,50,0,60 },
        { 0,70,0, 80,0 }
    }};
    matrix<int, 5, 5> test1(puddles_5x5_3_3);


    std::array<std::array<int, 5>, 5>  puddles_5x5_3_nonzero = 
    {{
        { 0,12,0,0,18 },
        { 21,0,15,28,0 },
        { 0,27,0, 0,30 },
        { 40,1,50,0,60 },
        { 0,70,0, 80,0 }
    }};
    matrix<int, 5, 5> test2(puddles_5x5_3_nonzero);

    std::array<std::array<int, 5>, 5>  puddles_5x5_3_2 = 
    {{
        { 0,12,0,0,18 },
        { 21,0,15,28,0 },
        { 0,27,0, 0,30 },
        { 40,0,50,0,60 },
        { 0, 0, 0, 80,0 } //open  on cell in the bottom to exclude 
    }};
    matrix<int, 5, 5> test3(puddles_5x5_3_2);

    std::array<std::array<int, 5>, 6>  puddles_6x5_2 = 
    {{
        { 0,1,0,0,1 },
        { 1,0,1,1,0 },
        { 0,1,0,0,1 },
        { 1,0,1,0,1 },
        { 1,0,0,1,1 },
        { 0,1,0,1,0 }
    }};
    matrix<int, 6, 5> test4(puddles_6x5_2);

    std::array<std::array<int, 8>, 8>  puddles_8x8_1 =
    {{
        { 1,1,1,1,1,1,1,1 },
        { 1,0,0,0,0,0,0,1 },
        { 1,0,1,1,1,1,0,1 },
        { 1,0,1,0,0,1,0,1 },
        { 1,0,1,1,0,1,0,1 },
        { 1,0,1,1,0,1,0,1 },
        { 1,0,0,0,0,1,0,1 },
        { 1,1,1,1,1,1,1,1 }
    }};
    matrix<int, 8, 8> test5(puddles_8x8_1);

    //open up m=7,n=6 from test5 should have 0 puddles
    std::array<std::array<int, 8>, 8>  puddles_8x8_0 =
    {{
        { 1,1,1,1,1,1,1,1 },
        { 1,0,0,0,0,0,0,1 },
        { 1,0,1,1,1,1,0,1 },
        { 1,0,1,0,0,1,0,1 },
        { 1,0,1,1,0,1,0,1 },
        { 1,0,1,1,0,1,0,1 },
        { 1,0,0,0,0,1,0,1 },
        { 1,1,1,1,1,1,0,1 }
    }};
    matrix<int, 8, 8> test6(puddles_8x8_0);

    //from Alexander Toropov
    std::array<std::array<int, 5>, 5>  puddles_5x5_1 =
    {{
        {2,2,2,2,2},
        {2,1,1,1,2},
        {2,1,1,1,2},
        {2,1,1,1,2},
        {2,2,2,2,2}
    }};
    matrix<int, 5, 5> test7(puddles_5x5_1);

    //from Vladimir Venediktov
    std::array<std::array<int, 5>, 5>  puddles_5x5_2 =
    {{
        { 2,2,2,2,2 },
        { 2,1,3,1,2 },
        { 2,1,3,1,2 },
        { 2,1,3,1,2 },
        { 2,2,2,2,2 }
    }};
    matrix<int, 5, 5> test8(puddles_5x5_2);

    //Hope no more cases will break the algo ...

    try {
        assert_(3 == test1.count_encircled_chunks());
        assert_(3 == test2.count_encircled_chunks());
        assert_(2 == test3.count_encircled_chunks());
        assert_(2 == test4.count_encircled_chunks());
        assert_(1 == test5.count_encircled_chunks());
        assert_(0 == test6.count_encircled_chunks());
        assert_(1 == test7.count_encircled_chunks());
        assert_(2 == test8.count_encircled_chunks());
    }
    catch (const std::exception &e) {
        std::clog << "One of the test cases failed broken ALGO " << e.what() << std::endl;
        return 1;
    }

    std::clog << "All test cases passed" << std::endl;
    return 0;
}

