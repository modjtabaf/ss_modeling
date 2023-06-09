
#include <algorithm>
#include <iostream>
#include <iterator>
#include <math.h>
#include <vector>

#include "blocks.hpp"
#include "helper.hpp"
#include "solver.hpp"
#include "gp-ios.hpp"

#include "front_wheel_angle_Rq.hpp"

using namespace blocks;

class PT : public Submodel
{
public:
    PT(const Nodes& iports, const Node& y_out) : Submodel("PT", iports, {y_out})
    {
        // nodes
        const auto& y_in = _iports[0];
        std::cout << "y_in: " << y_in << "\n";
        const auto& tau  = _iports[1];
        const auto& y0   = _iports[2];

        // blocks
        enter();
        {
            new AddSub("+-1", "+-", {y_in, y_out}, "001");
            new MulDiv("*/", "*/", {"001", tau}, "002");
            new Integrator("Int", "002", -1);
            new InitialValue("IV", y0, "003");
            new AddSub("+-2", "++", {-1, "003"}, y_out);
        }
        exit();
    }
};

class ComputeFrontWheelAngleRightLeftPinpoint : public Submodel
{
public:
    ComputeFrontWheelAngleRightLeftPinpoint(const Node& front_wheel_angle, const Nodes& oports) :
        Submodel("ComputeFrontWheelAngleRightLeftPinpoint", front_wheel_angle, oports)
    {
        // nodes
        const auto& front_wheel_angle_right = oports[0];
        const auto& front_wheel_angle_left  = oports[1];
        Node tractor_wheelbase("tractor_wheelbase");
        Node tractor_Width("tractor_Width");

        // blocks
        enter();
        {
            new MulDiv("*/1", "*/", {tractor_wheelbase, front_wheel_angle}, -1);
            new AddSub("+-1", "++", {-1, tractor_Width}, "004");
            new MulDiv("*/2", "*/", {tractor_wheelbase, "004"}, front_wheel_angle_right);
            new Gain("K", 0.5, tractor_Width, {"005"});
            new AddSub("+-2", "+-", {-1, "005"}, "006");
            new MulDiv("*/3", "*/", {tractor_wheelbase, "006"}, front_wheel_angle_left);
        }
        exit();
    }
};

class SteeringSystem : public Submodel
{
public:
    SteeringSystem(const Node& ad_DsrdFtWhlAngl_Rq_VD, const Node& steering_info) :
        Submodel("Steering_System", ad_DsrdFtWhlAngl_Rq_VD, steering_info)
    {
        // nodes
        Node front_wheel_angle("front_wheel_angle");
        Node front_wheel_angle_rate("front_wheel_angle_rate");
        Node front_wheel_angle_neg("front_wheel_angle_neg");
        Node front_wheel_angle_rate_neg("front_wheel_angle_rate_neg");
        Node AxFr_front_right("AxFr_front_right");
        Node AxFr_front_left("AxFr_front_left");

        // blocks
        enter();
        {
            new MulDiv("*/", "**", {ad_DsrdFtWhlAngl_Rq_VD, "front_wheel_ang_gain"}, "007");
            new Delay("Delay", {"007", "front_wheel_ang_delay", "front_wheel_ang_init_value"}, {-2});
            new Function("Clamp",
                [](double /*t*/, const Value& x) -> Value
                {
                    return x.max(0.001).min(10);
                }, "front_wheel_ang_t_const", "008");
            new PT({-2, "008", -2}, front_wheel_angle);
            new Derivative("Derivative", front_wheel_angle, front_wheel_angle_rate);
            new Gain("K1", -1, front_wheel_angle, front_wheel_angle_neg);
            new Gain("K2", -1, front_wheel_angle_rate, front_wheel_angle_rate_neg);
            new ComputeFrontWheelAngleRightLeftPinpoint(front_wheel_angle, {AxFr_front_right, AxFr_front_left});
            new Bus("Bus", {
                front_wheel_angle,
                front_wheel_angle_rate,
                front_wheel_angle_neg,
                front_wheel_angle_rate_neg,
                AxFr_front_right,
                AxFr_front_left
                }, steering_info);
        }
        exit();
    }
};

double interp1d(double x, const std::vector<double>& X, const std::vector<double>& Y)
{
    assert((X.size() == Y.size()) and (not X.empty()));

    if (x <= X.front())
        return Y.front();
    if (x >= X.back())
        return Y.back();
    auto xlb_it = std::lower_bound(X.begin(), X.end(), x);
    double x0 = *xlb_it;
    double x1 = *(xlb_it + 1);
    auto ylb_it = Y.begin() + std::distance(X.begin(), xlb_it);
    double y0 = *ylb_it;
    double y1 = *(ylb_it + 1);
    return (y1 - y0)*(x - x0)/(x1 - x0) + y0;
}

int main()
{
    // front_wheel_angle_Rq = helper.load_mat_files_as_bus(
    //     "/home/fathi/torc/git/playground/py_ss/data/processed_mat",
    //     "front_wheel_angle_Rq")

    NodeValues parameters = {
        {"tractor_wheelbase", 5.8325},
        {"tractor_Width", 2.5},
        {"front_wheel_ang_t_const", 0.1},
        {"front_wheel_ang_delay", 0.02},
        {"front_wheel_ang_gain", 1.0},
        {"front_wheel_ang_init_value", 0.0},
        };

    auto steering_system = SteeringSystem(
        "front_wheel_angle_Rq",
        "steering_info");
    auto history = run(steering_system,
        [](uint k, double& t) -> bool
        {
            return arange(k, t, FRONT_WHEEL_ANGLE_RQ_X.front(),
                FRONT_WHEEL_ANGLE_RQ_X.back(), 0.1);
        },
        [](double t, const NodeValues& /*x*/, NodeValues& inputs) -> void
        {
            inputs.insert_or_assign("front_wheel_angle_Rq",
                interp1d(t, FRONT_WHEEL_ANGLE_RQ_X, FRONT_WHEEL_ANGLE_RQ_Y));
        },
        parameters, rk4);

    // steering_info = helper.load_mat_files_as_bus(
    //     "/home/fathi/torc/git/playground/py_ss/data/processed_mat",
    //     "steering_info")

    // print(history.keys())

    // T = history["t"]

    // for f, mat in steering_info.items():
    //     print(f)
    //     if callable(mat):
    //         mat = mat.y
    //     mat = mat[slc]
        
    //     blk = history["Steering_System." + f]
        
    //     plt.figure()
        
    //     if blk.ndim == 2:
    //         n = blk.shape[1]
    //         for k in range(n):
    //             plt.subplot(n, 1, k+1)
    //             if k == 0:
    //                 plt.title(f)
    //             blk_k = blk[:, k]
    //             if mat.ndim == 2:
    //                 mat_k = mat[:, k]
    //                 plt.plot(T, mat_k, "rx")
    //             else:
    //                 mat_k = mat[k]
    //                 plt.plot(T[0], mat_k, "rx")
    //             plt.plot(T, blk_k, "b-")
    //             plt.plot(T, blk_k - mat_k, "g-")
    //             plt.grid()
    //     else:
    //         plt.title(f)
    //         plt.plot(T, mat, "rx")
    //         plt.plot(T, blk, "b-")
    //         plt.plot(T, blk - mat, "g-")
    //         plt.grid()

    // plt.show()
}
