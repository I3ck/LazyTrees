///@todo copyright

#define CATCH_CONFIG_MAIN
#include "../dependencies/Catch.h" //https://github.com/philsquared/Catch

#include <cmath>
#include <vector>

#include "LazyKdTree.h"

using namespace std;
using namespace lazyTrees;

class Point2D
{
public:
    double x, y;

    Point2D() :
        x(0),
        y(0)
    {}

    Point2D(double x, double y) :
        x(x),
        y(y)
    {}





    static size_t dimensions()
    {
        return 2;
    }

    const double& operator[](size_t idx) const
    {
        if(idx == 0) return x;
        else if(idx == 1) return y;
        else throw std::out_of_range ("Point can only be accessed with index 0 or 1");
    }
};


TEST_CASE("LazyKdTree") {
    SECTION("2D") {
        auto search = Point2D(13.37, 1337.0);

        auto pts = std::vector<Point2D>{
           Point2D(0.0, 1.0),
           Point2D(0.0, 2.0),
           Point2D(0.0, 3.0),
           Point2D(0.0, 4.0),
           Point2D(0.0, 5.0),
           Point2D(0.0, 6.0),
           Point2D(0.0, 7.0),
        };

        LazyKdTree<Point2D> tree(std::move(pts));

        auto nearest = tree.nearest(search);

    }

}
