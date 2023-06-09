
#include <iostream>
#include <math.h>
#include <vector>

#include "blocks.hpp"
#include "helper.hpp"
#include "solver.hpp"
#include "gp-ios.hpp"

using namespace blocks;

class SSModel : public Submodel
{
public:
    SSModel() : Submodel("")
    {
        Node x("x");
        Node xd("xd");

        enter();
        {
            new Const("TimeDelay", 2.7435, {"-time_delay"});
            new Const("Initial", 0.0, {"-initial"});
            new Delay("Delay", {x, "-time_delay", "-initial"}, xd);
        }
        exit();
    }
};

int main()
{
    auto model = SSModel();
    auto history = run(model,
        [](uint k, double& t) -> bool
        {
            return arange(k, t, 0, 10, 0.1);
        },
        [](double t, const NodeValues& /*x*/, NodeValues& inputs) -> void
        {
            inputs.insert_or_assign("x", std::sin(M_PI * t / 5));
        });

    Gnuplot gp;
	gp << "set xrange [0:100]\n";
    gp << "set yrange [-1:1]\n";
	gp << "plot" << gp.file1d(history["x"]) << "with lines title 'x',"
		<< gp.file1d(history["xd"]) << "with lines title 'xd'\n";

    return 0;
}
