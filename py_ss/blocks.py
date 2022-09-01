#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np

import solver

class State:
    def __init__(self, state, deriv, value):
        self._state = state
        self._deriv = deriv
        self._value = value
    
    def __repr__(self):
        return self._state + ", " + self._deriv + ', ' + str(self._value)

class Node(str):
    pass

def N(s):
    return s if isinstance(s, Node) else Node(s)

class Base:
    _all_iports = []
    _all_oports = []
    
    def __init__(self, name, iports=[], oports=[], **kwargs):
        parent = Submodel.current()

        self._name = name
        self._processed = False
        
        if parent:
            if parent._name:
                self._name = parent._name + '.' + self._name
            parent.add_component(self)

            iports = [parent.make_signal_name(p) for p in iports]
            oports = [parent.make_signal_name(p) for p in oports]

        self._iports = iports
        self._oports = oports

        if kwargs.get('register_oports', True):
            for port in oports:
                assert port not in self._all_oports, 'Port ' + port \
                    + ' already exists in oports: {}'.format(self._all_oports)
                self._all_oports.append(port)

        for port in iports:
            if port not in self._all_iports:
                self._all_iports.append(port)

    def get_states(self, states):
        return

    def step(self, t, states):
        return

    def get_ports(self):
        return self._iports, self._oports

    @property
    def is_processed(self): return self._processed

    def _process(self, t, x, reset):
        if reset:
            self._processed = False

        if self._processed:
            return 0
        else:
            for iport in self._iports:
                if iport not in x:
                    return 0

            for oport in self._oports:
                assert oport not in x, self._name + ': oport ' \
                    + oport + ' already exists in oports'

            input_values = [x[iport] for iport in self._iports]
            output_values = self.activation_function(t, input_values)
            for oport, output_value in zip(self._oports, output_values):
                x[oport] = output_value

            self._processed = True
            return 1

    def __repr__(self):
        return str(type(self)) + ":" + self._name + ", iports:" + str(self._iports) + ", oports:" + str(self._oports)

    def traverse(self, cb):
        return cb(self)

class Signal(Base):
    def __init__(self, name, iport, oport):
        super().__init__(name, [iport], [oport])

    def activation_function(self, t, x):
        return [x[0]]

class Bus(Base):
    def __init__(self, name, iports, oport):
        super().__init__(name, iports, [oport])

    def activation_function(self, t, x):
        return [x]

class InitialValue(Base):
    def __init__(self, name, iport, oport):
        super().__init__(name, [iport], [oport])
        self._value = None

    def activation_function(self, t, x):
        if self._value is None:
            self._value = x[0]
            self._iports = []
        return [self._value]

class Const(Base):
    def __init__(self, name, v, oport):
        super().__init__(name, [], [oport])
        self._v = v

    def activation_function(self, t, x):
        return [self._v]

class Gain(Base):
    def __init__(self, name, k, iport, oport):
        super().__init__(name, [iport], [oport])
        self._k = k

    def activation_function(self, t, x):
        return [self._k * x[0]]

class Function(Base):
    def __init__(self, name, iport, oport, act_func):
        super().__init__(name, [iport], [oport])
        self._act_func = act_func

    def activation_function(self, t, x):
        return [self._act_func(t, x[0])]

class AddSub(Base):
    def __init__(self, name, signs, iports, oport, initial=0.0):
        assert len(signs) == len(iports)
        super().__init__(name, iports, [oport])
        self._signs = signs
        self._initial = initial

    def activation_function(self, t, x):
        ret = self._initial
        for sign, v in zip(self._signs, x):
            if sign == '+':
                ret += v
            elif sign == '-':
                ret -= v
        return [ret]

class MulDiv(Base):
    def __init__(self, name, operations, iports, oport, initial=1.0):
        assert len(operations) == len(iports)
        super().__init__(name, iports, [oport])
        self._operations = operations
        self._initial = initial

    def activation_function(self, t, x):
        ret = self._initial
        for operation, v in zip(self._operations, x):
            if operation == '*':
                ret *= v
            elif operation == '/':
                ret /= v
        return [ret]

class Integrator(Base):
    def __init__(self, x0, name, iport, oport):
        super().__init__(name, [iport], [oport])
        self._value = x0

    def get_states(self, states):
        states.append(State(self._oports[0], self._iports[0], self._value))

    def step(self, t, states):
        self._value = states[self._oports[0]]

    def _process(self, t, x, reset):
        # self._processed = True
        # return 1 if reset else 0
        if reset:
            self._processed = False

        if self._processed:
            return 0
        else:
            self._processed = self._iports[0] in x
            return 1 if self._processed else 0

# it is still unclear how to deal with states when using this numerical integrator
# class NumericalIntegrator(Base):
#     def __init__(self, y0, name, iport, oport):
#         super().__init__(name, [iport], [oport])
#         self._t = None
#         self._x = None
#         self._y = y0
    
#     def step(self, t, states):
#         self._t = t
#         self._x = states[self._iports[0]]
#         self._y = states[self._oports[0]]

#     def activation_function(self, t, x):
#         if self._t is None:
#             self._t = t
#             self._x = x[0]
#             return [self._y]
#         else:
#             return [self._y + 0.5*(t - self._t)*(x[0] + self._x)]

class Delay(Base):
    def __init__(self, name, iports, oport):
        super().__init__(name, iports, [oport])
        self._t = []
        self._x = []

    def step(self, t, states):
        assert (not self._t) or (t > self._t[-1])
        self._t.append(t)
        self._x.append(states[self._iports[0]])

    def activation_function(self, t, x):
        if self._t:
            delay = x[1]
            return [np.interp(t - delay, self._t, self._x)]
        return [x[2]]

class Derivative(Base):
    def __init__(self, name, iport, oport, y0=0):
        super().__init__(name, [iport], [oport])
        self._t = None
        self._x = None
        self._y = y0

    def step(self, t, states):
        self._t = t
        self._x = states[self._iports[0]]
        self._y = states[self._oports[0]]

    def activation_function(self, t, x):
        if self._t is None:
            self._t = t
            self._x = x[0]
            return [self._y]
        elif self._t == t:
            return [self._y]
        else:
            return [(x[0] - self._x)/(t - self._t)]

class Submodel(Base):
    _current_submodels = []

    def current():
        return Submodel._current_submodels[-1] if len(Submodel._current_submodels) > 0 else None

    def __init__(self, name, iports=[], oports=[]):
        super().__init__(name, iports, oports, register_oports=False)
        self._components = []

    def __enter__(self):
        Submodel._current_submodels.append(self)
        return self

    def __exit__(self, type, value, traceback):
        assert Submodel._current_submodels[-1] == self
        del Submodel._current_submodels[-1]
        return True

    def add_component(self, component):
        self._components.append(component)

    def get_states(self, states):
        for component in self._components:
            component.get_states(states)

    def step(self, t, states):
        for component in self._components:
            component.step(t, states)

    def make_signal_name(self, name):
        if isinstance(name, Node):
            return name
        if not isinstance(name, str):
            name = str(name)
        if self._name:
            name = self._name + '.' + name
        return Node(name)

    def _process(self, t, x, reset):
        if reset:
            self._processed = False

        n_processed = 0
        if not self._processed:
            self._processed = True
            for component in self._components:
                n_processed += component._process(t, x, reset)
                if not component.is_processed:
                    self._processed = False

        return n_processed

    def traverse(self, cb):
        for component in self._components:
            if not component.traverse(cb):
                return False
        return super().traverse(cb)

class MainModel(Submodel):
    def __init__(self, name, iports=[], oports=[]):
        super().__init__(name, iports, oports)
        self._parameters = {}
        self._inputs = {}
        self._history = {}

    def process(self, t, x):
        n_processed = self._process(t, x, True)
        while True:
            n = self._process(t, x, False)
            if n == 0:
                break
            n_processed += n

        unprocessed = []
        def find_unprocessed_cb(c):
            if not c.is_processed:
                unprocessed.append(c)
            return True
        self.traverse(find_unprocessed_cb)
        if unprocessed:
            for c in unprocessed:
                print('-', c._name)
                for p in c._iports:
                    print('  - i: ', '*' if p not in x else ' ', p)
                for p in c._oports:
                    print('  - o: ', '*' if p not in x else ' ', p)

        return n_processed

    def run(self, inputs_cb=lambda t, x: {}, parameters={},
            h=0.01, t0=0.0, t_end=1.0, integrator=solver.rk4):
        states = []
        self.get_states(states)
        state_names = [state._state for state in states]
    
        def solver_callback(t, x):
            x = {k: v for k, v in zip(state_names, x)}
            for p, v in parameters.items():
                x[p] = v
            for p, v in inputs.items():
                x[p] = v
            self.process(t, x)
            return np.array([x[state._deriv] for state in states])
    
        x = np.array([state._value for state in states])
        t = t0

        def update_history(t, x):
            y = {k: v for k, v in zip(state_names, x)}
            for p, v in parameters.items():
                y[p] = v
            for p, v in inputs.items():
                y[p] = v
            self.process(t, y)
            self.step(t, y)
        
            if not self._history:
                self._history['t'] = []
                for k, v in y.items():
                    if k not in parameters and isinstance(v, (int, float)):
                        self._history[k] = []
        
            self._history['t'].append(t)
            for k, v in y.items():
                if k not in parameters and isinstance(v, (int, float)):
                    self._history[k].append(v)
        
        inputs = inputs_cb(t, x)
        update_history(t, x)
        
        k = 0
        while t <= t_end:
            inputs = inputs_cb(t, x)
            x = integrator(solver_callback, t0=t, x0=x, h=h)
            k += 1
            t = k*h
            print(k, t)

            update_history(t, x)

        return self._history
