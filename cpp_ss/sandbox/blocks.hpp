
#ifndef __BLOCKS_HPP__
#define __BLOCKS_HPP__

#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <cassert>

#include "../3rdparty/eigen/Eigen/Core"

using namespace Eigen;

namespace blocks
{

// class State:
// class State
// {
// protected:
//     std::string _state;
//     double _deriv;
//     double _value;

// public:
//     State(const std::string& state, double deriv, double value) :
//         _state(state), _deriv(deriv), _value(value) {}

//     std::string repr() const
//     {
//         std::stringstream ss;
//         ss << _state << ", " << _deriv << ", " << _value;
//         return ss.str();
//     }
// };

using Signal = ArrayXd;

class StateData
{
public:
    StateData(const std::string& deriv, const Signal& value) :
        _deriv(deriv), _value(value) {}
    StateData(const std::string& deriv, double value) :
        _deriv(deriv), _value(1)
    {
        _value << value;
    }

    std::string _deriv;
    Signal      _value;
};

class Base;

using Scalars          = std::vector<double>;
using Signals          = std::vector<Signal>;
using NamedSignals     = std::map<std::string, Signal>;
using States           = std::map<std::string, StateData>;
using TraverseCallback = std::function<bool(const Base&)>;
using ActFunction      = std::function<Signal(double, const Signal&)>;

inline Signal S(double x)
{
    Signal ret(1);
    ret[0] = x;
    return ret;
}

class Node : public std::string
{
protected:
    bool _name_locked = true;

public:
    Node(const std::string& str="", bool name_locked=true) : std::string(str), _name_locked(name_locked) {}

    bool name_locked() const {return _name_locked;}
};

template<class T>
Node N(const T& s, bool name_locked=true)
{
    return dynamic_cast<const Node*>(&s) ? s : Node(s, name_locked);
}

inline Node N(const char* s, bool name_locked=true)
{
    return Node(s, name_locked);
}

inline Node N()
{
    return Node("-", false);
}

template<class T>
std::vector<Node> N(const std::vector<T>& s, bool name_locked=true)
{
    std::vector<Node> ret;
    ret.reserve(s.size());
    for (auto& p: s)
    {
        ret.push_back(N(p, name_locked));
    }
    return ret;
}

using Ports = std::vector<Node>;
// class Ports : public std::vector<Node>
// {
// public:
//     Ports() = default;
//     Ports(const Node& node)
//     {
//         push_back(node);
//     }
// };

class Base
{
protected:
    static Ports _all_iports;
    static Ports _all_oports;

    Ports _iports;
    Ports _oports;
    NamedSignals _known_values;

    std::string _name;
    bool _processed{false};

public:
    Base(const char* name, const Ports& iports=Ports(), const Ports& oports=Ports(), bool register_oports=true);

    virtual void get_states(States& states) {}
    virtual void step(double t, const NamedSignals& states) {}
    virtual Signals activation_function(double t, const Signals& x)
    {
        assert(false);
        return Signals();
    }

    // def get_ports(self):
    //     return self._iports, self._oports

    bool is_processed() const {return _processed;}
    const std::string& name() const {return _name;}
    const Ports& iports() const {return _iports;}
    const Ports& oports() const {return _oports;}

    virtual uint _process(double t, NamedSignals& x, bool reset);

    // def __repr__(self):
    //     return str(type(self)) + ":" + self._name + ", iports:" + str(self._iports) + ", oports:" + str(self._oports)

    virtual bool traverse(TraverseCallback cb)
    {
        return cb(*this);
    }
}; // class Base

// class Signal(Base):
//     def activation_function(self, t, x):
//         return [x[0]]

// class Bus(Base):
//     class BusValues(list):
//         def __init__(self, names, *args):
//             super().__init__(args)
//             self._names = names
        
//         def __repr__(self):
//             return 'BusValues(' + super().__repr__() + ')'

//     def __init__(self, name, iports='-', oport='-'):
//         super().__init__(name, iports, oport)
        
//         if not isinstance(iports, (list, tuple)):
//             iports = [iports]
//         self._raw_names = [n for n in iports]

//     def activation_function(self, t, x):
//         ret = [Bus.BusValues(self._raw_names, *x)]
//         return ret

// class BusSelector(Base):
//     def __init__(self, name, iport='-', signals=[], oports=[]):
//         if not isinstance(oports, (list, tuple)):
//             oports = [oports]
//         if not isinstance(signals, (list, tuple)):
//             signals = [signals]
            
//         if not signals:
//             signals = oports
//         elif not oports:
//             oports = signals
//         assert len(signals) == len(oports)

//         super().__init__(name, iport, oports)
//         self._signals = signals
//         self._indices = None

//     def activation_function(self, t, x):
//         bus_values = x[0]
//         if self._indices is None:
//             self._indices = [bus_values._names.index(s)
//                              for s in self._signals]
//         return [bus_values[k] for k in self._indices]

// class InitialValue(Base):
//     def __init__(self, name, iport='-', oport='-'):
//         super().__init__(name, iport, oport)
//         self._value = None

//     def activation_function(self, t, x):
//         if self._value is None:
//             self._value = x[0]
//             self._iports = []
//         return [self._value]

class Const : public Base
{
protected:
    Signal _value;

public:
    Const(const char* name, const Signal& value, const Ports& oport=Ports({N()})) :
        Base(name, Ports(), oport), _value(value) {}

    Const(const char* name, double value, const Ports& oport={N()}) :
        Base(name, Ports(), oport), _value(1)
    {
         _value << value;
    }

    Signals activation_function(double t, const Signals& x) override
    {
        return Signals{{_value}};
    }
};

class Gain : public Base
{
protected:
    double _k;

public:
    Gain(const char* name, double k, const Ports& iport=Ports({N()}), const Ports& oport=Ports({N()})) :
        Base(name, iport, oport), _k(k) {}

    Signals activation_function(double t, const Signals& x) override
    {
        return {_k * x[0]};
    }
};

class Sin : public Base
{
public:
    Sin(const char* name, const Ports& iports=Ports({N()}), const Ports& oports=Ports({N()})) :
        Base(name, iports, oports) {}

    Signals activation_function(double t, const Signals& x) override
    {
        return {x[0].sin()};
    }
};

class Function : public Base
{
protected:
    ActFunction _act_func;

public:
    Function(const char* name, ActFunction act_func, const Node& iport=N(), const Node& oport=N()) :
        Base(name, {iport}, {oport}), _act_func(act_func) {}

    Signals activation_function(double t, const Signals& x) override
    {
        return {_act_func(t, x[0])};
    }
};

// class MIMOFunction(Base):
//     def __init__(self, name, act_func, iports, oports):
//         super().__init__(name, iports=iports, oports=oports)
//         self._act_func = act_func

//     def activation_function(self, t, x):
//         return self._act_func(t, x)

class AddSub : public Base
{
protected:
    std::string _operators;
    double      _initial;

public:
    AddSub(const char* name, const char* operators, const Ports& iports, const Node& oport=N(), double initial=0.0) :
        Base(name, iports, {oport}), _operators(operators), _initial(initial)
    {
        assert(std::strlen(operators) == iports.size());
    }

    Signals activation_function(double t, const Signals& x) override
    {
        Signal ret = Signal::Constant(x[0].size(), _initial);
        const char* p = _operators.c_str();
        for (const auto& v: x)
        {
            if (*p == '+')
                ret += v;
            else if (*p == '-')
                ret -= v;
            else
                 assert(false);
            p++;
        }
        return {ret};
    }
};

class MulDiv : public Base
{
protected:
    std::string _operators;
    double      _initial;

public:
    MulDiv(const char* name, const char* operators, const Ports& iports, const Node& oport=N(), double initial=1.0) :
        Base(name, iports, {oport}), _operators(operators), _initial(initial)
    {
        assert(std::strlen(operators) == iports.size());
    }

    Signals activation_function(double t, const Signals& x) override
    {
        Signal ret = Signal::Constant(x[0].size(), _initial);
        const char* p = _operators.c_str();
        for (const auto& v: x)
        {
            if (*p == '*')
                ret *= v;
            else if (*p == '/')
                ret /= v;
            else
                 assert(false);
            p++;
        }
        return {ret};
    }
};

class Integrator : public Base
{
protected:
    double _value;

public:
    Integrator(const char* name, const Node& iport=N(), const Node& oport=N(), double ic=0.0) :
        Base(name, Ports({iport}), Ports({oport})), _value(ic) {}

    void get_states(States& states) override
    {
        states.insert_or_assign(_oports.front(), StateData(_iports.front(), _value));
    }

    void step(double t, const NamedSignals& states) override
    {
        _value = states.at(_oports.front())[0];
    }

    uint _process(double t, NamedSignals& x, bool reset) override;
};

// # it is still unclear how to deal with states when using this numerical integrator
// # class NumericalIntegrator(Base):
// #     def __init__(self, y0, name, iport='-', oport='-'):
// #         super().__init__(name, [iport], [oport])
// #         self._t = None
// #         self._x = None
// #         self._y = y0
    
// #     def step(self, t, states):
// #         self._t = t
// #         self._x = states[self._iports[0]]
// #         self._y = states[self._oports[0]]

// #     def activation_function(self, t, x):
// #         if self._t is None:
// #             self._t = t
// #             self._x = x[0]
// #             return [self._y]
// #         else:
// #             return [self._y + 0.5*(t - self._t)*(x[0] + self._x)]

class Delay : public Base
{
protected:
    double  _lifespan;
    Scalars _t;
    Signals  _x;

public:
    Delay(const char* name, const Ports& iports, const Ports& oport=Ports({N()}), double lifespan=10.0) :
        Base(name, iports, oport), _lifespan(lifespan) {}

    void step(double t, const NamedSignals& states) override
    {
        if (not _t.empty())
        {
            double t1 = t - _lifespan;
            int k = 0;
            for (const auto& v: _t)
            {
                if (v >= t1)
                    break;
                k++;
            }
            _t.erase(_t.begin(), _t.begin() + k);
            _x.erase(_x.begin(), _x.begin() + k);
        }

        assert(_t.empty() or (t > _t.back()));
        _t.push_back(t);
        _x.push_back(states.at(_iports.front()));
    }

    Signals activation_function(double t, const Signals& x) override
    {
        if (_t.empty())
            return {S(x[2][0])};
    
        const double delay = x[1][0];
        t -= delay;
        if (t <= _t.front())
        {
            return {x[2]};
        }
        else if (t >= _t.back())
        {
            return {_x.back()};
        }

        int k = 0;
        for (const auto& v: _t)
        {
            if (v >= t)
                break;
            k++;
        }

        return {S((_x[k][0] - _x[k - 1][0])*(t - _t[k - 1])/(_t[k] - _t[k - 1]) + _x[k - 1][0])};
    }
};

class Memory : public Base
{
protected:
    Signal _value;

public:
    Memory(const char* name, const Node& iport=N(), const Node& oport=N(), const Signal& ic=Signal::Zero(1)) :
        Base(name, Ports({iport}), Ports({oport})), _value(ic) {}

    void step(double t, const NamedSignals& states) override
    {
        _value = states.at(_iports.front());
    }

    // # Memory can be implemented either by defining the following activation function
    // #   (which is more straightforward) or through overloading the _process method
    // #   which is more efficient since it deosn't rely on the input signal being known.
    // #   Both approaches are supposed to lead to the exact same results.

    // # def activation_function(self, t, x):
    // #     return [self._value]

    uint _process(double t, NamedSignals& x, bool reset) override;
};

// class Derivative(Base):
//     def __init__(self, name, iport='-', oport='-', y0=0):
//         super().__init__(name, iports=iport, oports=oport)
//         self._t = None
//         self._x = None
//         self._y = y0

//     def step(self, t, states):
//         self._t = t
//         self._x = states[self._iports[0]]
//         self._y = states[self._oports[0]]

//     def activation_function(self, t, x):
//         if self._t is None:
//             self._t = t
//             self._x = x[0]
//             return [self._y]
//         elif self._t == t:
//             return [self._y]
//         else:
//             return [(x[0] - self._x)/(t - self._t)]

class Submodel : public Base
{
protected:
    static std::vector<Submodel*> _current_submodels;

public:
    static Submodel* current()
    {
        return Submodel::_current_submodels.empty() ? nullptr : Submodel::_current_submodels.back();
    }

protected:
    std::vector<Base*> _components;
    std::string _auto_signal_name;

public:
    Submodel(const char* name, const Ports& iports=Ports(), const Ports& oports=Ports()) :
        Base(name, iports, oports, false) {}

    void enter() {_current_submodels.push_back(this);}
    void exit()
    {
        assert(_current_submodels.back() == this);
        _current_submodels.pop_back();
    }

    void add_component(Base& component)
    {
        _components.push_back(&component);
    }

    void get_states(States& states) override
    {
        for (auto* component: _components)
            component->get_states(states);
    }

    void step(double t, const NamedSignals& states) override
    {
        for (auto& component: _components)
            component->step(t, states);
    }

    Node make_signal_name(const Node& name);
    Node auto_signal_name(bool makenew);
    uint _process(double t, NamedSignals& x, bool reset) override;
    bool traverse(TraverseCallback cb) override;

}; // class Submodel

}

#endif // __BLOCKS_HPP__
