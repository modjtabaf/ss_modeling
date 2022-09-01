#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import matplotlib.pyplot as plt
import numpy as np
from scipy.io import loadmat

import blocks
from blocks import N as N

class PT(blocks.Submodel):
    def __init__(self, iports, oport):
        super().__init__('PT', iports, [oport])

        # nodes
        y_in  = N(self._iports[0])
        tau   = N(self._iports[1])
        y0    = N(self._iports[2])
        y_out = N(self._oports[0])

        # blocks
        with self:
            blocks.AddSub('AddSub', '+-', [y_in, y_out], 1)
            blocks.MulDiv('MulDiv', '*/', [1, tau], 2)
            blocks.Integrator(0.0, '', 2, 3)
            blocks.InitialValue('IV', y0, 4)
            blocks.AddSub('AddSub', '++', [3, 4], y_out)

class ComputeFrontWheelAngleRightLeftPinpoint(blocks.Submodel):
    def __init__(self, iport, oports):
        super().__init__('ComputeFrontWheelAngleRightLeftPinpoint', [iport], oports)

        # nodes
        front_wheel_angle       = N(iport)
        front_wheel_angle_right = N(oports[0])
        front_wheel_angle_left  = N(oports[1])
        tractor_wheelbase = N('tractor_wheelbase')
        tractor_Width = N('tractor_Width')

        # blocks
        with self:
            blocks.MulDiv('MulDiv1', '*/', [tractor_wheelbase, front_wheel_angle], 1)
            blocks.AddSub('AddSub1', '++', [1, tractor_Width], 2)
            blocks.MulDiv('MulDiv2', '*/', [tractor_wheelbase, 2], front_wheel_angle_right)
            blocks.Gain('Gain', 0.5, tractor_Width, 3)
            blocks.AddSub('AddSub2', '+-', [1, 3], 4)
            blocks.MulDiv('MulDiv3', '*/', [tractor_wheelbase, 4], front_wheel_angle_left)

class SteeringSystem(blocks.MainModel):
    def __init__(self, iport, oport):
        super().__init__('Steering_System', [iport], [oport])

        # nodes
        ad_DsrdFtWhlAngl_Rq_VD     = N(self._iports[0])
        front_wheel_angle          = N('front_wheel_angle')
        front_wheel_angle_rate     = N('front_wheel_angle_rate')
        front_wheel_angle_neg      = N('front_wheel_angle_neg')
        front_wheel_angle_rate_neg = N('front_wheel_angle_rate_neg')
        AxFr_front_right           = N('AxFr_front_right')
        AxFr_front_left            = N('AxFr_front_left')
        steering_info              = N(self._oports[0])

        # blocks
        with self:
            blocks.MulDiv('MulDiv', '**', [ad_DsrdFtWhlAngl_Rq_VD, N('front_wheel_ang_gain')], 1)
            blocks.Delay('Delay', [1, N('front_wheel_ang_delay'), N('front_wheel_ang_init_value')], 2)
            blocks.Function('Function', N('front_wheel_ang_t_const'), 3, lambda t, x: max(0.001, min(10, x)))
            PT([2, 3, 2], front_wheel_angle)
            blocks.Derivative('Derivative', front_wheel_angle, front_wheel_angle_rate)
            blocks.Gain('Gain1', -1, front_wheel_angle, front_wheel_angle_neg)
            blocks.Gain('Gain2', -1, front_wheel_angle_rate, front_wheel_angle_rate_neg)
            ComputeFrontWheelAngleRightLeftPinpoint(front_wheel_angle, [
                AxFr_front_right, AxFr_front_left])
            blocks.Bus('bus', [
                front_wheel_angle,
                front_wheel_angle_rate,
                front_wheel_angle_neg,
                front_wheel_angle_rate_neg,
                AxFr_front_right,
                AxFr_front_left,
                ], steering_info)

def main():
    parameters = {
        'tractor_wheelbase': 5.8325,
        'tractor_Width': 2.5,
        'front_wheel_ang_t_const': 0.1,
        'front_wheel_ang_delay': 0.02,
        'front_wheel_ang_gain': 1.0,
        'front_wheel_ang_init_value': 0.0,
        }

    dat = loadmat('/home/fathi/torc/git/playground/py_ss/data/matlab.mat')
    front_wheel_angle_Rq_t = dat['front_wheel_angle_Rq_t'].ravel()[:5000]
    front_wheel_angle_Rq_data = dat['front_wheel_angle_Rq_data'].ravel()[:5000]

    mat_time = dat['simpletruck_analyzer_results'][0, 0][0]
    mean_front_wheel_steering_angle = dat['simpletruck_analyzer_results'][0, 0][29][0, 0][0]
    # for k in range(1, 81):
    #     print(k, dat['simpletruck_analyzer_results'][0, 0][k][0, 0][2])
    del dat

    def inputs_cb(t, x):
        front_wheel_angle_Rq = np.interp(t, front_wheel_angle_Rq_t,
                                         front_wheel_angle_Rq_data)
        inputs = {
            'front_wheel_angle_Rq': front_wheel_angle_Rq,
            }
        
        return inputs

    history = SteeringSystem('front_wheel_angle_Rq', 'steering_info').run(
        parameters=parameters, inputs_cb=inputs_cb,
        t0=front_wheel_angle_Rq_t[0],
        t_end=front_wheel_angle_Rq_t[-1])

    print(history.keys())
    plt.figure()
    plt.plot(history['t'], history['front_wheel_angle'], 'b-')
    plt.plot(mat_time, mean_front_wheel_steering_angle, 'r:')
    plt.xlabel('t'); plt.ylabel('front whee angle')
    plt.show()
    
if __name__ == '__main__':
    main()
