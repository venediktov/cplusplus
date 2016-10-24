
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
 *  is_sector_enclosed , then matrix  increments total_count.
 *  Last addition: the counters are stored as std::shared_ptr and are actually 
 *  shared for each incomplete on the first pass puddle , when more iterations
 *  is needed.
 *  Memory overhead: one std::shared_ptr per puddle
 *
 *  Improvements:
 *  for algo optimization it's possible to skip to n+1 when detecting is_sector_enclosed 
 *  as it's obviouse the next n+1 is not equal to zero see comment in the code @see optimization
 *
 *  @TODO: Further code improvements if this algo really works:
 *  1. use one internal matric_impl<std::pair<T,side_counter>,M,N> combining value and counters
 *  thus avoiding extra code like conter_matrix_.at(m).at(n) and calling this way
 *
 *   std::size_t count_encircled_chunks()
 *   {
 *       std::size_t total_count{};
 *       for (std::size_t m = 1; m < M - 1; ++m) {
 *           for (std::size_t n = 1; n < N - 1; ++n) {
 *               auto value = impl_.at(m).at(n); //will hold both T and side_counter
 *               if (!value) {                
 *                   auto left = impl_.at(m).at(n - 1).first?1:0 ;
 *                   auto right = impl_.at(m).at(n + 1).first?1:0 ;
 *                   auto top = impl_.at(m - 1).at(n).first?1:0 ;
 *                   auto bottom = impl_.at(m + 1).at(n).first?1:0 ;                   
 *                   side_counter current_counter(top,bottom,left,right) ;
 *                   if ( current_counter.is_sector_enclosed() ) {
 *                       ++n; //optimization to skip non-zero to the right
 *                       ++total_count;
 *                   } else if ( get_assign_counter(value,current_counter)->is_sector_enclosed()) {
 *                       ++total_count ;
 *                   }                  
 *               }
 *           }
 *       }
 *  
 */

#include <string>
#include <array>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <stdexcept>
#include <memory>

#define assert_(e) ((void) ((e) ? 0 : my_assert (#e, __FILE__, __LINE__)))
#define my_assert( e, file, line ) ( throw std::runtime_error(\
   std::string("file:")+boost::lexical_cast<std::string>(line)+": failed assertion "+e))

struct side_counter {
    std::size_t top{};
    std::size_t bottom{};
    std::size_t left{};
    std::size_t right{};
    
    side_counter() {}
    side_counter(std::size_t value) : top{value},bottom{value},left{value},right{value} {}
    side_counter(std::size_t t, std::size_t b, std::size_t l, std::size_t r) : top{t},bottom{b},left{l},right{r} {}
    
    bool is_sector_enclosed() {
        return top == bottom and left == right ;
    }

    void operator= (std::size_t value) {
        top=value,bottom=value,left=value,right=value;
    }
    void operator+= (const side_counter &other) {
        top    += other.top;
        bottom += other.bottom;
        left   += other.left ;
        right  += other.right;
    }

    friend std::ostream & operator<<(std::ostream &os, const side_counter &other) {
        os << "top=" << other.top << ",bottom=" << other.bottom << ",left=" << other.left << ",right=" << other.right;
        return os;
    }
};


template<typename T, std::size_t N , std::size_t M>
struct matrix {
    template <typename TT, std::size_t ROW, std::size_t COL>
    using matrix_impl = std::array<std::array<TT, COL>, ROW>;
    matrix_impl<T,N,M> impl_;
    using side_counter_ptr = std::shared_ptr<side_counter> ;
    matrix_impl<side_counter_ptr,N,M> counter_matrix_ ;
    
    matrix(const matrix_impl<T,N,M> &impl)  : impl_(impl) {}
        
    std::size_t count_encircled_chunks()
    {
        std::size_t total_count{};
        for (std::size_t m = 1; m < M - 1; ++m) {
            for (std::size_t n = 1; n < N - 1; ++n) {
                T value = impl_.at(m).at(n);
                if (!value) {                
                    T left = impl_.at(m).at(n - 1) ;
                    T right = impl_.at(m).at(n + 1) ;
                    T top = impl_.at(m - 1).at(n) ;
                    T bottom = impl_.at(m + 1).at(n) ;                   
                    side_counter current_counter((top?1:0),(bottom?1:0),(left?1:0),(right?1:0)) ;
                    if ( current_counter.is_sector_enclosed() ) {
                        ++n; //optimization to skip non-zero to the right
                        ++total_count;
                    } else if ( get_assign_counter(m,n,current_counter)->is_sector_enclosed()) {
                        ++total_count ;
                    }                  
                }
            }
        }
        return  total_count;
    }

private:    
    side_counter_ptr get_assign_counter ( std::size_t m , std::size_t n, const side_counter &counter) {
        auto assign_update = [](side_counter_ptr &ptr, side_counter_ptr &this_ptr){
            if ( !ptr ) {
                ptr = this_ptr;
            } else if ( ptr != this_ptr ) {
                *this_ptr += *ptr;
                ptr = this_ptr;
            }
            return;
        };
        
        side_counter_ptr &this_ptr = counter_matrix_.at(m).at(n) ;
        if ( !this_ptr ) {
            this_ptr.reset(new side_counter) ;
        }
        *this_ptr += counter;
        
        if ( !counter.bottom ) {
            side_counter_ptr &bottom_ptr = counter_matrix_.at(m + 1).at(n);
            assign_update(bottom_ptr,this_ptr) ;
        }
        
        if ( !counter.top ) {
            side_counter_ptr &top_ptr = counter_matrix_.at(m - 1).at(n);
             assign_update(top_ptr,this_ptr) ;
        }
        
        if ( !counter.left ) {
            side_counter_ptr &left_ptr = counter_matrix_.at(m).at(n - 1);
             assign_update(left_ptr,this_ptr) ;
        }
        
        if ( !counter.right ) {
            side_counter_ptr &right_ptr = counter_matrix_.at(m).at(n + 1);
             assign_update(right_ptr,this_ptr) ;
        }
        
        return this_ptr;
    }
};


int main(int argc, char** argv) {
 
    
    std::array<std::array<int, 5>, 5>  puddles_3_3 = {{
        {0,12,0,0,18},
        {21,0,15,28,0},
        {0,27,0, 0,30},
        {40,0,50,0,60},
        {0,70,0, 80,0}
    }};
    
    matrix<int,5,5> test1(puddles_3_3) ;
    
    
    std::array<std::array<int, 5>, 5>  puddles_2_2 = {{
        {0,12,0,0,18},
        {21,0,15,28,0},
        {0,27,0, 0,30},
        {40,1,50,0,60},
        {0,70,0, 80,0}
    }};

    matrix<int,5,5> test2(puddles_2_2) ;
    
     std::array<std::array<int, 5>, 5>  puddles_3_2 = {{
        {0,12,0,0,18},
        {21,0,15,28,0},
        {0,27,0, 0,30},
        {40,0,50,0,60},        
        {0, 0, 0, 80,0} //open  on cell in the bottom to exclude 
    }};
    
    matrix<int,5,5> test3(puddles_3_2) ;
    
    try {
        assert_(3 == test1.count_encircled_chunks());
        assert_(2 == test2.count_encircled_chunks());
        assert_(2 == test3.count_encircled_chunks());
    } catch (const std::exception &e) {
        std::clog << "One of the test cases failed broken ALGO " << e.what() << std::endl;
        return 1;
    }
    
    std::clog << "All test cases passed" << std::endl;
    return 0;
}

