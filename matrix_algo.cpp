
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
 *  Using side_counter where top/bottom/left/right correspond to surrounding 
 *  0 value cell of a matrix . Starting from n=1,m=1 iteration ends N-1 / M-1 since no need
 *  to test the edges of the matix becasue the water escapes on the edges.
 *  Main algo :
 *  When side_counter identifies that area is encircled by calling 
 *  is_sector_enclosed , then matrix algo resets the side_counter to all zeros and
 *  increments total_count .
 * 
 *  Improvements:
 *  for algo potimization it's possible to skip to n+1 when detecting is_sector_enclosed 
 *  as it's obviouse the next n+1 is not equal to zero see comment in the code @see optimization
 * 
 *  
 */

#include <string>
#include <array>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <stdexcept>

#define assert_(e) ((void) ((e) ? 0 : my_assert (#e, __FILE__, __LINE__)))
#define my_assert( e, file, line ) ( throw std::runtime_error(\
   std::string("file:")+boost::lexical_cast<std::string>(line)+": failed assertion "+e))

struct side_counter {
    std::size_t top=0;
    std::size_t bottom=0;
    std::size_t left=0;
    std::size_t right=0;
    bool is_sector_enclosed() {
        return std::min(std::min(top,bottom), std::min(left,right)) ;
    }
    void reset() {
        top=0,bottom=0,left=0,right=0;
    }
    friend std::ostream & operator<<(std::ostream &os, const side_counter &other) {
        std::clog << "top=" << other.top << ",bottom=" << other.bottom << ",left=" << other.left << ",right=" << other.right;
    }
};


template<typename T, std::size_t N , std::size_t M>
struct matrix {
    template <typename TT, std::size_t ROW, std::size_t COL>
    using matrix_impl = std::array<std::array<T, COL>, ROW>;
    matrix_impl<T,N,M> impl_;
    side_counter counter_ ;
    
    matrix(const matrix_impl<T,N,M> &impl)  : impl_(impl) {}
        
    int count_encircled_chunks()
    {
        std::size_t total_count = 0;
        for (std::size_t m = 1; m < M - 1; ++m) {
            for (std::size_t n = 1; n < N - 1; ++n) {
                auto value = impl_.at(m).at(n);
                if (!value) {
                    //std::clog << "m=" << m << ",n=" << n << " counter {" << counter_ << "}" << std::endl;
                    int left = impl_.at(m).at(n - 1) ;
                    int right = impl_.at(m).at(n + 1) ;
                    int top = impl_.at(m - 1).at(n) ;
                    int bottom = impl_.at(m + 1).at(n) ;
                    //std::clog << "l=" << left << ",r=" << right << ",b=" << bottom << ",t" << top << std::endl;
                    counter_.left   += left   ? 1 : 0;
                    counter_.right  += right  ? 1 : 0;
                    counter_.top    += top    ? 1 : 0;
                    counter_.bottom += bottom ? 1 : 0;
                    //std::clog <<  "counter {" << counter_ << "}" << std::endl;
                    if ( counter_.is_sector_enclosed() ) {
                        ++total_count ;
                        counter_.reset() ;
                        ++n; //optimization to skip non-zero to the right
                    }
                }
            }
        }
        return  total_count;
    }
};


int main(int argc, char** argv) {
 
    
    std::array<std::array<int, 5>, 5>  puddles_3 = {{
        {0,12,0,0,18},
        {21,0,15,28,0},
        {0,27,0, 0,30},
        {40,0,50,0,60},
        {0,70,0, 80,0}
    }};
    
    matrix<int,5,5> test1(puddles_3) ;
    
    
    std::array<std::array<int, 5>, 5>  puddles_2 = {{
        {0,12,0,0,18},
        {21,0,15,28,0},
        {0,27,0, 0,30},
        {40,1,50,0,60},
        {0,70,0, 80,0}
    }};

    matrix<int,5,5> test2(puddles_2) ;

    try {
        assert_(3 == test1.count_encircled_chunks());
        assert_(2 == test2.count_encircled_chunks());
        std::clog << "TEST1 passed found " << test1.count_encircled_chunks() << " encircled chunks aka puddles from the test" << std::endl;
        std::clog << "TEST2 passed found " << test2.count_encircled_chunks() << " encircled chunks aka puddles from the test" << std::endl;
    } catch (const std::exception &e) {
        std::clog << "One of the test cases failed " << e.what() << std::endl;
    }
    
    return 0;
}

