///@todo copyright

#define CATCH_CONFIG_MAIN
#include "../dependencies/Catch.h" //https://github.com/philsquared/Catch

#include <cmath>
#include <vector>
#include <chrono> //tmp!

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
        else throw std::out_of_range ("Point2D can only be accessed with index 0 or 1");
    }
};


TEST_CASE("LazyKdTree") {
    SECTION("2D") { ///@todo copy tests of lib_2d kdtree
        auto search  = Point2D(13.37, 1337.0);
        auto boxSize = Point2D(30.0, 2.0 * 1334.0);

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

        auto nearest  = tree.nearest(search);
        auto nearest3 = tree.k_nearest(search, 3);
        auto inBox	  = tree.in_box(search, boxSize);
        auto inCircle = tree.in_circle(search, 1334.0);


        auto tmp = nearest;
    }

    SECTION("Performance LazyKdTree") { ///@todo move out of test
        const size_t nPts = 1000000;
        std::vector<Point2D> pts;
        pts.reserve(nPts);

        auto tVecStart = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < nPts; ++i)
            pts.push_back(Point2D(0.5 * i, 2.0 * i));
        std::chrono::duration<double, std::milli> tVec = std::chrono::high_resolution_clock::now() - tVecStart;
        std::cout << "Creation Vec: " << tVec.count() << std::endl;


        auto tTreeStart = std::chrono::high_resolution_clock::now();
        LazyKdTree<Point2D> tree(std::move(pts));
        std::chrono::duration<double, std::milli> tTree = std::chrono::high_resolution_clock::now() - tTreeStart;
        std::cout << "Creation Tree: " << tTree.count() << std::endl;


        auto tNearest1Start = std::chrono::high_resolution_clock::now();
        auto nearest1 = tree.nearest(Point2D(250000, 1000000));
        std::chrono::duration<double, std::milli> tNearest1 = std::chrono::high_resolution_clock::now() - tNearest1Start;
        std::cout << "First nearest fetch: " << tNearest1.count() << std::endl;


        auto tNearest2Start = std::chrono::high_resolution_clock::now();
        auto nearest2 = tree.nearest(Point2D(250000, 1000000));
        std::chrono::duration<double, std::milli> tNearest2 = std::chrono::high_resolution_clock::now() - tNearest2Start;
        std::cout << "Second nearest fetch: " << tNearest2.count() << std::endl;

        auto tEvaluateStart = std::chrono::high_resolution_clock::now();
        tree.evaluate_recursive();
        std::chrono::duration<double, std::milli> tEvaluate = std::chrono::high_resolution_clock::now() - tEvaluateStart;
        std::cout << "Fully evaluating: " << tEvaluate.count() << std::endl;

        std::ofstream outfile;

        outfile.open("timeLog.txt", std::ios_base::app);
        outfile
            << __DATE__
            << " -- "
            << __TIME__
            << ";"
            << tVec.count()
            << ";"
            << tTree.count()
            << ";"
            << tNearest1.count()
            << ";"
            << tNearest2.count()
            << ";"
            << tEvaluate.count()
            << std::endl;
    }

}
