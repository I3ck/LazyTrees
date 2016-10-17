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

    bool operator ==(const Point2D &b) const
    {
        return x == b.x && y == b.y;
    }



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


TEST_CASE("LazyKdTree - Point2D") {
    SECTION("Creation and size") {
        auto pts = std::vector<Point2D>{
           Point2D(0.0, 1.0),
           Point2D(0.0, 2.0),
           Point2D(0.0, 3.0),
           Point2D(0.0, 4.0),
           Point2D(0.0, 5.0),
           Point2D(0.0, 6.0),
           Point2D(0.0, 7.0),
        };
        auto inputSize = pts.size();

        LazyKdTree<Point2D> tree(std::move(pts));

        REQUIRE(tree.size() == inputSize);
        tree.evaluate_recursive();
        REQUIRE(tree.size() == inputSize);
    }

    SECTION("Nearest") {
        auto pts = std::vector<Point2D>{
           Point2D(0.0, 1.0),
           Point2D(0.0, 2.0),
           Point2D(0.0, 3.0),
           Point2D(0.0, 4.0),
           Point2D(0.0, 5.0),
           Point2D(15.0, 6.0),
           Point2D(0.0, 7.0),
        };
        LazyKdTree<Point2D> tree(std::move(pts));

        REQUIRE(tree.nearest(Point2D(0.3, 0.7)) 		== Point2D(0.0, 1.0));
        REQUIRE(tree.nearest(Point2D(-100.0, -100.0)) 	== Point2D(0.0, 1.0));
        REQUIRE(tree.nearest(Point2D(0.5, 2.1)) 		== Point2D(0.0, 2.0));
        REQUIRE(tree.nearest(Point2D(14.0, 7.0)) 		== Point2D(15.0, 6.0));
        REQUIRE(tree.nearest(Point2D(0.0, 300.0)) 		== Point2D(0.0, 7.0));
    }

    SECTION("k_nearest") {
        auto pts = std::vector<Point2D>{
           Point2D(0.0, 1.0),
           Point2D(0.0, 2.0),
           Point2D(0.0, 3.0),
           Point2D(0.0, 4.0),
           Point2D(0.0, 5.0),
           Point2D(15.0, 6.0),
           Point2D(0.0, 7.0),
        };
        LazyKdTree<Point2D> tree(std::move(pts));

        auto result = tree.k_nearest(Point2D(0.0, 0.0), 3);

        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == Point2D(0.0, 1.0));
        REQUIRE(result[1] == Point2D(0.0, 2.0));
        REQUIRE(result[2] == Point2D(0.0, 3.0));
    }

    SECTION("in_box") {
        auto pts = std::vector<Point2D>{
           Point2D(0.0, 1.0),
           Point2D(0.0, 2.0),
           Point2D(0.0, 3.0),
           Point2D(0.0, 4.0),
           Point2D(0.0, 5.0),
           Point2D(15.0, 6.0),
           Point2D(0.0, 7.0),
        };
        LazyKdTree<Point2D> tree(std::move(pts));

        auto result = tree.in_box(Point2D(0.0, 2.0), Point2D(2.1, 2.1));
        REQUIRE(result.size() == 3);
    }

    SECTION("in_circle") {
        auto pts = std::vector<Point2D>{
           Point2D(0.0, 1.0),
           Point2D(0.0, 2.0),
           Point2D(0.0, 3.0),
           Point2D(0.0, 4.0),
           Point2D(0.0, 5.0),
           Point2D(15.0, 6.0),
           Point2D(0.0, 7.0),
        };
        LazyKdTree<Point2D> tree(std::move(pts));

        auto result = tree.in_circle(Point2D(0.0, 2.0), 1.1);
        REQUIRE(result.size() == 3);
    }

    SECTION("Performance LazyKdTree") { ///@todo move out of test
        const size_t nPts = 1000000;
        std::vector<Point2D> pts;
        pts.reserve(nPts);

        auto tVecStart = std::chrono::high_resolution_clock::now();
        for (auto i = 0; i < nPts; ++i)
            pts.push_back(Point2D(0.5 * i, 2.0 * i));
        std::chrono::duration<double, std::milli> tVec = std::chrono::high_resolution_clock::now() - tVecStart;


        auto tTreeStart = std::chrono::high_resolution_clock::now();
        LazyKdTree<Point2D> tree(std::move(pts));
        std::chrono::duration<double, std::milli> tTree = std::chrono::high_resolution_clock::now() - tTreeStart;


        auto tNearest1Start = std::chrono::high_resolution_clock::now();
        auto nearest1 = tree.nearest(Point2D(250000, 1000000));
        std::chrono::duration<double, std::milli> tNearest1 = std::chrono::high_resolution_clock::now() - tNearest1Start;


        auto tNearest2Start = std::chrono::high_resolution_clock::now();
        auto nearest2 = tree.nearest(Point2D(250000, 1000000));
        std::chrono::duration<double, std::milli> tNearest2 = std::chrono::high_resolution_clock::now() - tNearest2Start;

        auto tEvaluateStart = std::chrono::high_resolution_clock::now();
        tree.evaluate_recursive();
        std::chrono::duration<double, std::milli> tEvaluate = std::chrono::high_resolution_clock::now() - tEvaluateStart;




        std::ifstream fileExistTest("timeLog.txt");
        auto fileExists = (bool)fileExistTest;
        fileExistTest.close();

        std::ofstream outfile;
        outfile.open("timeLog.txt", std::ios_base::app);

        if (!fileExists)
        {
            outfile
                << "VERSION;"
                << "vec creation;"
                << "tree creation;"
                << "first nearest;"
                << "second nearest;"
                << "full evaluation;"
                << std::endl;
        }
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
